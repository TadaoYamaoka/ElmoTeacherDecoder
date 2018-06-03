#include <unordered_set>
#include <atomic>

#include "position.hpp"
#include "move.hpp"
#include "generateMoves.hpp"

using namespace std;

const constexpr int64_t HASH_SIZE_MB = 4096;
const constexpr int64_t MAX_SEARCH_NODE = 2097152;
const constexpr int REPEAT = INT_MAX;

// --- 詰み将棋探索

// df-pn with Threshold Controlling Algorithm (TCA)の実装。
// 岸本章宏氏の "Dealing with infinite loops, underestimation, and overestimation of depth-first
// proof-number search." に含まれる擬似コードを元に実装しています。
//
// TODO(someone): 優越関係の実装
// TODO(someone): 証明駒の実装
// TODO(someone): Source Node Detection Algorithm (SNDA)の実装
// 
// リンク＆参考文献
//
// Ayumu Nagai , Hiroshi Imai , "df-pnアルゴリズムの詰将棋を解くプログラムへの応用",
// 情報処理学会論文誌,43(6),1769-1777 (2002-06-15) , 1882-7764
// http://id.nii.ac.jp/1001/00011597/
//
// Nagai, A.: Df-pn algorithm for searching AND/OR trees and its applications, PhD thesis,
// Department of Information Science, The University of Tokyo (2002)
//
// Ueda T., Hashimoto T., Hashimoto J., Iida H. (2008) Weak Proof-Number Search. In: van den Herik
// H.J., Xu X., Ma Z., Winands M.H.M. (eds) Computers and Games. CG 2008. Lecture Notes in Computer
// Science, vol 5131. Springer, Berlin, Heidelberg
//
// Toru Ueda, Tsuyoshi Hashimoto, Junichi Hashimoto, Hiroyuki Iida, Weak Proof - Number Search,
// Proceedings of the 6th international conference on Computers and Games, p.157 - 168, September 29
// - October 01, 2008, Beijing, China
//
// Kishimoto, A.: Dealing with infinite loops, underestimation, and overestimation of depth-first
// proof-number search. In: Proceedings of the AAAI-10, pp. 108-113 (2010)
//
// A. Kishimoto, M. Winands, M. Muller and J. Saito. Game-Tree Search Using Proof Numbers: The First
// Twenty Years. ICGA Journal 35(3), 131-156, 2012. 
//
// A. Kishimoto and M. Mueller, Tutorial 4: Proof-Number Search Algorithms
// 
// df-pnアルゴリズム学習記(1) - A Succulent Windfall
// http://caprice-j.hatenablog.com/entry/2014/02/14/010932
//
// IS将棋の詰将棋解答プログラムについて
// http://www.is.titech.ac.jp/~kishi/pdf_file/csa.pdf
//
// df-pn探索おさらい - 思うだけで学ばない日記
// http://d.hatena.ne.jp/GMA0BN/20090520/1242825044
//
// df-pn探索のコード - 思うだけで学ばない日記
// http://d.hatena.ne.jp/GMA0BN/20090521/1242911867
//

// 詰将棋エンジン用のMovePicker
class MovePicker {
public:
	explicit MovePicker(const Position& pos, bool or_node) {
		if (or_node) {
			last_ = generateMoves<Check>(moveList_, pos);
			if (pos.inCheck()) {
				// 自玉が王手の場合、逃げる手かつ王手をかける手を生成
				ExtMove* curr = moveList_;
				const Bitboard pinned = pos.pinnedBB();
				while (curr != last_) {
					if (!pos.pseudoLegalMoveIsEvasion(curr->move, pinned))
						curr->move = (--last_)->move;
					else
						++curr;
				}
			}
		}
		else {
			last_ = generateMoves<Evasion>(moveList_, pos);
			// 玉の移動による自殺手と、pinされている駒の移動による自殺手を削除
			ExtMove* curr = moveList_;
			const Bitboard pinned = pos.pinnedBB();
			while (curr != last_) {
				if (!pos.pseudoLegalMoveIsLegal<false, false>(curr->move, pinned))
					curr->move = (--last_)->move;
				else
					++curr;
			}
		}
		assert(size() <= 65);
	}
	size_t size() const { return static_cast<size_t>(last_ - moveList_); }
	ExtMove* begin() { return &moveList_[0]; }
	ExtMove* end() { return last_; }
	bool empty() const { return size() == 0; }

private:
	ExtMove moveList_[65];
	ExtMove* last_;
};

// 置換表
// 通常の探索エンジンとは置換表に保存したい値が異なるため
// 詰め将棋専用の置換表を用いている
// ただしSmallTreeGCは実装せず、Stockfishの置換表の実装を真似ている
struct TranspositionTable {
	static const constexpr uint32_t kInfiniteDepth = 1000000;
	struct TTEntry {
		// ハッシュの上位32ビット
		uint32_t hash_high; // 0
		int depth;
		Hand hand; // 手駒（常に先手の手駒）
		// TTEntryのインスタンスを作成したタイミングで先端ノードを表すよう1で初期化する
		int pn; // 1
		int dn; // 1
		uint32_t generation/* : 8*/; // 0
		// ルートノードからの最短距離
		// 初期値を∞として全てのノードより最短距離が長いとみなす
		//int minimum_distance : 24; // UINT_MAX
		int num_searched; // 0
	};

	struct Cluster {
		TTEntry entries[256];
	};

	virtual ~TranspositionTable() {
		if (tt_raw) {
			std::free(tt_raw);
			tt_raw = nullptr;
			tt = nullptr;
		}
	}

