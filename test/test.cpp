#include "position.hpp"
#include "init.hpp"
#include "generateMoves.hpp"

#include <iostream>

using namespace std;

#if 1
// 王手生成テスト
int main() {
	initTable();
	Position pos;
	//pos.set("l2+S1p2K/1B4G2/p4+N1pG/7sR/5P1s1/P1G2Gp1p/2P1Prk+n1/6N2/LNS5L b L8Pbp 1", nullptr); // 開き王手 金
	//pos.set("l2+S1p2K/1B4G2/p4+N1pG/7sR/4SP1s1/P1G3p1p/2P1Prk+n1/6N2/LNG5L b L8Pbp 1", nullptr); // 開き王手 銀
	//pos.set("lnsgkgsnl/1r1L3b1/pp1pppppp/2p6/B8/9/PPPPPPPPP/7R1/LNSGKGSN1 b - 1", nullptr); // 開き王手 香車
	//pos.set("lnsgk1snl/4g2b1/pp+Rp1pppp/2p2p3/B8/9/PPPPPPPPP/7R1/LNSGKGSNL b - 1", nullptr); // 開き王手 竜
	//pos.set("ln5+LK/1r1G+B2S1/pksp5/4p1pp1/1PPP1P3/2S1P3+l/P1B2S3/1R2G2+p1/LN3G3 b GN5Pnp 1", nullptr); // 開き王手 歩
	//pos.set("2S1G4/9/R1S1k1S1R/9/2G3G2/3L1L3/B1N3N1B/4K4/4L4 b GS2NL18P 1", nullptr); // 最大数
	//pos.set("9/R1S1k1S1R/2+P3G2/2G3G2/9/B1NL1LN1B/9/4K4/4L4 b G2S2NL17P 1", nullptr); // 最大数 65
	//pos.set("5S1S1/RS5k1/5G3/9/5NL1L/9/9/1K7/B8 b RB3GS3N2L18P 1", nullptr); // 最大数 67
	//pos.set("B7B/1R7/6R2/9/4k4/9/9/9/K1N6 b 4G4S3N4L18P 1", nullptr); // 最大数 83 成り含む
	//pos.set("4S4/R1S3k1S/4+P3+P/9/8N/4N3N/1L7/B8/5L1LK b RB4GSNL16P 1", nullptr); // 最大数 74(73香の2段目不成を除く)
	pos.set("lnsgkgsnl/1r5b1/ppppNp1pp/6pN1/4R3B/9/PPPPPPPPP/9/L1SGKGS1L b p 1", nullptr); // 1，2段目に移動する桂馬で開き王手

	// 王手生成
	int cnt = 0;
	for (MoveList<Check> ml(pos); !ml.end(); ++ml) {
		std::cout << ml.move().toUSI() << std::endl;
		cnt++;
	}
	std::cout << cnt << std::endl;

	// 検証
	cnt = 0;
	StateInfo si;
	for (MoveList<Legal> ml(pos); !ml.end(); ++ml) {
		pos.doMove(ml.move(), si);
		if (pos.inCheck()) {
			std::cout << ml.move().toUSI() << std::endl;
			cnt++;
		}
		pos.undoMove(ml.move());
	}
	std::cout << cnt << std::endl;

	return 0;
}
#endif

#if 0
// 近接王手生成テスト
int main() {
	initTable();
	Position pos;
	//pos.set("1n3g3/7B1/ppSPp1ppp/7P1/R1p1kp1R1/2Pp5/PP3GP1P/1BS4S1/L1NG1KN1L b GSN2LPp 1", nullptr); // 近接王手
	//pos.set("ln2g3l/2+Rskg3/p2sppL2/2pp1sP1p/2P2n3/B2P1N1p1/P1NKPP2P/1G1S1+p1P1/7+rL b B2Pg 98", nullptr); // 近接王手
	pos.set("4k4/4p4/9/9/9/9/9/9/1K2L4 b 2r2b4g4s4n3l17p 1", nullptr); // 近接王手 香車の2段目不成

	// 王手生成
	for (MoveList<NeighborCheck> ml(pos); !ml.end(); ++ml) {
		std::cout << ml.move().toUSI() << std::endl;
	}

	return 0;
}
#endif

#if 0
// 3手詰めテスト
int main() {
	initTable();
	Position pos;
	pos.set("lns3kn1/1r3g3/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b G3Pl3p 51", nullptr); // mate 3
	//pos.set("lns3kn1/1r4g2/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b G2Pl4p 53", nullptr); // mate 1
	//pos.set("1n1g3+Pl/k1p1s4/1ng5p/pSP1p1pp1/1n3p3/P1K3P1P/1P7/9/L1G5L b 2R2BG2SL5Pn 161", nullptr); // mate 15

	Move move = pos.mateMoveIn3Ply();

	cout << move.toUSI() << endl;

	return 0;
}
#endif