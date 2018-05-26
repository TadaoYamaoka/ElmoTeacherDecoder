#include <unordered_set>
#include <atomic>

#include "position.hpp"
#include "move.hpp"
#include "generateMoves.hpp"

using namespace std;

const int64_t HASH_SIZE_MB = 4096;
const int MAX_PLY = 28;
const int64_t MAX_SEARCH_NODE = 2097152;

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
	explicit MovePicker(const Position& pos, bool or_node) : curr_(moveList_) {
		if (or_node) {
			last_ = generateMoves<Check>(moveList_, pos);
			if (pos.inCheck()) {
				// ���ʂ�����̏ꍇ�A������肩������������𐶐�
				last_ = std::remove_if(moveList_, last_, [&pos](const auto& move) {
					return !pos.pseudoLegalMoveIsEvasion(move, pos.pinnedBB());
				});
			}
		}
		else {
			last_ = generateMoves<Legal>(moveList_, pos);
		}
	}
	void operator ++ () { ++curr_; }
	Move move() const { return curr_->move; }
	size_t size() const { return static_cast<size_t>(last_ - moveList_); }
	ExtMove* begin() { return &moveList_[0]; }
	ExtMove* end() { return last_; }
	bool empty() const { return size() == 0; }

private:
	ExtMove moveList_[MaxLegalMoves];
	ExtMove* curr_;
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
		// ���������ɍ��v����G���g����Ԃ�
		for (size_t i = 0; i < sizeof(entries.entries) / sizeof(TTEntry); i++) {
			TTEntry& entry = entries.entries[i];
			if (entry.hash_high == 0 || generation != entry.generation) {
				// ��̃G���g�������������ꍇ
				entry.hash_high = hash_high;
				entry.depth = depth;
				entry.hand = hand;
				entry.pn = 1;
				entry.dn = 1;
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
							if (hand.isEqualOrSuperior(entry_rest.hand)) {
								if (entry_rest.pn == 0) {
									entry_rest.generation = generation;
									return entry_rest;
								}
							}
						}
					}
					entry.generation = generation;
					return entry;
				}
				// �D�z�֌W�𖞂����ǖʂɏؖ��ς݂̋ǖʂ�����ꍇ�A�����Ԃ�
				if (hand.isEqualOrSuperior(entry.hand)) {
					if (entry.pn == 0) {
						entry.generation = generation;
						return entry;
					}
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
		generation = (generation + 1) & 0xff;
	}

	int tt_mask = 0;
	void* tt_raw = nullptr;
	Cluster* tt = nullptr;
	int64_t num_clusters = 0;
	int64_t clusters_mask = 0;
	uint32_t generation = 0; // 256�ň������
};

static const constexpr int kInfinitePnDn = 100000000;
static const constexpr int kMaxDepth = MAX_PLY;

TranspositionTable transposition_table;

