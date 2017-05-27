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
    {Binary<         0>::value, 0}, // BKing 邇峨・菴咲ｽｮ縺ｯ蛻･騾斐∽ｽ咲ｽｮ繧堤ｬｦ蜿ｷ蛹悶☆繧九ゆｽｿ逕ｨ縺励↑縺・・縺ｧ numOfBit 繧・0 縺ｫ縺励※縺翫￥縲・    {Binary<      1001>::value, 4}, // BProPawn
    {Binary<    100011>::value, 6}, // BProLance
    {Binary<    100111>::value, 6}, // BProKnight
    {Binary<    101011>::value, 6}, // BProSilver
    {Binary<  10011111>::value, 8}, // BHorse
    {Binary<  10111111>::value, 8}, // BDragona
    {Binary<         0>::value, 0}, // 菴ｿ逕ｨ縺励↑縺・・縺ｧ numOfBit 繧・0 縺ｫ縺励※縺翫￥縲・    {Binary<         0>::value, 0}, // 菴ｿ逕ｨ縺励↑縺・・縺ｧ numOfBit 繧・0 縺ｫ縺励※縺翫￥縲・    {Binary<       101>::value, 4}, // WPawn
    {Binary<     10011>::value, 6}, // WLance
    {Binary<     10111>::value, 6}, // WKnight
    {Binary<     11011>::value, 6}, // WSilver
    {Binary<   1011111>::value, 8}, // WBishop
    {Binary<   1111111>::value, 8}, // WRook
    {Binary<    101111>::value, 6}, // WGold
    {Binary<         0>::value, 0}, // WKing 邇峨・菴咲ｽｮ縺ｯ蛻･騾斐∽ｽ咲ｽｮ繧堤ｬｦ蜿ｷ蛹悶☆繧九・    {Binary<      1101>::value, 4}, // WProPawn
    {Binary<    110011>::value, 6}, // WProLance
    {Binary<    110111>::value, 6}, // WProKnight
    {Binary<    111011>::value, 6}, // WProSilver
    {Binary<  11011111>::value, 8}, // WHorse
    {Binary<  11111111>::value, 8}, // WDragon
};

// 逶､荳翫・ bit 謨ｰ - 1 縺ｧ陦ｨ迴ｾ蜃ｺ譚･繧九ｈ縺・↓縺吶ｋ縲よ戟縺｡鬧偵′縺ゅｋ縺ｨ縲∫乢荳翫↓縺ｯ Empty 縺ｮ 1 bit 縺悟｢励∴繧九・縺ｧ縲・// 縺薙ｌ縺ｧ螻髱｢縺ｮ bit 謨ｰ縺悟崋螳壼喧縺輔ｌ繧九・const HuffmanCode HuffmanCodedPos::handCodeTable[HandPieceNum][ColorNum] = {
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
    // todo: 縺薙％縺ｧ AVX2 菴ｿ縺医◎縺・・    //       checkBB 縺ｮread繧｢繧ｯ繧ｻ繧ｹ縺ｯ switch (pt) 縺ｧ蝣ｴ蜷亥・縺代＠縺ｦ縲∽ｽ呵ｨ医↑繧ｳ繝斐・貂帙ｉ縺励◆譁ｹ縺瑚憶縺・°繧ゅ・    checkBB[ProPawn  ] = checkBB[Gold];
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

// 螳滄圀縺ｫ謖・＠謇九′蜷域ｳ墓焔縺九←縺・°蛻､螳・// 騾｣邯夂視謇九・蜊・律謇九・謗帝勁縺励↑縺・・// 遒ｺ螳溘↓鬧呈遠縺｡縺ｧ縺ｯ縺ｪ縺・→縺阪・縲｀USTNOTDROP == true 縺ｨ縺吶ｋ縲・// 遒ｺ螳溘↓邇峨・遘ｻ蜍輔〒辟｡縺・→縺阪・縲：ROMMUSTNOTKING == true 縺ｨ縺吶ｋ縲り恭隱槭→縺励※豁｣縺励＞・・// 驕髫秘ｧ偵〒邇区焔縺輔ｌ縺ｦ縺・ｋ縺ｨ縺阪√◎縺ｮ鬧偵・蛻ｩ縺阪′縺ゅｋ蝣ｴ謇縺ｫ騾・￡繧区焔繧呈､懷・蜃ｺ譚･縺ｪ縺・ｴ蜷医′縺ゅｋ縺ｮ縺ｧ縲・// 縺昴・繧医≧縺ｪ謇九ｒ謖・＠謇狗函謌舌＠縺ｦ縺ｯ縺・￠縺ｪ縺・・template <bool MUSTNOTDROP, bool FROMMUSTNOTKING>
bool Position::pseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const {
    // 鬧呈遠縺｡縺ｯ縲∵遠縺｡豁ｩ隧ｰ繧√ｄ莠梧ｭｩ縺ｯ謖・＠謇狗函謌先凾繧・〔iller繧樽ovePicker::nextMove() 蜀・〒謗帝勁縺励※縺・ｋ縺ｮ縺ｧ縲∝ｸｸ縺ｫ蜷域ｳ墓焔
    // (騾｣邯夂視謇九・蜊・律謇九・逵√＞縺ｦ縺・↑縺・￠繧後←縲・
    if (!MUSTNOTDROP && move.isDrop())
        return true;
    assert(!move.isDrop());

    const Color us = turn();
    const Square from = move.from();

    if (!FROMMUSTNOTKING && pieceToPieceType(piece(from)) == King) {
        const Color them = oppositeColor(us);
        // 邇峨・遘ｻ蜍募・縺ｫ逶ｸ謇九・鬧偵・蛻ｩ縺阪′縺ゅｌ縺ｰ縲∝粋豕墓焔縺ｧ縺ｪ縺・・縺ｧ縲’alse
        return !attackersToIsAny(them, move.to());
    }
    // 邇我ｻ･螟悶・鬧偵・遘ｻ蜍・    return !isPinnedIllegal(from, move.to(), kingSquare(us), pinned);
}

template bool Position::pseudoLegalMoveIsLegal<false, false>(const Move move, const Bitboard& pinned) const;
template bool Position::pseudoLegalMoveIsLegal<false, true >(const Move move, const Bitboard& pinned) const;
template bool Position::pseudoLegalMoveIsLegal<true,  false>(const Move move, const Bitboard& pinned) const;

bool Position::pseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const {
    assert(isOK());

    // 邇峨・遘ｻ蜍・    if (move.pieceTypeFrom() == King) {
        // 驕髫秘ｧ偵〒邇区焔縺輔ｌ縺溘→縺阪∫視謇九＠縺ｦ縺・ｋ驕髫秘ｧ偵・蛻ｩ縺阪↓縺ｯ遘ｻ蜍輔＠縺ｪ縺・ｈ縺・↓謖・＠謇九ｒ逕滓・縺励※縺・ｋ縲・        // 縺昴・轤ｺ縲∫ｧｻ蜍募・縺ｫ莉悶・鬧偵・蛻ｩ縺阪′辟｡縺・°隱ｿ縺ｹ繧九□縺代〒濶ｯ縺・・        const bool canMove = !attackersToIsAny(oppositeColor(turn()), move.to());
        assert(canMove == (pseudoLegalMoveIsLegal<false, false>(move, pinned)));
        return canMove;
    }

    // 邇峨・遘ｻ蜍穂ｻ･螟・    Bitboard target = checkersBB();
    const Square checkSq = target.firstOneFromSQ11();

    if (target)
        // 荳｡邇区焔縺ｮ縺ｨ縺阪∫脂縺ｮ遘ｻ蜍穂ｻ･螟悶・謇九・謖・○縺ｪ縺・・        return false;

    const Color us = turn();
    const Square to = move.to();
    // 遘ｻ蜍輔∝処縺ｯ謇薙▲縺滄ｧ偵′縲∫視謇九ｒ縺輔∴縺弱ｋ縺九∫視謇九＠縺ｦ縺・ｋ鬧偵ｒ蜿悶ｋ蠢・ｦ√′縺ゅｋ縲・    target = betweenBB(checkSq, kingSquare(us)) | checkersBB();
    return target.isSet(to) && pseudoLegalMoveIsLegal<false, true>(move, pinned);
}

