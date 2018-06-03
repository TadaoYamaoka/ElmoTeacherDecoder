#include <unordered_set>
#include <atomic>

#include "position.hpp"
#include "move.hpp"
#include "generateMoves.hpp"

using namespace std;

const constexpr int64_t HASH_SIZE_MB = 4096;
const constexpr int64_t MAX_SEARCH_NODE = 2097152;
const constexpr int REPEAT = INT_MAX;

// --- �l�ݏ����T��

// df-pn with Threshold Controlling Algorithm (TCA)�̎����B
// �ݖ{�͍G���� "Dealing with infinite loops, underestimation, and overestimation of depth-first
// proof-number search." �Ɋ܂܂��[���R�[�h�����Ɏ������Ă��܂��B
//
// TODO(someone): �D�z�֌W�̎���
// TODO(someone): �ؖ���̎���
// TODO(someone): Source Node Detection Algorithm (SNDA)�̎���
// 
// �����N���Q�l����
//
// Ayumu Nagai , Hiroshi Imai , "df-pn�A���S���Y���̋l�����������v���O�����ւ̉��p",
// ��񏈗��w��_����,43(6),1769-1777 (2002-06-15) , 1882-7764
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
// df-pn�A���S���Y���w�K�L(1) - A Succulent Windfall
// http://caprice-j.hatenablog.com/entry/2014/02/14/010932
//
// IS�����̋l�����𓚃v���O�����ɂ���
// http://www.is.titech.ac.jp/~kishi/pdf_file/csa.pdf
//
// df-pn�T�������炢 - �v�������Ŋw�΂Ȃ����L
// http://d.hatena.ne.jp/GMA0BN/20090520/1242825044
//
// df-pn�T���̃R�[�h - �v�������Ŋw�΂Ȃ����L
// http://d.hatena.ne.jp/GMA0BN/20090521/1242911867
//

