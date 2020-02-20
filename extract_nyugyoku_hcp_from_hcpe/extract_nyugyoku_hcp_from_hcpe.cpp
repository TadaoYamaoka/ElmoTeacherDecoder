#include "init.hpp"
#include "position.hpp"

#include <iostream>

bool before_nyugyoku(const Position& pos) {
	// 入玉宣言勝ちの直前の局面か判定する
	// 3段目に7～8枚、25～26点

	// CSA ルールでは、一 から 六 の条件を全て満たすとき、入玉勝ち宣言が出来る。
	// 判定が高速に出来るものから順に判定していく事にする。
	// 一 宣言側の手番である。
	// 六 宣言側の持ち時間が残っている。
	// 五 宣言側の玉に王手がかかっていない。

	// 王手がかかっていても良い
	/*if (pos.inCheck())
		return false;*/

	const Color us = pos.turn();
	// 敵陣のマスク
	const Bitboard opponentsField = (us == Black ? inFrontMask<Black, Rank4>() : inFrontMask<White, Rank6>());

	// 二 宣言側の玉が敵陣三段目以内に入っている。
	if (!pos.bbOf(King, us).andIsAny(opponentsField))
		return false;

	// 四 宣言側の敵陣三段目以内の駒は、玉を除いて10枚以上存在する。
	const int ownPiecesCount = (pos.bbOf(us) & opponentsField).popCount() - 1;
	// 7未満を除外
	if (ownPiecesCount < 7)
		return false;

	// 三 宣言側が、大駒5点小駒1点で計算して
	//     先手の場合28点以上の持点がある。
	//     後手の場合27点以上の持点がある。
	//     点数の対象となるのは、宣言側の持駒と敵陣三段目以内に存在する玉を除く宣言側の駒のみである。
	const int ownBigPiecesCount = (pos.bbOf(Rook, Dragon, Bishop, Horse) & opponentsField & pos.bbOf(us)).popCount();
	const int ownSmallPiecesCount = ownPiecesCount - ownBigPiecesCount;
	const Hand hand = pos.hand(us);
	const int val = ownSmallPiecesCount
		+ hand.numOf<HPawn>() + hand.numOf<HLance>() + hand.numOf<HKnight>()
		+ hand.numOf<HSilver>() + hand.numOf<HGold>()
		+ (ownBigPiecesCount + hand.numOf<HRook>() + hand.numOf<HBishop>()) * 5;

	// 25点未満を除外
	if (val < (us == Black ? 25 : 24))
		return false;

	// 残り2手以内もしくはすでに条件を満たしている場合を除外
	if (ownPiecesCount >= 9 && val >= (us == Black ? 27 : 26))
		return false;

	return true;
}

int main(int argc, char *argv[]) {
	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();

	if (argc <= 2) {
		std::cout << "extract_nyugyoku_hcp_from_hcpe input_hcpe output_hcp" << std::endl;
		return 0;
	}

	const char* inHcpe = argv[1];
	const char* outHcp = argv[2];

	std::ifstream ifs(inHcpe, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << inHcpe << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNum = ifs.tellg() / sizeof(HuffmanCodedPosAndEval);
	ifs.seekg(0);
	std::cout << "input num = " << entryNum << std::endl;

	const s64 num_per_file = 1024 * 1024 * 1024 / sizeof(HuffmanCodedPosAndEval); // 1GB
	HuffmanCodedPosAndEval *inhcpevec = new HuffmanCodedPosAndEval[num_per_file];

	// サイズ確認
	s64 existEntryNum = 0;
	std::ifstream ifsOutHcp(outHcp, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (ifsOutHcp) {
		existEntryNum = ifsOutHcp.tellg() / sizeof(HuffmanCodedPos);
		ifsOutHcp.close();
	}
	std::cout << "exist num = " << existEntryNum << std::endl;

	// 追記する
	std::ofstream ofs(outHcp, std::ios::binary | std::ios::app);
	if (!ofs) {
		std::cerr << "Error: cannot open " << outHcp << std::endl;
		exit(EXIT_FAILURE);
	}

	Position pos;
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
		pos.set(hcpe.hcp, nullptr);

		if (before_nyugyoku(pos)) {
			ofs.write(reinterpret_cast<char*>(&hcpe.hcp), sizeof(HuffmanCodedPos));
			outNum++;
		}
	}

	std::cout << "output num = " << outNum << std::endl;
	std::cout << "total num = " << existEntryNum + outNum << std::endl;
}