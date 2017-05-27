#define BOOST_PYTHON_STATIC_LIB
#define BOOST_NUMPY_STATIC_LIB
#include <boost/python/numpy.hpp>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

#include "piece.hpp"
#include "color.hpp"
#include "position.hpp"

namespace p = boost::python;
namespace np = boost::python::numpy;

void decode(np::ndarray ndhcpe, np::ndarray ndresult) {
	int nd = ndhcpe.get_nd();
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());
	int *result = reinterpret_cast<int *>(ndresult.get_data());
	Position position;
	for (int i = 0; i < nd; i++, hcpe++, result++) {
		position.set(hcpe->hcp);
		*result = hcpe->gameResult;
	}
}

BOOST_PYTHON_MODULE(hcp_decoder) {
	Py_Initialize();
	np::initialize();
	p::def("decode", decode);
}