// Searching: true 縺ｪ繧画爾邏｢譎ゅ↓蜀・Κ縺ｧ逕滓・縺励◆謇九・蜷域ｳ墓焔蛻､螳壹ｒ陦後≧縲・//            ttMove 縺ｧ hash 蛟､縺瑚｡晉ｪ√＠縺滓凾縺ｪ縺ｩ縺ｧ縲∝､ｧ鬧偵・荳肴・縺ｪ縺ｩ譏弱ｉ縺九↓萓｡蛟､縺ｮ菴弱＞謇九′逕滓・縺輔ｌ繧倶ｺ九′縺ゅｋ縲・//            縺薙ｌ縺ｯ髱槫粋豕墓焔縺ｨ縺励※逵√＞縺ｦ濶ｯ縺・・//            false 縺ｪ繧峨∝､夜Κ蜈･蜉帙・蜷域ｳ墓焔蛻､螳壹↑縺ｮ縺ｧ縲√Ν繝ｼ繝ｫ縺ｨ蜷御ｸ縺ｮ譚｡莉ｶ縺ｫ縺ｪ繧倶ｺ九′譛帙∪縺励＞縲・template <bool Searching> bool Position::moveIsPseudoLegal(const Move move) const {
    const Color us = turn();
    const Color them = oppositeColor(us);
    const Square to = move.to();

    if (move.isDrop()) {
        const PieceType ptFrom = move.pieceTypeDropped();
        if (!hand(us).exists(pieceTypeToHandPiece(ptFrom)) || piece(to) != Empty)
            return false;

        if (inCheck()) {
            // 邇区焔縺輔ｌ縺ｦ縺・ｋ縺ｮ縺ｧ縲∝粋鬧偵〒縺ｪ縺代ｌ縺ｰ縺ｪ繧峨↑縺・・            Bitboard target = checkersBB();
            const Square checksq = target.firstOneFromSQ11();

            if (target)
                // 荳｡邇区焔縺ｯ蜷磯ｧ貞・譚･辟｡縺・・                return false;

            target = betweenBB(checksq, kingSquare(us));
            if (!target.isSet(to))
                // 邇峨→縲∫視謇九＠縺滄ｧ偵→縺ｮ髢薙↓鬧偵ｒ謇薙▲縺ｦ縺・↑縺・・                return false;
        }

        if (ptFrom == Pawn) {
            if ((bbOf(Pawn, us) & fileMask(makeFile(to))))
                // 莠梧ｭｩ
                return false;
            const SquareDelta TDeltaN = (us == Black ? DeltaN : DeltaS);
            if (to + TDeltaN == kingSquare(them) && isPawnDropCheckMate(us, to))
                // 邇区焔縺九▽謇薙■豁ｩ隧ｰ繧・                return false;
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
                    // 1谿ｵ逶ｮ縺ｮ荳肴・縺ｯ髱槫粋豕輔↑縺ｮ縺ｧ逵√￥縲・谿ｵ逶ｮ縺ｮ荳肴・縺ｨ3谿ｵ逶ｮ縺ｮ鬧偵ｒ蜿悶ｉ縺ｪ縺・ｸ肴・繧ゅ▽縺・〒縺ｫ逵√￥縲・                    const Rank toRank = makeRank(to);
                    if (us == Black ? isInFrontOf<Black, Rank3, Rank7>(toRank) : isInFrontOf<White, Rank3, Rank7>(toRank))
                        return false;
                    if (canPromote(us, toRank) && !move.isCapture())
                        return false;
                }
                break;
            case Knight:
                // hash 蛟､縺瑚｡晉ｪ√＠縺ｦ蛻･縺ｮ螻髱｢縺ｮ蜷域ｳ墓焔縺ｮ ttMove 縺悟・蜉帙＆繧後※繧ゅ∵｡るｦｬ縺ｧ縺ゅｋ莠九・遒ｺ螳壹・譯るｦｬ縺ｯ遘ｻ蜍募・縲∫ｧｻ蜍募・縺檎音谿翫↑縺ｮ縺ｧ縲・
                // 繧医▲縺ｦ縲∬｡後″縺ｩ縺薙ｍ縺ｮ辟｡縺・ｧ偵↓縺ｪ繧・move 縺ｯ逕滓・縺輔ｌ縺ｪ縺・・                // 迚ｹ縺ｫ繝√ぉ繝・け縺吶∋縺堺ｺ九・辟｡縺・・縺ｧ縲｜reak
                break;
            case Silver: case Bishop: case Rook  :
                if (move.isPromotion())
                    if (!canPromote(us, makeRank(to)) && !canPromote(us, makeRank(from)))
                        return false;
                break;
            default: // 謌舌ｌ縺ｪ縺・ｧ・                if (move.isPromotion())
                    return false;
            }
        }

        if (inCheck()) {
            if (ptFrom == King) {
                Bitboard occ = occupiedBB();
                occ.clearBit(from);
                if (attackersToIsAny(them, to, occ))
                    // 邇区焔縺九ｉ騾・￡縺ｦ縺・↑縺・・                    return false;
            }
            else {
                // 邇我ｻ･螟悶・鬧偵ｒ遘ｻ蜍輔＆縺帙◆縺ｨ縺阪・                Bitboard target = checkersBB();
                const Square checksq = target.firstOneFromSQ11();

                if (target)
                    // 荳｡邇区焔縺ｪ縺ｮ縺ｧ縲∫脂縺碁・￡縺ｪ縺・焔縺ｯ鬧・岼
                    return false;

                target = betweenBB(checksq, kingSquare(us)) | checkersBB();
                if (!target.isSet(to))
                    // 邇峨→縲∫視謇九＠縺滄ｧ偵→縺ｮ髢薙↓遘ｻ蜍輔☆繧九°縲∫視謇九＠縺滄ｧ偵ｒ蜿悶ｋ莉･螟悶・鬧・岼縲・                    return false;
            }
        }
    }

    return true;
}

template bool Position::moveIsPseudoLegal<true >(const Move move) const;
template bool Position::moveIsPseudoLegal<false>(const Move move) const;

#if !defined NDEBUG
// 驕主悉(蜿医・迴ｾ蝨ｨ)縺ｫ逕滓・縺励◆謖・＠謇九′迴ｾ蝨ｨ縺ｮ螻髱｢縺ｧ繧よ怏蜉ｹ縺句愛螳壹・// 縺ゅ∪繧企溷ｺｦ縺瑚ｦ∵ｱゅ＆繧後ｋ蝣ｴ髱｢縺ｧ菴ｿ縺｣縺ｦ縺ｯ縺・￠縺ｪ縺・・bool Position::moveIsLegal(const Move move) const {
    return MoveList<LegalAll>(*this).contains(move);
}
#endif

