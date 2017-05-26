#pragma once

#include "piece.hpp"
#include "common.hpp"

enum GameResult : int8_t {
	Draw, BlackWin, WhiteWin, GameResultNum
};

union HuffmanCode {
	struct {
		u8 code;      // ���������� bit ��
		u8 numOfBits; // �g�p bit ��
	};
	u16 key; // std::unordered_map �� key �Ƃ��Ďg���B
};

struct HuffmanCodeToPieceHash : public std::unordered_map<u16, Piece> {
	Piece value(const u16 key) const {
		const auto it = find(key);
		if (it == std::end(*this))
			return PieceNone;
		return it->second;
	}
};

// Huffman ���������ꂽ�ǖʂ̃f�[�^�\���B256 bit �ŋǖʂ�\���B
struct HuffmanCodedPos {
	static const HuffmanCode boardCodeTable[PieceNone];
	static const HuffmanCode handCodeTable[HandPieceNum][ColorNum];
	static HuffmanCodeToPieceHash boardCodeToPieceHash;
	static HuffmanCodeToPieceHash handCodeToPieceHash;
	static void init() {
		for (Piece pc = Empty; pc <= BDragon; ++pc)
			if (pieceToPieceType(pc) != King) // �ʂ͈ʒu�ŕ���������̂ŁA��̎�ނł͕��������Ȃ��B
				boardCodeToPieceHash[boardCodeTable[pc].key] = pc;
		for (Piece pc = WPawn; pc <= WDragon; ++pc)
			if (pieceToPieceType(pc) != King) // �ʂ͈ʒu�ŕ���������̂ŁA��̎�ނł͕��������Ȃ��B
				boardCodeToPieceHash[boardCodeTable[pc].key] = pc;
		for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp)
			for (Color c = Black; c < ColorNum; ++c)
				handCodeToPieceHash[handCodeTable[hp][c].key] = colorAndPieceTypeToPiece(c, handPieceToPieceType(hp));
	}
	void clear() { std::fill(std::begin(data), std::end(data), 0); }

	u8 data[32];
};
static_assert(sizeof(HuffmanCodedPos) == 32, "");

struct HuffmanCodedPosAndEval {
	HuffmanCodedPos hcp;
	s16 eval;
	u16 bestMove16; // �g�����͕�����Ȃ������t�f�[�^�������ɂ��łɎ擾���Ă����B
	GameResult gameResult; // ���ȑ΋ǂŏ��������ǂ����B
};
static_assert(sizeof(HuffmanCodedPosAndEval) == 38, "");
