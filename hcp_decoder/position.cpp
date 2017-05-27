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
    {Binary<         0>::value, 0}, // BKing 玉�E位置は別途、位置を符号化する。使用しなぁE�Eで numOfBit めE0 にしておく、E    {Binary<      1001>::value, 4}, // BProPawn
    {Binary<    100011>::value, 6}, // BProLance
    {Binary<    100111>::value, 6}, // BProKnight
    {Binary<    101011>::value, 6}, // BProSilver
    {Binary<  10011111>::value, 8}, // BHorse
    {Binary<  10111111>::value, 8}, // BDragona
    {Binary<         0>::value, 0}, // 使用しなぁE�Eで numOfBit めE0 にしておく、E    {Binary<         0>::value, 0}, // 使用しなぁE�Eで numOfBit めE0 にしておく、E    {Binary<       101>::value, 4}, // WPawn
    {Binary<     10011>::value, 6}, // WLance
    {Binary<     10111>::value, 6}, // WKnight
    {Binary<     11011>::value, 6}, // WSilver
    {Binary<   1011111>::value, 8}, // WBishop
    {Binary<   1111111>::value, 8}, // WRook
    {Binary<    101111>::value, 6}, // WGold
    {Binary<         0>::value, 0}, // WKing 玉�E位置は別途、位置を符号化する、E    {Binary<      1101>::value, 4}, // WProPawn
    {Binary<    110011>::value, 6}, // WProLance
    {Binary<    110111>::value, 6}, // WProKnight
    {Binary<    111011>::value, 6}, // WProSilver
    {Binary<  11011111>::value, 8}, // WHorse
    {Binary<  11111111>::value, 8}, // WDragon
};

// 盤上�E bit 数 - 1 で表現出来るよぁE��する。持ち駒があると、盤上には Empty の 1 bit が増える�Eで、E// これで局面の bit 数が固定化される、Econst HuffmanCode HuffmanCodedPos::handCodeTable[HandPieceNum][ColorNum] = {
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
    // todo: ここで AVX2 使えそぁE��E    //       checkBB のreadアクセスは switch (pt) で場合�Eけして、余計なコピ�E減らした方が良ぁE��も、E    checkBB[ProPawn  ] = checkBB[Gold];
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

// 実際に持E��手が合法手かどぁE��判宁E// 連続王手�E十E��手�E排除しなぁE��E// 確実に駒打ちではなぁE��き�E、MUSTNOTDROP == true とする、E// 確実に玉�E移動で無ぁE��き�E、FROMMUSTNOTKING == true とする。英語として正しい�E�E// 遠隔駒で王手されてぁE��とき、その駒�E利きがある場所に送E��る手を検�E出来なぁE��合があるので、E// そ�Eような手を持E��手生成してはぁE��なぁE��Etemplate <bool MUSTNOTDROP, bool FROMMUSTNOTKING>
bool Position::pseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const {
    // 駒打ちは、打ち歩詰めや二歩は持E��手生成時めE��killerをMovePicker::nextMove() 冁E��排除してぁE��ので、常に合法手
    // (連続王手�E十E��手�E省いてぁE��ぁE��れど、E
    if (!MUSTNOTDROP && move.isDrop())
        return true;
    assert(!move.isDrop());

    const Color us = turn();
    const Square from = move.from();

    if (!FROMMUSTNOTKING && pieceToPieceType(piece(from)) == King) {
        const Color them = oppositeColor(us);
        // 玉�E移動�Eに相手�E駒�E利きがあれば、合法手でなぁE�Eで、false
        return !attackersToIsAny(them, move.to());
    }
    // 玉以外�E駒�E移勁E    return !isPinnedIllegal(from, move.to(), kingSquare(us), pinned);
}

template bool Position::pseudoLegalMoveIsLegal<false, false>(const Move move, const Bitboard& pinned) const;
template bool Position::pseudoLegalMoveIsLegal<false, true >(const Move move, const Bitboard& pinned) const;
template bool Position::pseudoLegalMoveIsLegal<true,  false>(const Move move, const Bitboard& pinned) const;

