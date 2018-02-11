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
	if (argc < 4) return 1;

	char* infile = argv[1];
	char* comparefile = argv[2];
	char* outfile = argv[3];

	// 入力ファイル
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


	// 比較ファイル
	std::ifstream ifs_comp(comparefile, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs_comp) {
		std::cerr << "Error: cannot open " << comparefile << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNumComp = ifs_comp.tellg() / sizeof(HuffmanCodedPosAndEval);

	std::cout << entryNumComp << std::endl;

	// 全て読む
	ifs_comp.seekg(std::ios_base::beg);
	HuffmanCodedPosAndEval *hcpevecComp = new HuffmanCodedPosAndEval[entryNumComp];
	ifs_comp.read(reinterpret_cast<char*>(hcpevecComp), sizeof(HuffmanCodedPosAndEval) * entryNumComp);
	ifs_comp.close();

	// ソート
	std::sort(hcpevecComp, hcpevecComp + entryNumComp);


	// 出力
	std::ofstream ofs(outfile, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << outfile << std::endl;
		exit(EXIT_FAILURE);
	}

	s64 count = 0;
	for (s64 i = 0, j = 0; i < uniqNum; i++) {
		while (hcpevecComp[j] < hcpevec[i] && j < entryNumComp - 1)
			j++;

		if (!(hcpevec[i] == hcpevecComp[j])) {
			ofs.write(reinterpret_cast<char*>(hcpevec + i), sizeof(HuffmanCodedPosAndEval));
			count++;
		}
	}
	std::cout << count << std::endl;

	return 0;
}