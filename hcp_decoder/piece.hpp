#pragma once

#include "common.hpp"
#include "overloadEnumOperators.hpp"
#include "color.hpp"

enum PieceType {
	// Pro* �� ���� ��̎�ނ� 8 �����Z�������́B
	PTPromote = 8,
	Occupied = 0, // �e PieceType �� or ���Ƃ������́B
	Pawn, Lance, Knight, Silver, Bishop, Rook, Gold, King,
	ProPawn, ProLance, ProKnight, ProSilver, Horse, Dragon,
	PieceTypeNum,

	GoldHorseDragon // �P��temnplate�����Ƃ��Ďg�p
};

enum Piece {
	// B* �� 16 �����Z���邱�ƂŁAW* ��\���B
	// Promoted �����Z���邱�ƂŁA�����\���B
	Empty = 0, UnPromoted = 0, Promoted = 8,
	BPawn = 1, BLance, BKnight, BSilver, BBishop, BRook, BGold, BKing,
	BProPawn, BProLance, BProKnight, BProSilver, BHorse, BDragon, // BDragon = 14
	WPawn = 17, WLance, WKnight, WSilver, WBishop, WRook, WGold, WKing,
	WProPawn, WProLance, WProKnight, WProSilver, WHorse, WDragon,
	PieceNone // PieceNone = 31  ����� 32 �ɂ����������d�z��̂Ƃ��ɗL�����B
};
OverloadEnumOperators(Piece);

enum HandPiece {
	HPawn, HLance, HKnight, HSilver, HGold, HBishop, HRook, HandPieceNum
};
OverloadEnumOperators(HandPiece);

// p == Empty �̂Ƃ��APieceType �� OccuPied �ɂȂ��Ă��܂��̂ŁA
// Position::bbOf(pieceToPieceType(p)) �Ƃ���ƁA
// Position::emptyBB() �ł͂Ȃ� Position::occupiedBB() �ɂȂ��Ă��܂��̂ŁA
// ���ӂ��邱�ƁB�o����ΏC���������B
inline PieceType pieceToPieceType(const Piece p) { return static_cast<PieceType>(p & 15); }

inline Piece colorAndPieceTypeToPiece(const Color c, const PieceType pt) { return static_cast<Piece>((c << 4) | pt); }

const PieceType HandPieceToPieceTypeTable[HandPieceNum] = {
	Pawn, Lance, Knight, Silver, Gold, Bishop, Rook
};
inline PieceType handPieceToPieceType(const HandPiece hp) { return HandPieceToPieceTypeTable[hp]; }
