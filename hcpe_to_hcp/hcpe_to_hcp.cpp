#include "position.hpp"
#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cout << "hcpe_to_hcp hcpeFile hcpFile evalThreshold" << std::endl;
		return 0;
	}

	char* hcpeFile = argv[1];
	char* hcpFile = argv[2];
	int evalThreshold = std::stoi(argv[3]);

	std::ifstream ifs(hcpeFile, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << hcpeFile << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNum = ifs.tellg() / sizeof(HuffmanCodedPosAndEval);
	ifs.seekg(0);
	std::cout << "input num = " << entryNum << std::endl;

	const s64 num_per_file = 1024 * 1024 * 1024 / sizeof(HuffmanCodedPosAndEval); // 1GB
	HuffmanCodedPosAndEval *inhcpevec = new HuffmanCodedPosAndEval[num_per_file];

	std::ofstream ofs(hcpFile, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << hcpFile << std::endl;
		exit(EXIT_FAILURE);
	}

	s64 outNum = 0;
	s64 d = 0;
	for (s64 i = 0; i < entryNum; i++, d++) {
		if (i % num_per_file == 0) {
			s64 num = num_per_file;
			if (i >= num_per_file * (entryNum / num_per_file))
				num = entryNum - num_per_file * (entryNum / num_per_file);
			ifs.read(reinterpret_cast<char*>(inhcpevec), sizeof(HuffmanCodedPosAndEval) * num_per_file);
			d = 0;
		}
		HuffmanCodedPosAndEval& hcpe = inhcpevec[d];
		if (std::abs(hcpe.eval) < evalThreshold) {
			ofs.write(reinterpret_cast<char*>(&hcpe.hcp), sizeof(HuffmanCodedPos));
			outNum++;
		}
	}

	std::cout << "output num = " << outNum << std::endl;

	return 0;
}