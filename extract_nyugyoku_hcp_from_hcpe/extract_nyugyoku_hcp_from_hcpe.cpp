#include "init.hpp"
#include "position.hpp"

#include <iostream>

bool before_nyugyoku(const Position& pos) {
	// ���ʐ錾�����̒��O�̋ǖʂ����肷��
	// 3�i�ڂ�7�`8���A25�`26�_

	// CSA ���[���ł́A�� ���� �Z �̏�����S�Ė������Ƃ��A���ʏ����錾���o����B
	// ���肪�����ɏo������̂��珇�ɔ��肵�Ă������ɂ���B
	// �� �錾���̎�Ԃł���B
	// �Z �錾���̎������Ԃ��c���Ă���B
	// �� �錾���̋ʂɉ��肪�������Ă��Ȃ��B

	// ���肪�������Ă��Ă��ǂ�
	/*if (pos.inCheck())
		return false;*/

	const Color us = pos.turn();
	// �G�w�̃}�X�N
	const Bitboard opponentsField = (us == Black ? inFrontMask<Black, Rank4>() : inFrontMask<White, Rank6>());

	// �� �錾���̋ʂ��G�w�O�i�ڈȓ��ɓ����Ă���B
	if (!pos.bbOf(King, us).andIsAny(opponentsField))
		return false;

	// �l �錾���̓G�w�O�i�ڈȓ��̋�́A�ʂ�������10���ȏ㑶�݂���B
	const int ownPiecesCount = (pos.bbOf(us) & opponentsField).popCount() - 1;
	// 7���������O
	if (ownPiecesCount < 7)
		return false;

	// �O �錾�����A���5�_����1�_�Ōv�Z����
	//     ���̏ꍇ28�_�ȏ�̎��_������B
	//     ���̏ꍇ27�_�ȏ�̎��_������B
	//     �_���̑ΏۂƂȂ�̂́A�錾���̎���ƓG�w�O�i�ڈȓ��ɑ��݂���ʂ������錾���̋�݂̂ł���B
	const int ownBigPiecesCount = (pos.bbOf(Rook, Dragon, Bishop, Horse) & opponentsField & pos.bbOf(us)).popCount();
	const int ownSmallPiecesCount = ownPiecesCount - ownBigPiecesCount;
	const Hand hand = pos.hand(us);
	const int val = ownSmallPiecesCount
		+ hand.numOf<HPawn>() + hand.numOf<HLance>() + hand.numOf<HKnight>()
		+ hand.numOf<HSilver>() + hand.numOf<HGold>()
		+ (ownBigPiecesCount + hand.numOf<HRook>() + hand.numOf<HBishop>()) * 5;

	// 25�_���������O
	if (val < (us == Black ? 25 : 24))
		return false;

	// �c��2��ȓ��������͂��łɏ����𖞂����Ă���ꍇ�����O
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

	// �T�C�Y�m�F
	s64 existEntryNum = 0;
	std::ifstream ifsOutHcp(outHcp, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (ifsOutHcp) {
		existEntryNum = ifsOutHcp.tellg() / sizeof(HuffmanCodedPos);
		ifsOutHcp.close();
	}
	std::cout << "exist num = " << existEntryNum << std::endl;

	// �ǋL����
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