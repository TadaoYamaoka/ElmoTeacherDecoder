#include "position.hpp"

#include <algorithm>
#include <random>
#include <string>

int main(int argc, char** argv)
{
	if (argc < 3) {
		return 1;
	}

	char* infile = argv[1];
	int num_per_file = std::atoi(argv[2]);

	std::ifstream ifs(infile, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << infile << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNum = ifs.tellg() / sizeof(HuffmanCodedPosAndEval);

	std::cout << entryNum << std::endl;

	// ëSÇƒì«Çﬁ
	ifs.seekg(std::ios_base::beg);
	HuffmanCodedPosAndEval *hcpevec = new HuffmanCodedPosAndEval[entryNum];
	ifs.read(reinterpret_cast<char*>(hcpevec), sizeof(HuffmanCodedPosAndEval) * entryNum);
	ifs.close();

	// shuffle
	std::random_device seed_gen;
	std::mt19937 engine(seed_gen());
	std::shuffle(hcpevec, hcpevec + entryNum, engine);

	// èoóÕ
	for (int i = 0; i < (entryNum + num_per_file - 1) / num_per_file; i++, hcpevec += num_per_file) {
		std::ostringstream sout;
		sout << infile << "-" << std::setfill('0') << std::setw(3) << i + 1;
		std::ofstream ofs(sout.str(), std::ios::binary);
		if (!ofs) {
			std::cerr << "Error: cannot open " << sout.str() << std::endl;
			exit(EXIT_FAILURE);
		}
		int num = num_per_file;
		if (num_per_file * (i + 1) > entryNum) {
			num = entryNum - num_per_file * i;
		}
		std::cout << num << std::endl;
		ofs.write(reinterpret_cast<char*>(hcpevec), sizeof(HuffmanCodedPosAndEval) * num);
	}

	return 0;
}