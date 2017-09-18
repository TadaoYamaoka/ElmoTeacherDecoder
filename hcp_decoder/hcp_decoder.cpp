﻿#define BOOST_PYTHON_STATIC_LIB
#define BOOST_NUMPY_STATIC_LIB
#include <boost/python/numpy.hpp>
#include <numeric>
#include <algorithm>

#include "init.hpp"
#include "position.hpp"
#include "move.hpp"

namespace p = boost::python;
namespace np = boost::python::numpy;

#define LEN(array) (sizeof(array) / sizeof(array[0]))

const int MAX_HPAWN_NUM = 8; // 歩の持ち駒の上限
const int MAX_HLANCE_NUM = 4;
const int MAX_HKNIGHT_NUM = 4;
const int MAX_HSILVER_NUM = 4;
const int MAX_HGOLD_NUM = 4;
const int MAX_HBISHOP_NUM = 2;
const int MAX_HROOK_NUM = 2;

const u32 MAX_PIECES_IN_HAND[] = {
	MAX_HPAWN_NUM, // PAWN
	MAX_HLANCE_NUM, // LANCE
	MAX_HKNIGHT_NUM, // KNIGHT
	MAX_HSILVER_NUM, // SILVER
	MAX_HGOLD_NUM, // GOLD
	MAX_HBISHOP_NUM, // BISHOP
	MAX_HROOK_NUM, // ROOK
};
const u32 MAX_PIECES_IN_HAND_SUM = MAX_HPAWN_NUM + MAX_HLANCE_NUM + MAX_HKNIGHT_NUM + MAX_HSILVER_NUM + MAX_HGOLD_NUM + MAX_HBISHOP_NUM + MAX_HROOK_NUM;
const u32 MAX_FEATURES2_HAND_NUM = (int)ColorNum * MAX_PIECES_IN_HAND_SUM;
const u32 MAX_FEATURES2_NUM = MAX_FEATURES2_HAND_NUM + 1/*王手*/;

// 移動の定数
enum MOVE_DIRECTION {
	UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN, DOWN_LEFT, DOWN_RIGHT,
	UP_PROMOTE, UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, LEFT_PROMOTE, RIGHT_PROMOTE, DOWN_PROMOTE, DOWN_LEFT_PROMOTE, DOWN_RIGHT_PROMOTE
};

const MOVE_DIRECTION MOVE_DIRECTION_PROMOTED[] = {
	UP_PROMOTE, UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, LEFT_PROMOTE, RIGHT_PROMOTE, DOWN_PROMOTE, DOWN_LEFT_PROMOTE, DOWN_RIGHT_PROMOTE
};

const MOVE_DIRECTION PAWN_MOVE_DIRECTION[] = { UP, UP_PROMOTE };
const MOVE_DIRECTION LANCE_MOVE_DIRECTION[] = { UP, UP_PROMOTE };
const MOVE_DIRECTION KNIGHT_MOVE_DIRECTION[] = {
	UP_LEFT, UP_RIGHT,
	UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE };
const MOVE_DIRECTION SILVER_MOVE_DIRECTION[] = {
	UP, UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT,
	UP_PROMOTE, UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, DOWN_LEFT_PROMOTE, DOWN_RIGHT_PROMOTE };
const MOVE_DIRECTION BISHOP_MOVE_DIRECTION[] = {
	UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT,
	UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, DOWN_LEFT_PROMOTE, DOWN_RIGHT_PROMOTE };
const MOVE_DIRECTION ROOK_MOVE_DIRECTION[] = {
	UP, LEFT, RIGHT, DOWN,
	UP_PROMOTE, LEFT_PROMOTE, RIGHT_PROMOTE, DOWN_PROMOTE };
const MOVE_DIRECTION GOLD_MOVE_DIRECTION[] = {
	UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN,
	UP_PROMOTE, UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, LEFT_PROMOTE, RIGHT_PROMOTE, DOWN };
const MOVE_DIRECTION KING_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN, DOWN_LEFT, DOWN_RIGHT };
const MOVE_DIRECTION PROM_PAWN_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN };
const MOVE_DIRECTION PROM_LANCE_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN };
const MOVE_DIRECTION PROM_KNIGHT_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN };
const MOVE_DIRECTION PROM_SILVER_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN };
const MOVE_DIRECTION PROM_BISHOP_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN, DOWN_LEFT, DOWN_RIGHT };
const MOVE_DIRECTION PROM_ROOK_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN, DOWN_LEFT, DOWN_RIGHT };

