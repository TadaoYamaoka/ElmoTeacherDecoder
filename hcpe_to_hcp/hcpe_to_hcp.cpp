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

	if (entryNum > UINT_MAX) {
		std::cerr << "Error: entryNum > UINT_MAX" << std::endl;
		return 1;
	}

	HuffmanCodedPosAndEval *inhcpevec = new HuffmanCodedPosAndEval[entryNum];
	ifs.read(reinterpret_cast<char*>(inhcpevec), sizeof(HuffmanCodedPosAndEval) * entryNum);

	std::ofstream ofs(hcpFile, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << hcpFile << std::endl;
		exit(EXIT_FAILURE);
	}

	s64 outNum = 0;
	for (s64 i = 0; i < entryNum; i++) {
		HuffmanCodedPosAndEval& hcpe = inhcpevec[i];
		if (std::abs(hcpe.eval) < evalThreshold) {
			ofs.write(reinterpret_cast<char*>(&hcpe.hcp), sizeof(HuffmanCodedPos));
			outNum++;
		}
	}

	std::cout << "input num = " << entryNum << std::endl;
	std::cout << "output num = " << outNum << std::endl;

	return 0;
}