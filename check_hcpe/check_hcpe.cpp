#include "init.hpp"
#include "position.hpp"
#include "move.hpp"

HuffmanCodedPosAndEval* read_hcpes(const char* path, s64& entryNum)
{
	std::ifstream ifs(path, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << path << std::endl;
		exit(EXIT_FAILURE);
	}
	entryNum = ifs.tellg() / sizeof(HuffmanCodedPosAndEval);

	std::cout << entryNum << std::endl;

	// ‘S‚Ä“Ç‚Þ
	ifs.seekg(std::ios_base::beg);
	HuffmanCodedPosAndEval *hcpes = new HuffmanCodedPosAndEval[entryNum];
	ifs.read(reinterpret_cast<char*>(hcpes), sizeof(HuffmanCodedPosAndEval) * entryNum);
	ifs.close();

	return hcpes;
}

int main(int argc, char** argv)
{
	if (argc < 1) {
		std::cout << "check hcpe" << std::endl;
		return 0;
	}

	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();

	const char* src_hcpe_path = argv[1];

	s64 srcEntryNum;
	HuffmanCodedPosAndEval *hcpes = read_hcpes(src_hcpe_path, srcEntryNum);

	Position pos;
	for (s64 i = 0; i < srcEntryNum; i++) {
		pos.set(hcpes[i].hcp, nullptr);
		Move move = move16toMove(Move(hcpes[i].bestMove16), pos);
		if (!pos.isOK()) {
			__debugbreak();
		}
	}

}