bool Position::pseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const {
    assert(isOK());

    // 玉�E移勁E    if (move.pieceTypeFrom() == King) {
        // 遠隔駒で王手されたとき、王手してぁE��遠隔駒�E利きには移動しなぁE��ぁE��持E��手を生�EしてぁE��、E        // そ�E為、移動�Eに他�E駒�E利きが無ぁE��調べるだけで良ぁE��E        const bool canMove = !attackersToIsAny(oppositeColor(turn()), move.to());
        assert(canMove == (pseudoLegalMoveIsLegal<false, false>(move, pinned)));
        return canMove;
    }

    // 玉�E移動以夁E    Bitboard target = checkersBB();
    const Square checkSq = target.firstOneFromSQ11();

    if (target)
        // 両王手のとき、玉の移動以外�E手�E持E��なぁE��E        return false;

    const Color us = turn();
    const Square to = move.to();
    // 移動、又は打った駒が、王手をさえぎるか、王手してぁE��駒を取る忁E��がある、E    target = betweenBB(checkSq, kingSquare(us)) | checkersBB();
    return target.isSet(to) && pseudoLegalMoveIsLegal<false, true>(move, pinned);
}

// Searching: true なら探索時に冁E��で生�Eした手�E合法手判定を行う、E//            ttMove で hash 値が衝突した時などで、大駒�E不�Eなど明らかに価値の低い手が生�Eされる事がある、E//            これは非合法手として省いて良ぁE��E//            false なら、外部入力�E合法手判定なので、ルールと同一の条件になる事が望ましい、Etemplate <bool Searching> bool Position::moveIsPseudoLegal(const Move move) const {
    const Color us = turn();
    const Color them = oppositeColor(us);
    const Square to = move.to();

    if (move.isDrop()) {
        const PieceType ptFrom = move.pieceTypeDropped();
        if (!hand(us).exists(pieceTypeToHandPiece(ptFrom)) || piece(to) != Empty)
            return false;

        if (inCheck()) {
            // 王手されてぁE��ので、合駒でなければならなぁE��E            Bitboard target = checkersBB();
            const Square checksq = target.firstOneFromSQ11();

            if (target)
                // 両王手は合駒�E来無ぁE��E                return false;

            target = betweenBB(checksq, kingSquare(us));
            if (!target.isSet(to))
                // 玉と、王手した駒との間に駒を打ってぁE��ぁE��E                return false;
        }

        if (ptFrom == Pawn) {
            if ((bbOf(Pawn, us) & fileMask(makeFile(to))))
                // 二歩
                return false;
            const SquareDelta TDeltaN = (us == Black ? DeltaN : DeltaS);
            if (to + TDeltaN == kingSquare(them) && isPawnDropCheckMate(us, to))
                // 王手かつ打ち歩詰めE                return false;
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
                    // 1段目の不�Eは非合法なので省く、E段目の不�Eと3段目の駒を取らなぁE���EもつぁE��に省く、E                    const Rank toRank = makeRank(to);
                    if (us == Black ? isInFrontOf<Black, Rank3, Rank7>(toRank) : isInFrontOf<White, Rank3, Rank7>(toRank))
                        return false;
                    if (canPromote(us, toRank) && !move.isCapture())
                        return false;
                }
                break;
            case Knight:
                // hash 値が衝突して別の局面の合法手の ttMove が�E力されても、桂馬である事�E確定、E桂馬は移動�E、移動�Eが特殊なので、E
                // よって、行きどころの無ぁE��になめEmove は生�EされなぁE��E                // 特にチェチE��すべき事�E無ぁE�Eで、break
                break;
            case Silver: case Bishop: case Rook  :
                if (move.isPromotion())
                    if (!canPromote(us, makeRank(to)) && !canPromote(us, makeRank(from)))
                        return false;
                break;
            default: // 成れなぁE��E                if (move.isPromotion())
                    return false;
            }
        }

        if (inCheck()) {
            if (ptFrom == King) {
                Bitboard occ = occupiedBB();
                occ.clearBit(from);
                if (attackersToIsAny(them, to, occ))
                    // 王手から送E��てぁE��ぁE��E                    return false;
            }
            else {
                // 玉以外�E駒を移動させたとき、E                Bitboard target = checkersBB();
                const Square checksq = target.firstOneFromSQ11();

                if (target)
                    // 両王手なので、玉が送E��なぁE��は駁E��
                    return false;

                target = betweenBB(checksq, kingSquare(us)) | checkersBB();
                if (!target.isSet(to))
                    // 玉と、王手した駒との間に移動するか、王手した駒を取る以外�E駁E��、E                    return false;
            }
        }
    }

    return true;
}

template bool Position::moveIsPseudoLegal<true >(const Move move) const;
template bool Position::moveIsPseudoLegal<false>(const Move move) const;

