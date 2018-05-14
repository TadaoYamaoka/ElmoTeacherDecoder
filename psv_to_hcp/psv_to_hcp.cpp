#include "init.hpp"
#include "position.hpp"
#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cout << "psv_to_hcp psvFile hcpFile evalThreshold" << std::endl;
		return 0;
	}

	char* psvFile = argv[1];
	char* hcpFile = argv[2];
	int evalThreshold = std::stoi(argv[3]);

	std::ifstream ifs(psvFile, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << psvFile << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNum = ifs.tellg() / sizeof(PackedSfenValue);
	ifs.seekg(0);
	std::cout << "input num = " << entryNum << std::endl;

	const s64 num_per_file = 1024 * 1024 * 1024 / sizeof(PackedSfenValue); // 1GB
	PackedSfenValue *inpsvvec = new PackedSfenValue[num_per_file];

	std::ofstream ofs(hcpFile, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << hcpFile << std::endl;
		exit(EXIT_FAILURE);
	}

	initTable();
	Position::initZobrist();
	PackedSfen::init();

	s64 outNum = 0;
	s64 d = 0;
	Position pos;
	for (s64 i = 0; i < entryNum; i++, d++) {
		if (i % num_per_file == 0) {
			s64 num = num_per_file;
			if (i >= num_per_file * (entryNum / num_per_file))
				num = entryNum - num_per_file * (entryNum / num_per_file);
			ifs.read(reinterpret_cast<char*>(inpsvvec), sizeof(PackedSfenValue) * num_per_file);
			d = 0;
		}
		PackedSfenValue& psv = inpsvvec[d];
		if (std::abs(psv.score) < evalThreshold) {
			pos.set(psv.sfen, nullptr);
			ofs.write(reinterpret_cast<char*>(&pos.toHuffmanCodedPos()), sizeof(HuffmanCodedPos));
			outNum++;
		}
	}

	std::cout << "output num = " << outNum << std::endl;

	return 0;
}