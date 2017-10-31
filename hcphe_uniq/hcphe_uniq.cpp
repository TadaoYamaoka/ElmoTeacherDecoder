#include "position.hpp"

bool operator <(const HuffmanCodedPosWithHistoryAndEval& l, const HuffmanCodedPosWithHistoryAndEval& r) {
	const long long* lhcp = (const long long*)&l.hcp;
	const long long* rhcp = (const long long*)&r.hcp;

	for (int i = 0; i < 4; i++) {
		if (lhcp[i] < rhcp[i])
			return true;
		else if (lhcp[i] > rhcp[i])
			return false;
	}

	const u16* lhistMove = (const u16*)l.historyMove16;
	const u16* rhistMove = (const u16*)r.historyMove16;

	for (int i = 0; i < 7; i++) {
		if (lhistMove[i] < rhistMove[i])
			return true;
		else if (lhistMove[i] > rhistMove[i])
			return false;
	}

	const u16* lbestMove16 = (const u16*)l.bestMove16;
	const u16* rbestMove16 = (const u16*)r.bestMove16;

	if (lbestMove16 < rbestMove16)
		return true;
	else if (lbestMove16 > rbestMove16)
		return false;

	return false;
}

bool operator ==(const HuffmanCodedPosWithHistoryAndEval& l, const HuffmanCodedPosWithHistoryAndEval& r) {
	const long long* lhcp = (const long long*)&l.hcp;
	const long long* rhcp = (const long long*)&r.hcp;

	for (int i = 0; i < 4; i++) {
		if (lhcp[i] != rhcp[i])
			return false;
	}

	const u16* lhistMove = (const u16*)l.historyMove16;
	const u16* rhistMove = (const u16*)r.historyMove16;

	for (int i = 0; i < 7; i++) {
		if (lhistMove[i] != rhistMove[i])
			return false;
	}

	const u16* lbestMove16 = (const u16*)l.bestMove16;
	const u16* rbestMove16 = (const u16*)r.bestMove16;

	if (lbestMove16 != rbestMove16)
		return false;

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
	const s64 entryNum = ifs.tellg() / sizeof(HuffmanCodedPosWithHistoryAndEval);

	std::cout << entryNum << std::endl;

	// 全て読む
	ifs.seekg(std::ios_base::beg);
	HuffmanCodedPosWithHistoryAndEval *hcphevec = new HuffmanCodedPosWithHistoryAndEval[entryNum];
	ifs.read(reinterpret_cast<char*>(hcphevec), sizeof(HuffmanCodedPosWithHistoryAndEval) * entryNum);
	ifs.close();

	// ソート
	std::sort(hcphevec, hcphevec + entryNum);

	// uniq
	HuffmanCodedPosWithHistoryAndEval* end = std::unique(hcphevec, hcphevec + entryNum);
	const s64 uniqNum = end - hcphevec;

	std::cout << uniqNum << std::endl;

	// 出力
	std::ofstream ofs(outfile, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << outfile << std::endl;
		exit(EXIT_FAILURE);
	}
	ofs.write(reinterpret_cast<char*>(hcphevec), sizeof(HuffmanCodedPosWithHistoryAndEval) * uniqNum);

	return 0;
}