#if !defined NDEBUG
// 過去(又�E現在)に生�Eした持E��手が現在の局面でも有効か判定、E// あまり速度が要求される場面で使ってはぁE��なぁE��Ebool Position::moveIsLegal(const Move move) const {
    return MoveList<LegalAll>(*this).contains(move);
}
#endif

// 局面の更新
void Position::doMove(const Move move, StateInfo& newSt) {
    const CheckInfo ci(*this);
    doMove(move, newSt, ci, moveGivesCheck(move, ci));
}

// 局面の更新
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
            // 駒を取ったとぁE            const HandPiece hpCaptured = pieceTypeToHandPiece(ptCaptured);
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
        // Occupied は to, from の位置のビットを操作するよりも、E        // Black と White の or を取る方が速いはず、E        byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);

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
                case DirecMisc: assert(false); break; // 最適化�E為のダミ�E
                case DirecFile:
                    // from の位置から縦に利きを調べると相手玉と、空き王手してぁE��駒に当たってぁE��はず。味方の駒が空き王手してぁE��駒、E                    st_->checkersBB |= rookAttackFile(from, occupiedBB()) & bbOf(us);
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
    // ここで先に turn_ を戻したので、以下、move は us の持E��手とする、E    if (move.isDrop()) {
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
        const PieceType ptCaptured = move.cap(); // todo: st_->capturedType 使え�E良ぁE��E
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
            // 駒を取ったとぁE            byTypeBB_[ptCaptured].xorBit(to);
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
            // 駒を取らなぁE��き�E、colorAndPieceTypeToPiece(us, ptCaptured) は 0 また�E 16 になる、E            // 16 になると困る�Eで、E��を取らなぁE��き�E明示皁E�� Empty にする、E            piece_[to] = Empty;
        byTypeBB_[ptFrom].xorBit(from);
        byTypeBB_[ptTo].xorBit(to);
        byColorBB_[us].xorBit(from, to);
        piece_[from] = colorAndPieceTypeToPiece(us, ptFrom);
    }
    // Occupied は to, from の位置のビットを操作するよりも、E    // Black と White の or を取る方が速いはず、E    byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);
    goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);

    // key などは StateInfo にまとめられてぁE��ので、E    // previous のポインタめEst_ に代入するだけで良ぁE��E    st_ = st_->previous;

    assert(isOK());
}

namespace {
    // SEE の頁E��
    template <PieceType PT> struct SEENextPieceType {}; // これはインスタンス化しなぁE��E    template <> struct SEENextPieceType<Pawn     > { static const PieceType value = Lance;     };
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
            // todo: 実際に移動した方向を基にattackersを更新すれば、template, inline を使用しなくても良さそぁE��E            //       そ�E場合、キャチE��ュに乗りめE��くなる�Eで送E��速くなるかも、E            if (PT == Pawn || PT == Lance)
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

    // nega max 皁E��駒�E取り合いの点数を求める、E    while (--slIndex)
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
    // them(相扁E 側の玉が送E��られるか、E    // sq : 王手した相手�E駒�E位置。紐付きか、桂馬の位置とする。よって、玉は sq には行けなぁE��E    // bb : sq の利き�Eある場所のBitboard。よって、玉は bb のビットが立ってぁE��場所には行けなぁE��E    // sq と ksq の位置の Occupied Bitboard のみは、ここで更新して評価し、�Eに戻す、E    // (実際にはチE��ポラリのOccupied Bitboard を使ぁE�Eで、�Eには戻さなぁE��E
    bool canKingEscape(const Position& pos, const Color us, const Square sq, const Bitboard& bb) {
        const Color them = oppositeColor(us);
        const Square ksq = pos.kingSquare(them);
        Bitboard kingMoveBB = bb.notThisAnd(pos.bbOf(them).notThisAnd(kingAttack(ksq)));
        kingMoveBB.clearBit(sq); // sq には行けなぁE�Eで、クリアする。xorBit(sq)ではダメ、E
        if (kingMoveBB) {
            Bitboard tempOccupied = pos.occupiedBB();
            tempOccupied.setBit(sq);
            tempOccupied.clearBit(ksq);
            do {
                const Square to = kingMoveBB.firstOneFromSQ11();
                // 玉�E移動�Eに、us 側の利きが無ければ、true
                if (!pos.attackersToIsAny(us, to, tempOccupied))
                    return true;
            } while (kingMoveBB);
        }
        // 玉�E移動�Eが無ぁE��E        return false;
    }
    // them(相扁E 側の玉以外�E駒が sq にある us 側の駒を取れるか、E    bool canPieceCapture(const Position& pos, const Color them, const Square sq, const Bitboard& dcBB) {
        // 玉以外で打った駒を取れる相手�Eの駒�E Bitboard
        Bitboard fromBB = pos.attackersToExceptKing(them, sq);