// 螻髱｢縺ｮ譖ｴ譁ｰ
void Position::doMove(const Move move, StateInfo& newSt) {
    const CheckInfo ci(*this);
    doMove(move, newSt, ci, moveGivesCheck(move, ci));
}

// 螻髱｢縺ｮ譖ｴ譁ｰ
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
            // 鬧偵ｒ蜿悶▲縺溘→縺・            const HandPiece hpCaptured = pieceTypeToHandPiece(ptCaptured);
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
        // Occupied 縺ｯ to, from 縺ｮ菴咲ｽｮ縺ｮ繝薙ャ繝医ｒ謫堺ｽ懊☆繧九ｈ繧翫ｂ縲・        // Black 縺ｨ White 縺ｮ or 繧貞叙繧区婿縺碁溘＞縺ｯ縺壹・        byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);

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
                case DirecMisc: assert(false); break; // 譛驕ｩ蛹悶・轤ｺ縺ｮ繝繝溘・
                case DirecFile:
                    // from 縺ｮ菴咲ｽｮ縺九ｉ邵ｦ縺ｫ蛻ｩ縺阪ｒ隱ｿ縺ｹ繧九→逶ｸ謇狗脂縺ｨ縲∫ｩｺ縺咲視謇九＠縺ｦ縺・ｋ鬧偵↓蠖薙◆縺｣縺ｦ縺・ｋ縺ｯ縺壹ょ袖譁ｹ縺ｮ鬧偵′遨ｺ縺咲視謇九＠縺ｦ縺・ｋ鬧偵・                    st_->checkersBB |= rookAttackFile(from, occupiedBB()) & bbOf(us);
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
    // 縺薙％縺ｧ蜈医↓ turn_ 繧呈綾縺励◆縺ｮ縺ｧ縲∽ｻ･荳九［ove 縺ｯ us 縺ｮ謖・＠謇九→縺吶ｋ縲・    if (move.isDrop()) {
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
        const PieceType ptCaptured = move.cap(); // todo: st_->capturedType 菴ｿ縺医・濶ｯ縺・・
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
            // 鬧偵ｒ蜿悶▲縺溘→縺・            byTypeBB_[ptCaptured].xorBit(to);
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
            // 鬧偵ｒ蜿悶ｉ縺ｪ縺・→縺阪・縲…olorAndPieceTypeToPiece(us, ptCaptured) 縺ｯ 0 縺ｾ縺溘・ 16 縺ｫ縺ｪ繧九・            // 16 縺ｫ縺ｪ繧九→蝗ｰ繧九・縺ｧ縲・ｧ偵ｒ蜿悶ｉ縺ｪ縺・→縺阪・譏守､ｺ逧・↓ Empty 縺ｫ縺吶ｋ縲・            piece_[to] = Empty;
        byTypeBB_[ptFrom].xorBit(from);
        byTypeBB_[ptTo].xorBit(to);
        byColorBB_[us].xorBit(from, to);
        piece_[from] = colorAndPieceTypeToPiece(us, ptFrom);
    }
    // Occupied 縺ｯ to, from 縺ｮ菴咲ｽｮ縺ｮ繝薙ャ繝医ｒ謫堺ｽ懊☆繧九ｈ繧翫ｂ縲・    // Black 縺ｨ White 縺ｮ or 繧貞叙繧区婿縺碁溘＞縺ｯ縺壹・    byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);
    goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);

    // key 縺ｪ縺ｩ縺ｯ StateInfo 縺ｫ縺ｾ縺ｨ繧√ｉ繧後※縺・ｋ縺ｮ縺ｧ縲・    // previous 縺ｮ繝昴う繝ｳ繧ｿ繧・st_ 縺ｫ莉｣蜈･縺吶ｋ縺縺代〒濶ｯ縺・・    st_ = st_->previous;

    assert(isOK());
}

namespace {
    // SEE 縺ｮ鬆・分
    template <PieceType PT> struct SEENextPieceType {}; // 縺薙ｌ縺ｯ繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ蛹悶＠縺ｪ縺・・    template <> struct SEENextPieceType<Pawn     > { static const PieceType value = Lance;     };
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
            // todo: 螳滄圀縺ｫ遘ｻ蜍輔＠縺滓婿蜷代ｒ蝓ｺ縺ｫattackers繧呈峩譁ｰ縺吶ｌ縺ｰ縲》emplate, inline 繧剃ｽｿ逕ｨ縺励↑縺上※繧り憶縺輔◎縺・・            //       縺昴・蝣ｴ蜷医√く繝｣繝・す繝･縺ｫ荵励ｊ繧・☆縺上↑繧九・縺ｧ騾・↓騾溘￥縺ｪ繧九°繧ゅ・            if (PT == Pawn || PT == Lance)
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

    // nega max 逧・↓鬧偵・蜿悶ｊ蜷医＞縺ｮ轤ｹ謨ｰ繧呈ｱゅａ繧九・    while (--slIndex)
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
    // them(逶ｸ謇・ 蛛ｴ縺ｮ邇峨′騾・￡繧峨ｌ繧九°縲・    // sq : 邇区焔縺励◆逶ｸ謇九・鬧偵・菴咲ｽｮ縲らｴ蝉ｻ倥″縺九∵｡るｦｬ縺ｮ菴咲ｽｮ縺ｨ縺吶ｋ縲ゅｈ縺｣縺ｦ縲∫脂縺ｯ sq 縺ｫ縺ｯ陦後￠縺ｪ縺・・    // bb : sq 縺ｮ蛻ｩ縺阪・縺ゅｋ蝣ｴ謇縺ｮBitboard縲ゅｈ縺｣縺ｦ縲∫脂縺ｯ bb 縺ｮ繝薙ャ繝医′遶九▲縺ｦ縺・ｋ蝣ｴ謇縺ｫ縺ｯ陦後￠縺ｪ縺・・    // sq 縺ｨ ksq 縺ｮ菴咲ｽｮ縺ｮ Occupied Bitboard 縺ｮ縺ｿ縺ｯ縲√％縺薙〒譖ｴ譁ｰ縺励※隧穂ｾ｡縺励∝・縺ｫ謌ｻ縺吶・    // (螳滄圀縺ｫ縺ｯ繝・Φ繝昴Λ繝ｪ縺ｮOccupied Bitboard 繧剃ｽｿ縺・・縺ｧ縲∝・縺ｫ縺ｯ謌ｻ縺輔↑縺・・
    bool canKingEscape(const Position& pos, const Color us, const Square sq, const Bitboard& bb) {
        const Color them = oppositeColor(us);
        const Square ksq = pos.kingSquare(them);
        Bitboard kingMoveBB = bb.notThisAnd(pos.bbOf(them).notThisAnd(kingAttack(ksq)));
        kingMoveBB.clearBit(sq); // sq 縺ｫ縺ｯ陦後￠縺ｪ縺・・縺ｧ縲√け繝ｪ繧｢縺吶ｋ縲ＹorBit(sq)縺ｧ縺ｯ繝繝｡縲・
        if (kingMoveBB) {
            Bitboard tempOccupied = pos.occupiedBB();
            tempOccupied.setBit(sq);
            tempOccupied.clearBit(ksq);
            do {
                const Square to = kingMoveBB.firstOneFromSQ11();
                // 邇峨・遘ｻ蜍募・縺ｫ縲「s 蛛ｴ縺ｮ蛻ｩ縺阪′辟｡縺代ｌ縺ｰ縲》rue
                if (!pos.attackersToIsAny(us, to, tempOccupied))
                    return true;
            } while (kingMoveBB);
        }
        // 邇峨・遘ｻ蜍募・縺檎┌縺・・        return false;
    }
    // them(逶ｸ謇・ 蛛ｴ縺ｮ邇我ｻ･螟悶・鬧偵′ sq 縺ｫ縺ゅｋ us 蛛ｴ縺ｮ鬧偵ｒ蜿悶ｌ繧九°縲・    bool canPieceCapture(const Position& pos, const Color them, const Square sq, const Bitboard& dcBB) {
        // 邇我ｻ･螟悶〒謇薙▲縺滄ｧ偵ｒ蜿悶ｌ繧狗嶌謇句・縺ｮ鬧偵・ Bitboard
        Bitboard fromBB = pos.attackersToExceptKing(them, sq);

