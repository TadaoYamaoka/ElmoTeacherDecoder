#define BOOST_PYTHON_STATIC_LIB
#define BOOST_NUMPY_STATIC_LIB
#include <boost/python/numpy.hpp>
#include <numeric>
#include <algorithm>

#include "init.hpp"
#include "position.hpp"

namespace p = boost::python;
namespace np = boost::python::numpy;

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

void decode(np::ndarray ndhcpe, np::ndarray ndfeatures1, np::ndarray ndfeatures2, np::ndarray ndresult) {
	int len = (int)ndhcpe.shape(0);
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
		//position.print();
		float (*features2_hand)[ColorNum][MAX_PIECES_IN_HAND_SUM][SquareNum] = reinterpret_cast<float(*)[ColorNum][MAX_PIECES_IN_HAND_SUM][SquareNum]>(features2);
		for (Color c = Black; c < ColorNum; ++c) {
			for (PieceType pt = Pawn; pt < PieceTypeNum; ++pt) {
				Bitboard bb = position.bbOf(pt, c);
				for (Square sq = SQ11; sq < SquareNum; ++sq) {
					if (bb.isSet(sq)) {
						(*features1)[c][pt-1][sq] = 1.0f;
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
				std::fill_n((*features2_hand)[c][p], (int)SquareNum * num, 1.0f);
				p += MAX_PIECES_IN_HAND[hp];
			}
		}

		// is check
		if (position.inCheck()) {
			std::fill_n((*features2)[MAX_FEATURES2_HAND_NUM], SquareNum, 1.0f);
		}

		// game result
		if (hcpe->gameResult == Draw) {
			*result = 0.0f;
		}
		else {
			if (position.turn() == Black && hcpe->gameResult == WhiteWin ||
				position.turn() == White && hcpe->gameResult == BlackWin) {
				*result = -1.0f;
			}
			else {
				*result = 1.0f;
			}
		}
	}
}

BOOST_PYTHON_MODULE(hcp_decoder) {
	Py_Initialize();
	np::initialize();

	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();

	p::def("decode", decode);
}