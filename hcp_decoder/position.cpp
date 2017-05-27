/*
  Apery, a USI shogi playing engine derived from Stockfish, a UCI chess playing engine.
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2011-2016 Hiraoka Takuya

  Apery is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Apery is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "position.hpp"
#include "move.hpp"
#include "mt64bit.hpp"
#include "generateMoves.hpp"
#include "tt.hpp"
#include "search.hpp"

Key Position::zobrist_[PieceTypeNum][SquareNum][ColorNum];
Key Position::zobHand_[HandPieceNum][ColorNum];
Key Position::zobExclusion_;

const HuffmanCode HuffmanCodedPos::boardCodeTable[PieceNone] = {
    {Binary<         0>::value, 1}, // Empty
    {Binary<         1>::value, 4}, // BPawn
    {Binary<        11>::value, 6}, // BLance
    {Binary<       111>::value, 6}, // BKnight
    {Binary<      1011>::value, 6}, // BSilver
    {Binary<     11111>::value, 8}, // BBishop
    {Binary<    111111>::value, 8}, // BRook
    {Binary<      1111>::value, 6}, // BGold
    {Binary<         0>::value, 0}, // BKing çãEä½ç½®ã¯å¥éãä½ç½®ãç¬¦å·åãããä½¿ç¨ããªãEEã§ numOfBit ãE0 ã«ãã¦ãããE    {Binary<      1001>::value, 4}, // BProPawn
    {Binary<    100011>::value, 6}, // BProLance
    {Binary<    100111>::value, 6}, // BProKnight
    {Binary<    101011>::value, 6}, // BProSilver
    {Binary<  10011111>::value, 8}, // BHorse
    {Binary<  10111111>::value, 8}, // BDragona
    {Binary<         0>::value, 0}, // ä½¿ç¨ããªãEEã§ numOfBit ãE0 ã«ãã¦ãããE    {Binary<         0>::value, 0}, // ä½¿ç¨ããªãEEã§ numOfBit ãE0 ã«ãã¦ãããE    {Binary<       101>::value, 4}, // WPawn
    {Binary<     10011>::value, 6}, // WLance
    {Binary<     10111>::value, 6}, // WKnight
    {Binary<     11011>::value, 6}, // WSilver
    {Binary<   1011111>::value, 8}, // WBishop
    {Binary<   1111111>::value, 8}, // WRook
    {Binary<    101111>::value, 6}, // WGold
    {Binary<         0>::value, 0}, // WKing çãEä½ç½®ã¯å¥éãä½ç½®ãç¬¦å·åãããE    {Binary<      1101>::value, 4}, // WProPawn
    {Binary<    110011>::value, 6}, // WProLance
    {Binary<    110111>::value, 6}, // WProKnight
    {Binary<    111011>::value, 6}, // WProSilver
    {Binary<  11011111>::value, 8}, // WHorse
    {Binary<  11111111>::value, 8}, // WDragon
};

// ç¤ä¸ãE bit æ° - 1 ã§è¡¨ç¾åºæ¥ãããE«ãããæã¡é§ãããã¨ãç¤ä¸ã«ã¯ Empty ã® 1 bit ãå¢ãããEã§ãE// ããã§å±é¢ã® bit æ°ãåºå®åããããEconst HuffmanCode HuffmanCodedPos::handCodeTable[HandPieceNum][ColorNum] = {
    {{Binary<        0>::value, 3}, {Binary<      100>::value, 3}}, // HPawn
    {{Binary<        1>::value, 5}, {Binary<    10001>::value, 5}}, // HLance
    {{Binary<       11>::value, 5}, {Binary<    10011>::value, 5}}, // HKnight
    {{Binary<      101>::value, 5}, {Binary<    10101>::value, 5}}, // HSilver
    {{Binary<      111>::value, 5}, {Binary<    10111>::value, 5}}, // HGold
    {{Binary<    11111>::value, 7}, {Binary<  1011111>::value, 7}}, // HBishop
    {{Binary<   111111>::value, 7}, {Binary<  1111111>::value, 7}}, // HRook
};

HuffmanCodeToPieceHash HuffmanCodedPos::boardCodeToPieceHash;
HuffmanCodeToPieceHash HuffmanCodedPos::handCodeToPieceHash;

const CharToPieceUSI g_charToPieceUSI;

namespace {
    const char* PieceToCharCSATable[PieceNone] = {
        " * ", "+FU", "+KY", "+KE", "+GI", "+KA", "+HI", "+KI", "+OU", "+TO", "+NY", "+NK", "+NG", "+UM", "+RY", "", "",
        "-FU", "-KY", "-KE", "-GI", "-KA", "-HI", "-KI", "-OU", "-TO", "-NY", "-NK", "-NG", "-UM", "-RY"
    };
    inline const char* pieceToCharCSA(const Piece pc) {
        return PieceToCharCSATable[pc];
    }
    const char* PieceToCharUSITable[PieceNone] = {
        "", "P", "L", "N", "S", "B", "R", "G", "K", "+P", "+L", "+N", "+S", "+B", "+R", "", "",
        "p", "l", "n", "s", "b", "r", "g", "k", "+p", "+l", "+n", "+s", "+b", "+r"
    };
    inline const char* pieceToCharUSI(const Piece pc) {
        return PieceToCharUSITable[pc];
    }
}

CheckInfo::CheckInfo(const Position& pos) {
    const Color them = oppositeColor(pos.turn());
    const Square ksq = pos.kingSquare(them);

    pinned = pos.pinnedBB();
    dcBB = pos.discoveredCheckBB();

    checkBB[Pawn     ] = pos.attacksFrom<Pawn  >(them, ksq);
    checkBB[Lance    ] = pos.attacksFrom<Lance >(them, ksq);
    checkBB[Knight   ] = pos.attacksFrom<Knight>(them, ksq);
    checkBB[Silver   ] = pos.attacksFrom<Silver>(them, ksq);
    checkBB[Bishop   ] = pos.attacksFrom<Bishop>(ksq);
    checkBB[Rook     ] = pos.attacksFrom<Rook  >(ksq);
    checkBB[Gold     ] = pos.attacksFrom<Gold  >(them, ksq);
    checkBB[King     ] = allZeroBB();
    // todo: ããã§ AVX2 ä½¿ãããEE    //       checkBB ã®readã¢ã¯ã»ã¹ã¯ switch (pt) ã§å ´ååEããã¦ãä½è¨ãªã³ããEæ¸ãããæ¹ãè¯ãEããE    checkBB[ProPawn  ] = checkBB[Gold];
    checkBB[ProLance ] = checkBB[Gold];
    checkBB[ProKnight] = checkBB[Gold];
    checkBB[ProSilver] = checkBB[Gold];
    checkBB[Horse    ] = checkBB[Bishop] | pos.attacksFrom<King>(ksq);
    checkBB[Dragon   ] = checkBB[Rook  ] | pos.attacksFrom<King>(ksq);
}

Bitboard Position::attacksFrom(const PieceType pt, const Color c, const Square sq, const Bitboard& occupied) {
    switch (pt) {
    case Occupied:  return allZeroBB();
    case Pawn:      return pawnAttack(c, sq);
    case Lance:     return lanceAttack(c, sq, occupied);
    case Knight:    return knightAttack(c, sq);
    case Silver:    return silverAttack(c, sq);
    case Bishop:    return bishopAttack(sq, occupied);
    case Rook:      return rookAttack(sq, occupied);
    case Gold:
    case ProPawn:
    case ProLance:
    case ProKnight:
    case ProSilver: return goldAttack(c, sq);
    case King:      return kingAttack(sq);
    case Horse:     return horseAttack(sq, occupied);
    case Dragon:    return dragonAttack(sq, occupied);
    default:        UNREACHABLE; return allOneBB();
    }
}

// å®éã«æEæãåæ³æãã©ãEå¤å®E// é£ç¶çæãEåE¥æãEæé¤ããªãEE// ç¢ºå®ã«é§æã¡ã§ã¯ãªãE¨ããEãMUSTNOTDROP == true ã¨ãããE// ç¢ºå®ã«çãEç§»åã§ç¡ãE¨ããEãFROMMUSTNOTKING == true ã¨ãããè±èªã¨ãã¦æ­£ããEE// é éé§ã§çæããã¦ãEã¨ãããã®é§ãEå©ããããå ´æã«éEãæãæ¤åEåºæ¥ãªãE ´åãããã®ã§ãE// ããEãããªæãæEæçæãã¦ã¯ãEãªãEEtemplate <bool MUSTNOTDROP, bool FROMMUSTNOTKING>
bool Position::pseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const {
    // é§æã¡ã¯ãæã¡æ­©è©°ããäºæ­©ã¯æEæçææãEkillerãMovePicker::nextMove() åE§æé¤ãã¦ãEã®ã§ãå¸¸ã«åæ³æ
    // (é£ç¶çæãEåE¥æãEçãã¦ãEªãEãã©ãE
    if (!MUSTNOTDROP && move.isDrop())
        return true;
    assert(!move.isDrop());

    const Color us = turn();
    const Square from = move.from();

    if (!FROMMUSTNOTKING && pieceToPieceType(piece(from)) == King) {
        const Color them = oppositeColor(us);
        // çãEç§»ååEã«ç¸æãEé§ãEå©ããããã°ãåæ³æã§ãªãEEã§ãfalse
        return !attackersToIsAny(them, move.to());
    }
    // çä»¥å¤ãEé§ãEç§»åE    return !isPinnedIllegal(from, move.to(), kingSquare(us), pinned);
}

template bool Position::pseudoLegalMoveIsLegal<false, false>(const Move move, const Bitboard& pinned) const;
template bool Position::pseudoLegalMoveIsLegal<false, true >(const Move move, const Bitboard& pinned) const;
template bool Position::pseudoLegalMoveIsLegal<true,  false>(const Move move, const Bitboard& pinned) const;

bool Position::pseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const {
    assert(isOK());

    // çãEç§»åE    if (move.pieceTypeFrom() == King) {
        // é éé§ã§çæãããã¨ããçæãã¦ãEé éé§ãEå©ãã«ã¯ç§»åããªãEãE«æEæãçæEãã¦ãEãE        // ããEçºãç§»ååEã«ä»ãEé§ãEå©ããç¡ãEèª¿ã¹ãã ãã§è¯ãEE        const bool canMove = !attackersToIsAny(oppositeColor(turn()), move.to());
        assert(canMove == (pseudoLegalMoveIsLegal<false, false>(move, pinned)));
        return canMove;
    }

    // çãEç§»åä»¥å¤E    Bitboard target = checkersBB();
    const Square checkSq = target.firstOneFromSQ11();

    if (target)
        // ä¸¡çæã®ã¨ããçã®ç§»åä»¥å¤ãEæãEæEãªãEE        return false;

    const Color us = turn();
    const Square to = move.to();
    // ç§»åãåã¯æã£ãé§ããçæãããããããçæãã¦ãEé§ãåãå¿E¦ããããE    target = betweenBB(checkSq, kingSquare(us)) | checkersBB();
    return target.isSet(to) && pseudoLegalMoveIsLegal<false, true>(move, pinned);
}

// Searching: true ãªãæ¢ç´¢æã«åE¨ã§çæEããæãEåæ³æå¤å®ãè¡ããE//            ttMove ã§ hash å¤ãè¡çªããæãªã©ã§ãå¤§é§ãEä¸æEãªã©æããã«ä¾¡å¤ã®ä½ãæãçæEãããäºããããE//            ããã¯éåæ³æã¨ãã¦çãã¦è¯ãEE//            false ãªããå¤é¨å¥åãEåæ³æå¤å®ãªã®ã§ãã«ã¼ã«ã¨åä¸ã®æ¡ä»¶ã«ãªãäºãæã¾ãããEtemplate <bool Searching> bool Position::moveIsPseudoLegal(const Move move) const {
    const Color us = turn();
    const Color them = oppositeColor(us);
    const Square to = move.to();

    if (move.isDrop()) {
        const PieceType ptFrom = move.pieceTypeDropped();
        if (!hand(us).exists(pieceTypeToHandPiece(ptFrom)) || piece(to) != Empty)
            return false;

        if (inCheck()) {
            // çæããã¦ãEã®ã§ãåé§ã§ãªããã°ãªããªãEE            Bitboard target = checkersBB();
            const Square checksq = target.firstOneFromSQ11();

            if (target)
                // ä¸¡çæã¯åé§åEæ¥ç¡ãEE                return false;

            target = betweenBB(checksq, kingSquare(us));
            if (!target.isSet(to))
                // çã¨ãçæããé§ã¨ã®éã«é§ãæã£ã¦ãEªãEE                return false;
        }

        if (ptFrom == Pawn) {
            if ((bbOf(Pawn, us) & fileMask(makeFile(to))))
                // äºæ­©
                return false;
            const SquareDelta TDeltaN = (us == Black ? DeltaN : DeltaS);
            if (to + TDeltaN == kingSquare(them) && isPawnDropCheckMate(us, to))
                // çæãã¤æã¡æ­©è©°ãE                return false;
        }
    }
    else {
        const Square from = move.from();
        const PieceType ptFrom = move.pieceTypeFrom();
        if (piece(from) != colorAndPieceTypeToPiece(us, ptFrom) || bbOf(us).isSet(to))
            return false;

        if (!attacksFrom(ptFrom, us, from).isSet(to))
            return false;

        if (Searching) {
            switch (ptFrom) {
            case Pawn  :
                if (move.isPromotion()) {
                    if (!canPromote(us, makeRank(to)))
                        return false;
                }
                else if (canPromote(us, makeRank(to)))
                    return false;
                break;
            case Lance :
                if (move.isPromotion()) {
                    if (!canPromote(us, makeRank(to)))
                        return false;
                }
                else {
                    // 1æ®µç®ã®ä¸æEã¯éåæ³ãªã®ã§çããEæ®µç®ã®ä¸æEã¨3æ®µç®ã®é§ãåããªãE¸æEãã¤ãE§ã«çããE                    const Rank toRank = makeRank(to);
                    if (us == Black ? isInFrontOf<Black, Rank3, Rank7>(toRank) : isInFrontOf<White, Rank3, Rank7>(toRank))
                        return false;
                    if (canPromote(us, toRank) && !move.isCapture())
                        return false;
                }
                break;
            case Knight:
                // hash å¤ãè¡çªãã¦å¥ã®å±é¢ã®åæ³æã® ttMove ãåEåããã¦ããæ¡é¦¬ã§ããäºãEç¢ºå®ãEæ¡é¦¬ã¯ç§»ååEãç§»ååEãç¹æ®ãªã®ã§ãE
                // ãã£ã¦ãè¡ãã©ããã®ç¡ãE§ã«ãªãEmove ã¯çæEãããªãEE                // ç¹ã«ãã§ãE¯ãã¹ãäºãEç¡ãEEã§ãbreak
                break;
            case Silver: case Bishop: case Rook  :
                if (move.isPromotion())
                    if (!canPromote(us, makeRank(to)) && !canPromote(us, makeRank(from)))
                        return false;
                break;
            default: // æããªãE§E                if (move.isPromotion())
                    return false;
            }
        }

        if (inCheck()) {
            if (ptFrom == King) {
                Bitboard occ = occupiedBB();
                occ.clearBit(from);
                if (attackersToIsAny(them, to, occ))
                    // çæããéEã¦ãEªãEE                    return false;
            }
            else {
                // çä»¥å¤ãEé§ãç§»åãããã¨ããE                Bitboard target = checkersBB();
                const Square checksq = target.firstOneFromSQ11();

                if (target)
                    // ä¸¡çæãªã®ã§ãçãéEãªãEã¯é§E®
                    return false;

                target = betweenBB(checksq, kingSquare(us)) | checkersBB();
                if (!target.isSet(to))
                    // çã¨ãçæããé§ã¨ã®éã«ç§»åããããçæããé§ãåãä»¥å¤ãEé§E®ãE                    return false;
            }
        }
    }

    return true;
}

template bool Position::moveIsPseudoLegal<true >(const Move move) const;
template bool Position::moveIsPseudoLegal<false>(const Move move) const;

#if !defined NDEBUG
// éå»(åãEç¾å¨)ã«çæEããæEæãç¾å¨ã®å±é¢ã§ãæå¹ãå¤å®ãE// ãã¾ãéåº¦ãè¦æ±ãããå ´é¢ã§ä½¿ã£ã¦ã¯ãEãªãEEbool Position::moveIsLegal(const Move move) const {
    return MoveList<LegalAll>(*this).contains(move);
}
#endif

// å±é¢ã®æ´æ°
void Position::doMove(const Move move, StateInfo& newSt) {
    const CheckInfo ci(*this);
    doMove(move, newSt, ci, moveGivesCheck(move, ci));
}

// å±é¢ã®æ´æ°
void Position::doMove(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck) {
    assert(isOK());
    assert(move);
    assert(&newSt != st_);

    Key boardKey = getBoardKey();
    Key handKey = getHandKey();
    boardKey ^= zobTurn();

    memcpy(&newSt, st_, offsetof(StateInfo, boardKey));
    newSt.previous = st_;
    st_ = &newSt;

    st_->cl.size = 1;

    const Color us = turn();
    const Square to = move.to();
    const PieceType ptCaptured = move.cap();
    PieceType ptTo;
    if (move.isDrop()) {
        ptTo = move.pieceTypeDropped();
        const HandPiece hpTo = pieceTypeToHandPiece(ptTo);

        handKey -= zobHand(hpTo, us);
        boardKey += zobrist(ptTo, to, us);

        prefetch(csearcher()->tt.firstEntry(boardKey + handKey));

        const int handnum = hand(us).numOf(hpTo);
        const int listIndex = evalList_.squareHandToList[HandPieceToSquareHand[us][hpTo] + handnum];
        const Piece pcTo = colorAndPieceTypeToPiece(us, ptTo);
        st_->cl.listindex[0] = listIndex;
        st_->cl.clistpair[0].oldlist[0] = evalList_.list0[listIndex];
        st_->cl.clistpair[0].oldlist[1] = evalList_.list1[listIndex];

        evalList_.list0[listIndex] = kppArray[pcTo         ] + to;
        evalList_.list1[listIndex] = kppArray[inverse(pcTo)] + inverse(to);
        evalList_.listToSquareHand[listIndex] = to;
        evalList_.squareHandToList[to] = listIndex;

        st_->cl.clistpair[0].newlist[0] = evalList_.list0[listIndex];
        st_->cl.clistpair[0].newlist[1] = evalList_.list1[listIndex];

        hand_[us].minusOne(hpTo);
        xorBBs(ptTo, to, us);
        piece_[to] = colorAndPieceTypeToPiece(us, ptTo);

        if (moveIsCheck) {
            // Direct checks
            st_->checkersBB = setMaskBB(to);
            st_->continuousCheck[us] += 2;
        }
        else {
            st_->checkersBB = allZeroBB();
            st_->continuousCheck[us] = 0;
        }
    }
    else {
        const Square from = move.from();
        const PieceType ptFrom = move.pieceTypeFrom();
        ptTo = move.pieceTypeTo(ptFrom);

        byTypeBB_[ptFrom].xorBit(from);
        byTypeBB_[ptTo].xorBit(to);
        byColorBB_[us].xorBit(from, to);
        piece_[from] = Empty;
        piece_[to] = colorAndPieceTypeToPiece(us, ptTo);
        boardKey -= zobrist(ptFrom, from, us);
        boardKey += zobrist(ptTo, to, us);

        if (ptCaptured) {
            // é§ãåã£ãã¨ãE            const HandPiece hpCaptured = pieceTypeToHandPiece(ptCaptured);
            const Color them = oppositeColor(us);

            boardKey -= zobrist(ptCaptured, to, them);
            handKey += zobHand(hpCaptured, us);

            byTypeBB_[ptCaptured].xorBit(to);
            byColorBB_[them].xorBit(to);

            hand_[us].plusOne(hpCaptured);
            const int toListIndex = evalList_.squareHandToList[to];
            st_->cl.listindex[1] = toListIndex;
            st_->cl.clistpair[1].oldlist[0] = evalList_.list0[toListIndex];
            st_->cl.clistpair[1].oldlist[1] = evalList_.list1[toListIndex];
            st_->cl.size = 2;

            const int handnum = hand(us).numOf(hpCaptured);
            evalList_.list0[toListIndex] = kppHandArray[us  ][hpCaptured] + handnum;
            evalList_.list1[toListIndex] = kppHandArray[them][hpCaptured] + handnum;
            const Square squarehand = HandPieceToSquareHand[us][hpCaptured] + handnum;
            evalList_.listToSquareHand[toListIndex] = squarehand;
            evalList_.squareHandToList[squarehand]  = toListIndex;

            st_->cl.clistpair[1].newlist[0] = evalList_.list0[toListIndex];
            st_->cl.clistpair[1].newlist[1] = evalList_.list1[toListIndex];

            st_->material += (us == Black ? capturePieceScore(ptCaptured) : -capturePieceScore(ptCaptured));
        }
        prefetch(csearcher()->tt.firstEntry(boardKey + handKey));
        // Occupied ã¯ to, from ã®ä½ç½®ã®ããããæä½ããããããE        // Black ã¨ White ã® or ãåãæ¹ãéãã¯ããE        byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);

        if (ptTo == King)
            kingSquare_[us] = to;
        else {
            const Piece pcTo = colorAndPieceTypeToPiece(us, ptTo);
            const int fromListIndex = evalList_.squareHandToList[from];

            st_->cl.listindex[0] = fromListIndex;
            st_->cl.clistpair[0].oldlist[0] = evalList_.list0[fromListIndex];
            st_->cl.clistpair[0].oldlist[1] = evalList_.list1[fromListIndex];

            evalList_.list0[fromListIndex] = kppArray[pcTo         ] + to;
            evalList_.list1[fromListIndex] = kppArray[inverse(pcTo)] + inverse(to);
            evalList_.listToSquareHand[fromListIndex] = to;
            evalList_.squareHandToList[to] = fromListIndex;

            st_->cl.clistpair[0].newlist[0] = evalList_.list0[fromListIndex];
            st_->cl.clistpair[0].newlist[1] = evalList_.list1[fromListIndex];
        }

        if (move.isPromotion())
            st_->material += (us == Black ? (pieceScore(ptTo) - pieceScore(ptFrom)) : -(pieceScore(ptTo) - pieceScore(ptFrom)));

        if (moveIsCheck) {
            // Direct checks
            st_->checkersBB = ci.checkBB[ptTo] & setMaskBB(to);

            // Discovery checks
            const Square ksq = kingSquare(oppositeColor(us));
            if (isDiscoveredCheck(from, to, ksq, ci.dcBB)) {
                switch (squareRelation(from, ksq)) {
                case DirecMisc: assert(false); break; // æé©åãEçºã®ãããE
                case DirecFile:
                    // from ã®ä½ç½®ããç¸¦ã«å©ããèª¿ã¹ãã¨ç¸æçã¨ãç©ºãçæãã¦ãEé§ã«å½ãã£ã¦ãEã¯ããå³æ¹ã®é§ãç©ºãçæãã¦ãEé§ãE                    st_->checkersBB |= rookAttackFile(from, occupiedBB()) & bbOf(us);
                    break;
                case DirecRank:
                    st_->checkersBB |= attacksFrom<Rook>(ksq) & bbOf(Rook, Dragon, us);
                    break;
                case DirecDiagNESW: case DirecDiagNWSE:
                    st_->checkersBB |= attacksFrom<Bishop>(ksq) & bbOf(Bishop, Horse, us);
                    break;
                default: UNREACHABLE;
                }
            }
            st_->continuousCheck[us] += 2;
        }
        else {
            st_->checkersBB = allZeroBB();
            st_->continuousCheck[us] = 0;
        }
    }
    goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);

    st_->boardKey = boardKey;
    st_->handKey = handKey;
    ++st_->pliesFromNull;

    turn_ = oppositeColor(us);
    st_->hand = hand(turn());

    assert(isOK());
}

void Position::undoMove(const Move move) {
    assert(isOK());
    assert(move);

    const Color them = turn();
    const Color us = oppositeColor(them);
    const Square to = move.to();
    turn_ = us;
    // ããã§åã« turn_ ãæ»ããã®ã§ãä»¥ä¸ãmove ã¯ us ã®æEæã¨ãããE    if (move.isDrop()) {
        const PieceType ptTo = move.pieceTypeDropped();
        byTypeBB_[ptTo].xorBit(to);
        byColorBB_[us].xorBit(to);
        piece_[to] = Empty;

        const HandPiece hp = pieceTypeToHandPiece(ptTo);
        hand_[us].plusOne(hp);

        const int toListIndex  = evalList_.squareHandToList[to];
        const int handnum = hand(us).numOf(hp);
        evalList_.list0[toListIndex] = kppHandArray[us  ][hp] + handnum;
        evalList_.list1[toListIndex] = kppHandArray[them][hp] + handnum;
        const Square squarehand = HandPieceToSquareHand[us][hp] + handnum;
        evalList_.listToSquareHand[toListIndex] = squarehand;
        evalList_.squareHandToList[squarehand]  = toListIndex;
    }
    else {
        const Square from = move.from();
        const PieceType ptFrom = move.pieceTypeFrom();
        const PieceType ptTo = move.pieceTypeTo(ptFrom);
        const PieceType ptCaptured = move.cap(); // todo: st_->capturedType ä½¿ããEè¯ãEE
        if (ptTo == King)
            kingSquare_[us] = from;
        else {
            const Piece pcFrom = colorAndPieceTypeToPiece(us, ptFrom);
            const int toListIndex = evalList_.squareHandToList[to];
            evalList_.list0[toListIndex] = kppArray[pcFrom         ] + from;
            evalList_.list1[toListIndex] = kppArray[inverse(pcFrom)] + inverse(from);
            evalList_.listToSquareHand[toListIndex] = from;
            evalList_.squareHandToList[from] = toListIndex;
        }

        if (ptCaptured) {
            // é§ãåã£ãã¨ãE            byTypeBB_[ptCaptured].xorBit(to);
            byColorBB_[them].xorBit(to);
            const HandPiece hpCaptured = pieceTypeToHandPiece(ptCaptured);
            const Piece pcCaptured = colorAndPieceTypeToPiece(them, ptCaptured);
            piece_[to] = pcCaptured;

            const int handnum = hand(us).numOf(hpCaptured);
            const int toListIndex = evalList_.squareHandToList[HandPieceToSquareHand[us][hpCaptured] + handnum];
            evalList_.list0[toListIndex] = kppArray[pcCaptured         ] + to;
            evalList_.list1[toListIndex] = kppArray[inverse(pcCaptured)] + inverse(to);
            evalList_.listToSquareHand[toListIndex] = to;
            evalList_.squareHandToList[to] = toListIndex;

            hand_[us].minusOne(hpCaptured);
        }
        else
            // é§ãåããªãE¨ããEãcolorAndPieceTypeToPiece(us, ptCaptured) ã¯ 0 ã¾ããE 16 ã«ãªããE            // 16 ã«ãªãã¨å°ããEã§ãE§ãåããªãE¨ããEæç¤ºçE« Empty ã«ãããE            piece_[to] = Empty;
        byTypeBB_[ptFrom].xorBit(from);
        byTypeBB_[ptTo].xorBit(to);
        byColorBB_[us].xorBit(from, to);
        piece_[from] = colorAndPieceTypeToPiece(us, ptFrom);
    }
    // Occupied ã¯ to, from ã®ä½ç½®ã®ããããæä½ããããããE    // Black ã¨ White ã® or ãåãæ¹ãéãã¯ããE    byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);
    goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);

    // key ãªã©ã¯ StateInfo ã«ã¾ã¨ãããã¦ãEã®ã§ãE    // previous ã®ãã¤ã³ã¿ãEst_ ã«ä»£å¥ããã ãã§è¯ãEE    st_ = st_->previous;

    assert(isOK());
}

namespace {
    // SEE ã®é Eª
    template <PieceType PT> struct SEENextPieceType {}; // ããã¯ã¤ã³ã¹ã¿ã³ã¹åããªãEE    template <> struct SEENextPieceType<Pawn     > { static const PieceType value = Lance;     };
    template <> struct SEENextPieceType<Lance    > { static const PieceType value = Knight;    };
    template <> struct SEENextPieceType<Knight   > { static const PieceType value = ProPawn;   };
    template <> struct SEENextPieceType<ProPawn  > { static const PieceType value = ProLance;  };
    template <> struct SEENextPieceType<ProLance > { static const PieceType value = ProKnight; };
    template <> struct SEENextPieceType<ProKnight> { static const PieceType value = Silver;    };
    template <> struct SEENextPieceType<Silver   > { static const PieceType value = ProSilver; };
    template <> struct SEENextPieceType<ProSilver> { static const PieceType value = Gold;      };
    template <> struct SEENextPieceType<Gold     > { static const PieceType value = Bishop;    };
    template <> struct SEENextPieceType<Bishop   > { static const PieceType value = Horse;     };
    template <> struct SEENextPieceType<Horse    > { static const PieceType value = Rook;      };
    template <> struct SEENextPieceType<Rook     > { static const PieceType value = Dragon;    };
    template <> struct SEENextPieceType<Dragon   > { static const PieceType value = King;      };

    template <PieceType PT> FORCE_INLINE PieceType nextAttacker(const Position& pos, const Square to, const Bitboard& opponentAttackers,
                                                                Bitboard& occupied, Bitboard& attackers, const Color turn)
    {
        if (opponentAttackers.andIsAny(pos.bbOf(PT))) {
            const Bitboard bb = opponentAttackers & pos.bbOf(PT);
            const Square from = bb.constFirstOneFromSQ11();
            occupied.xorBit(from);
            // todo: å®éã«ç§»åããæ¹åãåºã«attackersãæ´æ°ããã°ãtemplate, inline ãä½¿ç¨ããªãã¦ãè¯ãããEE            //       ããEå ´åãã­ã£ãE·ã¥ã«ä¹ããEããªããEã§éE«éããªããããE            if (PT == Pawn || PT == Lance)
                attackers |= (lanceAttack(oppositeColor(turn), to, occupied) & (pos.bbOf(Rook, Dragon) | pos.bbOf(Lance, turn)));
            if (PT == Gold || PT == ProPawn || PT == ProLance || PT == ProKnight || PT == ProSilver || PT == Horse || PT == Dragon)
                attackers |= (lanceAttack(oppositeColor(turn), to, occupied) & pos.bbOf(Lance, turn))
                    | (lanceAttack(turn, to, occupied) & pos.bbOf(Lance, oppositeColor(turn)))
                    | (rookAttack(to, occupied) & pos.bbOf(Rook, Dragon))
                    | (bishopAttack(to, occupied) & pos.bbOf(Bishop, Horse));
            if (PT == Silver)
                attackers |= (lanceAttack(oppositeColor(turn), to, occupied) & pos.bbOf(Lance, turn))
                    | (rookAttack(to, occupied) & pos.bbOf(Rook, Dragon))
                    | (bishopAttack(to, occupied) & pos.bbOf(Bishop, Horse));
            if (PT == Bishop)
                attackers |= (bishopAttack(to, occupied) & pos.bbOf(Bishop, Horse));
            if (PT == Rook)
                attackers |= (lanceAttack(oppositeColor(turn), to, occupied) & pos.bbOf(Lance, turn))
                    | (lanceAttack(turn, to, occupied) & pos.bbOf(Lance, oppositeColor(turn)))
                    | (rookAttack(to, occupied) & pos.bbOf(Rook, Dragon));

            if (PT == Pawn || PT == Lance || PT == Knight)
                if (canPromote(turn, makeRank(to)))
                    return PT + PTPromote;
            if (PT == Silver || PT == Bishop || PT == Rook)
                if (canPromote(turn, makeRank(to)) || canPromote(turn, makeRank(from)))
                    return PT + PTPromote;
            return PT;
        }
        return nextAttacker<SEENextPieceType<PT>::value>(pos, to, opponentAttackers, occupied, attackers, turn);
    }
    template <> FORCE_INLINE PieceType nextAttacker<King>(const Position&, const Square, const Bitboard&,
                                                          Bitboard&, Bitboard&, const Color)
    {
        return King;
    }
}

Score Position::see(const Move move, const int asymmThreshold) const {
    const Square to = move.to();
    Square from;
    PieceType ptCaptured;
    Bitboard occ = occupiedBB();
    Bitboard attackers;
    Bitboard opponentAttackers;
    Color turn = oppositeColor(this->turn());
    Score swapList[32];
    if (move.isDrop()) {
        opponentAttackers = attackersTo(turn, to, occ);
        if (!opponentAttackers)
            return ScoreZero;
        attackers = opponentAttackers | attackersTo(oppositeColor(turn), to, occ);
        swapList[0] = ScoreZero;
        ptCaptured = move.pieceTypeDropped();
    }
    else {
        from = move.from();
        occ.xorBit(from);
        opponentAttackers = attackersTo(turn, to, occ);
        if (!opponentAttackers) {
            if (move.isPromotion()) {
                const PieceType ptFrom = move.pieceTypeFrom();
                return capturePieceScore(move.cap()) + promotePieceScore(ptFrom);
            }
            return capturePieceScore(move.cap());
        }
        attackers = opponentAttackers | attackersTo(oppositeColor(turn), to, occ);
        swapList[0] = capturePieceScore(move.cap());
        ptCaptured = move.pieceTypeFrom();
        if (move.isPromotion()) {
            const PieceType ptFrom = move.pieceTypeFrom();
            swapList[0] += promotePieceScore(ptFrom);
            ptCaptured += PTPromote;
        }
    }

    int slIndex = 1;
    do {
        swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);

        ptCaptured = nextAttacker<Pawn>(*this, to, opponentAttackers, occ, attackers, turn);

        attackers &= occ;
        ++slIndex;
        turn = oppositeColor(turn);
        opponentAttackers = attackers & bbOf(turn);

        if (ptCaptured == King) {
            if (opponentAttackers)
                swapList[slIndex++] = CaptureKingScore;
            break;
        }
    } while (opponentAttackers);

    if (asymmThreshold) {
        for (int i = 0; i < slIndex; i += 2) {
            if (swapList[i] < asymmThreshold)
                swapList[i] = -CaptureKingScore;
        }
    }

    // nega max çE«é§ãEåãåãã®ç¹æ°ãæ±ãããE    while (--slIndex)
        swapList[slIndex-1] = std::min(-swapList[slIndex], swapList[slIndex-1]);
    return swapList[0];
}

Score Position::seeSign(const Move move) const {
    if (move.isCapture()) {
        const PieceType ptFrom = move.pieceTypeFrom();
        const Square to = move.to();
        if (capturePieceScore(ptFrom) <= capturePieceScore(piece(to)))
            return static_cast<Score>(1);
    }
    return see(move);
}

namespace {
    // them(ç¸æE å´ã®çãéEãããããE    // sq : çæããç¸æãEé§ãEä½ç½®ãç´ä»ãããæ¡é¦¬ã®ä½ç½®ã¨ããããã£ã¦ãçã¯ sq ã«ã¯è¡ããªãEE    // bb : sq ã®å©ããEããå ´æã®Bitboardããã£ã¦ãçã¯ bb ã®ããããç«ã£ã¦ãEå ´æã«ã¯è¡ããªãEE    // sq ã¨ ksq ã®ä½ç½®ã® Occupied Bitboard ã®ã¿ã¯ãããã§æ´æ°ãã¦è©ä¾¡ããåEã«æ»ããE    // (å®éã«ã¯ãE³ãã©ãªã®Occupied Bitboard ãä½¿ãEEã§ãåEã«ã¯æ»ããªãEE
    bool canKingEscape(const Position& pos, const Color us, const Square sq, const Bitboard& bb) {
        const Color them = oppositeColor(us);
        const Square ksq = pos.kingSquare(them);
        Bitboard kingMoveBB = bb.notThisAnd(pos.bbOf(them).notThisAnd(kingAttack(ksq)));
        kingMoveBB.clearBit(sq); // sq ã«ã¯è¡ããªãEEã§ãã¯ãªã¢ãããxorBit(sq)ã§ã¯ãã¡ãE
        if (kingMoveBB) {
            Bitboard tempOccupied = pos.occupiedBB();
            tempOccupied.setBit(sq);
            tempOccupied.clearBit(ksq);
            do {
                const Square to = kingMoveBB.firstOneFromSQ11();
                // çãEç§»ååEã«ãus å´ã®å©ããç¡ããã°ãtrue
                if (!pos.attackersToIsAny(us, to, tempOccupied))
                    return true;
            } while (kingMoveBB);
        }
        // çãEç§»ååEãç¡ãEE        return false;
    }
    // them(ç¸æE å´ã®çä»¥å¤ãEé§ã sq ã«ãã us å´ã®é§ãåããããE    bool canPieceCapture(const Position& pos, const Color them, const Square sq, const Bitboard& dcBB) {
        // çä»¥å¤ã§æã£ãé§ãåããç¸æåEã®é§ãE Bitboard
        Bitboard fromBB = pos.attackersToExceptKing(them, sq);

        if (fromBB) {
            const Square ksq = pos.kingSquare(them);
            do {
                const Square from = fromBB.firstOneFromSQ11();
                if (!pos.isDiscoveredCheck(from, sq, ksq, dcBB))
                    // them å´ããè¦ã¦ãpin ããã¦ãEªãE§ã§ãæãããé§ãåãããEã§ãtrue
                    return true;
            } while (fromBB);
        }
        // çä»¥å¤ãEé§ã§ãæã£ãé§ãåããªãEE        return false;
    }

    // pos.discoveredCheckBB<false>() ãéå»¶è©ä¾¡ããããEã¸ã§ã³ãE    bool canPieceCapture(const Position& pos, const Color them, const Square sq) {
        Bitboard fromBB = pos.attackersToExceptKing(them, sq);

        if (fromBB) {
            const Square ksq = pos.kingSquare(them);
            const Bitboard dcBB = pos.discoveredCheckBB<false>();
            do {
                const Square from = fromBB.firstOneFromSQ11();
                if (!pos.isDiscoveredCheck(from, sq, ksq, dcBB))
                    // them å´ããè¦ã¦ãpin ããã¦ãEªãE§ã§ãæãããé§ãåãããEã§ãtrue
                    return true;
            } while (fromBB);
        }
        // çä»¥å¤ãEé§ã§ãæã£ãé§ãåããªãEE        return false;
    }
}

// us ãEsq ã¸æ­©ãæã£ãã¨ããthem ã®çãè©°ãããE// us ãEsq ã¸æ­©ãæã¤ã®ã¯çæã§ããã¨ä»®å®ãããE// æã¡æ­©è©°ããEã¨ããtrue ãè¿ããEbool Position::isPawnDropCheckMate(const Color us, const Square sq) const {
    const Color them = oppositeColor(us);
    // çä»¥å¤ãEé§ã§ãæãããæ­©ãåãããªããæã¡æ­©è©°ãã§ã¯ãªãEE    if (canPieceCapture(*this, them, sq))
        return false;
    // todo: ããã§çãEä½ç½®ãæ±ãããEã¯ãä¸ä½ã§æ±ãããã®ã¨2éã«ãªããEã§ç¡é§Eå¾ã§æ´çEããã¨ãE    const Square ksq = kingSquare(them);

    // çä»¥å¤ã§æã£ãæ­©ãåããªãE¨ããçãæ­©ãåãããçãéEãããE
    // å©ããæ±ããéã«ãoccupied ã®æ­©ãæã£ãä½ç½®ã® bit ãç«ã¦ãEBitboard ãä½¿ç¨ãããE    // ããã§ã¯æ­©ã® Bitboard ã¯æ´æ°ããå¿E¦ããªãEE    // color ã® Bitboard ãæ´æ°ããå¿E¦ããªãEEç¸æçãåãã¨ãããã¡ããEæã£ãæ­©ã§çãåããã¨ã¯ç¡ãEºãE
    const Bitboard tempOccupied = occupiedBB() | setMaskBB(sq);
    Bitboard kingMoveBB = bbOf(them).notThisAnd(kingAttack(ksq));

    // å°ãªãã¨ãæ­©ãåãæ¹åã«ã¯çãåãããEããªã®ã§ãdo while ãä½¿ç¨ãE    assert(kingMoveBB);
    do {
        const Square to = kingMoveBB.firstOneFromSQ11();
        if (!attackersToIsAny(us, to, tempOccupied))
            // ç¸æçã®ç§»ååEã«èªé§ãEå©ãããªãEªããæã¡æ­©è©°ãã§ã¯ãªãEE            return false;
    } while (kingMoveBB);

    return true;
}

inline void Position::xorBBs(const PieceType pt, const Square sq, const Color c) {
    byTypeBB_[Occupied].xorBit(sq);
    byTypeBB_[pt].xorBit(sq);
    byColorBB_[c].xorBit(sq);
}

// ç¸æçãEæè©°ã¿ãã©ãEãå¤å®ãE// 1æè©°ã¿ãªããè©°ã¿ã«è³ãæãæã®ä¸é¨ã®æE ±(from, to ã®ã¿ã¨ãEãè¿ããE// 1æè©°ã¿ã§ãªãEªããMove::moveNone() ãè¿ããE// Bitboard ã®ç¶æãéä¸­ã§æ´æ°ããçºãconst é¢æ°ã§ã¯ãªãEEæ´æ°å¾ãåEã«æ»ãããE
template <Color US> Move Position::mateMoveIn1Ply() {
    const Color Them = oppositeColor(US);
    const Square ksq = kingSquare(Them);
    const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);

    assert(!attackersToIsAny(Them, kingSquare(US)));

    // é§æã¡ãèª¿ã¹ããE    const Bitboard dropTarget = nOccupiedBB(); // emptyBB() ã§ã¯ãªãEEã§æ³¨æãã¦ä½¿ãEã¨ãE    const Hand ourHand = hand(US);
    // çæããåãEç¶æãE dcBBãE    // éã«ããé§ãEç¸æåEã®é§ãE    // é§æã¡ã®ã¨ããEãæã£ãå¾ããæãããåã®ç¶æãE dcBB ãä½¿ç¨ãããE    const Bitboard dcBB_betweenIsThem = discoveredCheckBB<false>();

    // é£è»æã¡
    if (ourHand.exists<HRook>()) {
        // åé§ãããã¨ãEãããEEã§ãEæè©°ã¿é¢æ°ã®ä¸­ã§èª¿ã¹ããE        // ããã§ã¯é¢ããä½ç½®ããçæããã®ã¯èEãªãEE        Bitboard toBB = dropTarget & rookStepAttacks(ksq);
        while (toBB) {
            const Square to = toBB.firstOneFromSQ11();
            // é§ãæã£ãå ´æã«èªé§ãEå©ããããããEç¡ããã°çã§åããã¦è©°ã¾ãªãE
            if (attackersToIsAny(US, to)) {
                // çãéEããããä»ãEé§ã§åããã¨ãåEæ¥ãªãE
                if (!canKingEscape(*this, US, to, rookAttackToEdge(to))
                    && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                {
                    return makeDropMove(Rook, to);
                }
            }
        }
    }
    // é¦è»æã¡
    // é£è»ã§è©°ã¾ãªããã°é¦è»ã§ãè©°ã¾ãªãEEã§ãelse if ãä½¿ç¨ãE    // çã 9(1) æ®µç®ã«ãEã°é¦è»ã§çæåºæ¥ç¡ãEEã§ããããçããE    else if (ourHand.exists<HLance>() && isInFrontOf<US, Rank9, Rank1>(makeRank(ksq))) {
        const Square to = ksq + TDeltaS;
        if (piece(to) == Empty && attackersToIsAny(US, to)) {
            if (!canKingEscape(*this, US, to, lanceAttackToEdge(US, to))
                && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
            {
                return makeDropMove(Lance, to);
            }
        }
    }

    // è§æã¡
    if (ourHand.exists<HBishop>()) {
        Bitboard toBB = dropTarget & bishopStepAttacks(ksq);
        while (toBB) {
            const Square to = toBB.firstOneFromSQ11();
            if (attackersToIsAny(US, to)) {
                if (!canKingEscape(*this, US, to, bishopAttackToEdge(to))
                    && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                {
                    return makeDropMove(Bishop, to);
                }
            }
        }
    }

    // éæã¡
    if (ourHand.exists<HGold>()) {
        Bitboard toBB;
        if (ourHand.exists<HRook>())
            // é£è»æã¡ãåEã«èª¿ã¹ããEã§ãå°»éã ããEçããE            toBB = dropTarget & (goldAttack(Them, ksq) ^ pawnAttack(US, ksq));
        else
            toBB = dropTarget & goldAttack(Them, ksq);
        while (toBB) {
            const Square to = toBB.firstOneFromSQ11();
            if (attackersToIsAny(US, to)) {
                if (!canKingEscape(*this, US, to, goldAttack(US, to))
                    && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                {
                    return makeDropMove(Gold, to);
                }
            }
        }
    }

    if (ourHand.exists<HSilver>()) {
        Bitboard toBB;
        if (ourHand.exists<HGold>()) {
            // éæã¡ãåEã«èª¿ã¹ããEã§ãæãå¾ãããæã¤å ´åã ããèª¿ã¹ããE
            if (ourHand.exists<HBishop>())
                // è§æã¡ãåEã«èª¿ã¹ããEã§ãæããããEçæãé¤å¤ã§ãããéæã¡ãèª¿ã¹ãå¿E¦ããªãEE                goto silver_drop_end;
            // æãå¾ãããæã¤å ´åãèª¿ã¹ãå¿E¦ããããE            toBB = dropTarget & (silverAttack(Them, ksq) & inFrontMask(US, makeRank(ksq)));
        }
        else {
            if (ourHand.exists<HBishop>())
                // æãå¾ããé¤å¤ãåæ¹ããæã¤å ´åãèª¿ã¹ãå¿E¦ããããE                toBB = dropTarget & goldAndSilverAttacks(Them, ksq);
            else
                toBB = dropTarget & silverAttack(Them, ksq);
        }
        while (toBB) {
            const Square to = toBB.firstOneFromSQ11();
            if (attackersToIsAny(US, to)) {
                if (!canKingEscape(*this, US, to, silverAttack(US, to))
                    && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                {
                    return makeDropMove(Silver, to);
                }
            }
        }
    }
silver_drop_end:

    if (ourHand.exists<HKnight>()) {
        Bitboard toBB = dropTarget & knightAttack(Them, ksq);
        while (toBB) {
            const Square to = toBB.firstOneFromSQ11();
            // æ¡é¦¬ã¯ç´ãä»ãã¦ãEå¿E¦ãEãªãEE            // ãã£ã¦ããã®canKingEscape() åE§ã® to ã®ä½ç½®ã«éEãããªãEãE«ããå¦çEEç¡é§EE            if (!canKingEscape(*this, US, to, allZeroBB())
                && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
            {
                return makeDropMove(Knight, to);
            }
        }
    }

    // æ­©æã¡ã§è©°ã¾ãã¨ååãªã®ã§ãèª¿ã¹ãªãEE
    // é§ãç§»åããå ´åE    // moveTarget ã¯æ¡é¦¬ä»¥å¤ãEç§»ååEã®å¤§ã¾ããªä½ç½®ãé£è§é¦ãEé éçæãEå«ã¾ãªãEE    const Bitboard moveTarget = bbOf(US).notThisAnd(kingAttack(ksq));
    const Bitboard pinned = pinnedBB();
    const Bitboard dcBB_betweenIsUs = discoveredCheckBB<true>();

    {
        // ç«ã«ããç§»åE        Bitboard fromBB = bbOf(Dragon, US);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            // é éçæãEèEãªãEE            Bitboard toBB = moveTarget & attacksFrom<Dragon>(from);
            if (toBB) {
                xorBBs(Dragon, from, US);
                // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // çæããé§ãEå ´æã«èªé§ãEå©ããããããEç¡ããã°çã§åããã¦è©°ã¾ãªãE
                    if (unDropCheckIsSupported(US, to)) {
                        // çãéEãããªãE                        // ãã¤ãEç©ºãçæEã¾ããE ä»ãEé§ã§åããªãE
                        // ãã¤ãçæããé§ã pin ããã¦ãEªãE                        if (!canKingEscape(*this, US, to, attacksFrom<Dragon>(to, occupiedBB() ^ setMaskBB(ksq)))
                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                            && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                        {
                            xorBBs(Dragon, from, US);
                            return makeCaptureMove(Dragon, from, to, *this);
                        }
                    }
                } while (toBB);
                xorBBs(Dragon, from, US);
            }
        }
    }

    // Txxx ã¯åæãå¾æã®æE ±ãå¸åããå¤æ°ãæ°å­ãEåæã«åããã¦ãEãE    const Rank TRank4 = (US == Black ? Rank4 : Rank6);
    const Bitboard TRank123BB = inFrontMask<US, TRank4>();
    {
        // é£è»ã«ããç§»åE        Bitboard fromBB = bbOf(Rook, US);
        Bitboard fromOn123BB = fromBB & TRank123BB;
        // from ãE123 æ®µç®
        if (fromOn123BB) {
            fromBB.andEqualNot(TRank123BB);
            do {
                const Square from = fromOn123BB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Rook>(from);
                if (toBB) {
                    xorBBs(Rook, from, US);
                    // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                    do {
                        const Square to = toBB.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            if (!canKingEscape(*this, US, to, attacksFrom<Dragon>(to, occupiedBB() ^ setMaskBB(ksq)))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Rook, from, US);
                                return makeCapturePromoteMove(Rook, from, to, *this);
                            }
                        }
                    } while (toBB);
                    xorBBs(Rook, from, US);
                }
            } while (fromOn123BB);
        }

        // from ãE4~9 æ®µç®
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Rook>(from) & (rookStepAttacks(ksq) | TRank123BB);
            if (toBB) {
                xorBBs(Rook, from, US);
                // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();

                Bitboard toOn123BB = toBB & TRank123BB;
                // æã
                if (toOn123BB) {
                    do {
                        const Square to = toOn123BB.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            if (!canKingEscape(*this, US, to, attacksFrom<Dragon>(to, occupiedBB() ^ setMaskBB(ksq)))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Rook, from, US);
                                return makeCapturePromoteMove(Rook, from, to, *this);
                            }
                        }
                    } while (toOn123BB);

                    toBB.andEqualNot(TRank123BB);
                }
                // ä¸æE
                while (toBB) {
                    const Square to = toBB.firstOneFromSQ11();
                    if (unDropCheckIsSupported(US, to)) {
                        if (!canKingEscape(*this, US, to, rookAttackToEdge(to))
                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                            && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                        {
                            xorBBs(Rook, from, US);
                            return makeCaptureMove(Rook, from, to, *this);
                        }
                    }
                }
                xorBBs(Rook, from, US);
            }
        }
    }

    {
        // é¦¬ã«ããç§»åE        Bitboard fromBB = bbOf(Horse, US);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            // é éçæãEèEãªãEE            Bitboard toBB = moveTarget & attacksFrom<Horse>(from);
            if (toBB) {
                xorBBs(Horse, from, US);
                // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // çæããé§ãEå ´æã«èªé§ãEå©ããããããEç¡ããã°çã§åããã¦è©°ã¾ãªãE
                    if (unDropCheckIsSupported(US, to)) {
                        // çãéEãããªãE                        // ãã¤ãEç©ºãçæEã¾ããE ä»ãEé§ã§åããªãE
                        // ãã¤ãåãããé§ã pin ããã¦ãEªãE
                        if (!canKingEscape(*this, US, to, horseAttackToEdge(to)) // ç«ãEå ´åã¨éã£ã¦ãå¸¸ã«æå¤§ã®å©ããä½¿ç¨ãã¦è¯ãEE                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                            && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                        {
                            xorBBs(Horse, from, US);
                            return makeCaptureMove(Horse, from, to, *this);
                        }
                    }
                } while (toBB);
                xorBBs(Horse, from, US);
            }
        }
    }

    {
        // è§ã«ããç§»åE        Bitboard fromBB = bbOf(Bishop, US);
        Bitboard fromOn123BB = fromBB & TRank123BB;
        // from ãE123 æ®µç®
        if (fromOn123BB) {
            fromBB.andEqualNot(TRank123BB);
            do {
                const Square from = fromOn123BB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Bishop>(from);
                if (toBB) {
                    xorBBs(Bishop, from, US);
                    // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                    do {
                        const Square to = toBB.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            if (!canKingEscape(*this, US, to, horseAttackToEdge(to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Bishop, from, US);
                                return makeCapturePromoteMove(Bishop, from, to, *this);
                            }
                        }
                    } while (toBB);
                    xorBBs(Bishop, from, US);
                }
            } while (fromOn123BB);
        }

        // from ãE4~9 æ®µç®
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Bishop>(from) & (bishopStepAttacks(ksq) | TRank123BB);
            if (toBB) {
                xorBBs(Bishop, from, US);
                // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();

                Bitboard toOn123BB = toBB & TRank123BB;
                // æã
                if (toOn123BB) {
                    do {
                        const Square to = toOn123BB.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            if (!canKingEscape(*this, US, to, horseAttackToEdge(to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Bishop, from, US);
                                return makeCapturePromoteMove(Bishop, from, to, *this);
                            }
                        }
                    } while (toOn123BB);

                    toBB.andEqualNot(TRank123BB);
                }
                // ä¸æE
                while (toBB) {
                    const Square to = toBB.firstOneFromSQ11();
                    if (unDropCheckIsSupported(US, to)) {
                        if (!canKingEscape(*this, US, to, bishopAttackToEdge(to))
                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                            && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                        {
                            xorBBs(Bishop, from, US);
                            return makeCaptureMove(Bishop, from, to, *this);
                        }
                    }
                }
                xorBBs(Bishop, from, US);
            }
        }
    }

    {
        // éãæEãéßã«ããç§»åE        Bitboard fromBB = goldsBB(US) & goldCheckTable(US, ksq);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Gold>(US, from) & attacksFrom<Gold>(Them, ksq);
            if (toBB) {
                const PieceType pt = pieceToPieceType(piece(from));
                xorBBs(pt, from, US);
                goldsBB_.xorBit(from);
                // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // çæããé§ãEå ´æã«èªé§ãEå©ããããããEç¡ããã°çã§åããã¦è©°ã¾ãªãE
                    if (unDropCheckIsSupported(US, to)) {
                        // çãéEãããªãE                        // ãã¤ãEç©ºãçæEã¾ããE ä»ãEé§ã§åããªãE
                        // ãã¤ãåãããé§ã pin ããã¦ãEªãE
                        if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                            && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                        {
                            xorBBs(pt, from, US);
                            goldsBB_.xorBit(from);
                            return makeCaptureMove(pt, from, to, *this);
                        }
                    }
                } while (toBB);
                xorBBs(pt, from, US);
                goldsBB_.xorBit(from);
            }
        }
    }

    {
        // éã«ããç§»åE        Bitboard fromBB = bbOf(Silver, US) & silverCheckTable(US, ksq);
        if (fromBB) {
            // Txxx ã¯åæãå¾æã®æE ±ãå¸åããå¤æ°ãæ°å­ãEåæã«åããã¦ãEãE            const Bitboard TRank5_9BB = inFrontMask<Them, TRank4>();
            const Bitboard chkBB = attacksFrom<Silver>(Them, ksq);
            const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq);

            Bitboard fromOn123BB = fromBB & TRank123BB;
            // from ãæµé£
            if (fromOn123BB) {
                fromBB.andEqualNot(TRank123BB);
                do {
                    const Square from = fromOn123BB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
                    Bitboard toBB_promo = toBB & chkBB_promo;

                    toBB &= chkBB;
                    if ((toBB_promo | toBB)) {
                        xorBBs(Silver, from, US);
                        // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                        while (toBB_promo) {
                            const Square to = toBB_promo.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // æã
                                if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Silver, from, US);
                                    return makeCapturePromoteMove(Silver, from, to, *this);
                                }
                            }
                        }

                        // çãEåæ¹ã«ç§»åããå ´åãæEã§è©°ã¾ãªãã£ããä¸æEã§ãè©°ã¾ãªãEEã§ãããã§çããE                        // sakurapyon ã®ä½èEè¨ã£ã¦ããEã§å®è£EE                        toBB.andEqualNot(inFrontMask(Them, makeRank(ksq)));
                        while (toBB) {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // ä¸æE
                                if (!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Silver, from, US);
                                    return makeCaptureMove(Silver, from, to, *this);
                                }
                            }
                        }

                        xorBBs(Silver, from, US);
                    }
                } while (fromOn123BB);
            }

            // from ãE5~9æ®µç® (å¿Eä¸æE)
            Bitboard fromOn5_9BB = fromBB & TRank5_9BB;
            if (fromOn5_9BB) {
                fromBB.andEqualNot(TRank5_9BB);
                do {
                    const Square from = fromOn5_9BB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from) & chkBB;

                    if (toBB) {
                        xorBBs(Silver, from, US);
                        // åããå¾ãE dcBB, pinned: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                        while (toBB) {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // ä¸æE
                                if (!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Silver, from, US);
                                    return makeCaptureMove(Silver, from, to, *this);
                                }
                            }
                        }

                        xorBBs(Silver, from, US);
                    }
                } while (fromOn5_9BB);
            }

            // æ®ã 4 æ®µç®ã®ã¿
            // åé²ããã¨ããEæããããå¾éããã¨ããEæããªãEE            while (fromBB) {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
                Bitboard toBB_promo = toBB & TRank123BB & chkBB_promo; // 3 æ®µç®ã«ããæããªãEE
                toBB &= chkBB;
                if ((toBB_promo | toBB)) {
                    xorBBs(Silver, from, US);
                    // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // æã
                            if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Silver, from, US);
                                return makeCapturePromoteMove(Silver, from, to, *this);
                            }
                        }
                    }

                    while (toBB) {
                        const Square to = toBB.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // ä¸æE
                            if (!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Silver, from, US);
                                return makeCaptureMove(Silver, from, to, *this);
                            }
                        }
                    }

                    xorBBs(Silver, from, US);
                }
            }
        }
    }

    {
        // æ¡ã«ããç§»åE        Bitboard fromBB = bbOf(Knight, US) & knightCheckTable(US, ksq);
        if (fromBB) {
            const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq) & TRank123BB;
            const Bitboard chkBB = attacksFrom<Knight>(Them, ksq);

            do {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = bbOf(US).notThisAnd(attacksFrom<Knight>(US, from));
                Bitboard toBB_promo = toBB & chkBB_promo;
                toBB &= chkBB;
                if ((toBB_promo | toBB)) {
                    xorBBs(Knight, from, US);
                    // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // æã
                            if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                && (isDiscoveredCheck<true>(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal<true>(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Knight, from, US);
                                return makeCapturePromoteMove(Knight, from, to, *this);
                            }
                        }
                    }

                    while (toBB) {
                        const Square to = toBB.firstOneFromSQ11();
                        // æ¡é¦¬ã¯ç´ãä»ãã¦ãªãã¦è¯ãEEã§ãç´ãä»ãã¦ãEããEèª¿ã¹ãªãEE                        // ä¸æE
                        if (!canKingEscape(*this, US, to, allZeroBB())
                            && (isDiscoveredCheck<true>(from, to, ksq, dcBB_betweenIsUs)
                                || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                            && !isPinnedIllegal<true>(from, to, kingSquare(US), pinned))
                        {
                            xorBBs(Knight, from, US);
                            return makeCaptureMove(Knight, from, to, *this);
                        }
                    }
                    xorBBs(Knight, from, US);
                }
            } while (fromBB);
        }
    }

    {
        // é¦è»ã«ããç§»åE        Bitboard fromBB = bbOf(Lance, US) & lanceCheckTable(US, ksq);
        if (fromBB) {
            // Txxx ã¯åæãå¾æã®æE ±ãå¸åããå¤æ°ãæ°å­ãEåæã«åããã¦ãEãE            const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
            const Rank TRank2 = (US == Black ? Rank2 : Rank8);
            const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq) & TRank123BB;
            // çãEåæ¹1ãã¹ã®ã¿ãE            // çã 1 æ®µç®ã«ãEã¨ããEãæEã®ã¿ã§è¯ãEEã§çããE            const Bitboard chkBB = attacksFrom<Pawn>(Them, ksq) & inFrontMask<Them, TRank2>();

            do {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Lance>(US, from);
                Bitboard toBB_promo = toBB & chkBB_promo;

                toBB &= chkBB;

                if ((toBB_promo | toBB)) {
                    xorBBs(Lance, from, US);
                    // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE
                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // æã
                            if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Lance, from, US);
                                return makeCapturePromoteMove(Lance, from, to, *this);
                            }
                        }
                    }

                    if (toBB) {
                        assert(toBB.isOneBit());
                        // ä¸æEã§çæåºæ¥ããEã¯ãä¸ã¤ã®å ´æã ããªã®ã§ãã«ã¼ãã«ããå¿E¦ãç¡ãEE                        const Square to = ksq + TDeltaS;
                        if (unDropCheckIsSupported(US, to)) {
                            // ä¸æE
                            if (!canKingEscape(*this, US, to, lanceAttackToEdge(US, to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Lance, from, US);
                                return makeCaptureMove(Lance, from, to, *this);
                            }
                        }
                    }
                    xorBBs(Lance, from, US);
                }
            } while (fromBB);
        }
    }

    {
        // æ­©ã«ããç§»åE        // æããå ´åãEå¿EãªããE        // todo: PawnCheckBB ä½ã£ã¦ç°¡ç¥åãããE        const Rank krank = makeRank(ksq);
        // æ­©ãç§»åãã¦çæã«ãªããEã¯ãç¸æçãE~7æ®µç®ã®æãEã¿ãE        if (isInFrontOf<US, Rank8, Rank2>(krank)) {
            // Txxx ã¯åæãå¾æã®æE ±ãå¸åããå¤æ°ãæ°å­ãEåæã«åããã¦ãEãE            const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
            const SquareDelta TDeltaN = (US == Black ? DeltaN : DeltaS);

            Bitboard fromBB = bbOf(Pawn, US);
            // çãæµé£ã«ãEªãE¨æã§çæã«ãªããã¨ã¯ãªãEE            if (isInFrontOf<US, Rank4, Rank6>(krank)) {
                // æã£ãæã«çæã«ãªãä½ç½®
                const Bitboard toBB_promo = moveTarget & attacksFrom<Gold>(Them, ksq) & TRank123BB;
                Bitboard fromBB_promo = fromBB & pawnAttack<Them>(toBB_promo);
                while (fromBB_promo) {
                    const Square from = fromBB_promo.firstOneFromSQ11();
                    const Square to = from + TDeltaN;

                    xorBBs(Pawn, from, US);
                    // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                    if (unDropCheckIsSupported(US, to)) {
                        // æã
                        if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                            && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                        {
                            xorBBs(Pawn, from, US);
                            return makeCapturePromoteMove(Pawn, from, to, *this);
                        }
                    }
                    xorBBs(Pawn, from, US);
                }
            }

            // ä¸æE
            // çã 8,9 æ®µç®ã«ãEãã¨ã¯ç¡ãEEã§ãfrom,to ãé£ã®ç­ãæEãã¨ã¯ç¡ãEE            const Square to = ksq + TDeltaS;
            const Square from = to + TDeltaS;
            if (fromBB.isSet(from) && !bbOf(US).isSet(to)) {
                // çã 1, 2 æ®µç®ã«ãEãªããæEãã§çæåºæ¥ããEã§ä¸æEã¯èª¿ã¹ãªãEE                if (isBehind<US, Rank2, Rank8>(krank)) {
                    xorBBs(Pawn, from, US);
                    // åããå¾ãE dcBB: to ã®ä½ç½®ã® occupied ãEcheckers ã¯é¢ä¿ãªãEEã§ãããã§çæEã§ãããE                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to ã®ä½ç½®ã® Bitboard ã¯ canKingEscape ã®ä¸­ã§æ´æ°ãããE                    if (unDropCheckIsSupported(US, to)) {
                        // ä¸æE
                        if (!canKingEscape(*this, US, to, allZeroBB())
                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                            && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                        {
                            xorBBs(Pawn, from, US);
                            return makeCaptureMove(Pawn, from, to, *this);
                        }
                    }
                    xorBBs(Pawn, from, US);
                }
            }
        }
    }

    return Move::moveNone();
}

Move Position::mateMoveIn1Ply() {
    return (turn() == Black ? mateMoveIn1Ply<Black>() : mateMoveIn1Ply<White>());
}

void Position::initZobrist() {
    // zobTurn_ ã¯ 1 ã§ããããã®ä»ãE 1æ¡ç®ãä½¿ããªãEE    // zobTurn ã®ã¿ xor ã§æ´æ°ããçºãä»ãEæ¡ã«å½±é¿ããªãEãE«ããçºãE    // hashå¤ã®æ´æ°ã¯æ®éãEå¨ã¦ xor ãä½¿ãEãæã¡é§ãEæ´æ°ã®çºã« +, - ãä½¿ç¨ããæ¹ãéEåãè¯ãEE    for (PieceType pt = Occupied; pt < PieceTypeNum; ++pt) {
        for (Square sq = SQ11; sq < SquareNum; ++sq) {
            for (Color c = Black; c < ColorNum; ++c)
                zobrist_[pt][sq][c] = g_mt64bit.random() & ~UINT64_C(1);
        }
    }
    for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
        zobHand_[hp][Black] = g_mt64bit.random() & ~UINT64_C(1);
        zobHand_[hp][White] = g_mt64bit.random() & ~UINT64_C(1);
    }
    zobExclusion_ = g_mt64bit.random() & ~UINT64_C(1);
}

void Position::print() const {
    std::cout << "'  9  8  7  6  5  4  3  2  1" << std::endl;
    int i = 0;
    for (Rank r = Rank1; r < RankNum; ++r) {
        ++i;
        std::cout << "P" << i;
        for (File f = File9; File1 <= f; --f)
            std::cout << pieceToCharCSA(piece(makeSquare(f, r)));
        std::cout << std::endl;
    }
    printHand(Black);
    printHand(White);
    std::cout << (turn() == Black ? "+" : "-") << std::endl;
    std::cout << std::endl;
    std::cout << "key = " << getKey() << std::endl;
}

std::string Position::toSFEN(const Ply ply) const {
    std::stringstream ss;
    ss << "sfen ";
    int space = 0;
    for (Rank rank = Rank1; rank <= Rank9; ++rank) {
        for (File file = File9; file >= File1; --file) {
            const Square sq = makeSquare(file, rank);
            const Piece pc = piece(sq);
            if (pc == Empty)
                ++space;
            else {
                if (space) {
                    ss << space;
                    space = 0;
                }
                ss << pieceToCharUSI(pc);
            }
        }
        if (space) {
            ss << space;
            space = 0;
        }
        if (rank != Rank9)
            ss << "/";
    }
    ss << (turn() == Black ? " b " : " w ");
    if (hand(Black).value() == 0 && hand(White).value() == 0)
        ss << "- ";
    else {
        // USI ã®è¦æ ¼ã¨ãã¦ãæã¡é§ãEè¡¨è¨é EEæ±ºã¾ã£ã¦ãããåEæãå¾æã®é E§ãããããEé£ãè§ãEßãEãæ¡ãE¦ãæ­© ã®é EE        for (Color color = Black; color < ColorNum; ++color) {
            for (HandPiece hp : {HRook, HBishop, HGold, HSilver, HKnight, HLance, HPawn}) {
                const int num = hand(color).numOf(hp);
                if (num == 0)
                    continue;
                if (num != 1)
                    ss << num;
                const Piece pc = colorAndHandPieceToPiece(color, hp);
                ss << pieceToCharUSI(pc);
            }
        }
        ss << " ";
    }
    ss << ply;
    return ss.str();
}

HuffmanCodedPos Position::toHuffmanCodedPos() const {
    HuffmanCodedPos result;
    result.clear();
    BitStream bs(result.data);
    // æçª (1bit)
    bs.putBit(turn());

    // çãEä½ç½® (7bit * 2)
    bs.putBits(kingSquare(Black), 7);
    bs.putBits(kingSquare(White), 7);

    // ç¤ä¸ãEé§E    for (Square sq = SQ11; sq < SquareNum; ++sq) {
        Piece pc = piece(sq);
        if (pieceToPieceType(pc) == King)
            continue;
        const auto hc = HuffmanCodedPos::boardCodeTable[pc];
        bs.putBits(hc.code, hc.numOfBits);
    }

    // æã¡é§E    for (Color c = Black; c < ColorNum; ++c) {
        const Hand h = hand(c);
        for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
            const auto hc = HuffmanCodedPos::handCodeTable[hp][c];
            for (u32 n = 0; n < h.numOf(hp); ++n)
                bs.putBits(hc.code, hc.numOfBits);
        }
    }
    assert(bs.data() == std::end(result.data));
    assert(bs.curr() == 0);
    return result;
}

#if !defined NDEBUG
bool Position::isOK() const {
    static Key prevKey;
    const bool debugAll = true;

    const bool debugBitboards    = debugAll || false;
    const bool debugKingCount    = debugAll || false;
    const bool debugKingCapture  = debugAll || false;
    const bool debugCheckerCount = debugAll || false;
    const bool debugKey          = debugAll || false;
    const bool debugStateHand    = debugAll || false;
    const bool debugPiece        = debugAll || false;
    const bool debugMaterial     = debugAll || false;

    int failedStep = 0;
    if (debugBitboards) {
        if ((bbOf(Black) & bbOf(White)))
            goto incorrect_position;
        if ((bbOf(Black) | bbOf(White)) != occupiedBB())
            goto incorrect_position;
        if ((bbOf(Pawn     ) ^ bbOf(Lance    ) ^ bbOf(Knight) ^ bbOf(Silver ) ^ bbOf(Bishop  ) ^
             bbOf(Rook     ) ^ bbOf(Gold     ) ^ bbOf(King  ) ^ bbOf(ProPawn) ^ bbOf(ProLance) ^
             bbOf(ProKnight) ^ bbOf(ProSilver) ^ bbOf(Horse ) ^ bbOf(Dragon )) != occupiedBB())
        {
            goto incorrect_position;
        }
        for (PieceType pt1 = Pawn; pt1 < PieceTypeNum; ++pt1) {
            for (PieceType pt2 = pt1 + 1; pt2 < PieceTypeNum; ++pt2) {
                if ((bbOf(pt1) & bbOf(pt2)))
                    goto incorrect_position;
            }
        }
    }

    ++failedStep;
    if (debugKingCount) {
        int kingCount[ColorNum] = {0, 0};
        if (bbOf(King).popCount() != 2)
            goto incorrect_position;
        if (!bbOf(King, Black).isOneBit())
            goto incorrect_position;
        if (!bbOf(King, White).isOneBit())
            goto incorrect_position;
        for (Square sq = SQ11; sq < SquareNum; ++sq) {
            if (piece(sq) == BKing)
                ++kingCount[Black];
            if (piece(sq) == WKing)
                ++kingCount[White];
        }
        if (kingCount[Black] != 1 || kingCount[White] != 1)
            goto incorrect_position;
    }

    ++failedStep;
    if (debugKingCapture) {
        // ç¸æçãåããªãEã¨ãç¢ºèªE        const Color us = turn();
        const Color them = oppositeColor(us);
        const Square ksq = kingSquare(them);
        if (attackersTo(us, ksq))
            goto incorrect_position;
    }

    ++failedStep;
    if (debugCheckerCount) {
        if (2 < st_->checkersBB.popCount())
            goto incorrect_position;
    }

    ++failedStep;
    if (debugKey) {
        if (getKey() != computeKey())
            goto incorrect_position;
    }

    ++failedStep;
    if (debugStateHand) {
        if (st_->hand != hand(turn()))
            goto incorrect_position;
    }

    ++failedStep;
    if (debugPiece) {
        for (Square sq = SQ11; sq < SquareNum; ++sq) {
            const Piece pc = piece(sq);
            if (pc == Empty) {
                if (!emptyBB().isSet(sq))
                    goto incorrect_position;
            }
            else {
                if (!bbOf(pieceToPieceType(pc), pieceToColor(pc)).isSet(sq))
                    goto incorrect_position;
            }
        }
    }

    ++failedStep;
    if (debugMaterial) {
        if (material() != computeMaterial())
            goto incorrect_position;
    }

    ++failedStep;
    {
        int i;
        if ((i = debugSetEvalList()) != 0) {
            std::cout << "debugSetEvalList() error = " << i << std::endl;
            goto incorrect_position;
        }
    }

    prevKey = getKey();
    return true;

incorrect_position:
    std::cout << "Error! failedStep = " << failedStep << std::endl;
    std::cout << "prevKey = " << prevKey << std::endl;
    std::cout << "currKey = " << getKey() << std::endl;
    print();
    return false;
}
#endif

#if !defined NDEBUG
int Position::debugSetEvalList() const {
    // not implement
    return 0;
}
#endif

Key Position::computeBoardKey() const {
    Key result = 0;
    for (Square sq = SQ11; sq < SquareNum; ++sq) {
        if (piece(sq) != Empty)
            result += zobrist(pieceToPieceType(piece(sq)), sq, pieceToColor(piece(sq)));
    }
    if (turn() == White)
        result ^= zobTurn();
    return result;
}

Key Position::computeHandKey() const {
    Key result = 0;
    for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
        for (Color c = Black; c < ColorNum; ++c) {
            const int num = hand(c).numOf(hp);
            for (int i = 0; i < num; ++i)
                result += zobHand(hp, c);
        }
    }
    return result;
}

// todo: isRepetition() ã«ååå¤ããæ¹ãè¯ãããEE//       åä¸å±é¢4åããã¡ãã¨æ°ãã¦ãEªãEã©åé¡ãªãEãERepetitionType Position::isDraw(const int checkMaxPly) const {
    const int Start = 4;
    int i = Start;
    const int e = std::min(st_->pliesFromNull, checkMaxPly);

    // 4ææããªãE¨åE¥æã«ã¯çµ¶å¯¾ã«ãªããªãEE    if (i <= e) {
        // ç¾å¨ã®å±é¢ã¨ãå°ãªãã¨ãE4 ææ»ããªãE¨åãå±é¢ã«ãªããªãEE        // ããã§ã¾ãE2 ææ»ããE        StateInfo* stp = st_->previous->previous;

        do {
            // æ´ã« 2 ææ»ããE            stp = stp->previous->previous;
            if (stp->key() == st_->key()) {
                if (i <= st_->continuousCheck[turn()])
                    return RepetitionLose;
                else if (i <= st_->continuousCheck[oppositeColor(turn())])
                    return RepetitionWin;
#if defined BAN_BLACK_REPETITION
                return (turn() == Black ? RepetitionLose : RepetitionWin);
#elif defined BAN_WHITE_REPETITION
                return (turn() == White ? RepetitionLose : RepetitionWin);
#else
                return RepetitionDraw;
#endif
            }
            else if (stp->boardKey == st_->boardKey) {
                if (st_->hand.isEqualOrSuperior(stp->hand)) return RepetitionSuperior;
                if (stp->hand.isEqualOrSuperior(st_->hand)) return RepetitionInferior;
            }
            i += 2;
        } while (i <= e);
    }
    return NotRepetition;
}

namespace {
    void printHandPiece(const Position& pos, const HandPiece hp, const Color c, const std::string& str) {
        if (pos.hand(c).numOf(hp)) {
            const char* sign = (c == Black ? "+" : "-");
            std::cout << "P" << sign;
            for (u32 i = 0; i < pos.hand(c).numOf(hp); ++i)
                std::cout << "00" << str;
            std::cout << std::endl;
        }
    }
}
void Position::printHand(const Color c) const {
    printHandPiece(*this, HPawn  , c, "FU");
    printHandPiece(*this, HLance , c, "KY");
    printHandPiece(*this, HKnight, c, "KE");
    printHandPiece(*this, HSilver, c, "GI");
    printHandPiece(*this, HGold  , c, "KI");
    printHandPiece(*this, HBishop, c, "KA");
    printHandPiece(*this, HRook  , c, "HI");
}

Position& Position::operator = (const Position& pos) {
    memcpy(this, &pos, sizeof(Position));
    startState_ = *st_;
    st_ = &startState_;
    nodes_ = 0;

    assert(isOK());
    return *this;
}

void Position::set(const std::string& sfen, Thread* th) {
    Piece promoteFlag = UnPromoted;
    std::istringstream ss(sfen);
    char token;
    Square sq = SQ91;

    Searcher* s = std::move(searcher_);
    clear();
    setSearcher(s);

    // ç¤ä¸ãEé§E    while (ss.get(token) && token != ' ') {
        if (isdigit(token))
            sq += DeltaE * (token - '0');
        else if (token == '/')
            sq += (DeltaW * 9) + DeltaS;
        else if (token == '+')
            promoteFlag = Promoted;
        else if (g_charToPieceUSI.isLegalChar(token)) {
            if (isInSquare(sq)) {
                setPiece(g_charToPieceUSI.value(token) + promoteFlag, sq);
                promoteFlag = UnPromoted;
                sq += DeltaE;
            }
            else
                goto INCORRECT;
        }
        else
            goto INCORRECT;
    }
    kingSquare_[Black] = bbOf(King, Black).constFirstOneFromSQ11();
    kingSquare_[White] = bbOf(King, White).constFirstOneFromSQ11();
    goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);

    // æçª
    while (ss.get(token) && token != ' ') {
        if (token == 'b')
            turn_ = Black;
        else if (token == 'w')
            turn_ = White;
        else
            goto INCORRECT;
    }

    // æã¡é§E    for (int digits = 0; ss.get(token) && token != ' '; ) {
        if (token == '-')
            memset(hand_, 0, sizeof(hand_));
        else if (isdigit(token))
            digits = digits * 10 + token - '0';
        else if (g_charToPieceUSI.isLegalChar(token)) {
            // æã¡é§ã32bit ã« pack ãã
            const Piece piece = g_charToPieceUSI.value(token);
            setHand(piece, (digits == 0 ? 1 : digits));

            digits = 0;
        }
        else
            goto INCORRECT;
    }

    // æ¬¡ã®æãä½æç®ãE    ss >> gamePly_;

    // æ®ãæé, hash key, (ããå®è£EããªãEé§çªå·ãªã©ãããã§è¨­å®E    st_->boardKey = computeBoardKey();
    st_->handKey = computeHandKey();
    st_->hand = hand(turn());

    setEvalList();
    findCheckers();
    st_->material = computeMaterial();
    thisThread_ = th;

    return;
INCORRECT:
    std::cout << "incorrect SFEN string : " << sfen << std::endl;
}

bool Position::set(const HuffmanCodedPos& hcp, Thread* th) {
    Searcher* s = std::move(searcher_);
    clear();
    setSearcher(s);

    HuffmanCodedPos tmp = hcp; // ã­ã¼ã«ã«ã«ã³ããE
    BitStream bs(tmp.data);

    // æçª
    turn_ = static_cast<Color>(bs.getBit());

    // çãEä½ç½®
    Square sq0 = (Square)bs.getBits(7);
    Square sq1 = (Square)bs.getBits(7);
    setPiece(BKing, static_cast<Square>(sq0));
    setPiece(WKing, static_cast<Square>(sq1));

    // ç¤ä¸ãEé§E    for (Square sq = SQ11; sq < SquareNum; ++sq) {
        if (pieceToPieceType(piece(sq)) == King) // piece(sq) ã¯ BKing, WKing, Empty ã®ã©ãããE            continue;
        HuffmanCode hc = {0, 0};
        while (hc.numOfBits <= 8) {
            hc.code |= bs.getBit() << hc.numOfBits++;
            if (HuffmanCodedPos::boardCodeToPieceHash.value(hc.key) != PieceNone) {
                const Piece pc = HuffmanCodedPos::boardCodeToPieceHash.value(hc.key);
                if (pc != Empty)
                    setPiece(HuffmanCodedPos::boardCodeToPieceHash.value(hc.key), sq);
                break;
            }
        }
        if (HuffmanCodedPos::boardCodeToPieceHash.value(hc.key) == PieceNone)
            goto INCORRECT_HUFFMAN_CODE;
    }
    while (bs.data() != std::end(tmp.data)) {
        HuffmanCode hc = {0, 0};
        while (hc.numOfBits <= 8) {
            hc.code |= bs.getBit() << hc.numOfBits++;
            const Piece pc = HuffmanCodedPos::handCodeToPieceHash.value(hc.key);
            if (pc != PieceNone) {
                hand_[pieceToColor(pc)].plusOne(pieceTypeToHandPiece(pieceToPieceType(pc)));
                break;
            }
        }
        if (HuffmanCodedPos::handCodeToPieceHash.value(hc.key) == PieceNone)
            goto INCORRECT_HUFFMAN_CODE;
    }

    kingSquare_[Black] = bbOf(King, Black).constFirstOneFromSQ11();
    kingSquare_[White] = bbOf(King, White).constFirstOneFromSQ11();
    goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);

    gamePly_ = 1; // ply ã®æE ±ã¯æã£ã¦ãEªãEEã§ 1 ã«ãã¦ãããE
    st_->boardKey = computeBoardKey();
    st_->handKey = computeHandKey();
    st_->hand = hand(turn());

    setEvalList();
    findCheckers();
    st_->material = computeMaterial();
    thisThread_ = th;

    return true;
INCORRECT_HUFFMAN_CODE:
    std::cout << "incorrect Huffman code." << std::endl;
    return false;
}

bool Position::moveGivesCheck(const Move move) const {
    return moveGivesCheck(move, CheckInfo(*this));
}

// move ãçæãªãEtrue
bool Position::moveGivesCheck(const Move move, const CheckInfo& ci) const {
    assert(isOK());
    assert(ci.dcBB == discoveredCheckBB());

    const Square to = move.to();
    if (move.isDrop()) {
        const PieceType ptTo = move.pieceTypeDropped();
        // Direct Check ?
        if (ci.checkBB[ptTo].isSet(to))
            return true;
    }
    else {
        const Square from = move.from();
        const PieceType ptFrom = move.pieceTypeFrom();
        const PieceType ptTo = move.pieceTypeTo(ptFrom);
        assert(ptFrom == pieceToPieceType(piece(from)));
        // Direct Check ?
        if (ci.checkBB[ptTo].isSet(to))
            return true;

        // Discovery Check ?
        if (isDiscoveredCheck(from, to, kingSquare(oppositeColor(turn())), ci.dcBB))
            return true;
    }

    return false;
}

Piece Position::movedPiece(const Move m) const {
    return colorAndPieceTypeToPiece(turn(), m.pieceTypeFromOrDropped());
}

void Position::clear() {
    memset(this, 0, sizeof(Position));
    st_ = &startState_;
}

// åæãå¾æã«é¢ããããsq ã¸ç§»åå¯è½ãª Bitboard ãè¿ããEBitboard Position::attackersTo(const Square sq, const Bitboard& occupied) const {
    const Bitboard golds = goldsBB();
    return (((attacksFrom<Pawn  >(Black, sq          ) & bbOf(Pawn  ))
             | (attacksFrom<Lance >(Black, sq, occupied) & bbOf(Lance ))
             | (attacksFrom<Knight>(Black, sq          ) & bbOf(Knight))
             | (attacksFrom<Silver>(Black, sq          ) & bbOf(Silver))
             | (attacksFrom<Gold  >(Black, sq          ) & golds       ))
            & bbOf(White))
        | (((attacksFrom<Pawn  >(White, sq          ) & bbOf(Pawn  ))
            | (attacksFrom<Lance >(White, sq, occupied) & bbOf(Lance ))
            | (attacksFrom<Knight>(White, sq          ) & bbOf(Knight))
            | (attacksFrom<Silver>(White, sq          ) & bbOf(Silver))
            | (attacksFrom<Gold  >(White, sq          ) & golds))
           & bbOf(Black))
        | (attacksFrom<Bishop>(sq, occupied) & bbOf(Bishop, Horse        ))
        | (attacksFrom<Rook  >(sq, occupied) & bbOf(Rook  , Dragon       ))
        | (attacksFrom<King  >(sq          ) & bbOf(King  , Horse, Dragon));
}

// occupied ãEPosition::occupiedBB() ä»¥å¤ãEããEãä½¿ç¨ããå ´åã«ä½¿ç¨ãããEBitboard Position::attackersTo(const Color c, const Square sq, const Bitboard& occupied) const {
    const Color opposite = oppositeColor(c);
    return ((attacksFrom<Pawn  >(opposite, sq          ) & bbOf(Pawn  ))
            | (attacksFrom<Lance >(opposite, sq, occupied) & bbOf(Lance ))
            | (attacksFrom<Knight>(opposite, sq          ) & bbOf(Knight))
            | (attacksFrom<Silver>(opposite, sq          ) & bbOf(Silver, King, Dragon))
            | (attacksFrom<Gold  >(opposite, sq          ) & (bbOf(King  , Horse) | goldsBB()))
            | (attacksFrom<Bishop>(          sq, occupied) & bbOf(Bishop, Horse        ))
            | (attacksFrom<Rook  >(          sq, occupied) & bbOf(Rook  , Dragon       )))
        & bbOf(c);
}

// çä»¥å¤ã§ sq ã¸ç§»åå¯è½ãª c å´ã®é§ãE Bitboard ãè¿ããEBitboard Position::attackersToExceptKing(const Color c, const Square sq) const {
    const Color opposite = oppositeColor(c);
    return ((attacksFrom<Pawn  >(opposite, sq) & bbOf(Pawn  ))
            | (attacksFrom<Lance >(opposite, sq) & bbOf(Lance ))
            | (attacksFrom<Knight>(opposite, sq) & bbOf(Knight))
            | (attacksFrom<Silver>(opposite, sq) & bbOf(Silver, Dragon))
            | (attacksFrom<Gold  >(opposite, sq) & (goldsBB() | bbOf(Horse)))
            | (attacksFrom<Bishop>(          sq) & bbOf(Bishop, Horse ))
            | (attacksFrom<Rook  >(          sq) & bbOf(Rook  , Dragon)))
        & bbOf(c);
}

Score Position::computeMaterial() const {
    Score s = ScoreZero;
    for (PieceType pt = Pawn; pt < PieceTypeNum; ++pt) {
        const int num = bbOf(pt, Black).popCount() - bbOf(pt, White).popCount();
        s += num * pieceScore(pt);
    }
    for (PieceType pt = Pawn; pt < King; ++pt) {
        const int num = hand(Black).numOf(pieceTypeToHandPiece(pt)) - hand(White).numOf(pieceTypeToHandPiece(pt));
        s += num * pieceScore(pt);
    }
    return s;
}