        if (fromBB) {
            const Square ksq = pos.kingSquare(them);
            do {
                const Square from = fromBB.firstOneFromSQ11();
                if (!pos.isDiscoveredCheck(from, sq, ksq, dcBB))
                    // them 蛛ｴ縺九ｉ隕九※縲｝in 縺輔ｌ縺ｦ縺・↑縺・ｧ偵〒縲∵遠縺溘ｌ縺滄ｧ偵ｒ蜿悶ｌ繧九・縺ｧ縲》rue
                    return true;
            } while (fromBB);
        }
        // 邇我ｻ･螟悶・鬧偵〒縲∵遠縺｣縺滄ｧ偵ｒ蜿悶ｌ縺ｪ縺・・        return false;
    }

    // pos.discoveredCheckBB<false>() 繧帝≦蟒ｶ隧穂ｾ｡縺吶ｋ繝舌・繧ｸ繝ｧ繝ｳ縲・    bool canPieceCapture(const Position& pos, const Color them, const Square sq) {
        Bitboard fromBB = pos.attackersToExceptKing(them, sq);

        if (fromBB) {
            const Square ksq = pos.kingSquare(them);
            const Bitboard dcBB = pos.discoveredCheckBB<false>();
            do {
                const Square from = fromBB.firstOneFromSQ11();
                if (!pos.isDiscoveredCheck(from, sq, ksq, dcBB))
                    // them 蛛ｴ縺九ｉ隕九※縲｝in 縺輔ｌ縺ｦ縺・↑縺・ｧ偵〒縲∵遠縺溘ｌ縺滄ｧ偵ｒ蜿悶ｌ繧九・縺ｧ縲》rue
                    return true;
            } while (fromBB);
        }
        // 邇我ｻ･螟悶・鬧偵〒縲∵遠縺｣縺滄ｧ偵ｒ蜿悶ｌ縺ｪ縺・・        return false;
    }
}

// us 縺・sq 縺ｸ豁ｩ繧呈遠縺｣縺溘→縺阪》hem 縺ｮ邇峨′隧ｰ繧縺九・// us 縺・sq 縺ｸ豁ｩ繧呈遠縺､縺ｮ縺ｯ邇区焔縺ｧ縺ゅｋ縺ｨ莉ｮ螳壹☆繧九・// 謇薙■豁ｩ隧ｰ繧√・縺ｨ縺阪》rue 繧定ｿ斐☆縲・bool Position::isPawnDropCheckMate(const Color us, const Square sq) const {
    const Color them = oppositeColor(us);
    // 邇我ｻ･螟悶・鬧偵〒縲∵遠縺溘ｌ縺滓ｭｩ縺悟叙繧後ｋ縺ｪ繧峨∵遠縺｡豁ｩ隧ｰ繧√〒縺ｯ縺ｪ縺・・    if (canPieceCapture(*this, them, sq))
        return false;
    // todo: 縺薙％縺ｧ邇峨・菴咲ｽｮ繧呈ｱゅａ繧九・縺ｯ縲∽ｸ贋ｽ阪〒豎ゅａ縺溘ｂ縺ｮ縺ｨ2驥阪↓縺ｪ繧九・縺ｧ辟｡鬧・ょｾ後〒謨ｴ逅・☆繧九％縺ｨ縲・    const Square ksq = kingSquare(them);

    // 邇我ｻ･螟悶〒謇薙▲縺滓ｭｩ繧貞叙繧後↑縺・→縺阪∫脂縺梧ｭｩ繧貞叙繧九°縲∫脂縺碁・￡繧九°縲・
    // 蛻ｩ縺阪ｒ豎ゅａ繧矩圀縺ｫ縲｛ccupied 縺ｮ豁ｩ繧呈遠縺｣縺滉ｽ咲ｽｮ縺ｮ bit 繧堤ｫ九※縺・Bitboard 繧剃ｽｿ逕ｨ縺吶ｋ縲・    // 縺薙％縺ｧ縺ｯ豁ｩ縺ｮ Bitboard 縺ｯ譖ｴ譁ｰ縺吶ｋ蠢・ｦ√′縺ｪ縺・・    // color 縺ｮ Bitboard 繧よ峩譁ｰ縺吶ｋ蠢・ｦ√′縺ｪ縺・・逶ｸ謇狗脂縺悟虚縺上→縺阪√％縺｡繧峨・謇薙▲縺滓ｭｩ縺ｧ邇峨ｒ蜿悶ｋ縺薙→縺ｯ辟｡縺・ぜ縲・
    const Bitboard tempOccupied = occupiedBB() | setMaskBB(sq);
    Bitboard kingMoveBB = bbOf(them).notThisAnd(kingAttack(ksq));

    // 蟆代↑縺上→繧よｭｩ繧貞叙繧区婿蜷代↓縺ｯ邇峨′蜍輔￠繧九・縺壹↑縺ｮ縺ｧ縲‥o while 繧剃ｽｿ逕ｨ縲・    assert(kingMoveBB);
    do {
        const Square to = kingMoveBB.firstOneFromSQ11();
        if (!attackersToIsAny(us, to, tempOccupied))
            // 逶ｸ謇狗脂縺ｮ遘ｻ蜍募・縺ｫ閾ｪ鬧偵・蛻ｩ縺阪′縺ｪ縺・↑繧峨∵遠縺｡豁ｩ隧ｰ繧√〒縺ｯ縺ｪ縺・・            return false;
    } while (kingMoveBB);

    return true;
}

inline void Position::xorBBs(const PieceType pt, const Square sq, const Color c) {
    byTypeBB_[Occupied].xorBit(sq);
    byTypeBB_[pt].xorBit(sq);
    byColorBB_[c].xorBit(sq);
}

