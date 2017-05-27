/*
  Apery, a USI shogi playing engine derived from Stockfish, a UCI chess playing engine.
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2011-2017 Hiraoka Takuya

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

#ifndef APERY_POSITION_HPP
#define APERY_POSITION_HPP

#include "piece.hpp"
#include "common.hpp"
#include "hand.hpp"
#include "bitboard.hpp"
#include "pieceScore.hpp"
#include "evalList.hpp"
#include <stack>
#include <memory>

class Position;

enum GameResult : int8_t {
    Draw, BlackWin, WhiteWin, GameResultNum
};

enum RepetitionType {
    NotRepetition, RepetitionDraw, RepetitionWin, RepetitionLose,
    RepetitionSuperior, RepetitionInferior
};

struct CheckInfo {
    explicit CheckInfo(const Position&);
    Bitboard dcBB; // discoverd check candidates bitboard
    Bitboard pinned;
    Bitboard checkBB[PieceTypeNum];
};

struct ChangedListPair {
    int newlist[2];
    int oldlist[2];
};

struct ChangedLists {
    ChangedListPair clistpair[2]; // 荳謇九〒蜍輔￥鬧偵・譛螟ｧ2縺､縲・蜍輔￥鬧偵∝叙繧峨ｌ繧矩ｧ・
    int listindex[2]; // 荳謇九〒蜍輔￥鬧偵・譛螟ｧ2縺､縲・蜍輔￥鬧偵∝叙繧峨ｌ繧矩ｧ・
    size_t size;
};

struct StateInfo {
    // Copied when making a move
    Score material; // stocfish 縺ｮ npMaterial 縺ｯ 蜈域焔縲∝ｾ梧焔縺ｮ轤ｹ謨ｰ繧帝・蛻励〒謖√▲縺ｦ縺・ｋ縺代←縲・                    // 迚ｹ縺ｫ蛻・￠繧句ｿ・ｦ√・辟｡縺・ｰ励′縺吶ｋ縲・    int pliesFromNull;
    int continuousCheck[ColorNum]; // Stockfish 縺ｫ縺ｯ辟｡縺・・
    // Not copied when making a move (will be recomputed anyhow)
    Key boardKey;
    Key handKey;
    Bitboard checkersBB; // 謇狗分蛛ｴ縺ｮ邇峨∈ check 縺励※縺・ｋ鬧偵・ Bitboard
#if 0
    Piece capturedPiece;
#endif
    StateInfo* previous;
    Hand hand; // 謇狗分蛛ｴ縺ｮ謖√■鬧・    ChangedLists cl;

    Key key() const { return boardKey + handKey; }
};

using StateListPtr = std::unique_ptr<std::deque<StateInfo>>;

class BitStream {
public:
    // 隱ｭ縺ｿ霎ｼ繧蜈磯ｭ繝・・繧ｿ縺ｮ繝昴う繝ｳ繧ｿ繧偵そ繝・ヨ縺吶ｋ縲・    BitStream(u8* d) : data_(d), curr_() {}
    // 隱ｭ縺ｿ霎ｼ繧蜈磯ｭ繝・・繧ｿ縺ｮ繝昴う繝ｳ繧ｿ繧偵そ繝・ヨ縺吶ｋ縲・    void set(u8* d) {
        data_ = d;
        curr_ = 0;
    }
    // ・・bit 隱ｭ縺ｿ霎ｼ繧縲ゅ←縺薙∪縺ｧ隱ｭ縺ｿ霎ｼ繧薙□縺九ｒ陦ｨ縺・bit 縺ｮ菴咲ｽｮ繧・1 蛟矩ｲ繧√ｋ縲・    u8 getBit() {
        const u8 result = (*data_ & (1 << curr_++)) ? 1 : 0;
        if (curr_ == 8) {
            ++data_;
            curr_ = 0;
        }
        return result;
    }
    // numOfBits bit隱ｭ縺ｿ霎ｼ繧縲ゅ←縺薙∪縺ｧ隱ｭ縺ｿ霎ｼ繧薙□縺九ｒ陦ｨ縺・bit 縺ｮ菴咲ｽｮ繧・numOfBits 蛟矩ｲ繧√ｋ縲・    u8 getBits(const int numOfBits) {
        assert(numOfBits <= 8);
        u8 result = 0;
        for (int i = 0; i < numOfBits; ++i)
            result |= getBit() << i;
        return result;
    }
    // 1 bit 譖ｸ縺崎ｾｼ繧縲・    void putBit(const u8 bit) {
        assert(bit <= 1);
        *data_ |= bit << curr_++;
        if (curr_ == 8) {
            ++data_;
            curr_ = 0;
        }
    }
    // val 縺ｮ蛟､繧・numOfBits bit 譖ｸ縺崎ｾｼ繧縲・ bit 縺ｾ縺ｧ縲・    void putBits(u8 val, const int numOfBits) {
        assert(numOfBits <= 8);
        for (int i = 0; i < numOfBits; ++i) {
            const u8 bit = val & 1;
            val >>= 1;
            putBit(bit);
        }
    }
    u8* data() const { return data_; }
    int curr() const { return curr_; }

private:
    u8* data_;
    int curr_; // 1byte 荳ｭ縺ｮ bit 縺ｮ菴咲ｽｮ
};

union HuffmanCode {
    struct {
        u8 code;      // 隨ｦ蜿ｷ蛹匁凾縺ｮ bit 蛻・        u8 numOfBits; // 菴ｿ逕ｨ bit 謨ｰ
    };
    u16 key; // std::unordered_map 縺ｮ key 縺ｨ縺励※菴ｿ縺・・};

struct HuffmanCodeToPieceHash : public std::unordered_map<u16, Piece> {
    Piece value(const u16 key) const {
        const auto it = find(key);
        if (it == std::end(*this))
            return PieceNone;
        return it->second;
    }
};

// Huffman 隨ｦ蜿ｷ蛹悶＆繧後◆螻髱｢縺ｮ繝・・繧ｿ讒矩縲・56 bit 縺ｧ螻髱｢繧定｡ｨ縺吶・struct HuffmanCodedPos {
    static const HuffmanCode boardCodeTable[PieceNone];
    static const HuffmanCode handCodeTable[HandPieceNum][ColorNum];
    static HuffmanCodeToPieceHash boardCodeToPieceHash;
    static HuffmanCodeToPieceHash handCodeToPieceHash;
    static void init() {
        for (Piece pc = Empty; pc <= BDragon; ++pc)
            if (pieceToPieceType(pc) != King) // 邇峨・菴咲ｽｮ縺ｧ隨ｦ蜿ｷ蛹悶☆繧九・縺ｧ縲・ｧ偵・遞ｮ鬘槭〒縺ｯ隨ｦ蜿ｷ蛹悶＠縺ｪ縺・・                boardCodeToPieceHash[boardCodeTable[pc].key] = pc;
        for (Piece pc = WPawn; pc <= WDragon; ++pc)
            if (pieceToPieceType(pc) != King) // 邇峨・菴咲ｽｮ縺ｧ隨ｦ蜿ｷ蛹悶☆繧九・縺ｧ縲・ｧ偵・遞ｮ鬘槭〒縺ｯ隨ｦ蜿ｷ蛹悶＠縺ｪ縺・・                boardCodeToPieceHash[boardCodeTable[pc].key] = pc;
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
    u16 bestMove16; // 菴ｿ縺・°縺ｯ蛻・°繧峨↑縺・′謨吝ｸｫ繝・・繧ｿ逕滓・譎ゅ↓縺､縺・〒縺ｫ蜿門ｾ励＠縺ｦ縺翫￥縲・    GameResult gameResult; // 閾ｪ蟾ｱ蟇ｾ螻縺ｧ蜍昴▲縺溘°縺ｩ縺・°縲・};
static_assert(sizeof(HuffmanCodedPosAndEval) == 38, "");

class Move;
struct Thread;
struct Searcher;

class Position {
public:
    Position() {}
    explicit Position(Searcher* s) : searcher_(s) {}
    Position(const Position& pos) { *this = pos; }
    Position(const Position& pos, Thread* th) {
        *this = pos;
        thisThread_ = th;
    }
    Position(const std::string& sfen, Thread* th, Searcher* s) {
        set(sfen, th);
        setSearcher(s);
    }

    Position& operator = (const Position& pos);
    void set(const std::string& sfen, Thread* th);
    bool set(const HuffmanCodedPos& hcp, Thread* th);

    Bitboard bbOf(const PieceType pt) const                                            { return byTypeBB_[pt]; }
    Bitboard bbOf(const Color c) const                                                 { return byColorBB_[c]; }
    Bitboard bbOf(const PieceType pt, const Color c) const                             { return bbOf(pt) & bbOf(c); }
    Bitboard bbOf(const PieceType pt1, const PieceType pt2) const                      { return bbOf(pt1) | bbOf(pt2); }
    Bitboard bbOf(const PieceType pt1, const PieceType pt2, const Color c) const       { return bbOf(pt1, pt2) & bbOf(c); }
    Bitboard bbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3) const { return bbOf(pt1, pt2) | bbOf(pt3); }
    Bitboard bbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3, const PieceType pt4) const {
        return bbOf(pt1, pt2, pt3) | bbOf(pt4);
    }
    Bitboard bbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3,
                  const PieceType pt4, const PieceType pt5) const
    {
        return bbOf(pt1, pt2, pt3, pt4) | bbOf(pt5);
    }
    Bitboard occupiedBB() const { return bbOf(Occupied); }
    // emptyBB() 繧医ｊ繧ゅｏ縺壹°縺ｫ騾溘＞縺ｯ縺壹・    // emptyBB() 縺ｨ縺ｯ逡ｰ縺ｪ繧翫∝・縺丈ｽｿ逕ｨ縺励↑縺・ｽ咲ｽｮ(0 縺九ｉ謨ｰ縺医※縲〉ight 縺ｮ 63bit逶ｮ縲〕eft 縺ｮ 18 ~ 63bit逶ｮ)
    // 縺ｮ bit 縺・1 縺ｫ縺ｪ縺｣縺ｦ繧よｧ九ｏ縺ｪ縺・→縺阪√％縺｡繧峨ｒ菴ｿ縺・・    // todo: SSE縺ｫ繝薙ャ繝亥渚霆｢縺檎┌縺・・縺ｧ螳溘・縺昴ｓ縺ｪ縺ｫ騾溘￥縺ｪ縺・・縺壹ゆｸ崎ｦ√・    Bitboard nOccupiedBB() const          { return ~occupiedBB(); }
    Bitboard emptyBB() const              { return occupiedBB() ^ allOneBB(); }
    // 驥代∵・繧企≡ 縺ｮ Bitboard
    Bitboard goldsBB() const              { return goldsBB_; }
    Bitboard goldsBB(const Color c) const { return goldsBB() & bbOf(c); }

    Piece piece(const Square sq) const    { return piece_[sq]; }

    // hand
    Hand hand(const Color c) const { return hand_[c]; }

    // turn() 蛛ｴ縺・pin 縺輔ｌ縺ｦ縺・ｋ Bitboard 繧定ｿ斐☆縲・    // checkersBB 縺梧峩譁ｰ縺輔ｌ縺ｦ縺・ｋ蠢・ｦ√′縺ゅｋ縲・    Bitboard pinnedBB() const { return hiddenCheckers<true, true>(); }
    // 髢薙・鬧偵ｒ蜍輔°縺吶％縺ｨ縺ｧ縲》urn() 蛛ｴ縺檎ｩｺ縺咲視謇九′蜃ｺ譚･繧矩ｧ偵・Bitboard繧定ｿ斐☆縲・    // checkersBB 縺梧峩譁ｰ縺輔ｌ縺ｦ縺・ｋ蠢・ｦ√・縺ｪ縺・・    // BetweenIsUs == true  : 髢薙・鬧偵′閾ｪ鬧偵・    // BetweenIsUs == false : 髢薙・鬧偵′謨ｵ鬧偵・    template <bool BetweenIsUs = true> Bitboard discoveredCheckBB() const { return hiddenCheckers<false, BetweenIsUs>(); }

    // toFile 縺ｨ蜷後§遲九↓ us 縺ｮ豁ｩ縺後↑縺・↑繧・true
    bool noPawns(const Color us, const File toFile) const { return !bbOf(Pawn, us).andIsAny(fileMask(toFile)); }
    bool isPawnDropCheckMate(const Color us, const Square sq) const;
    // Pin縺輔ｌ縺ｦ縺・ｋfrom縺ｮ鬧偵′to縺ｫ遘ｻ蜍募・譚･縺ｪ縺代ｌ縺ｰtrue繧定ｿ斐☆縲・    template <bool IsKnight = false>
    bool isPinnedIllegal(const Square from, const Square to, const Square ksq, const Bitboard& pinned) const {
        // 譯るｦｬ縺ｪ繧峨←縺薙↓蜍輔＞縺ｦ繧るｧ・岼縲・        return pinned.isSet(from) && (IsKnight || !isAligned<true>(from, to, ksq));
    }
    // 遨ｺ縺咲視謇九°縺ｩ縺・°縲・    template <bool IsKnight = false>
    bool isDiscoveredCheck(const Square from, const Square to, const Square ksq, const Bitboard& dcBB) const {
        // 譯るｦｬ縺ｪ繧峨←縺薙↓蜍輔＞縺ｦ繧らｩｺ縺咲視謇九↓縺ｪ繧九・        return dcBB.isSet(from) && (IsKnight || !isAligned<true>(from, to, ksq));
    }

    Bitboard checkersBB() const     { return st_->checkersBB; }
    Bitboard prevCheckersBB() const { return st_->previous->checkersBB; }
    // 邇区焔縺梧寺縺九▲縺ｦ縺・ｋ縺九・    bool inCheck() const            { return checkersBB().isAny(); }

    Score material() const { return st_->material; }
    Score materialDiff() const { return st_->material - st_->previous->material; }

    FORCE_INLINE Square kingSquare(const Color c) const {
        assert(kingSquare_[c] == bbOf(King, c).constFirstOneFromSQ11());
        return kingSquare_[c];
    }

    bool moveGivesCheck(const Move m) const;
    bool moveGivesCheck(const Move move, const CheckInfo& ci) const;
    Piece movedPiece(const Move m) const;

    // attacks
    Bitboard attackersTo(const Square sq, const Bitboard& occupied) const;
    Bitboard attackersTo(const Color c, const Square sq) const { return attackersTo(c, sq, occupiedBB()); }
    Bitboard attackersTo(const Color c, const Square sq, const Bitboard& occupied) const;
    Bitboard attackersToExceptKing(const Color c, const Square sq) const;
    // todo: 蛻ｩ縺阪ｒ繝・・繧ｿ縺ｨ縺励※謖√▲縺溘→縺阪∥ttackersToIsAny() 繧帝ｫ倬溷喧縺吶ｋ縺薙→縲・    bool attackersToIsAny(const Color c, const Square sq) const { return attackersTo(c, sq).isAny(); }
    bool attackersToIsAny(const Color c, const Square sq, const Bitboard& occupied) const {
        return attackersTo(c, sq, occupied).isAny();
    }
    // 遘ｻ蜍慕視謇九′蜻ｳ譁ｹ縺ｮ蛻ｩ縺阪↓謾ｯ縺医ｉ繧後※縺・ｋ縺九Ｇalse 縺ｪ繧臥嶌謇狗脂縺ｧ蜿悶ｌ縺ｰ隧ｰ縺ｾ縺ｪ縺・・    bool unDropCheckIsSupported(const Color c, const Square sq) const { return attackersTo(c, sq).isAny(); }
    // 蛻ｩ縺阪・逕滓・

    // 莉ｻ諢上・ occupied 縺ｫ蟇ｾ縺吶ｋ蛻ｩ縺阪ｒ逕滓・縺吶ｋ縲・    template <PieceType PT> static Bitboard attacksFrom(const Color c, const Square sq, const Bitboard& occupied);
    // 莉ｻ諢上・ occupied 縺ｫ蟇ｾ縺吶ｋ蛻ｩ縺阪ｒ逕滓・縺吶ｋ縲・    template <PieceType PT> Bitboard attacksFrom(const Square sq, const Bitboard& occupied) const {
        static_assert(PT == Bishop || PT == Rook || PT == Horse || PT == Dragon, "");
        // Color 縺ｯ菴輔〒繧り憶縺・・        return attacksFrom<PT>(ColorNum, sq, occupied);
    }

    template <PieceType PT> Bitboard attacksFrom(const Color c, const Square sq) const {
        static_assert(PT == Gold, ""); // Gold 莉･螟悶・ template 迚ｹ谿雁喧縺吶ｋ縲・        return goldAttack(c, sq);
    }
    template <PieceType PT> Bitboard attacksFrom(const Square sq) const {
        static_assert(PT == Bishop || PT == Rook || PT == King || PT == Horse || PT == Dragon, "");
        // Color 縺ｯ菴輔〒繧り憶縺・・        return attacksFrom<PT>(ColorNum, sq);
    }
    Bitboard attacksFrom(const PieceType pt, const Color c, const Square sq) const { return attacksFrom(pt, c, sq, occupiedBB()); }
    static Bitboard attacksFrom(const PieceType pt, const Color c, const Square sq, const Bitboard& occupied);

    // 谺｡縺ｮ謇狗分
    Color turn() const { return turn_; }

    // pseudoLegal 縺ｨ縺ｯ
    // 繝ｻ邇峨′逶ｸ謇矩ｧ偵・蛻ｩ縺阪′縺ゅｋ蝣ｴ謇縺ｫ遘ｻ蜍輔☆繧・    // 繝ｻpin 縺ｮ鬧偵ｒ遘ｻ蜍輔＆縺帙ｋ
    // 繝ｻ騾｣邯夂視謇九・蜊・律謇九・謇九ｒ謖・☆
    // 縺薙ｌ繧峨・蜿榊援謇九ｒ蜷ｫ繧√◆謇九・莠九→螳夂ｾｩ縺吶ｋ縲・    // 繧医▲縺ｦ縲∵遠縺｡豁ｩ隧ｰ繧√ｄ莠梧ｭｩ縺ｮ謇九・ pseudoLegal 縺ｧ縺ｯ辟｡縺・・    template <bool MUSTNOTDROP, bool FROMMUSTNOTBEKING>
    bool pseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const;
    bool pseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const;
    template <bool Searching = true> bool moveIsPseudoLegal(const Move move) const;
#if !defined NDEBUG
    bool moveIsLegal(const Move move) const;
#endif

    void doMove(const Move move, StateInfo& newSt);
    void doMove(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck);
    void undoMove(const Move move);
    template <bool DO> void doNullMove(StateInfo& backUpSt);

    Score see(const Move move, const int asymmThreshold = 0) const;
    Score seeSign(const Move move) const;

    template <Color US> Move mateMoveIn1Ply();
    Move mateMoveIn1Ply();

    Ply gamePly() const         { return gamePly_; }

    Key getBoardKey() const     { return st_->boardKey; }
    Key getHandKey() const      { return st_->handKey; }
    Key getKey() const          { return st_->key(); }
    Key getExclusionKey() const { return st_->key() ^ zobExclusion_; }
    Key getKeyExcludeTurn() const {
        static_assert(zobTurn_ == 1, "");
        return getKey() >> 1;
    }
    void print() const;
    std::string toSFEN(const Ply ply) const;
    std::string toSFEN() const { return toSFEN(gamePly()); }

    HuffmanCodedPos toHuffmanCodedPos() const;

    s64 nodesSearched() const          { return nodes_; }
    void setNodesSearched(const s64 n) { nodes_ = n; }
    RepetitionType isDraw(const int checkMaxPly = std::numeric_limits<int>::max()) const;

    Thread* thisThread() const { return thisThread_; }

    void setStartPosPly(const Ply ply) { gamePly_ = ply; }

    static constexpr int nlist() { return EvalList::ListSize; }
    int list0(const int index) const { return evalList_.list0[index]; }
    int list1(const int index) const { return evalList_.list1[index]; }
    int squareHandToList(const Square sq) const { return evalList_.squareHandToList[sq]; }
    Square listToSquareHand(const int i) const { return evalList_.listToSquareHand[i]; }
    int* plist0() { return &evalList_.list0[0]; }
    int* plist1() { return &evalList_.list1[0]; }
    const int* cplist0() const { return &evalList_.list0[0]; }
    const int* cplist1() const { return &evalList_.list1[0]; }
    const ChangedLists& cl() const { return st_->cl; }

    const Searcher* csearcher() const { return searcher_; }
    Searcher* searcher() const { return searcher_; }
    void setSearcher(Searcher* s) { searcher_ = s; }

#if !defined NDEBUG
    // for debug
    bool isOK() const;
#endif

    static void initZobrist();

    static Score pieceScore(const Piece pc)            { return PieceScore[pc]; }
    // Piece 繧・index 縺ｨ縺励※繧ゅ・PieceType 繧・index 縺ｨ縺励※繧ゅ・    // 蜷後§蛟､縺悟叙蠕怜・譚･繧九ｈ縺・↓縺励※縺・ｋ縺ｮ縺ｧ縲￣ieceType => Piece 縺ｸ縺ｮ螟画鋤縺ｯ蠢・ｦ√↑縺・・    static Score pieceScore(const PieceType pt)        { return PieceScore[pt]; }
    static Score capturePieceScore(const Piece pc)     { return CapturePieceScore[pc]; }
    // Piece 繧・index 縺ｨ縺励※繧ゅ・PieceType 繧・index 縺ｨ縺励※繧ゅ・    // 蜷後§蛟､縺悟叙蠕怜・譚･繧九ｈ縺・↓縺励※縺・ｋ縺ｮ縺ｧ縲￣ieceType => Piece 縺ｸ縺ｮ螟画鋤縺ｯ蠢・ｦ√↑縺・・    static Score capturePieceScore(const PieceType pt) { return CapturePieceScore[pt]; }
    static Score promotePieceScore(const PieceType pt) {
        assert(pt < Gold);
        return PromotePieceScore[pt];
    }

private:
    void clear();
    void setPiece(const Piece piece, const Square sq) {
        const Color c = pieceToColor(piece);
        const PieceType pt = pieceToPieceType(piece);

        piece_[sq] = piece;

        byTypeBB_[pt].setBit(sq);
        byColorBB_[c].setBit(sq);
        byTypeBB_[Occupied].setBit(sq);
    }
    void setHand(const HandPiece hp, const Color c, const int num) { hand_[c].orEqual(num, hp); }
    void setHand(const Piece piece, const int num) {
        const Color c = pieceToColor(piece);
        const PieceType pt = pieceToPieceType(piece);
        const HandPiece hp = pieceTypeToHandPiece(pt);
        setHand(hp, c, num);
    }

    // 謇狗分蛛ｴ縺ｮ邇峨∈ check 縺励※縺・ｋ鬧偵ｒ蜈ｨ縺ｦ謗｢縺励※ checkersBB_ 縺ｫ繧ｻ繝・ヨ縺吶ｋ縲・    // 譛蠕後・謇九′菴輔°隕壹∴縺ｦ縺翫￠縺ｰ縲∥ttackersTo() 繧剃ｽｿ逕ｨ縺励↑縺上※繧り憶縺・・縺壹〒縲∝・逅・′霆ｽ縺上↑繧九・    void findCheckers() { st_->checkersBB = attackersToExceptKing(oppositeColor(turn()), kingSquare(turn())); }

    Score computeMaterial() const;

    void xorBBs(const PieceType pt, const Square sq, const Color c);
    // turn() 蛛ｴ縺・    // pin 縺輔ｌ縺ｦ(縺励※)縺・ｋ鬧偵・ Bitboard 繧定ｿ斐☆縲・    // BetweenIsUs == true  : 髢薙・鬧偵′閾ｪ鬧偵・    // BetweenIsUs == false : 髢薙・鬧偵′謨ｵ鬧偵・    template <bool FindPinned, bool BetweenIsUs> Bitboard hiddenCheckers() const {
        Bitboard result = allZeroBB();
        const Color us = turn();
        const Color them = oppositeColor(us);
        // pin 縺吶ｋ驕髫秘ｧ・        // 縺ｾ縺壹・閾ｪ鬧偵°謨ｵ鬧偵°縺ｧ螟ｧ髮第滑縺ｫ蛻､蛻･
        Bitboard pinners = bbOf(FindPinned ? them : us);

        const Square ksq = kingSquare(FindPinned ? us : them);

        // 髫懷ｮｳ迚ｩ縺檎┌縺代ｌ縺ｰ邇峨↓蛻ｰ驕泌・譚･繧矩ｧ偵・Bitboard縺縺第ｮ九☆縲・        pinners &= (bbOf(Lance) & lanceAttackToEdge((FindPinned ? us : them), ksq)) |
            (bbOf(Rook, Dragon) & rookAttackToEdge(ksq)) | (bbOf(Bishop, Horse) & bishopAttackToEdge(ksq));

        while (pinners) {
            const Square sq = pinners.firstOneFromSQ11();
            // pin 縺吶ｋ驕髫秘ｧ偵→邇峨・髢薙↓縺ゅｋ鬧偵・菴咲ｽｮ縺ｮ Bitboard
            const Bitboard between = betweenBB(sq, ksq) & occupiedBB();

            // pin 縺吶ｋ驕髫秘ｧ偵→邇峨・髢薙↓縺ゅｋ鬧偵′1縺､縺ｧ縲√°縺､縲∝ｼ墓焚縺ｮ濶ｲ縺ｮ縺ｨ縺阪√◎縺ｮ鬧偵・(繧・ pin 縺輔ｌ縺ｦ(縺励※)縺・ｋ縲・            if (between
                && between.isOneBit<false>()
                && between.andIsAny(bbOf(BetweenIsUs ? us : them)))
            {
                result |= between;
            }
        }

        return result;
    }

#if !defined NDEBUG
    int debugSetEvalList() const;
#endif
    void setEvalList() { evalList_.set(*this); }

    Key computeBoardKey() const;
    Key computeHandKey() const;
    Key computeKey() const { return computeBoardKey() + computeHandKey(); }

    void printHand(const Color c) const;

    static Key zobrist(const PieceType pt, const Square sq, const Color c) { return zobrist_[pt][sq][c]; }
    static Key zobTurn()                                                   { return zobTurn_; }
    static Key zobHand(const HandPiece hp, const Color c)                  { return zobHand_[hp][c]; }

    // byTypeBB 縺ｯ謨ｵ縲∝袖譁ｹ縺ｮ鬧偵ｒ蛹ｺ蛻･縺励↑縺・・    // byColorBB 縺ｯ鬧偵・遞ｮ鬘槭ｒ蛹ｺ蛻･縺励↑縺・・    Bitboard byTypeBB_[PieceTypeNum];
    Bitboard byColorBB_[ColorNum];
    Bitboard goldsBB_;

    // 蜷・・繧ｹ縺ｮ迥ｶ諷・    Piece piece_[SquareNum];
    Square kingSquare_[ColorNum];

    // 謇矩ｧ・    Hand hand_[ColorNum];
    Color turn_;

    EvalList evalList_;

    StateInfo startState_;
    StateInfo* st_;
    // 譎る俣邂｡逅・↓菴ｿ逕ｨ縺吶ｋ縲・    Ply gamePly_;
    Thread* thisThread_;
    s64 nodes_;

    Searcher* searcher_;

    static Key zobrist_[PieceTypeNum][SquareNum][ColorNum];
    static const Key zobTurn_ = 1;
    static Key zobHand_[HandPieceNum][ColorNum];
    static Key zobExclusion_; // todo: 縺薙ｌ縺悟ｿ・ｦ√°縲∬ｦ∵､懆ｨ・};

template <> inline Bitboard Position::attacksFrom<Lance >(const Color c, const Square sq, const Bitboard& occupied) { return  lanceAttack(c, sq, occupied); }
template <> inline Bitboard Position::attacksFrom<Bishop>(const Color  , const Square sq, const Bitboard& occupied) { return bishopAttack(   sq, occupied); }
template <> inline Bitboard Position::attacksFrom<Rook  >(const Color  , const Square sq, const Bitboard& occupied) { return   rookAttack(   sq, occupied); }
template <> inline Bitboard Position::attacksFrom<Horse >(const Color  , const Square sq, const Bitboard& occupied) { return  horseAttack(   sq, occupied); }
template <> inline Bitboard Position::attacksFrom<Dragon>(const Color  , const Square sq, const Bitboard& occupied) { return dragonAttack(   sq, occupied); }

template <> inline Bitboard Position::attacksFrom<Pawn  >(const Color c, const Square sq) const { return   pawnAttack(c, sq              ); }
template <> inline Bitboard Position::attacksFrom<Lance >(const Color c, const Square sq) const { return  lanceAttack(c, sq, occupiedBB()); }
template <> inline Bitboard Position::attacksFrom<Knight>(const Color c, const Square sq) const { return knightAttack(c, sq              ); }
template <> inline Bitboard Position::attacksFrom<Silver>(const Color c, const Square sq) const { return silverAttack(c, sq              ); }
template <> inline Bitboard Position::attacksFrom<Bishop>(const Color  , const Square sq) const { return bishopAttack(   sq, occupiedBB()); }
template <> inline Bitboard Position::attacksFrom<Rook  >(const Color  , const Square sq) const { return   rookAttack(   sq, occupiedBB()); }
template <> inline Bitboard Position::attacksFrom<King  >(const Color  , const Square sq) const { return   kingAttack(   sq              ); }
template <> inline Bitboard Position::attacksFrom<Horse >(const Color  , const Square sq) const { return  horseAttack(   sq, occupiedBB()); }
template <> inline Bitboard Position::attacksFrom<Dragon>(const Color  , const Square sq) const { return dragonAttack(   sq, occupiedBB()); }

// position sfen R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b PLNSGBR17p3n3g 1
// 縺ｮ螻髱｢縺梧怙螟ｧ蜷域ｳ墓焔螻髱｢縺ｧ 593 謇九ら分蜈ｵ縺ｮ蛻・・ 1 縺励※縺翫￥縲・const int MaxLegalMoves = 593 + 1;

class CharToPieceUSI : public std::map<char, Piece> {
public:
    CharToPieceUSI() {
        (*this)['P'] = BPawn;   (*this)['p'] = WPawn;
        (*this)['L'] = BLance;  (*this)['l'] = WLance;
        (*this)['N'] = BKnight; (*this)['n'] = WKnight;
        (*this)['S'] = BSilver; (*this)['s'] = WSilver;
        (*this)['B'] = BBishop; (*this)['b'] = WBishop;
        (*this)['R'] = BRook;   (*this)['r'] = WRook;
        (*this)['G'] = BGold;   (*this)['g'] = WGold;
        (*this)['K'] = BKing;   (*this)['k'] = WKing;
    }
    Piece value(char c) const      { return this->find(c)->second; }
    bool isLegalChar(char c) const { return (this->find(c) != this->end()); }
};
extern const CharToPieceUSI g_charToPieceUSI;

#endif // #ifndef APERY_POSITION_HPP