const int PIECE_MOVE_DIRECTION_INDEX[][16] = {
	{ 0, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1 }, // PAWN
	{ 0, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1 }, // LANCE
	{ 0, 1, 2, 3, 4, -1, -1, -1, -1, 5, 6, 7, 8, -1, -1, -1 }, // KNIGHT
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, -1, 13, 14 }, // SILVER
	{ 0, 1, 2, 3, 4, 5, 6, 7, -1, 8, 9, 10, 11, -1, 12, 13 }, // BISHOP
	{ 0, -1, -1, 1, 2, 3, -1, -1, 4, -1, -1, 5, 6, 7, -1, -1 }, // ROOK
	{ 0, 1, 2, 3, 4, 5, -1, -1, 6, 7, 8, 9, 10, -1, -1, -1 }, // GOLD
	{ 0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1 }, // KING
	{ 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_PAWN
	{ 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_LANCE
	{ 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_KNIGHT
	{ 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_SILVER
	{ 0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_BISHOP
	{ 0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_ROOK
};

// classification label
const int PAWN_MOVE_DIRECTION_LABEL = 0;
const int LANCE_MOVE_DIRECTION_LABEL = LEN(PAWN_MOVE_DIRECTION);
const int KNIGHT_MOVE_DIRECTION_LABEL = LANCE_MOVE_DIRECTION_LABEL + LEN(LANCE_MOVE_DIRECTION);
const int SILVER_MOVE_DIRECTION_LABEL = KNIGHT_MOVE_DIRECTION_LABEL + LEN(KNIGHT_MOVE_DIRECTION);
const int BISHOP_MOVE_DIRECTION_LABEL = SILVER_MOVE_DIRECTION_LABEL + LEN(GOLD_MOVE_DIRECTION);
const int ROOK_MOVE_DIRECTION_LABEL = BISHOP_MOVE_DIRECTION_LABEL + LEN(BISHOP_MOVE_DIRECTION);
const int GOLD_MOVE_DIRECTION_LABEL = ROOK_MOVE_DIRECTION_LABEL + LEN(SILVER_MOVE_DIRECTION);
const int KING_MOVE_DIRECTION_LABEL = GOLD_MOVE_DIRECTION_LABEL + LEN(ROOK_MOVE_DIRECTION);
const int PROM_PAWN_MOVE_DIRECTION_LABEL = KING_MOVE_DIRECTION_LABEL + LEN(KING_MOVE_DIRECTION);
const int PROM_LANCE_MOVE_DIRECTION_LABEL = PROM_PAWN_MOVE_DIRECTION_LABEL + LEN(PROM_PAWN_MOVE_DIRECTION);
const int PROM_KNIGHT_MOVE_DIRECTION_LABEL = PROM_LANCE_MOVE_DIRECTION_LABEL + LEN(PROM_LANCE_MOVE_DIRECTION);
const int PROM_SILVER_MOVE_DIRECTION_LABEL = PROM_KNIGHT_MOVE_DIRECTION_LABEL + LEN(PROM_KNIGHT_MOVE_DIRECTION);
const int PROM_BISHOP_MOVE_DIRECTION_LABEL = PROM_SILVER_MOVE_DIRECTION_LABEL + LEN(PROM_SILVER_MOVE_DIRECTION);
const int PROM_ROOK_MOVE_DIRECTION_LABEL = PROM_BISHOP_MOVE_DIRECTION_LABEL + LEN(PROM_BISHOP_MOVE_DIRECTION);
const int MOVE_DIRECTION_LABEL_NUM = PROM_ROOK_MOVE_DIRECTION_LABEL + LEN(PROM_ROOK_MOVE_DIRECTION);

const int PIECE_MOVE_DIRECTION_LABEL[] = {
	0,
	PAWN_MOVE_DIRECTION_LABEL, LANCE_MOVE_DIRECTION_LABEL, KNIGHT_MOVE_DIRECTION_LABEL, SILVER_MOVE_DIRECTION_LABEL,
	BISHOP_MOVE_DIRECTION_LABEL, ROOK_MOVE_DIRECTION_LABEL,
	GOLD_MOVE_DIRECTION_LABEL,
	KING_MOVE_DIRECTION_LABEL,
	PROM_PAWN_MOVE_DIRECTION_LABEL, PROM_LANCE_MOVE_DIRECTION_LABEL, PROM_KNIGHT_MOVE_DIRECTION_LABEL, PROM_SILVER_MOVE_DIRECTION_LABEL,
	PROM_BISHOP_MOVE_DIRECTION_LABEL, PROM_ROOK_MOVE_DIRECTION_LABEL
};

// make input features
inline void make_input_features(const Position& position, float(*features1)[ColorNum][PieceTypeNum - 1][SquareNum], float(*features2)[MAX_FEATURES2_NUM][SquareNum]) {
	float(*features2_hand)[ColorNum][MAX_PIECES_IN_HAND_SUM][SquareNum] = reinterpret_cast<float(*)[ColorNum][MAX_PIECES_IN_HAND_SUM][SquareNum]>(features2);
	for (Color c = Black; c < ColorNum; ++c) {
		// 白の場合、色を反転
		Color c2 = c;
		if (position.turn() == White) {
			c2 = oppositeColor(c);
		}

		for (PieceType pt = Pawn; pt < PieceTypeNum; ++pt) {
			Bitboard bb = position.bbOf(pt, c);
			for (Square sq = SQ11; sq < SquareNum; ++sq) {
				// 白の場合、盤面を180度回転
				Square sq2 = sq;
				if (position.turn() == White) {
					sq2 = SQ99 - sq;
				}

				if (bb.isSet(sq)) {
					(*features1)[c2][pt - 1][sq2] = 1.0f;
				}
			}
		}
		// hand
		Hand hand = position.hand(c);
		int p = 0;
		for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
			u32 num = hand.numOf(hp);
			if (num >= MAX_PIECES_IN_HAND[hp]) {
				num = MAX_PIECES_IN_HAND[hp];
			}
			std::fill_n((*features2_hand)[c2][p], (int)SquareNum * num, 1.0f);
			p += MAX_PIECES_IN_HAND[hp];
		}
	}

	// is check
	if (position.inCheck()) {
		std::fill_n((*features2)[MAX_FEATURES2_HAND_NUM], SquareNum, 1.0f);
	}
}

// make result
inline void make_result(const GameResult gameResult, const Position& position, float *result) {
	if (gameResult == Draw) {
		*result = 0.0f;
	}
	else {
		if (position.turn() == Black && gameResult == WhiteWin ||
			position.turn() == White && gameResult == BlackWin) {
			*result = -1.0f;
		}
		else {
			*result = 1.0f;
		}
	}
}

// make move
inline void make_move(const u16 bestMove16, const Position& position, int *move) {
	// see: move.hpp : 30
	// xxxxxxxx x1111111  移動先
	// xx111111 1xxxxxxx  移動元。駒打ちの際には、PieceType + SquareNum - 1
	// x1xxxxxx xxxxxxxx  1 なら成り
	u16 to_sq = bestMove16 & 0b1111111;
	u16 from_sq = (bestMove16 >> 7) & 0b1111111;

	// move direction
	int move_direction_label;
	if (from_sq < SquareNum) {
		// 駒の種類
		PieceType move_piece = pieceToPieceType(position.piece((Square)from_sq));

		// 白の場合、盤面を180度回転
		if (position.turn() == White) {
			to_sq = (u16)SQ99 - to_sq;
			from_sq = (u16)SQ99 - from_sq;
		}

		const div_t to_d = div(to_sq, 9);
		const int to_x = to_d.quot;
		const int to_y = to_d.rem;
		const div_t from_d = div(from_sq, 9);
		const int from_x = from_d.quot;
		const int from_y = from_d.rem;
		const int dir_x = from_x - to_x;
		const int dir_y = to_y - from_y;

		MOVE_DIRECTION move_direction;
		if (dir_y < 0 && dir_x == 0) {
			move_direction = UP;
		}
		else if (dir_y < 0 && dir_x < 0) {
			move_direction = UP_LEFT;
		}
		else if (dir_y < 0 && dir_x > 0) {
			move_direction = UP_RIGHT;
		}
		else if (dir_y == 0 && dir_x < 0) {
			move_direction = LEFT;
		}
		else if (dir_y == 0 && dir_x > 0) {
			move_direction = RIGHT;
		}
		else if (dir_y > 0 && dir_x == 0) {
			move_direction = DOWN;
		}
		else if (dir_y > 0 && dir_x < 0) {
			move_direction = DOWN_LEFT;
		}
		else if (dir_y > 0 && dir_x > 0) {
			move_direction = DOWN_RIGHT;
		}

		// promote
		if ((bestMove16 & 0b100000000000000) > 0) {
			move_direction = MOVE_DIRECTION_PROMOTED[move_direction];
		}
		move_direction_label = PIECE_MOVE_DIRECTION_LABEL[move_piece] + PIECE_MOVE_DIRECTION_INDEX[move_piece][move_direction];
	}
	// 持ち駒の場合
	else {
		// 白の場合、盤面を180度回転
		if (position.turn() == White) {
			to_sq = (u16)SQ99 - to_sq;
		}
		const int hand_piece = from_sq - (int)SquareNum;
		move_direction_label = MOVE_DIRECTION_LABEL_NUM + hand_piece;
	}

	*move = 9 * 9 * move_direction_label + to_sq;
}

/*
	HuffmanCodedPosAndEvalの配列からpolicy networkの入力ミニバッチに変換する。
	ndhcpe : Python側で以下の構造体を定義して、その配列を入力する。
		HuffmanCodedPosAndEval = np.dtype([
			('hcp', np.uint8, 32),
			('eval', np.int16),
			('bestMove16', np.uint16),
			('gameResult', np.uint8),
			('dummy', np.uint8),
		])
	ndfeatures1, ndfeatures2, ndresult : 変換結果を受け取る。Python側でnp.emptyで事前に領域を確保する。
*/
void decode_with_result(np::ndarray ndhcpe, np::ndarray ndfeatures1, np::ndarray ndfeatures2, np::ndarray ndresult) {
	const int len = (int)ndhcpe.shape(0);
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());
	float (*features1)[ColorNum][PieceTypeNum-1][SquareNum] = reinterpret_cast<float(*)[ColorNum][PieceTypeNum-1][SquareNum]>(ndfeatures1.get_data());
	float (*features2)[MAX_FEATURES2_NUM][SquareNum] = reinterpret_cast<float(*)[MAX_FEATURES2_NUM][SquareNum]>(ndfeatures2.get_data());
	float *result = reinterpret_cast<float *>(ndresult.get_data());

	// set all zero
	std::fill_n((float*)features1, (int)ColorNum * (PieceTypeNum-1) * (int)SquareNum * len, 0.0f);
	std::fill_n((float*)features2, MAX_FEATURES2_NUM * (int)SquareNum * len, 0.0f);

	Position position;
	for (int i = 0; i < len; i++, hcpe++, features1++, features2++, result++) {
		position.set(hcpe->hcp, nullptr);

		// input features
		make_input_features(position, features1, features2);

		// game result
		make_result(hcpe->gameResult, position, result);
	}
}

void decode_with_move(np::ndarray ndhcpe, np::ndarray ndfeatures1, np::ndarray ndfeatures2, np::ndarray ndmove) {
	const int len = (int)ndhcpe.shape(0);
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());
	float(*features1)[ColorNum][PieceTypeNum - 1][SquareNum] = reinterpret_cast<float(*)[ColorNum][PieceTypeNum - 1][SquareNum]>(ndfeatures1.get_data());
	float(*features2)[MAX_FEATURES2_NUM][SquareNum] = reinterpret_cast<float(*)[MAX_FEATURES2_NUM][SquareNum]>(ndfeatures2.get_data());
	int *move = reinterpret_cast<int *>(ndmove.get_data());

	// set all zero
	std::fill_n((float*)features1, (int)ColorNum * (PieceTypeNum - 1) * (int)SquareNum * len, 0.0f);
	std::fill_n((float*)features2, MAX_FEATURES2_NUM * (int)SquareNum * len, 0.0f);

	Position position;
	for (int i = 0; i < len; i++, hcpe++, features1++, features2++, move++) {
		position.set(hcpe->hcp, nullptr);

		// input features
		make_input_features(position, features1, features2);

		// move
		make_move(hcpe->bestMove16, position, move);
	}
}

void decode_with_value(np::ndarray ndhcpe, np::ndarray ndfeatures1, np::ndarray ndfeatures2, np::ndarray ndvalue, np::ndarray ndmove, np::ndarray ndresult) {
	const int len = (int)ndhcpe.shape(0);
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());
	float(*features1)[ColorNum][PieceTypeNum - 1][SquareNum] = reinterpret_cast<float(*)[ColorNum][PieceTypeNum - 1][SquareNum]>(ndfeatures1.get_data());
	float(*features2)[MAX_FEATURES2_NUM][SquareNum] = reinterpret_cast<float(*)[MAX_FEATURES2_NUM][SquareNum]>(ndfeatures2.get_data());
	float *value = reinterpret_cast<float *>(ndvalue.get_data());
	int *move = reinterpret_cast<int *>(ndmove.get_data());
	float *result = reinterpret_cast<float *>(ndresult.get_data());

	// set all zero
	std::fill_n((float*)features1, (int)ColorNum * (PieceTypeNum - 1) * (int)SquareNum * len, 0.0f);
	std::fill_n((float*)features2, MAX_FEATURES2_NUM * (int)SquareNum * len, 0.0f);

	Position position;
	for (int i = 0; i < len; i++, hcpe++, features1++, features2++, value++, move++, result++) {
		position.set(hcpe->hcp, nullptr);

		// input features
		make_input_features(position, features1, features2);

		// eval
		*value = tanh((float)hcpe->eval * 0.00067492f);

		// move
		make_move(hcpe->bestMove16, position, move);

		// game result
		make_result(hcpe->gameResult, position, result);
	}
}

void print_sfen_from_hcp(np::ndarray ndhcp) {
	const int len = (int)ndhcp.shape(0);
	HuffmanCodedPos *hcp = reinterpret_cast<HuffmanCodedPos *>(ndhcp.get_data());
	Position position;
	for (int i = 0; i < len; i++, hcp++) {
		position.set(*hcp, nullptr);
		std::cout << position.toSFEN() << std::endl;
	}
}

void print_sfen_from_hcpe(np::ndarray ndhcpe) {
	const int len = (int)ndhcpe.shape(0);
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());
	Position position;
	for (int i = 0; i < len; i++, hcpe++) {
		position.set(hcpe->hcp, nullptr);

		const Move move = Move(hcpe->bestMove16);

		std::cout << position.toSFEN() << " : " << hcpe->eval << " : " << hcpe->gameResult << " : " << move.toUSI() << std::endl;
	}
}