// 逶ｸ謇狗脂縺・謇玖ｩｰ縺ｿ縺九←縺・°繧貞愛螳壹・// 1謇玖ｩｰ縺ｿ縺ｪ繧峨∬ｩｰ縺ｿ縺ｫ閾ｳ繧区欠縺玲焔縺ｮ荳驛ｨ縺ｮ諠・ｱ(from, to 縺ｮ縺ｿ縺ｨ縺・繧定ｿ斐☆縲・// 1謇玖ｩｰ縺ｿ縺ｧ縺ｪ縺・↑繧峨｀ove::moveNone() 繧定ｿ斐☆縲・// Bitboard 縺ｮ迥ｶ諷九ｒ騾比ｸｭ縺ｧ譖ｴ譁ｰ縺吶ｋ轤ｺ縲…onst 髢｢謨ｰ縺ｧ縺ｯ縺ｪ縺・・譖ｴ譁ｰ蠕後∝・縺ｫ謌ｻ縺吶′縲・
template <Color US> Move Position::mateMoveIn1Ply() {
    const Color Them = oppositeColor(US);
    const Square ksq = kingSquare(Them);
    const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);

    assert(!attackersToIsAny(Them, kingSquare(US)));

    // 鬧呈遠縺｡繧定ｪｿ縺ｹ繧九・    const Bitboard dropTarget = nOccupiedBB(); // emptyBB() 縺ｧ縺ｯ縺ｪ縺・・縺ｧ豕ｨ諢上＠縺ｦ菴ｿ縺・％縺ｨ縲・    const Hand ourHand = hand(US);
    // 邇区焔縺吶ｋ蜑阪・迥ｶ諷九・ dcBB縲・    // 髢薙↓縺ゅｋ鬧偵・逶ｸ謇句・縺ｮ鬧偵・    // 鬧呈遠縺｡縺ｮ縺ｨ縺阪・縲∵遠縺｣縺溷ｾ後ｂ縲∵遠縺溘ｌ繧句燕縺ｮ迥ｶ諷九・ dcBB 繧剃ｽｿ逕ｨ縺吶ｋ縲・    const Bitboard dcBB_betweenIsThem = discoveredCheckBB<false>();

    // 鬟幄ｻ頑遠縺｡
    if (ourHand.exists<HRook>()) {
        // 蜷磯ｧ偵＆繧後ｋ縺ｨ繧・ｄ縺薙＠縺・・縺ｧ縲・謇玖ｩｰ縺ｿ髢｢謨ｰ縺ｮ荳ｭ縺ｧ隱ｿ縺ｹ繧九・        // 縺薙％縺ｧ縺ｯ髮｢繧後◆菴咲ｽｮ縺九ｉ邇区焔縺吶ｋ縺ｮ縺ｯ閠・∴縺ｪ縺・・        Bitboard toBB = dropTarget & rookStepAttacks(ksq);
        while (toBB) {
            const Square to = toBB.firstOneFromSQ11();
            // 鬧偵ｒ謇薙▲縺溷ｴ謇縺ｫ閾ｪ鬧偵・蛻ｩ縺阪′縺ゅｋ縺九・辟｡縺代ｌ縺ｰ邇峨〒蜿悶ｉ繧後※隧ｰ縺ｾ縺ｪ縺・
            if (attackersToIsAny(US, to)) {
                // 邇峨′騾・￡繧峨ｌ縺壹∽ｻ悶・鬧偵〒蜿悶ｋ縺薙→繧ょ・譚･縺ｪ縺・°
                if (!canKingEscape(*this, US, to, rookAttackToEdge(to))
                    && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                {
                    return makeDropMove(Rook, to);
                }
            }
        }
    }
    // 鬥呵ｻ頑遠縺｡
    // 鬟幄ｻ翫〒隧ｰ縺ｾ縺ｪ縺代ｌ縺ｰ鬥呵ｻ翫〒繧りｩｰ縺ｾ縺ｪ縺・・縺ｧ縲‘lse if 繧剃ｽｿ逕ｨ縲・    // 邇峨′ 9(1) 谿ｵ逶ｮ縺ｫ縺・ｌ縺ｰ鬥呵ｻ翫〒邇区焔蜃ｺ譚･辟｡縺・・縺ｧ縲√◎繧後ｂ逵√￥縲・    else if (ourHand.exists<HLance>() && isInFrontOf<US, Rank9, Rank1>(makeRank(ksq))) {
        const Square to = ksq + TDeltaS;
        if (piece(to) == Empty && attackersToIsAny(US, to)) {
            if (!canKingEscape(*this, US, to, lanceAttackToEdge(US, to))
                && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
            {
                return makeDropMove(Lance, to);
            }
        }
    }

    // 隗呈遠縺｡
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

    // 驥第遠縺｡
    if (ourHand.exists<HGold>()) {
        Bitboard toBB;
        if (ourHand.exists<HRook>())
            // 鬟幄ｻ頑遠縺｡繧貞・縺ｫ隱ｿ縺ｹ縺溘・縺ｧ縲∝ｰｻ驥代□縺代・逵√￥縲・            toBB = dropTarget & (goldAttack(Them, ksq) ^ pawnAttack(US, ksq));
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
            // 驥第遠縺｡繧貞・縺ｫ隱ｿ縺ｹ縺溘・縺ｧ縲∵万繧∝ｾ後ｍ縺九ｉ謇薙▽蝣ｴ蜷医□縺代ｒ隱ｿ縺ｹ繧九・
            if (ourHand.exists<HBishop>())
                // 隗呈遠縺｡繧貞・縺ｫ隱ｿ縺ｹ縺溘・縺ｧ縲∵万繧√°繧峨・邇区焔繧る勁螟悶〒縺阪ｋ縲る橿謇薙■繧定ｪｿ縺ｹ繧句ｿ・ｦ√′縺ｪ縺・・                goto silver_drop_end;
            // 譁懊ａ蠕後ｍ縺九ｉ謇薙▽蝣ｴ蜷医ｒ隱ｿ縺ｹ繧句ｿ・ｦ√′縺ゅｋ縲・            toBB = dropTarget & (silverAttack(Them, ksq) & inFrontMask(US, makeRank(ksq)));
        }
        else {
            if (ourHand.exists<HBishop>())
                // 譁懊ａ蠕後ｍ繧帝勁螟悶ょ燕譁ｹ縺九ｉ謇薙▽蝣ｴ蜷医ｒ隱ｿ縺ｹ繧句ｿ・ｦ√′縺ゅｋ縲・                toBB = dropTarget & goldAndSilverAttacks(Them, ksq);
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
            // 譯るｦｬ縺ｯ邏舌′莉倥＞縺ｦ縺・ｋ蠢・ｦ√・縺ｪ縺・・            // 繧医▲縺ｦ縲√％縺ｮcanKingEscape() 蜀・〒縺ｮ to 縺ｮ菴咲ｽｮ縺ｫ騾・￡繧峨ｌ縺ｪ縺・ｈ縺・↓縺吶ｋ蜃ｦ逅・・辟｡鬧・・            if (!canKingEscape(*this, US, to, allZeroBB())
                && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
            {
                return makeDropMove(Knight, to);
            }
        }
    }

    // 豁ｩ謇薙■縺ｧ隧ｰ縺ｾ縺吶→蜿榊援縺ｪ縺ｮ縺ｧ縲∬ｪｿ縺ｹ縺ｪ縺・・
    // 鬧偵ｒ遘ｻ蜍輔☆繧句ｴ蜷・    // moveTarget 縺ｯ譯るｦｬ莉･螟悶・遘ｻ蜍募・縺ｮ螟ｧ縺ｾ縺九↑菴咲ｽｮ縲る｣幄ｧ帝ｦ吶・驕髫皮視謇九・蜷ｫ縺ｾ縺ｪ縺・・    const Bitboard moveTarget = bbOf(US).notThisAnd(kingAttack(ksq));
    const Bitboard pinned = pinnedBB();
    const Bitboard dcBB_betweenIsUs = discoveredCheckBB<true>();

    {
        // 遶懊↓繧医ｋ遘ｻ蜍・        Bitboard fromBB = bbOf(Dragon, US);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            // 驕髫皮視謇九・閠・∴縺ｪ縺・・            Bitboard toBB = moveTarget & attacksFrom<Dragon>(from);
            if (toBB) {
                xorBBs(Dragon, from, US);
                // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // 邇区焔縺励◆鬧偵・蝣ｴ謇縺ｫ閾ｪ鬧偵・蛻ｩ縺阪′縺ゅｋ縺九・辟｡縺代ｌ縺ｰ邇峨〒蜿悶ｉ繧後※隧ｰ縺ｾ縺ｪ縺・
                    if (unDropCheckIsSupported(US, to)) {
                        // 邇峨′騾・￡繧峨ｌ縺ｪ縺・                        // 縺九▽縲・遨ｺ縺咲視謇・縺ｾ縺溘・ 莉悶・鬧偵〒蜿悶ｌ縺ｪ縺・
                        // 縺九▽縲∫視謇九＠縺滄ｧ偵′ pin 縺輔ｌ縺ｦ縺・↑縺・                        if (!canKingEscape(*this, US, to, attacksFrom<Dragon>(to, occupiedBB() ^ setMaskBB(ksq)))
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

    // Txxx 縺ｯ蜈域焔縲∝ｾ梧焔縺ｮ諠・ｱ繧貞精蜿弱＠縺溷､画焚縲よ焚蟄励・蜈域焔縺ｫ蜷医ｏ縺帙※縺・ｋ縲・    const Rank TRank4 = (US == Black ? Rank4 : Rank6);
    const Bitboard TRank123BB = inFrontMask<US, TRank4>();
    {
        // 鬟幄ｻ翫↓繧医ｋ遘ｻ蜍・        Bitboard fromBB = bbOf(Rook, US);
        Bitboard fromOn123BB = fromBB & TRank123BB;
        // from 縺・123 谿ｵ逶ｮ
        if (fromOn123BB) {
            fromBB.andEqualNot(TRank123BB);
            do {
                const Square from = fromOn123BB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Rook>(from);
                if (toBB) {
                    xorBBs(Rook, from, US);
                    // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                    do {
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

        // from 縺・4~9 谿ｵ逶ｮ
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Rook>(from) & (rookStepAttacks(ksq) | TRank123BB);
            if (toBB) {
                xorBBs(Rook, from, US);
                // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();

                Bitboard toOn123BB = toBB & TRank123BB;
                // 謌舌ｊ
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
                // 荳肴・
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
        // 鬥ｬ縺ｫ繧医ｋ遘ｻ蜍・        Bitboard fromBB = bbOf(Horse, US);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            // 驕髫皮視謇九・閠・∴縺ｪ縺・・            Bitboard toBB = moveTarget & attacksFrom<Horse>(from);
            if (toBB) {
                xorBBs(Horse, from, US);
                // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // 邇区焔縺励◆鬧偵・蝣ｴ謇縺ｫ閾ｪ鬧偵・蛻ｩ縺阪′縺ゅｋ縺九・辟｡縺代ｌ縺ｰ邇峨〒蜿悶ｉ繧後※隧ｰ縺ｾ縺ｪ縺・
                    if (unDropCheckIsSupported(US, to)) {
                        // 邇峨′騾・￡繧峨ｌ縺ｪ縺・                        // 縺九▽縲・遨ｺ縺咲視謇・縺ｾ縺溘・ 莉悶・鬧偵〒蜿悶ｌ縺ｪ縺・
                        // 縺九▽縲∝虚縺九＠縺滄ｧ偵′ pin 縺輔ｌ縺ｦ縺・↑縺・
                        if (!canKingEscape(*this, US, to, horseAttackToEdge(to)) // 遶懊・蝣ｴ蜷医→驕輔▲縺ｦ縲∝ｸｸ縺ｫ譛螟ｧ縺ｮ蛻ｩ縺阪ｒ菴ｿ逕ｨ縺励※濶ｯ縺・・                            && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
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
        // 隗偵↓繧医ｋ遘ｻ蜍・        Bitboard fromBB = bbOf(Bishop, US);
        Bitboard fromOn123BB = fromBB & TRank123BB;
        // from 縺・123 谿ｵ逶ｮ
        if (fromOn123BB) {
            fromBB.andEqualNot(TRank123BB);
            do {
                const Square from = fromOn123BB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Bishop>(from);
                if (toBB) {
                    xorBBs(Bishop, from, US);
                    // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                    do {
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

        // from 縺・4~9 谿ｵ逶ｮ
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Bishop>(from) & (bishopStepAttacks(ksq) | TRank123BB);
            if (toBB) {
                xorBBs(Bishop, from, US);
                // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();

                Bitboard toOn123BB = toBB & TRank123BB;
                // 謌舌ｊ
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
                // 荳肴・
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
        // 驥代∵・繧企≡縺ｫ繧医ｋ遘ｻ蜍・        Bitboard fromBB = goldsBB(US) & goldCheckTable(US, ksq);
        while (fromBB) {
            const Square from = fromBB.firstOneFromSQ11();
            Bitboard toBB = moveTarget & attacksFrom<Gold>(US, from) & attacksFrom<Gold>(Them, ksq);
            if (toBB) {
                const PieceType pt = pieceToPieceType(piece(from));
                xorBBs(pt, from, US);
                goldsBB_.xorBit(from);
                // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                do {
                    const Square to = toBB.firstOneFromSQ11();
                    // 邇区焔縺励◆鬧偵・蝣ｴ謇縺ｫ閾ｪ鬧偵・蛻ｩ縺阪′縺ゅｋ縺九・辟｡縺代ｌ縺ｰ邇峨〒蜿悶ｉ繧後※隧ｰ縺ｾ縺ｪ縺・
                    if (unDropCheckIsSupported(US, to)) {
                        // 邇峨′騾・￡繧峨ｌ縺ｪ縺・                        // 縺九▽縲・遨ｺ縺咲視謇・縺ｾ縺溘・ 莉悶・鬧偵〒蜿悶ｌ縺ｪ縺・
                        // 縺九▽縲∝虚縺九＠縺滄ｧ偵′ pin 縺輔ｌ縺ｦ縺・↑縺・
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
        // 驫縺ｫ繧医ｋ遘ｻ蜍・        Bitboard fromBB = bbOf(Silver, US) & silverCheckTable(US, ksq);
        if (fromBB) {
            // Txxx 縺ｯ蜈域焔縲∝ｾ梧焔縺ｮ諠・ｱ繧貞精蜿弱＠縺溷､画焚縲よ焚蟄励・蜈域焔縺ｫ蜷医ｏ縺帙※縺・ｋ縲・            const Bitboard TRank5_9BB = inFrontMask<Them, TRank4>();
            const Bitboard chkBB = attacksFrom<Silver>(Them, ksq);
            const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq);

            Bitboard fromOn123BB = fromBB & TRank123BB;
            // from 縺梧雰髯｣
            if (fromOn123BB) {
                fromBB.andEqualNot(TRank123BB);
                do {
                    const Square from = fromOn123BB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
                    Bitboard toBB_promo = toBB & chkBB_promo;

                    toBB &= chkBB;
                    if ((toBB_promo | toBB)) {
                        xorBBs(Silver, from, US);
                        // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                        while (toBB_promo) {
                            const Square to = toBB_promo.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 謌舌ｊ
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

                        // 邇峨・蜑肴婿縺ｫ遘ｻ蜍輔☆繧句ｴ蜷医∵・縺ｧ隧ｰ縺ｾ縺ｪ縺九▲縺溘ｉ荳肴・縺ｧ繧りｩｰ縺ｾ縺ｪ縺・・縺ｧ縲√％縺薙〒逵√￥縲・                        // sakurapyon 縺ｮ菴懆・′險縺｣縺ｦ縺溘・縺ｧ螳溯｣・・                        toBB.andEqualNot(inFrontMask(Them, makeRank(ksq)));
                        while (toBB) {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 荳肴・
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

            // from 縺・5~9谿ｵ逶ｮ (蠢・★荳肴・)
            Bitboard fromOn5_9BB = fromBB & TRank5_9BB;
            if (fromOn5_9BB) {
                fromBB.andEqualNot(TRank5_9BB);
                do {
                    const Square from = fromOn5_9BB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from) & chkBB;

                    if (toBB) {
                        xorBBs(Silver, from, US);
                        // 蜍輔＞縺溷ｾ後・ dcBB, pinned: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                        while (toBB) {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 荳肴・
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

            // 谿九ｊ 4 谿ｵ逶ｮ縺ｮ縺ｿ
            // 蜑埼ｲ縺吶ｋ縺ｨ縺阪・謌舌ｌ繧九′縲∝ｾ碁縺吶ｋ縺ｨ縺阪・謌舌ｌ縺ｪ縺・・            while (fromBB) {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
                Bitboard toBB_promo = toBB & TRank123BB & chkBB_promo; // 3 谿ｵ逶ｮ縺ｫ縺励°謌舌ｌ縺ｪ縺・・
                toBB &= chkBB;
                if ((toBB_promo | toBB)) {
                    xorBBs(Silver, from, US);
                    // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // 謌舌ｊ
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
                            // 荳肴・
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
        // 譯ゅ↓繧医ｋ遘ｻ蜍・        Bitboard fromBB = bbOf(Knight, US) & knightCheckTable(US, ksq);
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
                    // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // 謌舌ｊ
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
                        // 譯るｦｬ縺ｯ邏舌′莉倥＞縺ｦ縺ｪ縺上※濶ｯ縺・・縺ｧ縲∫ｴ舌′莉倥＞縺ｦ縺・ｋ縺九・隱ｿ縺ｹ縺ｪ縺・・                        // 荳肴・
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
        // 鬥呵ｻ翫↓繧医ｋ遘ｻ蜍・        Bitboard fromBB = bbOf(Lance, US) & lanceCheckTable(US, ksq);
        if (fromBB) {
            // Txxx 縺ｯ蜈域焔縲∝ｾ梧焔縺ｮ諠・ｱ繧貞精蜿弱＠縺溷､画焚縲よ焚蟄励・蜈域焔縺ｫ蜷医ｏ縺帙※縺・ｋ縲・            const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
            const Rank TRank2 = (US == Black ? Rank2 : Rank8);
            const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq) & TRank123BB;
            // 邇峨・蜑肴婿1繝槭せ縺ｮ縺ｿ縲・            // 邇峨′ 1 谿ｵ逶ｮ縺ｫ縺・ｋ縺ｨ縺阪・縲∵・縺ｮ縺ｿ縺ｧ濶ｯ縺・・縺ｧ逵√￥縲・            const Bitboard chkBB = attacksFrom<Pawn>(Them, ksq) & inFrontMask<Them, TRank2>();

            do {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Lance>(US, from);
                Bitboard toBB_promo = toBB & chkBB_promo;

                toBB &= chkBB;

                if ((toBB_promo | toBB)) {
                    xorBBs(Lance, from, US);
                    // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・
                    while (toBB_promo) {
                        const Square to = toBB_promo.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            // 謌舌ｊ
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
                        // 荳肴・縺ｧ邇区焔蜃ｺ譚･繧九・縺ｯ縲∽ｸ縺､縺ｮ蝣ｴ謇縺縺代↑縺ｮ縺ｧ縲√Ν繝ｼ繝励↓縺吶ｋ蠢・ｦ√′辟｡縺・・                        const Square to = ksq + TDeltaS;
                        if (unDropCheckIsSupported(US, to)) {
                            // 荳肴・
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
        // 豁ｩ縺ｫ繧医ｋ遘ｻ蜍・        // 謌舌ｌ繧句ｴ蜷医・蠢・★縺ｪ繧九・        // todo: PawnCheckBB 菴懊▲縺ｦ邁｡逡･蛹悶☆繧九・        const Rank krank = makeRank(ksq);
        // 豁ｩ縺檎ｧｻ蜍輔＠縺ｦ邇区焔縺ｫ縺ｪ繧九・縺ｯ縲∫嶌謇狗脂縺・~7谿ｵ逶ｮ縺ｮ譎ゅ・縺ｿ縲・        if (isInFrontOf<US, Rank8, Rank2>(krank)) {
            // Txxx 縺ｯ蜈域焔縲∝ｾ梧焔縺ｮ諠・ｱ繧貞精蜿弱＠縺溷､画焚縲よ焚蟄励・蜈域焔縺ｫ蜷医ｏ縺帙※縺・ｋ縲・            const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
            const SquareDelta TDeltaN = (US == Black ? DeltaN : DeltaS);

            Bitboard fromBB = bbOf(Pawn, US);
            // 邇峨′謨ｵ髯｣縺ｫ縺・↑縺・→謌舌〒邇区焔縺ｫ縺ｪ繧九％縺ｨ縺ｯ縺ｪ縺・・            if (isInFrontOf<US, Rank4, Rank6>(krank)) {
                // 謌舌▲縺滓凾縺ｫ邇区焔縺ｫ縺ｪ繧倶ｽ咲ｽｮ
                const Bitboard toBB_promo = moveTarget & attacksFrom<Gold>(Them, ksq) & TRank123BB;
                Bitboard fromBB_promo = fromBB & pawnAttack<Them>(toBB_promo);
                while (fromBB_promo) {
                    const Square from = fromBB_promo.firstOneFromSQ11();
                    const Square to = from + TDeltaN;

                    xorBBs(Pawn, from, US);
                    // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                    if (unDropCheckIsSupported(US, to)) {
                        // 謌舌ｊ
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

            // 荳肴・
            // 邇峨′ 8,9 谿ｵ逶ｮ縺ｫ縺・ｋ縺薙→縺ｯ辟｡縺・・縺ｧ縲’rom,to 縺碁團縺ｮ遲九ｒ謖・☆縺薙→縺ｯ辟｡縺・・            const Square to = ksq + TDeltaS;
            const Square from = to + TDeltaS;
            if (fromBB.isSet(from) && !bbOf(US).isSet(to)) {
                // 邇峨′ 1, 2 谿ｵ逶ｮ縺ｫ縺・ｋ縺ｪ繧峨∵・繧翫〒邇区焔蜃ｺ譚･繧九・縺ｧ荳肴・縺ｯ隱ｿ縺ｹ縺ｪ縺・・                if (isBehind<US, Rank2, Rank8>(krank)) {
                    xorBBs(Pawn, from, US);
                    // 蜍輔＞縺溷ｾ後・ dcBB: to 縺ｮ菴咲ｽｮ縺ｮ occupied 繧・checkers 縺ｯ髢｢菫ゅ↑縺・・縺ｧ縲√％縺薙〒逕滓・縺ｧ縺阪ｋ縲・                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to 縺ｮ菴咲ｽｮ縺ｮ Bitboard 縺ｯ canKingEscape 縺ｮ荳ｭ縺ｧ譖ｴ譁ｰ縺吶ｋ縲・                    if (unDropCheckIsSupported(US, to)) {
                        // 荳肴・
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
    // zobTurn_ 縺ｯ 1 縺ｧ縺ゅｊ縲√◎縺ｮ莉悶・ 1譯∫岼繧剃ｽｿ繧上↑縺・・    // zobTurn 縺ｮ縺ｿ xor 縺ｧ譖ｴ譁ｰ縺吶ｋ轤ｺ縲∽ｻ悶・譯√↓蠖ｱ髻ｿ縺励↑縺・ｈ縺・↓縺吶ｋ轤ｺ縲・    // hash蛟､縺ｮ譖ｴ譁ｰ縺ｯ譎ｮ騾壹・蜈ｨ縺ｦ xor 繧剃ｽｿ縺・′縲∵戟縺｡鬧偵・譖ｴ譁ｰ縺ｮ轤ｺ縺ｫ +, - 繧剃ｽｿ逕ｨ縺励◆譁ｹ縺碁・蜷医′濶ｯ縺・・    for (PieceType pt = Occupied; pt < PieceTypeNum; ++pt) {
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
        // USI 縺ｮ隕乗ｼ縺ｨ縺励※縲∵戟縺｡鬧偵・陦ｨ險倬・・豎ｺ縺ｾ縺｣縺ｦ縺翫ｊ縲∝・謇九∝ｾ梧焔縺ｮ鬆・〒縲√◎繧後◇繧・鬟帙∬ｧ偵・≡縲・橿縲∵｡ゅ・ｦ吶∵ｭｩ 縺ｮ鬆・・        for (Color color = Black; color < ColorNum; ++color) {
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
    // 謇狗分 (1bit)
    bs.putBit(turn());

    // 邇峨・菴咲ｽｮ (7bit * 2)
    bs.putBits(kingSquare(Black), 7);
    bs.putBits(kingSquare(White), 7);

    // 逶､荳翫・鬧・    for (Square sq = SQ11; sq < SquareNum; ++sq) {
        Piece pc = piece(sq);
        if (pieceToPieceType(pc) == King)
            continue;
        const auto hc = HuffmanCodedPos::boardCodeTable[pc];
        bs.putBits(hc.code, hc.numOfBits);
    }

    // 謖√■鬧・    for (Color c = Black; c < ColorNum; ++c) {
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
        // 逶ｸ謇狗脂繧貞叙繧後↑縺・％縺ｨ繧堤｢ｺ隱・        const Color us = turn();
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

// todo: isRepetition() 縺ｫ蜷榊燕螟峨∴縺滓婿縺瑚憶縺輔◎縺・・//       蜷御ｸ螻髱｢4蝗槭ｒ縺阪■繧薙→謨ｰ縺医※縺・↑縺・￠縺ｩ蝠城｡後↑縺・°縲・RepetitionType Position::isDraw(const int checkMaxPly) const {
    const int Start = 4;
    int i = Start;
    const int e = std::min(st_->pliesFromNull, checkMaxPly);

    // 4謇区寺縺代↑縺・→蜊・律謇九↓縺ｯ邨ｶ蟇ｾ縺ｫ縺ｪ繧峨↑縺・・    if (i <= e) {
        // 迴ｾ蝨ｨ縺ｮ螻髱｢縺ｨ縲∝ｰ代↑縺上→繧・4 謇区綾繧峨↑縺・→蜷後§螻髱｢縺ｫ縺ｪ繧峨↑縺・・        // 縺薙％縺ｧ縺ｾ縺・2 謇区綾繧九・        StateInfo* stp = st_->previous->previous;

        do {
            // 譖ｴ縺ｫ 2 謇区綾繧九・            stp = stp->previous->previous;
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

    // 逶､荳翫・鬧・    while (ss.get(token) && token != ' ') {
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

    // 謇狗分
    while (ss.get(token) && token != ' ') {
        if (token == 'b')
            turn_ = Black;
        else if (token == 'w')
            turn_ = White;
        else
            goto INCORRECT;
    }

    // 謖√■鬧・    for (int digits = 0; ss.get(token) && token != ' '; ) {
        if (token == '-')
            memset(hand_, 0, sizeof(hand_));
        else if (isdigit(token))
            digits = digits * 10 + token - '0';
        else if (g_charToPieceUSI.isLegalChar(token)) {
            // 謖√■鬧偵ｒ32bit 縺ｫ pack 縺吶ｋ
            const Piece piece = g_charToPieceUSI.value(token);
            setHand(piece, (digits == 0 ? 1 : digits));

            digits = 0;
        }
        else
            goto INCORRECT;
    }

    // 谺｡縺ｮ謇九′菴墓焔逶ｮ縺・    ss >> gamePly_;

    // 谿九ｊ譎る俣, hash key, (繧ゅ＠螳溯｣・☆繧九↑繧・鬧堤分蜿ｷ縺ｪ縺ｩ繧偵％縺薙〒險ｭ螳・    st_->boardKey = computeBoardKey();
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

    HuffmanCodedPos tmp = hcp; // 繝ｭ繝ｼ繧ｫ繝ｫ縺ｫ繧ｳ繝斐・
    BitStream bs(tmp.data);

    // 謇狗分
    turn_ = static_cast<Color>(bs.getBit());

    // 邇峨・菴咲ｽｮ
    Square sq0 = (Square)bs.getBits(7);
    Square sq1 = (Square)bs.getBits(7);
    setPiece(BKing, static_cast<Square>(sq0));
    setPiece(WKing, static_cast<Square>(sq1));

    // 逶､荳翫・鬧・    for (Square sq = SQ11; sq < SquareNum; ++sq) {
        if (pieceToPieceType(piece(sq)) == King) // piece(sq) 縺ｯ BKing, WKing, Empty 縺ｮ縺ｩ繧後°縲・            continue;
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

    gamePly_ = 1; // ply 縺ｮ諠・ｱ縺ｯ謖√▲縺ｦ縺・↑縺・・縺ｧ 1 縺ｫ縺励※縺翫￥縲・
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

// move 縺檎視謇九↑繧・true
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

// 蜈域焔縲∝ｾ梧焔縺ｫ髢｢繧上ｉ縺壹《q 縺ｸ遘ｻ蜍募庄閭ｽ縺ｪ Bitboard 繧定ｿ斐☆縲・Bitboard Position::attackersTo(const Square sq, const Bitboard& occupied) const {
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

// occupied 繧・Position::occupiedBB() 莉･螟悶・繧ゅ・繧剃ｽｿ逕ｨ縺吶ｋ蝣ｴ蜷医↓菴ｿ逕ｨ縺吶ｋ縲・Bitboard Position::attackersTo(const Color c, const Square sq, const Bitboard& occupied) const {
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

// 邇我ｻ･螟悶〒 sq 縺ｸ遘ｻ蜍募庄閭ｽ縺ｪ c 蛛ｴ縺ｮ鬧偵・ Bitboard 繧定ｿ斐☆縲・Bitboard Position::attackersToExceptKing(const Color c, const Square sq) const {
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
