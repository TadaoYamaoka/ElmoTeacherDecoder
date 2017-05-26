#pragma once

#include "piece.hpp"
#include "common.hpp"

enum GameResult : int8_t {
	Draw, BlackWin, WhiteWin, GameResultNum
};

union HuffmanCode {
	struct {
		u8 code;      // 符号化時の bit 列
		u8 numOfBits; // 使用 bit 数
	};
	u16 key; // std::unordered_map の key として使う。
};

struct HuffmanCodeToPieceHash : public std::unordered_map<u16, Piece> {
	Piece value(const u16 key) const {
		const auto it = find(key);
		if (it == std::end(*this))
			return PieceNone;
		return it->second;
	}
};

// Huffman 符号化された局面のデータ構造。256 bit で局面を表す。
struct HuffmanCodedPos {
	static const HuffmanCode boardCodeTable[PieceNone];
	static const HuffmanCode handCodeTable[HandPieceNum][ColorNum];
	static HuffmanCodeToPieceHash boardCodeToPieceHash;
	static HuffmanCodeToPieceHash handCodeToPieceHash;
	static void init() {
		for (Piece pc = Empty; pc <= BDragon; ++pc)
			if (pieceToPieceType(pc) != King) // 玉は位置で符号化するので、駒の種類では符号化しない。
				boardCodeToPieceHash[boardCodeTable[pc].key] = pc;
		for (Piece pc = WPawn; pc <= WDragon; ++pc)
			if (pieceToPieceType(pc) != King) // 玉は位置で符号化するので、駒の種類では符号化しない。
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
	u16 bestMove16; // 使うかは分からないが教師データ生成時についでに取得しておく。
	GameResult gameResult; // 自己対局で勝ったかどうか。
};
static_assert(sizeof(HuffmanCodedPosAndEval) == 38, "");
