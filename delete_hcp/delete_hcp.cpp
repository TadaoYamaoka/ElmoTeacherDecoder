#include "position.hpp"
#include <iostream>
#include <set>

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
	HuffmanCodedPos *hcpes = new HuffmanCodedPos[entryNum];
	ifs.read(reinterpret_cast<char*>(hcpes), sizeof(HuffmanCodedPos) * entryNum);
	ifs.close();

	return hcpes;
}

bool operator<(const HuffmanCodedPos& l, const HuffmanCodedPos& r)
{
	for (size_t i = 0; i < 32; i++) {
		if (l.data[i] < r.data[i])
			return true;
		if (l.data[i] > r.data[i])
			return false;
	}
	return false;
}

int main(int argc, char** argv)
{
	if (argc < 3) {
		std::cout << "delete_hcp src del dst" << std::endl;
		return 0;
	}

	const char* src_hcp_path = argv[1];
	const char* delete_hcp_path = argv[2];
	const char* dst_hcp_path = argv[3];

	s64 srcEntryNum;
	s64 delEntryNum;
	HuffmanCodedPos *srcHcpes = read_hcps(src_hcp_path, srcEntryNum);
	HuffmanCodedPos *delHcpes = read_hcps(delete_hcp_path, delEntryNum);

	std::ofstream ofs(dst_hcp_path, std::ios::binary);

	std::set<HuffmanCodedPos> delSet;
	for (s64 i = 0; i < delEntryNum; i++) {
		delSet.insert(delHcpes[i]);
	}

	s64 dstEntryNum = 0;
	for (s64 i = 0; i < srcEntryNum; i++) {
		if (delSet.find(srcHcpes[i]) == delSet.end()) {
			ofs.write(reinterpret_cast<char*>(&srcHcpes[i]), sizeof(HuffmanCodedPos));
			dstEntryNum++;
		}
	}
	std::cout << dstEntryNum << std::endl;
}