        if (fromBB) {
            const Square ksq = pos.kingSquare(them);
            do {
                const Square from = fromBB.firstOneFromSQ11();
                if (!pos.isDiscoveredCheck(from, sq, ksq, dcBB))
                    // them 側から見て、pin されてぁE��ぁE��で、打たれた駒を取れる�Eで、true
                    return true;
            } while (fromBB);
        }
        // 玉以外�E駒で、打った駒を取れなぁE��E        return false;
    }

    // pos.discoveredCheckBB<false>() を遅延評価するバ�Eジョン、E    bool canPieceCapture(const Position& pos, const Color them, const Square sq) {
        Bitboard fromBB = pos.attackersToExceptKing(them, sq);

        if (fromBB) {
            const Square ksq = pos.kingSquare(them);
            const Bitboard dcBB = pos.discoveredCheckBB<false>();
            do {
                const Square from = fromBB.firstOneFromSQ11();
                if (!pos.isDiscoveredCheck(from, sq, ksq, dcBB))
                    // them 側から見て、pin されてぁE��ぁE��で、打たれた駒を取れる�Eで、true
                    return true;
            } while (fromBB);
        }
        // 玉以外�E駒で、打った駒を取れなぁE��E        return false;
    }
}

// us ぁEsq へ歩を打ったとき、them の玉が詰むか、E// us ぁEsq へ歩を打つのは王手であると仮定する、E// 打ち歩詰め�Eとき、true を返す、Ebool Position::isPawnDropCheckMate(const Color us, const Square sq) const {
    const Color them = oppositeColor(us);
    // 玉以外�E駒で、打たれた歩が取れるなら、打ち歩詰めではなぁE��E    if (canPieceCapture(*this, them, sq))
        return false;
    // todo: ここで玉�E位置を求める�Eは、上位で求めたものと2重になる�Eで無駁E��後で整琁E��ること、E    const Square ksq = kingSquare(them);

    // 玉以外で打った歩を取れなぁE��き、玉が歩を取るか、玉が送E��るか、E
    // 利きを求める際に、occupied の歩を打った位置の bit を立てぁEBitboard を使用する、E    // ここでは歩の Bitboard は更新する忁E��がなぁE��E    // color の Bitboard も更新する忁E��がなぁE��E相手玉が動くとき、こちら�E打った歩で玉を取ることは無ぁE��、E
    const Bitboard tempOccupied = occupiedBB() | setMaskBB(sq);
    Bitboard kingMoveBB = bbOf(them).notThisAnd(kingAttack(ksq));

    // 少なくとも歩を取る方向には玉が動ける�Eずなので、do while を使用、E    assert(kingMoveBB);
    do {
        const Square to = kingMoveBB.firstOneFromSQ11();
        if (!attackersToIsAny(us, to, tempOccupied))
            // 相手玉の移動�Eに自駒�E利きがなぁE��ら、打ち歩詰めではなぁE��E            return false;
    } while (kingMoveBB);

    return true;
}

inline void Position::xorBBs(const PieceType pt, const Square sq, const Color c) {
    byTypeBB_[Occupied].xorBit(sq);
    byTypeBB_[pt].xorBit(sq);
    byColorBB_[c].xorBit(sq);
}

