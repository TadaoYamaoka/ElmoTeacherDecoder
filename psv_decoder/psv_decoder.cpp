#define BOOST_PYTHON_STATIC_LIB
#define BOOST_NUMPY_STATIC_LIB
#include <boost/python/numpy.hpp>
#include <numeric>
#include <algorithm>

#include "init.hpp"
#include "position.hpp"
#include "move.hpp"

namespace p = boost::python;
namespace np = boost::python::numpy;

void print_sfen_from_psv(np::ndarray ndpsv) {
	const int len = (int)ndpsv.shape(0);
	PackedSfenValue *psv = reinterpret_cast<PackedSfenValue *>(ndpsv.get_data());
	Position position;
	for (int i = 0; i < len; i++, psv++) {
		position.set(psv->sfen, nullptr);

		// �w���� bit0..6 = �ړ����Square�Abit7..13 = �ړ�����Square(��ł��̂Ƃ��͋��)�Abit14..��ł����Abit15..���肩
		u16 bestMove16 = psv->move & 0x7f; // �ړ���
		if ((psv->move & 0x4000) == 0) {
			// �ړ�
			bestMove16 |= psv->move & 0x3f80;
		}
		else {
			// ��ł�
			bestMove16 |= (((psv->move >> 7) & 0x7f) + SquareNum - 1) << 7;
		}
		// ����
		if ((psv->move & 0x8000) != 0)
			bestMove16 |= 0x4000;

		const Move move = Move(bestMove16);

		std::cout << position.toSFEN() << " : " << psv->score << " : " << (int)psv->game_result << " : " << move.toUSI() << std::endl;
	}
}

BOOST_PYTHON_MODULE(psv_decoder) {
	Py_Initialize();
	np::initialize();

	initTable();
	Position::initZobrist();
	PackedSfen::init();

	p::def("print_sfen_from_psv", print_sfen_from_psv);
}