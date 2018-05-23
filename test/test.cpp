#include "position.hpp"
#include "init.hpp"
#include "generateMoves.hpp"

#include <iostream>

using namespace std;

#if 0
// 王手生成テスト
int main() {
	initTable();
	Position pos;
	pos.set("1n3g3/7B1/ppSPp1ppp/7P1/R1p1kp1R1/2Pp5/PP3GP1P/1BS4S1/L1NG1KN1L b GSN2LPp 1", nullptr); // 近接王手

	// 王手生成
	for (MoveList<NeighborCheck> ml(pos); !ml.end(); ++ml) {
		std::cout << ml.move().toUSI() << std::endl;
	}

	return 0;
}
#endif

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