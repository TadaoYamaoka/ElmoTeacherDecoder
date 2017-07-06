#include "position.hpp"

bool operator <(const HuffmanCodedPosAndEval& l, const HuffmanCodedPosAndEval& r) {
	const long long* lhcp = (const long long*)&l.hcp;
	const long long* rhcp = (const long long*)&r.hcp;

	for (int i = 0; i < 4; i++) {
		if (lhcp[i] < rhcp[i])
			return true;
		else if (lhcp[i] > rhcp[i])
			return false;
	}
	return false;
}

bool operator ==(const HuffmanCodedPosAndEval& l, const HuffmanCodedPosAndEval& r) {
	const long long* lhcp = (const long long*)&l.hcp;
	const long long* rhcp = (const long long*)&r.hcp;

	for (int i = 0; i < 4; i++) {
		if (lhcp[i] != rhcp[i])
			return false;
	}
	return true;
}

int main(int argc, char** argv)
{
	if (argc < 3) return 1;

	char* infile = argv[1];
	char* outfile = argv[2];

	std::ifstream ifs(infile, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << infile << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNum = ifs.tellg() / sizeof(HuffmanCodedPosAndEval);

	std::cout << entryNum << std::endl;

	// 全て読む
	ifs.seekg(std::ios_base::beg);
	HuffmanCodedPosAndEval *hcpevec = new HuffmanCodedPosAndEval[entryNum];
	ifs.read(reinterpret_cast<char*>(hcpevec), sizeof(HuffmanCodedPosAndEval) * entryNum);
	ifs.close();

	// ソート
	std::sort(hcpevec, hcpevec + entryNum);

	// uniq
	HuffmanCodedPosAndEval* end = std::unique(hcpevec, hcpevec + entryNum);
	const s64 uniqNum = end - hcpevec;

	std::cout << uniqNum << std::endl;

	// 出力
	std::ofstream ofs(outfile, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << outfile << std::endl;
		exit(EXIT_FAILURE);
	}
	ofs.write(reinterpret_cast<char*>(hcpevec), sizeof(HuffmanCodedPosAndEval) * uniqNum);

	return 0;
}