	TTEntry& LookUp(const Key key, const Hand hand, const int depth) {
		auto& entries = tt[key & clusters_mask];
		uint32_t hash_high = key >> 32;
		int max_pn = 1;
		int max_dn = 1;
		// 検索条件に合致するエントリを返す
		for (size_t i = 0; i < sizeof(entries.entries) / sizeof(TTEntry); i++) {
			TTEntry& entry = entries.entries[i];
			if (entry.hash_high == 0 || generation != entry.generation) {
				// 空のエントリが見つかった場合
				entry.hash_high = hash_high;
				entry.depth = depth;
				entry.hand = hand;
				entry.pn = max_pn;
				entry.dn = max_dn;
				entry.generation = generation;
				//entry.minimum_distance = kInfiniteDepth;
				entry.num_searched = 0;
				return entry;
			}

			if (hash_high == entry.hash_high && generation == entry.generation) {
				if (hand == entry.hand && depth == entry.depth) {
					// keyが合致するエントリを見つけた場合
					// 残りのエントリに優越関係を満たす局面があり証明済みの場合、それを返す
					for (i++; i < sizeof(entries.entries) / sizeof(TTEntry); i++) {
						TTEntry& entry_rest = entries.entries[i];
						if (entry_rest.hash_high == 0) break;
						if (hash_high == entry_rest.hash_high) {
							if (entry_rest.pn == 0) {
								if (hand.isEqualOrSuperior(entry_rest.hand) && entry_rest.num_searched != REPEAT) {
									entry_rest.generation = generation;
									return entry_rest;
								}
							}
							else if (entry_rest.dn == 0) {
								if (entry_rest.hand.isEqualOrSuperior(hand) && entry_rest.num_searched != REPEAT) {
									entry_rest.generation = generation;
									return entry_rest;
								}
							}
						}
					}
					return entry;
				}
				// 優越関係を満たす局面に証明済みの局面がある場合、それを返す
				if (entry.pn == 0) {
					if (hand.isEqualOrSuperior(entry.hand) && entry.num_searched != REPEAT) {
						return entry;
					}
				}
				else if (entry.dn == 0) {
					if (entry.hand.isEqualOrSuperior(hand) && entry.num_searched != REPEAT) {
						return entry;
					}
				}
				else if (entry.hand.isEqualOrSuperior(hand)) {
					if (entry.pn > max_pn) max_pn = entry.pn;
				}
				else if (hand.isEqualOrSuperior(entry.hand)) {
					if (entry.dn > max_dn) max_dn = entry.dn;
				}
			}
		}

		//cout << "hash entry full" << endl;
		// 合致するエントリが見つからなかったので
		// 古いエントリをつぶす
		TTEntry* best_entry = nullptr;
		uint32_t best_num_searched = UINT_MAX;
		for (auto& entry : entries.entries) {
			if (best_num_searched > entry.num_searched && entry.pn != 0) {
				best_entry = &entry;
				best_num_searched = entry.num_searched;
			}
		}
		best_entry->hash_high = hash_high;
		best_entry->hand = hand;
		best_entry->depth = depth;
		best_entry->pn = 1;
		best_entry->dn = 1;
		best_entry->generation = generation;
		//best_entry->minimum_distance = kInfiniteDepth;
		best_entry->num_searched = 0;
		return *best_entry;
	}

	TTEntry& LookUp(const Position& n, const bool or_node, const int depth) {
		return LookUp(n.getBoardKey(), or_node ? n.hand(n.turn()) : n.hand(oppositeColor(n.turn())), depth);
	}

	// moveを指した後の子ノードの置換表エントリを返す
	TTEntry& LookUpChildEntry(const Position& n, const Move move, const bool or_node, const int depth) {
		// 手駒は常に先手の手駒で表す
		Hand hand;
		if (or_node) {
			hand = n.hand(n.turn());
			if (move.isDrop()) {
				hand.minusOne(move.handPieceDropped());
			}
			else {
				const Piece to_pc = n.piece(move.to());
				if (to_pc != Empty) {
					const PieceType pt = pieceToPieceType(to_pc);
					hand.plusOne(pieceTypeToHandPiece(pt));
				}
			}
		}
		else {
			hand = n.hand(oppositeColor(n.turn()));
		}
		return LookUp(n.getBoardKeyAfter(move), hand, depth + 1);
	}

	void Resize() {
		int64_t hash_size_mb = HASH_SIZE_MB;
		if (hash_size_mb == 16) {
			hash_size_mb = 4096;
		}
		int64_t new_num_clusters = 1LL << msb((hash_size_mb * 1024 * 1024) / sizeof(Cluster));
		if (new_num_clusters == num_clusters) {
			return;
		}

		num_clusters = new_num_clusters;

		if (tt_raw) {
			std::free(tt_raw);
			tt_raw = nullptr;
			tt = nullptr;
		}

		tt_raw = std::calloc(new_num_clusters * sizeof(Cluster) + CacheLineSize, 1);
		tt = (Cluster*)((uintptr_t(tt_raw) + CacheLineSize - 1) & ~(CacheLineSize - 1));
		clusters_mask = num_clusters - 1;
	}

	void NewSearch() {
		++generation;
	}

	void* tt_raw = nullptr;
	Cluster* tt = nullptr;
	int64_t num_clusters = 0;
	int64_t clusters_mask = 0;
	uint32_t generation = 0;
};

static const constexpr int kInfinitePnDn = 100000000;
static const constexpr int kMaxDepth = 256;

TranspositionTable transposition_table;

