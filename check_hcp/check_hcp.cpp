#include "init.hpp"
#include "position.hpp"

HuffmanCodedPos* read_hcps(const char* path, s64& entryNum)
{
	std::ifstream ifs(path, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << path << std::endl;
		exit(EXIT_FAILURE);
	}
	entryNum = ifs.tellg() / sizeof(HuffmanCodedPos);

	std::cout << entryNum << std::endl;

	// 全て読む
	ifs.seekg(std::ios_base::beg);
	HuffmanCodedPos *hcps = new HuffmanCodedPos[entryNum];
	ifs.read(reinterpret_cast<char*>(hcps), sizeof(HuffmanCodedPos) * entryNum);
	ifs.close();

	return hcps;
}

int main(int argc, char** argv)
{
	if (argc < 1) {
		std::cout << "check hcp" << std::endl;
		return 0;
	}

	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();

	const std::string src_hcp_path = argv[1];

	s64 srcEntryNum;
	HuffmanCodedPos *hcps = read_hcps(src_hcp_path.c_str(), srcEntryNum);

	std::vector<int> errs;

	Position pos;
	for (s64 i = 0; i < srcEntryNum; i++) {
		pos.set(hcps[i], nullptr);
		if (!pos.isOK()) {
			std::cout << i << std::endl;
			errs.push_back(i);
		}
	}

	if (errs.size() == 0)
		return 0;

	std::ofstream ofs_err(src_hcp_path + "_err", std::ios::binary);
	if (!ofs_err) {
		std::cerr << "Error: cannot open " << src_hcp_path + "_err" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::ofstream ofs(src_hcp_path + "_", std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << src_hcp_path + "_" << std::endl;
		exit(EXIT_FAILURE);
	}

	s64 pre_i = -1;
	for (s64 i : errs) {
		// エラー局面出力
		ofs_err.write(reinterpret_cast<char*>(&hcps[i]), sizeof(HuffmanCodedPos));

		// エラー局面を除く局面を出力
		ofs.write(reinterpret_cast<char*>(hcps + pre_i + 1), sizeof(HuffmanCodedPos) * (i - pre_i - 1));
		pre_i = i;
	}
	ofs.write(reinterpret_cast<char*>(hcps + pre_i + 1), sizeof(HuffmanCodedPos) * (srcEntryNum - pre_i - 1));
}