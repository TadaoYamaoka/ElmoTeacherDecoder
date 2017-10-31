#include "position.hpp"

#include <algorithm>
#include <random>
#include <string>
#include <climits>

int main(int argc, char** argv)
{
	if (argc < 4) {
		return 1;
	}

	char* infile = argv[1];
	s64 num_per_file = std::atoi(argv[2]);
	int divNum = std::atoi(argv[3]);

	std::ifstream ifs(infile, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << infile << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNum = ifs.tellg() / sizeof(HuffmanCodedPosWithHistoryAndEval);

	std::cout << entryNum << std::endl;

	if (entryNum > UINT_MAX) {
		std::cerr << "Error: entryNum > UINT_MAX" << std::endl;
		return 1;
	}

	// インデックス配列
	u32* indexes = new u32[entryNum];
	for (s64 i = 0; i < entryNum; i++) {
		indexes[i] = (u32)i;
	}

	// shuffle
	std::random_device seed_gen;
	std::mt19937 engine(seed_gen());
	std::shuffle(indexes, indexes + entryNum, engine);

	// ファイルサイズが物理メモリを超えても処理できるように分割して処理(分割数は適宜調整する)
	const s64 blockNum = (entryNum + divNum - 1) / divNum;
	HuffmanCodedPosWithHistoryAndEval *inhcphevec = new HuffmanCodedPosWithHistoryAndEval[blockNum];

	// 出力
	HuffmanCodedPosWithHistoryAndEval *hcphevec = new HuffmanCodedPosWithHistoryAndEval[num_per_file];
	for (s64 i = 0; i < (entryNum + num_per_file - 1) / num_per_file; i++) {
		std::ostringstream sout;
		sout << infile << "-" << std::setfill('0') << std::setw(2) << i + 1;
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

		// 分割して処理
		for (int j = 0; j < divNum; j++) {
			// 読み込み
			s64 readNum = blockNum;
			if (j + 1 == divNum) {
				readNum = entryNum - blockNum * j;
			}
			ifs.seekg(sizeof(HuffmanCodedPosWithHistoryAndEval) * blockNum * j);
			ifs.read(reinterpret_cast<char*>(inhcphevec), sizeof(HuffmanCodedPosWithHistoryAndEval) * readNum);

			size_t p = num_per_file * i;
			for (int k = 0; k < num; k++, p++) {
				const u32 index = indexes[p];
				if (blockNum * j <= index && index < blockNum * (j + 1))
					hcphevec[k] = inhcphevec[index - blockNum * j];
			}
		}

		ofs.write(reinterpret_cast<char*>(hcphevec), sizeof(HuffmanCodedPosWithHistoryAndEval) * num);
	}

	return 0;
}