bool operator<(const HuffmanCodedPos& l, const HuffmanCodedPos& r) {
	for (int i = 0; i < 32; i++) {
		if (l.data[i] < r.data[i]) {
			return true;
		}
	}
	return false;
}
void check_duplicates(np::ndarray ndhcpe) {
	const int len = (int)ndhcpe.shape(0);
	std::cout << "len:" << len << std::endl;
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());

	std::set<HuffmanCodedPos> set;
	int dup_cnt = 0;
	for (int i = 0; i < len; i++, hcpe++) {
		if (set.find(hcpe->hcp) == set.end()) {
			set.insert(hcpe->hcp);
		}
		else {
			dup_cnt++;
		}
	}
	std::cout << "dup_cnt:" << dup_cnt << std::endl;
}

void sfen_to_hcp(const char* sfen, np::ndarray ndhcp) {
	HuffmanCodedPos *hcp = reinterpret_cast<HuffmanCodedPos *>(ndhcp.get_data());
	Position position;
	position.set(sfen, nullptr);
	*hcp = position.toHuffmanCodedPos();
}

BOOST_PYTHON_MODULE(hcp_decoder) {
	Py_Initialize();
	np::initialize();

	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();

	p::def("decode_with_result", decode_with_result);
	p::def("decode_with_move", decode_with_move);
	p::def("decode_with_value", decode_with_value);
	p::def("print_sfen_from_hcp", print_sfen_from_hcp);
	p::def("print_sfen_from_hcpe", print_sfen_from_hcpe);
	p::def("check_duplicates", check_duplicates);
	p::def("sfen_to_hcp", sfen_to_hcp);
}