void DFPNwithTCA(Position& n, int thpn, int thdn/*, bool inc_flag*/, bool or_node, int depth, int64_t& searchedNode) {
	auto& entry = transposition_table.LookUp(n, or_node, depth);

	if (depth > kMaxDepth) {
		entry.pn = kInfinitePnDn;
		entry.dn = 0;
		//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		return;
	}

	// if (n is a terminal node) { handle n and return; }

	// 1��ǂ݃��[�`���ɂ��`�F�b�N
	if (or_node && !n.inCheck() && n.mateMoveIn1Ply()) {
		entry.pn = 0;
		entry.dn = kInfinitePnDn;
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
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		else {
			entry.pn = kInfinitePnDn;
			entry.dn = 0;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		return;

	case RepetitionLose:
		//cout << "RepetitionLose" << endl;
		// �A������̐����ɂ�镉��
		if (or_node) {
			entry.pn = kInfinitePnDn;
			entry.dn = 0;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		else {
			// �����͒ʂ�Ȃ��͂�
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		}
		return;

	case RepetitionDraw:
		//cout << "RepetitionDraw" << endl;
		// ���ʂ̐����
		// �����͒ʂ�Ȃ��͂�
		entry.pn = kInfinitePnDn;
		entry.dn = 0;
		//entry.minimum_distance = std::min(entry.minimum_distance, depth);
		return;
	}

	MovePicker move_picker(n, or_node);
	if (move_picker.empty()) {
		// n����[�m�[�h

		if (or_node) {
			// �����̎�Ԃł����ɓ��B�����ꍇ�͉���̎肪���������A
			entry.pn = kInfinitePnDn;
			entry.dn = 0;
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

	// minimum distance��ۑ�����
	// TODO(nodchip): ���̃^�C�~���O��minimum distance��ۑ�����̂����������m���߂�
	//entry.minimum_distance = std::min(entry.minimum_distance, depth);

	bool first_time = true;
	while (searchedNode < MAX_SEARCH_NODE) {
		++entry.num_searched;

		// determine whether thpn and thdn are increased.
		// if (n is a leaf) inc flag = false;
		/*if (entry.pn == 1 && entry.dn == 1) {
			inc_flag = false;
		}*/

		// if (n has an unproven old child) inc flag = true;
		/*for (const auto& move : move_picker) {
			// unproven old child�̒�`��minimum distance�����̃m�[�h�����������m�[�h���Ɨ������Ă���̂����ǁA
			// �����Ă��邩���M�Ȃ�
			const auto& child_entry = transposition_table.LookUpChildEntry(n, move, or_node, depth);
			if (entry.minimum_distance > child_entry.minimum_distance &&
				child_entry.pn != kInfinitePnDn &&
				child_entry.dn != kInfinitePnDn) {
				inc_flag = true;
				break;
			}
		}*/

		// expand and compute pn(n) and dn(n);
		if (or_node) {
			entry.pn = kInfinitePnDn;
			entry.dn = 0;
			for (const auto& move : move_picker) {
				const auto& child_entry = transposition_table.LookUpChildEntry(n, move, or_node, depth);
				entry.pn = std::min(entry.pn, child_entry.pn);
				entry.dn += child_entry.dn;
			}
			entry.dn = std::min(entry.dn, kInfinitePnDn);
		}
		else {
			entry.pn = 0;
			entry.dn = kInfinitePnDn;
			for (const auto& move : move_picker) {
				const auto& child_entry = transposition_table.LookUpChildEntry(n, move, or_node, depth);
				entry.pn += child_entry.pn;
				entry.dn = std::min(entry.dn, child_entry.dn);
			}
			entry.pn = std::min(entry.pn, kInfinitePnDn);
		}

		// if (first time && inc flag) {
		//   // increase thresholds
		//   thpn = max(thpn, pn(n) + 1);
		//   thdn = max(thdn, dn(n) + 1);
		// }
		/*if (first_time && inc_flag) {
			thpn = std::max(thpn, entry.pn + 1);
			thpn = std::min(thpn, kInfinitePnDn);
			thdn = std::max(thdn, entry.dn + 1);
			thdn = std::min(thdn, kInfinitePnDn);
		}*/

		// if (pn(n) ? thpn || dn(n) ? thdn)
		//   break; // termination condition is satisfied
		if (entry.pn >= thpn || entry.dn >= thdn) {
			break;
		}

		// first time = false;
		first_time = false;

		// find the best child n1 and second best child n2;
		// if (n is an OR node) { /* set new thresholds */
		//   thpn child = min(thpn, pn(n2) + 1);
		//   thdn child = thdn - dn(n) + dn(n1);
		// else {
		//   thpn child = thpn - pn(n) + pn(n1);
		//   thdn child = min(thdn, dn(n2) + 1);
		// }
		Move best_move;
		int thpn_child;
		int thdn_child;
		if (or_node) {
			// OR�m�[�h�ł͍ł��ؖ����������� = �ʂ̓������̌������Ȃ� = �l�܂��₷���m�[�h��I��
			int best_pn = kInfinitePnDn;
			int second_best_pn = kInfinitePnDn;
			int best_dn = 0;
			int best_num_search = INT_MAX;
			for (const auto& move : move_picker) {
				const auto& child_entry = transposition_table.LookUpChildEntry(n, move, or_node, depth);
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

			thpn_child = std::min(thpn, second_best_pn + 1);
			thdn_child = std::min(thdn - entry.dn + best_dn, kInfinitePnDn);
		}
		else {
			// AND�m�[�h�ł͍ł����ؐ��̏����� = ����̊|�����̏��Ȃ� = �s�l�݂������₷���m�[�h��I��
			int best_dn = kInfinitePnDn;
			int second_best_dn = kInfinitePnDn;
			int best_pn = 0;
			int best_num_search = INT_MAX;
			for (const auto& move : move_picker) {
				const auto& child_entry = transposition_table.LookUpChildEntry(n, move, or_node, depth);
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

			thpn_child = std::min(thpn - entry.pn + best_pn, kInfinitePnDn);
			thdn_child = std::min(thdn, second_best_dn + 1);
		}

		StateInfo state_info;
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