bool nomate(const Position& pos) {
	// --- 駒の移動による王手

	// 王手になる指し手
	//  1) 成らない移動による直接王手
	//  2) 成る移動による直接王手
	//  3) pinされている駒の移動による間接王手
	// 集合としては1),2) <--> 3)は被覆している可能性があるのでこれを除外できるような指し手生成をしなくてはならない。
	// これを綺麗に実装するのは結構難しい。

	// x = 直接王手となる候補
	// y = 間接王手となる候補

	// ほとんどのケースにおいて y == emptyなのでそれを前提に最適化をする。
	// yと、yを含まないxとに分けて処理する。
	// すなわち、y と (x | y)^y

	const Color US = pos.turn();
	const Color opp = oppositeColor(US);
	const Square ksq = pos.kingSquare(opp);

	// 以下の方法だとxとして飛(龍)は100%含まれる。角・馬は60%ぐらいの確率で含まれる。事前条件でもう少し省ければ良いのだが…。
	const Bitboard x =
		(
		(pos.bbOf(Pawn)   & pawnCheckTable(US, ksq)) |
			(pos.bbOf(Lance)  & lanceCheckTable(US, ksq)) |
			(pos.bbOf(Knight) & knightCheckTable(US, ksq)) |
			(pos.bbOf(Silver) & silverCheckTable(US, ksq)) |
			(pos.bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver) & goldCheckTable(US, ksq)) |
			(pos.bbOf(Bishop) & bishopCheckTable(US, ksq)) |
			(pos.bbOf(Rook, Dragon)) | // ROOK,DRAGONは無条件全域
			(pos.bbOf(Horse)  & horseCheckTable(US, ksq))
			) & pos.bbOf(US);

	// ここには王を敵玉の8近傍に移動させる指し手も含まれるが、王が近接する形はレアケースなので
	// 指し手生成の段階では除外しなくても良いと思う。

	const Bitboard y = pos.discoveredCheckBB();
	const Bitboard target = ~pos.bbOf(US); // 自駒がない場所が移動対象升

	// yのみ。ただしxかつyである可能性もある。
	auto src = y;
	while (src)
	{
		const Square from = src.firstOneFromSQ11();

		// 両王手候補なので指し手を生成してしまう。

		// いまの敵玉とfromを通る直線上の升と違うところに移動させれば開き王手が確定する。
		const PieceType pt = pieceToPieceType(pos.piece(from));
		Bitboard toBB = pos.attacksFrom(pt, US, from) & target;
		while (toBB) {
			const Square to = toBB.firstOneFromSQ11();
			if (!isAligned<true>(from, to, ksq)) {
				return false;
			}
			// 直接王手にもなるのでx & fromの場合、直線上の升への指し手を生成。
			else if (x.isSet(from)) {
				const PieceType pt = pieceToPieceType(pos.piece(from));
				switch (pt) {
				case Pawn: // 歩
				{
					if (pawnAttack(US, from).isSet(to)) {
						return false;
					}
					break;
				}
				case Silver: // 銀
				{
					Bitboard toBB = silverAttack(opp, ksq) & silverAttack(US, from) & target;
					if ((silverAttack(opp, ksq) & silverAttack(US, from)).isSet(to)) {
						return false;
					}
					// 成って王手
					if ((goldAttack(opp, ksq) & silverAttack(US, from)).isSet(to)) {
						if (canPromote(US, makeRank(to)) | canPromote(US, makeRank(from))) {
							return false;
						}
					}
					break;
				}
				case Gold: // 金
				case ProPawn: // と金
				case ProLance: // 成香
				case ProKnight: // 成桂
				case ProSilver: // 成銀
				{
					if ((goldAttack(opp, ksq) & goldAttack(US, from)).isSet(to)) {
						return false;
					}
					break;
				}
				case Horse: // 馬
				{
					// 玉が対角上にない場合
					assert(abs(makeFile(ksq) - makeFile(from)) != abs(makeRank(ksq) - makeRank(from)));
					if ((horseAttack(ksq, pos.occupiedBB()) & horseAttack(from, pos.occupiedBB())).isSet(to)) {
						return false;
					}
					break;
				}
				case Dragon: // 竜
				{
					// 玉が直線上にない場合
					assert(makeFile(ksq) != makeFile(from) && makeRank(ksq) != makeRank(from));
					if ((dragonAttack(ksq, pos.occupiedBB()) & dragonAttack(from, pos.occupiedBB())).isSet(to)) {
						return false;
					}
					break;
				}
				case Lance: // 香車
				case Knight: // 桂馬
				case Bishop: // 角
				case Rook: // 飛車
				{
					assert(false);
					break;
				}
				default: UNREACHABLE;
				}
			}
		}
	}

	// yに被覆しないx
	src = (x | y) ^ y;
	while (src)
	{
		const Square from = src.firstOneFromSQ11();

		// 直接王手のみ。
		const PieceType pt = pieceToPieceType(pos.piece(from));
		switch (pt) {
		case Pawn: // 歩
		{
			Bitboard toBB = pawnAttack(US, from) & target;
			if (toBB) {
				return false;
			}
			break;
		}
		case Lance: // 香車
		{
			// 玉と筋が異なる場合
			if (makeFile(ksq) != makeFile(from)) {
				Bitboard toBB = goldAttack(opp, ksq) & lanceAttack(US, from, pos.occupiedBB()) & target;
				while (toBB) {
					const Square to = toBB.firstOneFromSQ11();
					// 成る
					if (canPromote(US, makeRank(to))) {
						return false;
					}
				}
			}
			// 筋が同じ場合
			else {
				// 間にある駒が一つで、敵駒の場合
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		case Knight: // 桂馬
		{
			Bitboard toBB = knightAttack(opp, ksq) & knightAttack(US, from) & target;
			if (toBB) {
				return false;
			}
			// 成って王手
			toBB = goldAttack(opp, ksq) & knightAttack(US, from) & target;
			while (toBB) {
				const Square to = toBB.firstOneFromSQ11();
				if (canPromote(US, makeRank(to))) {
					return false;
				}
			}
			break;
		}
		case Silver: // 銀
		{
			Bitboard toBB = silverAttack(opp, ksq) & silverAttack(US, from) & target;
			if (toBB) {
				return false;
			}
			// 成って王手
			toBB = goldAttack(opp, ksq) & silverAttack(US, from) & target;
			while (toBB) {
				const Square to = toBB.firstOneFromSQ11();
				if (canPromote(US, makeRank(to)) | canPromote(US, makeRank(from))) {
					return false;
				}
			}
			break;
		}
		case Gold: // 金
		case ProPawn: // と金
		case ProLance: // 成香
		case ProKnight: // 成桂
		case ProSilver: // 成銀
		{
			Bitboard toBB = goldAttack(opp, ksq) & goldAttack(US, from) & target;
			if (toBB) {
				return false;
			}
			break;
		}
		case Bishop: // 角
		{
			// 玉が対角上にない場合
			if (abs(makeFile(ksq) - makeFile(from)) != abs(makeRank(ksq) - makeRank(from))) {
				Bitboard toBB = horseAttack(ksq, pos.occupiedBB()) & bishopAttack(from, pos.occupiedBB()) & target;
				while (toBB) {
					const Square to = toBB.firstOneFromSQ11();
					// 成る
					if (canPromote(US, makeRank(to)) | canPromote(US, makeRank(from))) {
						return false;
					}
				}
			}
			// 対角上にある場合
			else {
				// 間にある駒が一つで、敵駒の場合
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		case Rook: // 飛車
		{
			// 玉が直線上にない場合
			if (makeFile(ksq) != makeFile(from) && makeRank(ksq) != makeRank(from)) {
				Bitboard toBB = dragonAttack(ksq, pos.occupiedBB()) & rookAttack(from, pos.occupiedBB()) & target;
				while (toBB) {
					const Square to = toBB.firstOneFromSQ11();
					// 成る
					if (canPromote(US, makeRank(to)) | canPromote(US, makeRank(from))) {
						return false;
					}
				}
			}
			// 直線上にある場合
			else {
				// 間にある駒が一つで、敵駒の場合
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		case Horse: // 馬
		{
			// 玉が対角上にない場合
			if (abs(makeFile(ksq) - makeFile(from)) != abs(makeRank(ksq) - makeRank(from))) {
				Bitboard toBB = horseAttack(ksq, pos.occupiedBB()) & horseAttack(from, pos.occupiedBB()) & target;
				if (toBB) {
					return false;
				}
			}
			// 対角上にある場合
			else {
				// 間にある駒が一つで、敵駒の場合
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		case Dragon: // 竜
		{
			// 玉が直線上にない場合
			if (makeFile(ksq) != makeFile(from) && makeRank(ksq) != makeRank(from)) {
				Bitboard toBB = dragonAttack(ksq, pos.occupiedBB()) & dragonAttack(from, pos.occupiedBB()) & target;
				if (toBB) {
					return false;
				}
			}
			// 直線上にある場合
			else {
				// 間にある駒が一つで、敵駒の場合
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		default: UNREACHABLE;
		}
	}

	const Bitboard pinned = pos.pinnedBB();

	// --- 駒打ちによる王手

	const Bitboard dropTarget = pos.nOccupiedBB(); // emptyBB() ではないので注意して使うこと。
	const Hand ourHand = pos.hand(US);

	// 歩打ち
	if (ourHand.exists<HPawn>()) {
		Bitboard toBB = dropTarget & pawnAttack(opp, ksq);
		// 二歩の回避
		Bitboard pawnsBB = pos.bbOf(Pawn, US);
		Square pawnsSquare;
		foreachBB(pawnsBB, pawnsSquare, [&](const int part) {
			toBB.set(part, toBB.p(part) & ~squareFileMask(pawnsSquare).p(part));
		});

		// 打ち歩詰めの回避
		const Rank TRank9 = (US == Black ? Rank9 : Rank1);
		const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);

		const Square ksq = pos.kingSquare(oppositeColor(US));
		// 相手玉が九段目なら、歩で王手出来ないので、打ち歩詰めを調べる必要はない。
		if (makeRank(ksq) != TRank9) {
			const Square pawnDropCheckSquare = ksq + TDeltaS;
			assert(isInSquare(pawnDropCheckSquare));
			if (toBB.isSet(pawnDropCheckSquare) && pos.piece(pawnDropCheckSquare) == Empty) {
				if (!pos.isPawnDropCheckMate(US, pawnDropCheckSquare))
					// ここで clearBit だけして MakeMove しないことも出来る。
					// 指し手が生成される順番が変わり、王手が先に生成されるが、後で問題にならないか?
					return false;
				toBB.xorBit(pawnDropCheckSquare);
			}
		}

		if (toBB) return false;
	}

	// 香車打ち
	if (ourHand.exists<HLance>()) {
		Bitboard toBB = dropTarget & lanceAttack(opp, ksq, pos.occupiedBB());
		if (toBB) return false;
	}

	// 桂馬打ち
	if (ourHand.exists<HKnight>()) {
		Bitboard toBB = dropTarget & knightAttack(opp, ksq);
		if (toBB) return false;
	}

	// 銀打ち
	if (ourHand.exists<HSilver>()) {
		Bitboard toBB = dropTarget & silverAttack(opp, ksq);
		if (toBB) return false;
	}

	// 金打ち
	if (ourHand.exists<HGold>()) {
		Bitboard toBB = dropTarget & goldAttack(opp, ksq);
		if (toBB) return false;
	}

	// 角打ち
	if (ourHand.exists<HBishop>()) {
		Bitboard toBB = dropTarget & bishopAttack(ksq, pos.occupiedBB());
		if (toBB) return false;
	}

	// 飛車打ち
	if (ourHand.exists<HRook>()) {
		Bitboard toBB = dropTarget & rookAttack(ksq, pos.occupiedBB());
		if (toBB) return false;
	}

	return true;
}

// 王手の指し手が近接王手か
bool moveGivesNeighborCheck(const Position pos, const Move move)
{
	const Color them = oppositeColor(pos.turn());
	const Square ksq = pos.kingSquare(them);

	const Square to = move.to();

	// 敵玉の8近傍
	if (pos.attacksFrom<King>(ksq).isSet(to))
		return true;

	// 桂馬による王手
	if (move.pieceTypeTo() == Lance)
		return true;

	return false;
}

// 反証駒を計算(持っている持ち駒を最大数にする(後手の持ち駒を加える))
u32 dp(const Hand& us, const Hand& them) {
	u32 dp = 0;
	u32 pawn = us.exists<HPawn>(); if (pawn > 0) dp += pawn + them.exists<HPawn>();
	u32 lance = us.exists<HLance>(); if (lance > 0) dp += lance + them.exists<HLance>();
	u32 knight = us.exists<HKnight>(); if (knight > 0) dp += knight + them.exists<HKnight>();
	u32 silver = us.exists<HSilver>(); if (silver > 0) dp += silver + them.exists<HSilver>();
	u32 gold = us.exists<HGold>(); if (gold > 0) dp += gold + them.exists<HGold>();
	u32 bishop = us.exists<HBishop>(); if (bishop > 0) dp += bishop + them.exists<HBishop>();
	u32 rook = us.exists<HRook>(); if (rook > 0) dp += rook + them.exists<HRook>();
	return dp;
}

void DFPNwithTCA(Position& n, int thpn, int thdn/*, bool inc_flag*/, bool or_node, int depth, int64_t& searchedNode) {
	auto& entry = transposition_table.LookUp(n, or_node, depth);

	if (depth > kMaxDepth) {
		entry.pn = kInfinitePnDn;
		entry.dn = 0;
		//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		return;
	}

	// if (n is a terminal node) { handle n and return; }

	if (entry.num_searched == 0) {
		if (or_node) {
			if (!n.inCheck()) {
				// 1手読みルーチンによるチェック
				if (Move move = n.mateMoveIn1Ply()) {
					entry.pn = 0;
					entry.dn = kInfinitePnDn;
					//entry.minimum_distance = std::min(entry.minimum_distance, depth);

					// 証明駒を初期化
					entry.hand.set(0);

					// 打つ手ならば証明駒に加える
					if (move.isDrop()) {
						entry.hand.plusOne(move.handPieceDropped());
					}
					// 後手が一枚も持っていない種類の先手の持ち駒を証明駒に設定する
					if (!moveGivesNeighborCheck(n, move))
						entry.hand.setPP(n.hand(n.turn()), n.hand(oppositeColor(n.turn())));

					return;
				}
				// 3手詰みチェック
				else {
					Color us = n.turn();
					Color them = oppositeColor(us);

					StateInfo si;
					StateInfo si2;

					// 近接王手で味方の利きがあり、敵の利きのない場所を探す。
					for (MoveList<NeighborCheck> ml(n); !ml.end(); ++ml)
					{
						const Move m = ml.move();

						// 近接王手で、この指し手による駒の移動先に敵の駒がない。
						Square to = m.to();

						if (
							// toに利きがあるかどうか。mが移動の指し手の場合、mの元の利きを取り除く必要がある。
							(m.isDrop() ? n.attackersTo(us, to) : (n.attackersTo(us, to, n.occupiedBB() ^ SetMaskBB[m.from()]) ^ SetMaskBB[m.from()]))
							// 敵玉の利きは必ずtoにあるのでそれを除いた利きがあるかどうか。
							&& (n.attackersTo(them, to) ^ SetMaskBB[n.kingSquare(them)])
							)
						{
							if (!n.pseudoLegalMoveIsLegal<false, false>(m, n.pinnedBB()))
								continue;

							n.doMove(m, si);

							// この局面ですべてのevasionを試す
							MovePicker move_picker(n, false);
							for (const auto& move : move_picker)
							{
								const Move m2 = move.move;

								// この指し手で逆王手になるなら、不詰めとして扱う
								if (n.moveGivesCheck(m2))
									goto NEXT_CHECK;

								n.doMove(m2, si2);

								if (n.mateMoveIn1Ply()) {
									//auto& entry1 = transposition_table.LookUp(n, true, depth + 2);
									//entry1.pn = 0;
									//entry1.dn = kInfinitePnDn;
									//entry1.minimum_distance = std::min(entry1.minimum_distance, depth + 2);
								}
								else {
									// 詰んでないので、m2で詰みを逃れている。
									n.undoMove(m2);
									goto NEXT_CHECK;
								}

								n.undoMove(m2);
							}

							// すべて詰んだ
							n.undoMove(m);

							entry.pn = 0;
							entry.dn = kInfinitePnDn;
							//entry.minimum_distance = std::min(entry.minimum_distance, depth);

							return;

						NEXT_CHECK:;
							n.undoMove(m);
						}
					}
				}
			}
		}
		else {
			// 2手読みチェック
			MovePicker move_picker(n, or_node);
			StateInfo si2;
			// この局面ですべてのevasionを試す
			for (const auto& move : move_picker)
			{
				const Move m2 = move.move;

				// この指し手で逆王手になるなら、不詰めとして扱う
				if (n.moveGivesCheck(m2))
					goto NO_MATE;

				n.doMove(m2, si2);

				if (Move move = n.mateMoveIn1Ply()) {
					auto& entry1 = transposition_table.LookUp(n, true, depth + 1);
					entry1.pn = 0;
					entry1.dn = kInfinitePnDn;
					//entry1.minimum_distance = std::min(entry1.minimum_distance, depth + 1);

					// 証明駒を初期化
					entry1.hand.set(0);

					// 打つ手ならば証明駒に加える
					if (move.isDrop()) {
						entry1.hand.plusOne(move.handPieceDropped());
					}
					// 後手が一枚も持っていない種類の先手の持ち駒を証明駒に設定する
					if (!moveGivesNeighborCheck(n, move))
						entry1.hand.setPP(n.hand(n.turn()), n.hand(oppositeColor(n.turn())));
				}
				else {
					// 詰んでないので、m2で詰みを逃れている。
					// 不詰みチェック
					if (nomate(n)) {
						auto& entry1 = transposition_table.LookUp(n, true, depth + 1);
						entry1.pn = kInfinitePnDn;
						entry1.dn = 0;
						// 反証駒
						// 持っている持ち駒を最大数にする(後手の持ち駒を加える)
						entry1.hand.set(dp(n.hand(n.turn()), n.hand(oppositeColor(n.turn()))));

						n.undoMove(m2);

						entry.pn = kInfinitePnDn;
						entry.dn = 0;
						//entry.minimum_distance = std::min(entry.minimum_distance, depth);
						// 子局面の反証駒を設定
						// 打つ手ならば、反証駒から削除する
						if (m2.isDrop()) {
							entry.hand = entry1.hand;
							entry.hand.minusOne(m2.handPieceDropped());
						}
						// 先手の駒を取る手ならば、反証駒に追加する
						else {
							const Piece to_pc = n.piece(m2.to());
							if (to_pc != Empty) {
								const PieceType pt = pieceToPieceType(to_pc);
								const HandPiece hp = pieceTypeToHandPiece(pt);
								if (entry.hand.numOf(hp) > entry1.hand.numOf(hp)) {
									entry.hand = entry1.hand;
									entry.hand.plusOne(hp);
								}
							}
						}
						goto NO_MATE;
					}
					n.undoMove(m2);
					goto NO_MATE;
				}

				n.undoMove(m2);
			}

			// すべて詰んだ
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
			return;

		NO_MATE:;
		}
	}

	MovePicker move_picker(n, or_node);
	if (move_picker.empty()) {
		// nが先端ノード

		if (or_node) {
			// 自分の手番でここに到達した場合は王手の手が無かった、
			entry.pn = kInfinitePnDn;
			entry.dn = 0;

			// 反証駒
			// 持っている持ち駒を最大数にする(後手の持ち駒を加える)
			entry.hand.set(dp(n.hand(n.turn()), n.hand(oppositeColor(n.turn()))));
		}
		else {
			// 相手の手番でここに到達した場合は王手回避の手が無かった、
			// 1手詰めを行っているため、ここに到達することはない
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
		}

		//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		return;
	}

	// 千日手のチェック
	switch (n.isDraw(16)) {
	case RepetitionWin:
		//cout << "RepetitionWin" << endl;
		// 連続王手の千日手による勝ち
		if (or_node) {
			// ここは通らないはず
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			entry.num_searched = REPEAT;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		else {
			entry.pn = kInfinitePnDn;
			entry.dn = 0;
			entry.num_searched = REPEAT;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		return;

	case RepetitionLose:
		//cout << "RepetitionLose" << endl;
		// 連続王手の千日手による負け
		if (or_node) {
			entry.pn = kInfinitePnDn;
			entry.dn = 0;
			entry.num_searched = REPEAT;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		else {
			// ここは通らないはず
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			entry.num_searched = REPEAT;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		return;

	case RepetitionDraw:
		//cout << "RepetitionDraw" << endl;
		// 普通の千日手
		// ここは通らないはず
		entry.pn = kInfinitePnDn;
		entry.dn = 0;
		entry.num_searched = REPEAT;
		//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		return;
	}

	// minimum distanceを保存する
	// TODO(nodchip): このタイミングでminimum distanceを保存するのが正しいか確かめる
	//entry.minimum_distance = std::min(entry.minimum_distance, depth);

	while (searchedNode < MAX_SEARCH_NODE) {
		++entry.num_searched;

		Move best_move;
		int thpn_child;
		int thdn_child;

		// expand and compute pn(n) and dn(n);
		if (or_node) {
			// ORノードでは最も証明数が小さい = 玉の逃げ方の個数が少ない = 詰ましやすいノードを選ぶ
			int best_pn = kInfinitePnDn;
			int second_best_pn = kInfinitePnDn;
			int best_dn = 0;
			int best_num_search = INT_MAX;

			entry.pn = kInfinitePnDn;
			entry.dn = 0;
			// 子局面の反証駒の積集合
			u32 pawn = UINT_MAX;
			u32 lance = UINT_MAX;
			u32 knight = UINT_MAX;
			u32 silver = UINT_MAX;
			u32 gold = UINT_MAX;
			u32 bishop = UINT_MAX;
			u32 rook = UINT_MAX;
			bool first = true;
			for (const auto& move : move_picker) {
				const auto& child_entry = transposition_table.LookUpChildEntry(n, move, or_node, depth);
				if (child_entry.pn == 0) {
					// 詰みの場合
					//cout << n.toSFEN() << " or" << endl;
					//cout << bitset<32>(entry.hand.value()) << endl;
					entry.pn = 0;
					entry.dn = kInfinitePnDn;
					// 子局面の証明駒を設定
					// 打つ手ならば、証明駒に追加する
					if (move.move.isDrop()) {
						const HandPiece hp = move.move.handPieceDropped();
						if (entry.hand.numOf(hp) > child_entry.hand.numOf(hp)) {
							entry.hand = child_entry.hand;
							entry.hand.plusOne(move.move.handPieceDropped());
						}
					}
					// 後手の駒を取る手ならば、証明駒から削除する
					else {
						const Piece to_pc = n.piece(move.move.to());
						if (to_pc != Empty) {
							entry.hand = child_entry.hand;
							const PieceType pt = pieceToPieceType(to_pc);
							const HandPiece hp = pieceTypeToHandPiece(pt);
							if (entry.hand.exists(hp))
								entry.hand.minusOne(hp);
						}
					}
					//cout << bitset<32>(entry.hand.value()) << endl;
					break;
				}
				else if (entry.dn == 0) {
					if (child_entry.dn == 0) {
						const Hand& child_dp = child_entry.hand;
						// 歩
						const u32 child_pawn = child_dp.exists<HPawn>();
						if (child_pawn < pawn) pawn = child_pawn;
						// 香車
						const u32 child_lance = child_dp.exists<HLance>();
						if (child_lance < lance) lance = child_lance;
						// 桂馬
						const u32 child_knight = child_dp.exists<HKnight>();
						if (child_knight < knight) knight = child_knight;
						// 銀
						const u32 child_silver = child_dp.exists<HSilver>();
						if (child_silver < silver) silver = child_silver;
						// 金
						const u32 child_gold = child_dp.exists<HGold>();
						if (child_gold < gold) gold = child_gold;
						// 角
						const u32 child_bishop = child_dp.exists<HBishop>();
						if (child_bishop < bishop) bishop = child_bishop;
						// 飛車
						const u32 child_rook = child_dp.exists<HRook>();
						if (child_rook < rook) rook = child_rook;
					}
				}
				entry.pn = std::min(entry.pn, child_entry.pn);
				entry.dn += child_entry.dn;

				if (child_entry.pn < best_pn ||
					child_entry.pn == best_pn && best_num_search > child_entry.num_searched) {
					second_best_pn = best_pn;
					best_pn = child_entry.pn;
					best_dn = child_entry.dn;
					best_move = move;
					best_num_search = child_entry.num_searched;
				}
				else if (child_entry.pn < second_best_pn) {
					second_best_pn = child_entry.pn;
				}
			}
			entry.dn = std::min(entry.dn, kInfinitePnDn);
			if (entry.dn == 0) {
				// 不詰みの場合
				//cout << n.hand(n.turn()).value() << "," << entry.hand.value() << ",";
				// 先手が一枚も持っていない種類の先手の持ち駒を反証駒から削除する
				u32 curr_pawn = entry.hand.exists<HPawn>(); if (curr_pawn == 0) pawn = 0; else if (pawn < curr_pawn) pawn = curr_pawn;
				u32 curr_lance = entry.hand.exists<HLance>(); if (curr_lance == 0) lance = 0; else if (lance < curr_lance) lance = curr_lance;
				u32 curr_knight = entry.hand.exists<HKnight>(); if (curr_knight == 0) knight = 0; else if (knight < curr_knight) knight = curr_knight;
				u32 curr_silver = entry.hand.exists<HSilver>(); if (curr_silver == 0) silver = 0; else if (silver < curr_silver) silver = curr_silver;
				u32 curr_gold = entry.hand.exists<HGold>(); if (curr_gold == 0) gold = 0; else if (gold < curr_gold) gold = curr_gold;
				u32 curr_bishop = entry.hand.exists<HBishop>(); if (curr_bishop == 0) bishop = 0; else if (bishop < curr_bishop) bishop = curr_bishop;
				u32 curr_rook = entry.hand.exists<HRook>(); if (curr_rook == 0) rook = 0; else if (rook < curr_rook) rook = curr_rook;
				// 反証駒に子局面の証明駒の積集合を設定
				entry.hand.set(pawn | lance | knight | silver | gold | bishop | rook);
				//cout << entry.hand.value() << endl;
			}
			else {
				thpn_child = std::min(thpn, second_best_pn + 1);
				thdn_child = std::min(thdn - entry.dn + best_dn, kInfinitePnDn);
			}
		}
		else {
			// ANDノードでは最も反証数の小さい = 王手の掛け方の少ない = 不詰みを示しやすいノードを選ぶ
			int best_dn = kInfinitePnDn;
			int second_best_dn = kInfinitePnDn;
			int best_pn = 0;
			int best_num_search = INT_MAX;

			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			// 子局面の証明駒の和集合
			u32 pawn = 0;
			u32 lance = 0;
			u32 knight = 0;
			u32 silver = 0;
			u32 gold = 0;
			u32 bishop = 0;
			u32 rook = 0;
			bool all_mate = true;
			for (const auto& move : move_picker) {
				const auto& child_entry = transposition_table.LookUpChildEntry(n, move, or_node, depth);
				if (all_mate) {
					if (child_entry.pn == 0) {
						const Hand& child_pp = child_entry.hand;
						// 歩
						const u32 child_pawn = child_pp.exists<HPawn>();
						if (child_pawn > pawn) pawn = child_pawn;
						// 香車
						const u32 child_lance = child_pp.exists<HLance>();
						if (child_lance > lance) lance = child_lance;
						// 桂馬
						const u32 child_knight = child_pp.exists<HKnight>();
						if (child_knight > knight) knight = child_knight;
						// 銀
						const u32 child_silver = child_pp.exists<HSilver>();
						if (child_silver > silver) silver = child_silver;
						// 金
						const u32 child_gold = child_pp.exists<HGold>();
						if (child_gold > gold) gold = child_gold;
						// 角
						const u32 child_bishop = child_pp.exists<HBishop>();
						if (child_bishop > bishop) bishop = child_bishop;
						// 飛車
						const u32 child_rook = child_pp.exists<HRook>();
						if (child_rook > rook) rook = child_rook;
					}
					else
						all_mate = false;
				}
				if (child_entry.dn == 0) {
					// 不詰みの場合
					entry.pn = kInfinitePnDn;
					entry.dn = 0;
					// 子局面の反証駒を設定
					// 打つ手ならば、反証駒から削除する
					if (move.move.isDrop()) {
						const HandPiece hp = move.move.handPieceDropped();
						if (entry.hand.numOf(hp) < child_entry.hand.numOf(hp)) {
							entry.hand = child_entry.hand;
							entry.hand.minusOne(hp);
						}
					}
					// 先手の駒を取る手ならば、反証駒に追加する
					else {
						const Piece to_pc = n.piece(move.move.to());
						if (to_pc != Empty) {
							const PieceType pt = pieceToPieceType(to_pc);
							const HandPiece hp = pieceTypeToHandPiece(pt);
							if (entry.hand.numOf(hp) > child_entry.hand.numOf(hp)) {
								entry.hand = child_entry.hand;
								entry.hand.plusOne(hp);
							}
						}
					}
					break;
				}
				entry.pn += child_entry.pn;
				entry.dn = std::min(entry.dn, child_entry.dn);

				if (child_entry.dn < best_dn ||
					child_entry.dn == best_dn && best_num_search > child_entry.num_searched) {
					second_best_dn = best_dn;
					best_dn = child_entry.dn;
					best_pn = child_entry.pn;
					best_move = move;
				}
				else if (child_entry.dn < second_best_dn) {
					second_best_dn = child_entry.dn;
				}
			}
			entry.pn = std::min(entry.pn, kInfinitePnDn);
			if (entry.pn == 0) {
				// 詰みの場合
				//cout << n.toSFEN() << " and" << endl;
				//cout << bitset<32>(entry.hand.value()) << endl;
				// 証明駒に子局面の証明駒の和集合を設定
				u32 curr_pawn = entry.hand.exists<HPawn>(); if (pawn > curr_pawn) pawn = curr_pawn;
				u32 curr_lance = entry.hand.exists<HLance>(); if (lance > curr_lance) lance = curr_lance;
				u32 curr_knight = entry.hand.exists<HKnight>(); if (knight > curr_knight) knight = curr_knight;
				u32 curr_silver = entry.hand.exists<HSilver>(); if (silver > curr_silver) silver = curr_silver;
				u32 curr_gold = entry.hand.exists<HGold>(); if (gold > curr_gold) gold = curr_gold;
				u32 curr_bishop = entry.hand.exists<HBishop>(); if (bishop > curr_bishop) bishop = curr_bishop;
				u32 curr_rook = entry.hand.exists<HRook>(); if (rook > curr_rook) rook = curr_rook;
				entry.hand.set(pawn | lance | knight | silver | gold | bishop | rook);
				//cout << bitset<32>(entry.hand.value()) << endl;
				// 後手が一枚も持っていない種類の先手の持ち駒を証明駒に設定する
				if (!(n.checkersBB() & n.attacksFrom<King>(n.kingSquare(n.turn())) || n.checkersBB() & n.attacksFrom<Knight>(n.turn(), n.kingSquare(n.turn()))))
					entry.hand.setPP(n.hand(oppositeColor(n.turn())), n.hand(n.turn()));
				//cout << bitset<32>(entry.hand.value()) << endl;
			}
			else {
				thpn_child = std::min(thpn - entry.pn + best_pn, kInfinitePnDn);
				thdn_child = std::min(thdn, second_best_dn + 1);
			}
		}

		// if (pn(n) >= thpn || dn(n) >= thdn)
		//   break; // termination condition is satisfied
		if (entry.pn >= thpn || entry.dn >= thdn) {
			break;
		}

		StateInfo state_info;
		//cout << n.toSFEN() << "," << best_move.toUSI() << endl;
		n.doMove(best_move, state_info);
		++searchedNode;
		DFPNwithTCA(n, thpn_child, thdn_child/*, inc_flag*/, !or_node, depth + 1, searchedNode);
		n.undoMove(best_move);
	}
}

// 詰み手順を1つ返す
// 最短の詰み手順である保証はない
bool dfs(bool or_node, Position& pos, std::vector<Move>& moves, std::unordered_set<Key>& visited) {
	// 一度探索したノードを探索しない
	if (visited.find(pos.getKey()) != visited.end()) {
		return false;
	}
	visited.insert(pos.getKey());

	MovePicker move_picker(pos, or_node);
	Move mate1ply = pos.mateMoveIn1Ply();
	if (mate1ply || move_picker.empty()) {
		if (mate1ply) {
			moves.push_back(mate1ply);
		}
		//std::ostringstream oss;
		//oss << "info string";
		//for (const auto& move : moves) {
		//  oss << " " << move;
		//}
		//sync_cout << oss.str() << sync_endl;
		//if (mate1ply) {
		//  moves.pop_back();
		//}
		return true;
	}

	for (const auto& move : move_picker) {
		const auto& child_entry = transposition_table.LookUpChildEntry(pos, move, or_node, 0);
		if (child_entry.pn != 0) {
			continue;
		}

		StateInfo state_info;
		pos.doMove(move, state_info);
		moves.push_back(move);
		if (dfs(!or_node, pos, moves, visited)) {
			pos.undoMove(move);
			return true;
		}
		moves.pop_back();
		pos.undoMove(move);
	}

	return false;
}

void dfpn_init()
{
	transposition_table.Resize();
}

int64_t searchedNode = 0;
// 詰将棋探索のエントリポイント
bool dfpn(Position& r) {
	// 自玉に王手がかかっていないこと

	// キャッシュの世代を進める
	transposition_table.NewSearch();

	searchedNode = 0;
	DFPNwithTCA(r, kInfinitePnDn, kInfinitePnDn/*, false*/, true, 0, searchedNode);
	const auto& entry = transposition_table.LookUp(r, true, 0);

	//cout << searchedNode << endl;

	/*std::vector<Move> moves;
	std::unordered_set<Key> visited;
	dfs(true, r, moves, visited);
	for (Move& move : moves)
		cout << move.toUSI() << " ";
	cout << endl;*/

	return entry.pn == 0;
}

// 詰将棋探索のエントリポイント
bool dfpn_andnode(Position& r) {
	// 自玉に王手がかかっている

	// キャッシュの世代を進める
	transposition_table.NewSearch();

	searchedNode = 0;
	DFPNwithTCA(r, kInfinitePnDn, kInfinitePnDn/*, false*/, false, 0, searchedNode);
	const auto& entry = transposition_table.LookUp(r, false, 0);

	//cout << searchedNode << endl;

	return entry.pn == 0;
}