// �l�����G���W���p��MovePicker
class MovePicker {
public:
	explicit MovePicker(const Position& pos, bool or_node) {
		if (or_node) {
			last_ = generateMoves<Check>(moveList_, pos);
			if (pos.inCheck()) {
				// ���ʂ�����̏ꍇ�A������肩������������𐶐�
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
			// �ʂ̈ړ��ɂ�鎩�E��ƁApin����Ă����̈ړ��ɂ�鎩�E����폜
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

// �u���\
// �ʏ�̒T���G���W���Ƃ͒u���\�ɕۑ��������l���قȂ邽��
// �l�ߏ�����p�̒u���\��p���Ă���
// ������SmallTreeGC�͎��������AStockfish�̒u���\�̎�����^���Ă���
struct TranspositionTable {
	static const constexpr uint32_t kInfiniteDepth = 1000000;
	struct TTEntry {
		// �n�b�V���̏��32�r�b�g
		uint32_t hash_high; // 0
		int depth;
		Hand hand; // ���i��ɐ��̎��j
		// TTEntry�̃C���X�^���X���쐬�����^�C�~���O�Ő�[�m�[�h��\���悤1�ŏ���������
		int pn; // 1
		int dn; // 1
		uint32_t generation/* : 8*/; // 0
		// ���[�g�m�[�h����̍ŒZ����
		// �����l�����Ƃ��đS�Ẵm�[�h���ŒZ�����������Ƃ݂Ȃ�
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
		// ���������ɍ��v����G���g����Ԃ�
		for (size_t i = 0; i < sizeof(entries.entries) / sizeof(TTEntry); i++) {
			TTEntry& entry = entries.entries[i];
			if (entry.hash_high == 0 || generation != entry.generation) {
				// ��̃G���g�������������ꍇ
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
					// key�����v����G���g�����������ꍇ
					// �c��̃G���g���ɗD�z�֌W�𖞂����ǖʂ�����ؖ��ς݂̏ꍇ�A�����Ԃ�
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
				// �D�z�֌W�𖞂����ǖʂɏؖ��ς݂̋ǖʂ�����ꍇ�A�����Ԃ�
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
		// ���v����G���g����������Ȃ������̂�
		// �Â��G���g�����Ԃ�
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

	// move���w������̎q�m�[�h�̒u���\�G���g����Ԃ�
	TTEntry& LookUpChildEntry(const Position& n, const Move move, const bool or_node, const int depth) {
		// ���͏�ɐ��̎��ŕ\��
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
	// --- ��̈ړ��ɂ�鉤��

	// ����ɂȂ�w����
	//  1) ����Ȃ��ړ��ɂ�钼�ډ���
	//  2) ����ړ��ɂ�钼�ډ���
	//  3) pin����Ă����̈ړ��ɂ��Ԑډ���
	// �W���Ƃ��Ă�1),2) <--> 3)�͔핢���Ă���\��������̂ł�������O�ł���悤�Ȏw���萶�������Ȃ��Ă͂Ȃ�Ȃ��B
	// ������Y��Ɏ�������̂͌��\����B

	// x = ���ډ���ƂȂ���
	// y = �Ԑډ���ƂȂ���

	// �قƂ�ǂ̃P�[�X�ɂ����� y == empty�Ȃ̂ł����O��ɍœK��������B
	// y�ƁAy���܂܂Ȃ�x�Ƃɕ����ď�������B
	// ���Ȃ킿�Ay �� (x | y)^y

	const Color US = pos.turn();
	const Color opp = oppositeColor(US);
	const Square ksq = pos.kingSquare(opp);

	// �ȉ��̕��@����x�Ƃ��Ĕ�(��)��100%�܂܂��B�p�E�n��60%���炢�̊m���Ŋ܂܂��B���O�����ł��������Ȃ���Ηǂ��̂����c�B
	const Bitboard x =
		(
		(pos.bbOf(Pawn)   & pawnCheckTable(US, ksq)) |
			(pos.bbOf(Lance)  & lanceCheckTable(US, ksq)) |
			(pos.bbOf(Knight) & knightCheckTable(US, ksq)) |
			(pos.bbOf(Silver) & silverCheckTable(US, ksq)) |
			(pos.bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver) & goldCheckTable(US, ksq)) |
			(pos.bbOf(Bishop) & bishopCheckTable(US, ksq)) |
			(pos.bbOf(Rook, Dragon)) | // ROOK,DRAGON�͖������S��
			(pos.bbOf(Horse)  & horseCheckTable(US, ksq))
			) & pos.bbOf(US);

	// �����ɂ͉���G�ʂ�8�ߖT�Ɉړ�������w������܂܂�邪�A�����ߐڂ���`�̓��A�P�[�X�Ȃ̂�
	// �w���萶���̒i�K�ł͏��O���Ȃ��Ă��ǂ��Ǝv���B

	const Bitboard y = pos.discoveredCheckBB();
	const Bitboard target = ~pos.bbOf(US); // ����Ȃ��ꏊ���ړ��Ώۏ�

	// y�̂݁B������x����y�ł���\��������B
	auto src = y;
	while (src)
	{
		const Square from = src.firstOneFromSQ11();

		// ��������Ȃ̂Ŏw����𐶐����Ă��܂��B

		// ���܂̓G�ʂ�from��ʂ钼����̏��ƈႤ�Ƃ���Ɉړ�������ΊJ�����肪�m�肷��B
		const PieceType pt = pieceToPieceType(pos.piece(from));
		Bitboard toBB = pos.attacksFrom(pt, US, from) & target;
		while (toBB) {
			const Square to = toBB.firstOneFromSQ11();
			if (!isAligned<true>(from, to, ksq)) {
				return false;
			}
			// ���ډ���ɂ��Ȃ�̂�x & from�̏ꍇ�A������̏��ւ̎w����𐶐��B
			else if (x.isSet(from)) {
				const PieceType pt = pieceToPieceType(pos.piece(from));
				switch (pt) {
				case Pawn: // ��
				{
					if (pawnAttack(US, from).isSet(to)) {
						return false;
					}
					break;
				}
				case Silver: // ��
				{
					Bitboard toBB = silverAttack(opp, ksq) & silverAttack(US, from) & target;
					if ((silverAttack(opp, ksq) & silverAttack(US, from)).isSet(to)) {
						return false;
					}
					// �����ĉ���
					if ((goldAttack(opp, ksq) & silverAttack(US, from)).isSet(to)) {
						if (canPromote(US, makeRank(to)) | canPromote(US, makeRank(from))) {
							return false;
						}
					}
					break;
				}
				case Gold: // ��
				case ProPawn: // �Ƌ�
				case ProLance: // ����
				case ProKnight: // ���j
				case ProSilver: // ����
				{
					if ((goldAttack(opp, ksq) & goldAttack(US, from)).isSet(to)) {
						return false;
					}
					break;
				}
				case Horse: // �n
				{
					// �ʂ��Ίp��ɂȂ��ꍇ
					assert(abs(makeFile(ksq) - makeFile(from)) != abs(makeRank(ksq) - makeRank(from)));
					if ((horseAttack(ksq, pos.occupiedBB()) & horseAttack(from, pos.occupiedBB())).isSet(to)) {
						return false;
					}
					break;
				}
				case Dragon: // ��
				{
					// �ʂ�������ɂȂ��ꍇ
					assert(makeFile(ksq) != makeFile(from) && makeRank(ksq) != makeRank(from));
					if ((dragonAttack(ksq, pos.occupiedBB()) & dragonAttack(from, pos.occupiedBB())).isSet(to)) {
						return false;
					}
					break;
				}
				case Lance: // ����
				case Knight: // �j�n
				case Bishop: // �p
				case Rook: // ���
				{
					assert(false);
					break;
				}
				default: UNREACHABLE;
				}
			}
		}
	}

	// y�ɔ핢���Ȃ�x
	src = (x | y) ^ y;
	while (src)
	{
		const Square from = src.firstOneFromSQ11();

		// ���ډ���̂݁B
		const PieceType pt = pieceToPieceType(pos.piece(from));
		switch (pt) {
		case Pawn: // ��
		{
			Bitboard toBB = pawnAttack(US, from) & target;
			if (toBB) {
				return false;
			}
			break;
		}
		case Lance: // ����
		{
			// �ʂƋ؂��قȂ�ꍇ
			if (makeFile(ksq) != makeFile(from)) {
				Bitboard toBB = goldAttack(opp, ksq) & lanceAttack(US, from, pos.occupiedBB()) & target;
				while (toBB) {
					const Square to = toBB.firstOneFromSQ11();
					// ����
					if (canPromote(US, makeRank(to))) {
						return false;
					}
				}
			}
			// �؂������ꍇ
			else {
				// �Ԃɂ�����ŁA�G��̏ꍇ
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		case Knight: // �j�n
		{
			Bitboard toBB = knightAttack(opp, ksq) & knightAttack(US, from) & target;
			if (toBB) {
				return false;
			}
			// �����ĉ���
			toBB = goldAttack(opp, ksq) & knightAttack(US, from) & target;
			while (toBB) {
				const Square to = toBB.firstOneFromSQ11();
				if (canPromote(US, makeRank(to))) {
					return false;
				}
			}
			break;
		}
		case Silver: // ��
		{
			Bitboard toBB = silverAttack(opp, ksq) & silverAttack(US, from) & target;
			if (toBB) {
				return false;
			}
			// �����ĉ���
			toBB = goldAttack(opp, ksq) & silverAttack(US, from) & target;
			while (toBB) {
				const Square to = toBB.firstOneFromSQ11();
				if (canPromote(US, makeRank(to)) | canPromote(US, makeRank(from))) {
					return false;
				}
			}
			break;
		}
		case Gold: // ��
		case ProPawn: // �Ƌ�
		case ProLance: // ����
		case ProKnight: // ���j
		case ProSilver: // ����
		{
			Bitboard toBB = goldAttack(opp, ksq) & goldAttack(US, from) & target;
			if (toBB) {
				return false;
			}
			break;
		}
		case Bishop: // �p
		{
			// �ʂ��Ίp��ɂȂ��ꍇ
			if (abs(makeFile(ksq) - makeFile(from)) != abs(makeRank(ksq) - makeRank(from))) {
				Bitboard toBB = horseAttack(ksq, pos.occupiedBB()) & bishopAttack(from, pos.occupiedBB()) & target;
				while (toBB) {
					const Square to = toBB.firstOneFromSQ11();
					// ����
					if (canPromote(US, makeRank(to)) | canPromote(US, makeRank(from))) {
						return false;
					}
				}
			}
			// �Ίp��ɂ���ꍇ
			else {
				// �Ԃɂ�����ŁA�G��̏ꍇ
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		case Rook: // ���
		{
			// �ʂ�������ɂȂ��ꍇ
			if (makeFile(ksq) != makeFile(from) && makeRank(ksq) != makeRank(from)) {
				Bitboard toBB = dragonAttack(ksq, pos.occupiedBB()) & rookAttack(from, pos.occupiedBB()) & target;
				while (toBB) {
					const Square to = toBB.firstOneFromSQ11();
					// ����
					if (canPromote(US, makeRank(to)) | canPromote(US, makeRank(from))) {
						return false;
					}
				}
			}
			// ������ɂ���ꍇ
			else {
				// �Ԃɂ�����ŁA�G��̏ꍇ
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		case Horse: // �n
		{
			// �ʂ��Ίp��ɂȂ��ꍇ
			if (abs(makeFile(ksq) - makeFile(from)) != abs(makeRank(ksq) - makeRank(from))) {
				Bitboard toBB = horseAttack(ksq, pos.occupiedBB()) & horseAttack(from, pos.occupiedBB()) & target;
				if (toBB) {
					return false;
				}
			}
			// �Ίp��ɂ���ꍇ
			else {
				// �Ԃɂ�����ŁA�G��̏ꍇ
				Bitboard dstBB = betweenBB(from, ksq) & pos.occupiedBB();
				if (dstBB.isOneBit() && dstBB & pos.bbOf(opp)) {
					return false;
				}
			}
			break;
		}
		case Dragon: // ��
		{
			// �ʂ�������ɂȂ��ꍇ
			if (makeFile(ksq) != makeFile(from) && makeRank(ksq) != makeRank(from)) {
				Bitboard toBB = dragonAttack(ksq, pos.occupiedBB()) & dragonAttack(from, pos.occupiedBB()) & target;
				if (toBB) {
					return false;
				}
			}
			// ������ɂ���ꍇ
			else {
				// �Ԃɂ�����ŁA�G��̏ꍇ
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

	// --- ��ł��ɂ�鉤��

	const Bitboard dropTarget = pos.nOccupiedBB(); // emptyBB() �ł͂Ȃ��̂Œ��ӂ��Ďg�����ƁB
	const Hand ourHand = pos.hand(US);

	// ���ł�
	if (ourHand.exists<HPawn>()) {
		Bitboard toBB = dropTarget & pawnAttack(opp, ksq);
		// ����̉��
		Bitboard pawnsBB = pos.bbOf(Pawn, US);
		Square pawnsSquare;
		foreachBB(pawnsBB, pawnsSquare, [&](const int part) {
			toBB.set(part, toBB.p(part) & ~squareFileMask(pawnsSquare).p(part));
		});

		// �ł����l�߂̉��
		const Rank TRank9 = (US == Black ? Rank9 : Rank1);
		const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);

		const Square ksq = pos.kingSquare(oppositeColor(US));
		// ����ʂ���i�ڂȂ�A���ŉ���o���Ȃ��̂ŁA�ł����l�߂𒲂ׂ�K�v�͂Ȃ��B
		if (makeRank(ksq) != TRank9) {
			const Square pawnDropCheckSquare = ksq + TDeltaS;
			assert(isInSquare(pawnDropCheckSquare));
			if (toBB.isSet(pawnDropCheckSquare) && pos.piece(pawnDropCheckSquare) == Empty) {
				if (!pos.isPawnDropCheckMate(US, pawnDropCheckSquare))
					// ������ clearBit �������� MakeMove ���Ȃ����Ƃ��o����B
					// �w���肪��������鏇�Ԃ��ς��A���肪��ɐ�������邪�A��Ŗ��ɂȂ�Ȃ���?
					return false;
				toBB.xorBit(pawnDropCheckSquare);
			}
		}

		if (toBB) return false;
	}

	// ���ԑł�
	if (ourHand.exists<HLance>()) {
		Bitboard toBB = dropTarget & lanceAttack(opp, ksq, pos.occupiedBB());
		if (toBB) return false;
	}

	// �j�n�ł�
	if (ourHand.exists<HKnight>()) {
		Bitboard toBB = dropTarget & knightAttack(opp, ksq);
		if (toBB) return false;
	}

	// ��ł�
	if (ourHand.exists<HSilver>()) {
		Bitboard toBB = dropTarget & silverAttack(opp, ksq);
		if (toBB) return false;
	}

	// ���ł�
	if (ourHand.exists<HGold>()) {
		Bitboard toBB = dropTarget & goldAttack(opp, ksq);
		if (toBB) return false;
	}

	// �p�ł�
	if (ourHand.exists<HBishop>()) {
		Bitboard toBB = dropTarget & bishopAttack(ksq, pos.occupiedBB());
		if (toBB) return false;
	}

	// ��ԑł�
	if (ourHand.exists<HRook>()) {
		Bitboard toBB = dropTarget & rookAttack(ksq, pos.occupiedBB());
		if (toBB) return false;
	}

	return true;
}

// ����̎w���肪�ߐډ��肩
bool moveGivesNeighborCheck(const Position pos, const Move move)
{
	const Color them = oppositeColor(pos.turn());
	const Square ksq = pos.kingSquare(them);

	const Square to = move.to();

	// �G�ʂ�8�ߖT
	if (pos.attacksFrom<King>(ksq).isSet(to))
		return true;

	// �j�n�ɂ�鉤��
	if (move.pieceTypeTo() == Lance)
		return true;

	return false;
}

// ���؋���v�Z(�����Ă��鎝������ő吔�ɂ���(���̎������������))
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
				// 1��ǂ݃��[�`���ɂ��`�F�b�N
				if (Move move = n.mateMoveIn1Ply()) {
					entry.pn = 0;
					entry.dn = kInfinitePnDn;
					//entry.minimum_distance = std::min(entry.minimum_distance, depth);

					// �ؖ����������
					entry.hand.set(0);

					// �ł�Ȃ�Ώؖ���ɉ�����
					if (move.isDrop()) {
						entry.hand.plusOne(move.handPieceDropped());
					}
					// ��肪�ꖇ�������Ă��Ȃ���ނ̐��̎�������ؖ���ɐݒ肷��
					if (!moveGivesNeighborCheck(n, move))
						entry.hand.setPP(n.hand(n.turn()), n.hand(oppositeColor(n.turn())));

					return;
				}
				// 3��l�݃`�F�b�N
				else {
					Color us = n.turn();
					Color them = oppositeColor(us);

					StateInfo si;
					StateInfo si2;

					// �ߐډ���Ŗ����̗���������A�G�̗����̂Ȃ��ꏊ��T���B
					for (MoveList<NeighborCheck> ml(n); !ml.end(); ++ml)
					{
						const Move m = ml.move();

						// �ߐډ���ŁA���̎w����ɂ���̈ړ���ɓG�̋�Ȃ��B
						Square to = m.to();

						if (
							// to�ɗ��������邩�ǂ����Bm���ړ��̎w����̏ꍇ�Am�̌��̗�������菜���K�v������B
							(m.isDrop() ? n.attackersTo(us, to) : (n.attackersTo(us, to, n.occupiedBB() ^ SetMaskBB[m.from()]) ^ SetMaskBB[m.from()]))
							// �G�ʂ̗����͕K��to�ɂ���̂ł�������������������邩�ǂ����B
							&& (n.attackersTo(them, to) ^ SetMaskBB[n.kingSquare(them)])
							)
						{
							if (!n.pseudoLegalMoveIsLegal<false, false>(m, n.pinnedBB()))
								continue;

							n.doMove(m, si);

							// ���̋ǖʂł��ׂĂ�evasion������
							MovePicker move_picker(n, false);
							for (const auto& move : move_picker)
							{
								const Move m2 = move.move;

								// ���̎w����ŋt����ɂȂ�Ȃ�A�s�l�߂Ƃ��Ĉ���
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
									// �l��łȂ��̂ŁAm2�ŋl�݂𓦂�Ă���B
									n.undoMove(m2);
									goto NEXT_CHECK;
								}

								n.undoMove(m2);
							}

							// ���ׂċl��
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
			// 2��ǂ݃`�F�b�N
			MovePicker move_picker(n, or_node);
			StateInfo si2;
			// ���̋ǖʂł��ׂĂ�evasion������
			for (const auto& move : move_picker)
			{
				const Move m2 = move.move;

				// ���̎w����ŋt����ɂȂ�Ȃ�A�s�l�߂Ƃ��Ĉ���
				if (n.moveGivesCheck(m2))
					goto NO_MATE;

				n.doMove(m2, si2);

				if (Move move = n.mateMoveIn1Ply()) {
					auto& entry1 = transposition_table.LookUp(n, true, depth + 1);
					entry1.pn = 0;
					entry1.dn = kInfinitePnDn;
					//entry1.minimum_distance = std::min(entry1.minimum_distance, depth + 1);

					// �ؖ����������
					entry1.hand.set(0);

					// �ł�Ȃ�Ώؖ���ɉ�����
					if (move.isDrop()) {
						entry1.hand.plusOne(move.handPieceDropped());
					}
					// ��肪�ꖇ�������Ă��Ȃ���ނ̐��̎�������ؖ���ɐݒ肷��
					if (!moveGivesNeighborCheck(n, move))
						entry1.hand.setPP(n.hand(n.turn()), n.hand(oppositeColor(n.turn())));
				}
				else {
					// �l��łȂ��̂ŁAm2�ŋl�݂𓦂�Ă���B
					// �s�l�݃`�F�b�N
					if (nomate(n)) {
						auto& entry1 = transposition_table.LookUp(n, true, depth + 1);
						entry1.pn = kInfinitePnDn;
						entry1.dn = 0;
						// ���؋�
						// �����Ă��鎝������ő吔�ɂ���(���̎������������)
						entry1.hand.set(dp(n.hand(n.turn()), n.hand(oppositeColor(n.turn()))));

						n.undoMove(m2);

						entry.pn = kInfinitePnDn;
						entry.dn = 0;
						//entry.minimum_distance = std::min(entry.minimum_distance, depth);
						// �q�ǖʂ̔��؋��ݒ�
						// �ł�Ȃ�΁A���؋��폜����
						if (m2.isDrop()) {
							entry.hand = entry1.hand;
							entry.hand.minusOne(m2.handPieceDropped());
						}
						// ���̋������Ȃ�΁A���؋�ɒǉ�����
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

			// ���ׂċl��
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
			return;

		NO_MATE:;
		}
	}

	MovePicker move_picker(n, or_node);
	if (move_picker.empty()) {
		// n����[�m�[�h

		if (or_node) {
			// �����̎�Ԃł����ɓ��B�����ꍇ�͉���̎肪���������A
			entry.pn = kInfinitePnDn;
			entry.dn = 0;

			// ���؋�
			// �����Ă��鎝������ő吔�ɂ���(���̎������������)
			entry.hand.set(dp(n.hand(n.turn()), n.hand(oppositeColor(n.turn()))));
		}
		else {
			// ����̎�Ԃł����ɓ��B�����ꍇ�͉������̎肪���������A
			// 1��l�߂��s���Ă��邽�߁A�����ɓ��B���邱�Ƃ͂Ȃ�
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
		}

		//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		return;
	}

	// �����̃`�F�b�N
	switch (n.isDraw(16)) {
	case RepetitionWin:
		//cout << "RepetitionWin" << endl;
		// �A������̐����ɂ�鏟��
		if (or_node) {
			// �����͒ʂ�Ȃ��͂�
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
		// �A������̐����ɂ�镉��
		if (or_node) {
			entry.pn = kInfinitePnDn;
			entry.dn = 0;
			entry.num_searched = REPEAT;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		else {
			// �����͒ʂ�Ȃ��͂�
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			entry.num_searched = REPEAT;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		return;

	case RepetitionDraw:
		//cout << "RepetitionDraw" << endl;
		// ���ʂ̐����
		// �����͒ʂ�Ȃ��͂�
		entry.pn = kInfinitePnDn;
		entry.dn = 0;
		entry.num_searched = REPEAT;
		//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		return;
	}

	// minimum distance��ۑ�����
	// TODO(nodchip): ���̃^�C�~���O��minimum distance��ۑ�����̂����������m���߂�
	//entry.minimum_distance = std::min(entry.minimum_distance, depth);

	while (searchedNode < MAX_SEARCH_NODE) {
		++entry.num_searched;

		Move best_move;
		int thpn_child;
		int thdn_child;

		// expand and compute pn(n) and dn(n);
		if (or_node) {
			// OR�m�[�h�ł͍ł��ؖ����������� = �ʂ̓������̌������Ȃ� = �l�܂��₷���m�[�h��I��
			int best_pn = kInfinitePnDn;
			int second_best_pn = kInfinitePnDn;
			int best_dn = 0;
			int best_num_search = INT_MAX;

			entry.pn = kInfinitePnDn;
			entry.dn = 0;
			// �q�ǖʂ̔��؋�̐ϏW��
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
					// �l�݂̏ꍇ
					//cout << n.toSFEN() << " or" << endl;
					//cout << bitset<32>(entry.hand.value()) << endl;
					entry.pn = 0;
					entry.dn = kInfinitePnDn;
					// �q�ǖʂ̏ؖ����ݒ�
					// �ł�Ȃ�΁A�ؖ���ɒǉ�����
					if (move.move.isDrop()) {
						const HandPiece hp = move.move.handPieceDropped();
						if (entry.hand.numOf(hp) > child_entry.hand.numOf(hp)) {
							entry.hand = child_entry.hand;
							entry.hand.plusOne(move.move.handPieceDropped());
						}
					}
					// ���̋������Ȃ�΁A�ؖ����폜����
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
						// ��
						const u32 child_pawn = child_dp.exists<HPawn>();
						if (child_pawn < pawn) pawn = child_pawn;
						// ����
						const u32 child_lance = child_dp.exists<HLance>();
						if (child_lance < lance) lance = child_lance;
						// �j�n
						const u32 child_knight = child_dp.exists<HKnight>();
						if (child_knight < knight) knight = child_knight;
						// ��
						const u32 child_silver = child_dp.exists<HSilver>();
						if (child_silver < silver) silver = child_silver;
						// ��
						const u32 child_gold = child_dp.exists<HGold>();
						if (child_gold < gold) gold = child_gold;
						// �p
						const u32 child_bishop = child_dp.exists<HBishop>();
						if (child_bishop < bishop) bishop = child_bishop;
						// ���
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
				// �s�l�݂̏ꍇ
				//cout << n.hand(n.turn()).value() << "," << entry.hand.value() << ",";
				// ��肪�ꖇ�������Ă��Ȃ���ނ̐��̎�����𔽏؋��폜����
				u32 curr_pawn = entry.hand.exists<HPawn>(); if (curr_pawn == 0) pawn = 0; else if (pawn < curr_pawn) pawn = curr_pawn;
				u32 curr_lance = entry.hand.exists<HLance>(); if (curr_lance == 0) lance = 0; else if (lance < curr_lance) lance = curr_lance;
				u32 curr_knight = entry.hand.exists<HKnight>(); if (curr_knight == 0) knight = 0; else if (knight < curr_knight) knight = curr_knight;
				u32 curr_silver = entry.hand.exists<HSilver>(); if (curr_silver == 0) silver = 0; else if (silver < curr_silver) silver = curr_silver;
				u32 curr_gold = entry.hand.exists<HGold>(); if (curr_gold == 0) gold = 0; else if (gold < curr_gold) gold = curr_gold;
				u32 curr_bishop = entry.hand.exists<HBishop>(); if (curr_bishop == 0) bishop = 0; else if (bishop < curr_bishop) bishop = curr_bishop;
				u32 curr_rook = entry.hand.exists<HRook>(); if (curr_rook == 0) rook = 0; else if (rook < curr_rook) rook = curr_rook;
				// ���؋�Ɏq�ǖʂ̏ؖ���̐ϏW����ݒ�
				entry.hand.set(pawn | lance | knight | silver | gold | bishop | rook);
				//cout << entry.hand.value() << endl;
			}
			else {
				thpn_child = std::min(thpn, second_best_pn + 1);
				thdn_child = std::min(thdn - entry.dn + best_dn, kInfinitePnDn);
			}
		}
		else {
			// AND�m�[�h�ł͍ł����ؐ��̏����� = ����̊|�����̏��Ȃ� = �s�l�݂������₷���m�[�h��I��
			int best_dn = kInfinitePnDn;
			int second_best_dn = kInfinitePnDn;
			int best_pn = 0;
			int best_num_search = INT_MAX;

			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			// �q�ǖʂ̏ؖ���̘a�W��
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
						// ��
						const u32 child_pawn = child_pp.exists<HPawn>();
						if (child_pawn > pawn) pawn = child_pawn;
						// ����
						const u32 child_lance = child_pp.exists<HLance>();
						if (child_lance > lance) lance = child_lance;
						// �j�n
						const u32 child_knight = child_pp.exists<HKnight>();
						if (child_knight > knight) knight = child_knight;
						// ��
						const u32 child_silver = child_pp.exists<HSilver>();
						if (child_silver > silver) silver = child_silver;
						// ��
						const u32 child_gold = child_pp.exists<HGold>();
						if (child_gold > gold) gold = child_gold;
						// �p
						const u32 child_bishop = child_pp.exists<HBishop>();
						if (child_bishop > bishop) bishop = child_bishop;
						// ���
						const u32 child_rook = child_pp.exists<HRook>();
						if (child_rook > rook) rook = child_rook;
					}
					else
						all_mate = false;
				}
				if (child_entry.dn == 0) {
					// �s�l�݂̏ꍇ
					entry.pn = kInfinitePnDn;
					entry.dn = 0;
					// �q�ǖʂ̔��؋��ݒ�
					// �ł�Ȃ�΁A���؋��폜����
					if (move.move.isDrop()) {
						const HandPiece hp = move.move.handPieceDropped();
						if (entry.hand.numOf(hp) < child_entry.hand.numOf(hp)) {
							entry.hand = child_entry.hand;
							entry.hand.minusOne(hp);
						}
					}
					// ���̋������Ȃ�΁A���؋�ɒǉ�����
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
				// �l�݂̏ꍇ
				//cout << n.toSFEN() << " and" << endl;
				//cout << bitset<32>(entry.hand.value()) << endl;
				// �ؖ���Ɏq�ǖʂ̏ؖ���̘a�W����ݒ�
				u32 curr_pawn = entry.hand.exists<HPawn>(); if (pawn > curr_pawn) pawn = curr_pawn;
				u32 curr_lance = entry.hand.exists<HLance>(); if (lance > curr_lance) lance = curr_lance;
				u32 curr_knight = entry.hand.exists<HKnight>(); if (knight > curr_knight) knight = curr_knight;
				u32 curr_silver = entry.hand.exists<HSilver>(); if (silver > curr_silver) silver = curr_silver;
				u32 curr_gold = entry.hand.exists<HGold>(); if (gold > curr_gold) gold = curr_gold;
				u32 curr_bishop = entry.hand.exists<HBishop>(); if (bishop > curr_bishop) bishop = curr_bishop;
				u32 curr_rook = entry.hand.exists<HRook>(); if (rook > curr_rook) rook = curr_rook;
				entry.hand.set(pawn | lance | knight | silver | gold | bishop | rook);
				//cout << bitset<32>(entry.hand.value()) << endl;
				// ��肪�ꖇ�������Ă��Ȃ���ނ̐��̎�������ؖ���ɐݒ肷��
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

// �l�ݎ菇��1�Ԃ�
// �ŒZ�̋l�ݎ菇�ł���ۏ؂͂Ȃ�
bool dfs(bool or_node, Position& pos, std::vector<Move>& moves, std::unordered_set<Key>& visited) {
	// ��x�T�������m�[�h��T�����Ȃ�
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
// �l�����T���̃G���g���|�C���g
bool dfpn(Position& r) {
	// ���ʂɉ��肪�������Ă��Ȃ�����

	// �L���b�V���̐����i�߂�
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

// �l�����T���̃G���g���|�C���g
bool dfpn_andnode(Position& r) {
	// ���ʂɉ��肪�������Ă���

	// �L���b�V���̐����i�߂�
	transposition_table.NewSearch();

	searchedNode = 0;
	DFPNwithTCA(r, kInfinitePnDn, kInfinitePnDn/*, false*/, false, 0, searchedNode);
	const auto& entry = transposition_table.LookUp(r, false, 0);

	//cout << searchedNode << endl;

	return entry.pn == 0;
}