// 相手玉ぁE手詰みかどぁE��を判定、E// 1手詰みなら、詰みに至る指し手の一部の惁E��(from, to のみとぁEを返す、E// 1手詰みでなぁE��ら、Move::moveNone() を返す、E// Bitboard の状態を途中で更新する為、const 関数ではなぁE��E更新後、�Eに戻すが、E
template <Color US> Move Position::mateMoveIn1Ply() {
    const Color Them = oppositeColor(US);
    const Square ksq = kingSquare(Them);
    const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);

    assert(!attackersToIsAny(Them, kingSquare(US)));

    // 駒打ちを調べる、E    const Bitboard dropTarget = nOccupiedBB(); // emptyBB() ではなぁE�Eで注意して使ぁE��と、E    const Hand ourHand = hand(US);
    // 王手する前�E状態�E dcBB、E    // 間にある駒�E相手�Eの駒、E    // 駒打ちのとき�E、打った後も、打たれる前の状態�E dcBB を使用する、E    const Bitboard dcBB_betweenIsThem = discoveredCheckBB<false>();

    // 飛車打ち
    if (ourHand.exists<HRook>()) {
        // 合駒されるとめE��こしぁE�Eで、E手詰み関数の中で調べる、E        // ここでは離れた位置から王手するのは老E��なぁE��E        Bitboard toBB = dropTarget & rookStepAttacks(ksq);
        while (toBB) {
            const Square to = toBB.firstOneFromSQ11();
            // 駒を打った場所に自駒�E利きがあるか、E無ければ玉で取られて詰まなぁE
            if (attackersToIsAny(US, to)) {
                // 玉が送E��られず、他�E駒で取ることも�E来なぁE��
                if (!canKingEscape(*this, US, to, rookAttackToEdge(to))
                    && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                {
                    return makeDropMove(Rook, to);
                }
            }
        }
    }
    // 香車打ち
    // 飛車で詰まなければ香車でも詰まなぁE�Eで、else if を使用、E    // 玉が 9(1) 段目にぁE��ば香車で王手出来無ぁE�Eで、それも省く、E    else if (ourHand.exists<HLance>() && isInFrontOf<US, Rank9, Rank1>(makeRank(ksq))) {
        const Square to = ksq + TDeltaS;
        if (piece(to) == Empty && attackersToIsAny(US, to)) {
            if (!canKingEscape(*this, US, to, lanceAttackToEdge(US, to))
                && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
            {
                return makeDropMove(Lance, to);
            }
        }
    }

    // 角打ち
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

    // 金打ち
    if (ourHand.exists<HGold>()) {
        Bitboard toBB;
        if (ourHand.exists<HRook>())
            // 飛車打ちを�Eに調べた�Eで、尻金だけ�E省く、E            toBB = dropTarget & (goldAttack(Them, ksq) ^ pawnAttack(US, ksq));
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
            // 金打ちを�Eに調べた�Eで、斜め後ろから打つ場合だけを調べる、E
            if (ourHand.exists<HBishop>())
                // 角打ちを�Eに調べた�Eで、斜めから�E王手も除外できる。銀打ちを調べる忁E��がなぁE��E                goto silver_drop_end;
            // 斜め後ろから打つ場合を調べる忁E��がある、E            toBB = dropTarget & (silverAttack(Them, ksq) & inFrontMask(US, makeRank(ksq)));
        }
        else {
            if (ourHand.exists<HBishop>())
                // 斜め後ろを除外。前方から打つ場合を調べる忁E��がある、E                toBB = dropTarget & goldAndSilverAttacks(Them, ksq);
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
            // 桂馬は紐が付いてぁE��忁E���EなぁE��E            // よって、このcanKingEscape() 冁E��の to の位置に送E��られなぁE��ぁE��する処琁E�E無駁E��E            if (!canKingEscape(*this, US, to, allZeroBB())
                && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
            {
                return makeDropMove(Knight, to);
            }
        }
    }

    // 歩打ちで詰ますと反則なので、調べなぁE��E
    // 駒を移動する場吁E    // moveTarget は桂馬以外�E移動�Eの大まかな位置。飛角香�E遠隔王手�E含まなぁE��E    const Bitboard moveTarget = bbOf(US).notThisAnd(kingAttack(ksq));
    const Bitboard pinned = pinnedBB();
    const Bitboard dcBB_betweenIsUs = discoveredCheckBB<true>();

    {
        // 竜による移勁E        Bitboard fromBB = bbOf(Dragon, US);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            // 遠隔王手�E老E��なぁE��E            Bitboard toBB = moveTarget & attacksFrom<Dragon>(from);
            if (toBB) {
                xorBBs(Dragon, from, US);
                // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to の位置の Bitboard は canKingEscape の中で更新する、E                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // 王手した駒�E場所に自駒�E利きがあるか、E無ければ玉で取られて詰まなぁE
                    if (unDropCheckIsSupported(US, to)) {
                        // 玉が送E��られなぁE                        // かつ、E空き王扁Eまた�E 他�E駒で取れなぁE
                        // かつ、王手した駒が pin されてぁE��ぁE                        if (!canKingEscape(*this, US, to, attacksFrom<Dragon>(to, occupiedBB() ^ setMaskBB(ksq)))
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

    // Txxx は先手、後手の惁E��を吸収した変数。数字�E先手に合わせてぁE��、E    const Rank TRank4 = (US == Black ? Rank4 : Rank6);
    const Bitboard TRank123BB = inFrontMask<US, TRank4>();
    {
        // 飛車による移勁E        Bitboard fromBB = bbOf(Rook, US);
        Bitboard fromOn123BB = fromBB & TRank123BB;
        // from ぁE123 段目
        if (fromOn123BB) {
            fromBB.andEqualNot(TRank123BB);
            do {
                const Square from = fromOn123BB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Rook>(from);
                if (toBB) {
                    xorBBs(Rook, from, US);
                    // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する、E                    do {
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

        // from ぁE4~9 段目
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Rook>(from) & (rookStepAttacks(ksq) | TRank123BB);
            if (toBB) {
                xorBBs(Rook, from, US);
                // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();

                Bitboard toOn123BB = toBB & TRank123BB;
                // 成り
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
                // 不�E
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
        // 馬による移勁E        Bitboard fromBB = bbOf(Horse, US);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            // 遠隔王手�E老E��なぁE��E            Bitboard toBB = moveTarget & attacksFrom<Horse>(from);
            if (toBB) {
                xorBBs(Horse, from, US);
                // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to の位置の Bitboard は canKingEscape の中で更新する、E                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // 王手した駒�E場所に自駒�E利きがあるか、E無ければ玉で取られて詰まなぁE
                    if (unDropCheckIsSupported(US, to)) {
                        // 玉が送E��られなぁE                        // かつ、E空き王扁Eまた�E 他�E駒で取れなぁE
                        // かつ、動かした駒が pin されてぁE��ぁE
                        if (!canKingEscape(*this, US, to, horseAttackToEdge(to)) // 竜�E場合と違って、常に最大の利きを使用して良ぁE��E                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
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
        // 角による移勁E        Bitboard fromBB = bbOf(Bishop, US);
        Bitboard fromOn123BB = fromBB & TRank123BB;
        // from ぁE123 段目
        if (fromOn123BB) {
            fromBB.andEqualNot(TRank123BB);
            do {
                const Square from = fromOn123BB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Bishop>(from);
                if (toBB) {
                    xorBBs(Bishop, from, US);
                    // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する、E                    do {
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

        // from ぁE4~9 段目
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Bishop>(from) & (bishopStepAttacks(ksq) | TRank123BB);
            if (toBB) {
                xorBBs(Bishop, from, US);
                // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();

                Bitboard toOn123BB = toBB & TRank123BB;
                // 成り
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
                // 不�E
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
        // 金、�Eり��による移勁E        Bitboard fromBB = goldsBB(US) & goldCheckTable(US, ksq);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Gold>(US, from) & attacksFrom<Gold>(Them, ksq);
            if (toBB) {
                const PieceType pt = pieceToPieceType(piece(from));
                xorBBs(pt, from, US);
                goldsBB_.xorBit(from);
                // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to の位置の Bitboard は canKingEscape の中で更新する、E                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // 王手した駒�E場所に自駒�E利きがあるか、E無ければ玉で取られて詰まなぁE
                    if (unDropCheckIsSupported(US, to)) {
                        // 玉が送E��られなぁE                        // かつ、E空き王扁Eまた�E 他�E駒で取れなぁE
                        // かつ、動かした駒が pin されてぁE��ぁE
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
        // 銀による移勁E        Bitboard fromBB = bbOf(Silver, US) & silverCheckTable(US, ksq);
        if (fromBB) {
            // Txxx は先手、後手の惁E��を吸収した変数。数字�E先手に合わせてぁE��、E            const Bitboard TRank5_9BB = inFrontMask<Them, TRank4>();
            const Bitboard chkBB = attacksFrom<Silver>(Them, ksq);
            const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq);

            Bitboard fromOn123BB = fromBB & TRank123BB;
            // from が敵陣
            if (fromOn123BB) {
                fromBB.andEqualNot(TRank123BB);
                do {
                    const Square from = fromOn123BB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
                    Bitboard toBB_promo = toBB & chkBB_promo;

                    toBB &= chkBB;
                    if ((toBB_promo | toBB)) {
                        xorBBs(Silver, from, US);
                        // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する、E                        while (toBB_promo) {
                            const Square to = toBB_promo.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 成り
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

                        // 玉�E前方に移動する場合、�Eで詰まなかったら不�Eでも詰まなぁE�Eで、ここで省く、E                        // sakurapyon の作老E��言ってた�Eで実裁E��E                        toBB.andEqualNot(inFrontMask(Them, makeRank(ksq)));
                        while (toBB) {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 不�E
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

            // from ぁE5~9段目 (忁E��不�E)
            Bitboard fromOn5_9BB = fromBB & TRank5_9BB;
            if (fromOn5_9BB) {
                fromBB.andEqualNot(TRank5_9BB);
                do {
                    const Square from = fromOn5_9BB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from) & chkBB;

                    if (toBB) {
                        xorBBs(Silver, from, US);
                        // 動いた後�E dcBB, pinned: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する、E                        while (toBB) {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 不�E
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

            // 残り 4 段目のみ
            // 前進するとき�E成れるが、後退するとき�E成れなぁE��E            while (fromBB) {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
                Bitboard toBB_promo = toBB & TRank123BB & chkBB_promo; // 3 段目にしか成れなぁE��E
                toBB &= chkBB;
                if ((toBB_promo | toBB)) {
                    xorBBs(Silver, from, US);
                    // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する、E                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // 成り
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
                            // 不�E
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
        // 桂による移勁E        Bitboard fromBB = bbOf(Knight, US) & knightCheckTable(US, ksq);
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
                    // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する、E                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // 成り
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
                        // 桂馬は紐が付いてなくて良ぁE�Eで、紐が付いてぁE��か�E調べなぁE��E                        // 不�E
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
        // 香車による移勁E        Bitboard fromBB = bbOf(Lance, US) & lanceCheckTable(US, ksq);
        if (fromBB) {
            // Txxx は先手、後手の惁E��を吸収した変数。数字�E先手に合わせてぁE��、E            const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
            const Rank TRank2 = (US == Black ? Rank2 : Rank8);
            const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq) & TRank123BB;
            // 玉�E前方1マスのみ、E            // 玉が 1 段目にぁE��とき�E、�Eのみで良ぁE�Eで省く、E            const Bitboard chkBB = attacksFrom<Pawn>(Them, ksq) & inFrontMask<Them, TRank2>();

            do {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Lance>(US, from);
                Bitboard toBB_promo = toBB & chkBB_promo;

                toBB &= chkBB;

                if ((toBB_promo | toBB)) {
                    xorBBs(Lance, from, US);
                    // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する、E
                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // 成り
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
                        // 不�Eで王手出来る�Eは、一つの場所だけなので、ループにする忁E��が無ぁE��E                        const Square to = ksq + TDeltaS;
                        if (unDropCheckIsSupported(US, to)) {
                            // 不�E
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
        // 歩による移勁E        // 成れる場合�E忁E��なる、E        // todo: PawnCheckBB 作って簡略化する、E        const Rank krank = makeRank(ksq);
        // 歩が移動して王手になる�Eは、相手玉ぁE~7段目の時�Eみ、E        if (isInFrontOf<US, Rank8, Rank2>(krank)) {
            // Txxx は先手、後手の惁E��を吸収した変数。数字�E先手に合わせてぁE��、E            const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
            const SquareDelta TDeltaN = (US == Black ? DeltaN : DeltaS);

            Bitboard fromBB = bbOf(Pawn, US);
            // 玉が敵陣にぁE��ぁE��成で王手になることはなぁE��E            if (isInFrontOf<US, Rank4, Rank6>(krank)) {
                // 成った時に王手になる位置
                const Bitboard toBB_promo = moveTarget & attacksFrom<Gold>(Them, ksq) & TRank123BB;
                Bitboard fromBB_promo = fromBB & pawnAttack<Them>(toBB_promo);
                while (fromBB_promo) {
                    const Square from = fromBB_promo.firstOneFromSQ11();
                    const Square to = from + TDeltaN;

                    xorBBs(Pawn, from, US);
                    // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する、E                    if (unDropCheckIsSupported(US, to)) {
                        // 成り
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

            // 不�E
            // 玉が 8,9 段目にぁE��ことは無ぁE�Eで、from,to が隣の筋を持E��ことは無ぁE��E            const Square to = ksq + TDeltaS;
            const Square from = to + TDeltaS;
            if (fromBB.isSet(from) && !bbOf(US).isSet(to)) {
                // 玉が 1, 2 段目にぁE��なら、�Eりで王手出来る�Eで不�Eは調べなぁE��E                if (isBehind<US, Rank2, Rank8>(krank)) {
                    xorBBs(Pawn, from, US);
                    // 動いた後�E dcBB: to の位置の occupied めEcheckers は関係なぁE�Eで、ここで生�Eできる、E                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する、E                    if (unDropCheckIsSupported(US, to)) {
                        // 不�E
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
    // zobTurn_ は 1 であり、その他�E 1桁目を使わなぁE��E    // zobTurn のみ xor で更新する為、他�E桁に影響しなぁE��ぁE��する為、E    // hash値の更新は普通�E全て xor を使ぁE��、持ち駒�E更新の為に +, - を使用した方が�E合が良ぁE��E    for (PieceType pt = Occupied; pt < PieceTypeNum; ++pt) {
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
        // USI の規格として、持ち駒�E表記頁E�E決まっており、�E手、後手の頁E��、それぞめE飛、角、E��、E��、桂、E��、歩 の頁E��E        for (Color color = Black; color < ColorNum; ++color) {
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
    // 手番 (1bit)
    bs.putBit(turn());

    // 玉�E位置 (7bit * 2)
    bs.putBits(kingSquare(Black), 7);
    bs.putBits(kingSquare(White), 7);

    // 盤上�E駁E    for (Square sq = SQ11; sq < SquareNum; ++sq) {
        Piece pc = piece(sq);
        if (pieceToPieceType(pc) == King)
            continue;
        const auto hc = HuffmanCodedPos::boardCodeTable[pc];
        bs.putBits(hc.code, hc.numOfBits);
    }

    // 持ち駁E    for (Color c = Black; c < ColorNum; ++c) {
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
        // 相手玉を取れなぁE��とを確誁E        const Color us = turn();
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

// todo: isRepetition() に名前変えた方が良さそぁE��E//       同一局面4回をきちんと数えてぁE��ぁE��ど問題なぁE��、ERepetitionType Position::isDraw(const int checkMaxPly) const {
    const int Start = 4;
    int i = Start;
    const int e = std::min(st_->pliesFromNull, checkMaxPly);

    // 4手掛けなぁE��十E��手には絶対にならなぁE��E    if (i <= e) {
        // 現在の局面と、少なくとめE4 手戻らなぁE��同じ局面にならなぁE��E        // ここでまぁE2 手戻る、E        StateInfo* stp = st_->previous->previous;

        do {
            // 更に 2 手戻る、E            stp = stp->previous->previous;
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

    // 盤上�E駁E    while (ss.get(token) && token != ' ') {
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

    // 手番
    while (ss.get(token) && token != ' ') {
        if (token == 'b')
            turn_ = Black;
        else if (token == 'w')
            turn_ = White;
        else
            goto INCORRECT;
    }

    // 持ち駁E    for (int digits = 0; ss.get(token) && token != ' '; ) {
        if (token == '-')
            memset(hand_, 0, sizeof(hand_));
        else if (isdigit(token))
            digits = digits * 10 + token - '0';
        else if (g_charToPieceUSI.isLegalChar(token)) {
            // 持ち駒を32bit に pack する
            const Piece piece = g_charToPieceUSI.value(token);
            setHand(piece, (digits == 0 ? 1 : digits));

            digits = 0;
        }
        else
            goto INCORRECT;
    }

    // 次の手が何手目ぁE    ss >> gamePly_;

    // 残り時間, hash key, (もし実裁E��るなめE駒番号などをここで設宁E    st_->boardKey = computeBoardKey();
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

    HuffmanCodedPos tmp = hcp; // ローカルにコピ�E
    BitStream bs(tmp.data);

    // 手番
    turn_ = static_cast<Color>(bs.getBit());

    // 玉�E位置
    Square sq0 = (Square)bs.getBits(7);
    Square sq1 = (Square)bs.getBits(7);
    setPiece(BKing, static_cast<Square>(sq0));
    setPiece(WKing, static_cast<Square>(sq1));

    // 盤上�E駁E    for (Square sq = SQ11; sq < SquareNum; ++sq) {
        if (pieceToPieceType(piece(sq)) == King) // piece(sq) は BKing, WKing, Empty のどれか、E            continue;
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

    gamePly_ = 1; // ply の惁E��は持ってぁE��ぁE�Eで 1 にしておく、E
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

// move が王手なめEtrue
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

// 先手、後手に関わらず、sq へ移動可能な Bitboard を返す、EBitboard Position::attackersTo(const Square sq, const Bitboard& occupied) const {
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

// occupied めEPosition::occupiedBB() 以外�Eも�Eを使用する場合に使用する、EBitboard Position::attackersTo(const Color c, const Square sq, const Bitboard& occupied) const {
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

// 玉以外で sq へ移動可能な c 側の駒�E Bitboard を返す、EBitboard Position::attackersToExceptKing(const Color c, const Square sq) const {
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
