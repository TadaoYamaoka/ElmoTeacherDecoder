#include "init.hpp"
#include "position.hpp"
#include "move.hpp"

#include "mate.h"

#include <iostream>

using namespace std;

void dfpn_init();
bool dfpn(Position& r);
bool dfpn_andnode(Position& r);

void test1()
{
	Position pos;

	/*pos.set("lns3kn1/1r3g3/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b G3Pl3p 51", nullptr);
	Move move = makeCaptureMove(PieceType::Dragon, SQ23, SQ21, pos);
	cout << move.toUSI() << ":" << pos.getKeyAfter(move) << endl;

	StateInfo state_info;
	pos.doMove(move, state_info);
	cout << move.toUSI() << ":" << pos.getKey() << endl;
	pos.undoMove(move);*/

	vector<string> sfens = {
		"1n1g3+Pl/k1p1s4/1ng5p/pSP1p1pp1/1n3p3/P1K3P1P/1P7/9/L1G5L b 2R2BG2SL5Pn 161", // mate 15
		"ln6K/9/1sp2+P3/pp4G1p/6P2/+rl+B+R5/k8/+b8/9 b 2G2SNL2Pgs2nl10p 1", // mate 15
		"ln1s+R3K/2s6/p1pp1p3/kp+r4pp/N3p4/1Sg6/P2B2P1P/5g3/LL3g1NL b BGS2Pn5p 1", // mate 17
		"n1gg2+R2/l2l5/p1k1p3K/1ppp5/3PBp3/9/P4P2P/g8/8L b RG2S2NL6Pb2sn2p 1", // mate 33
		"1+Rp4nl/3+b1+Rss1/7k1/p3p1p1p/1PBp1p1PP/4P1n1K/3S1PS2/4G2g1/5G1NL b NL4Pgl2p 1", // mate 37
		"l1+R5K/4p2+P1/1pp6/p2+b4p/2k2G1p1/PL2+n3P/N5+s2/2+nP5/L4+R2L b B2G2S3Pgsn5p 1", // mate 13
		"ln1gl1R2/ks1gn1R2/pp2pp2K/1PB3+P2/5P3/9/P7P/8p/+n4S2L b B2G2SNL3P5p 1", // mate 13
		"7n1/+R5glK/n4s1p1/p4pkl1/2s3n2/P3PP2P/1P+pG5/2P6/9 b R2B2G2S2L8Pn 1", // mate 13
		"ln6K/1ksPg1sG1/5s2p/ppp3p2/9/P1P3P2/4+p3P/1+r7/LN3L2L w RBSb2g2n7p 1", // mate 19
		"l6n1/b+R4glK/n4s1p1/p5kl1/2sp2n2/P1p1P3P/1P3P3/2P1G4/2+b6 b R2G2SNL7Pp 1", // mate 15
		"7+L1/1+B1nkg2+R/3p2s1K/1pp1g1p2/1n1P5/lSP4P1/1P7/1G7/1+l7 b RBG2S2NL9Pp 1", // mate 11
		"ln6K/9/1sp2+P3/pp4G1p/6P2/+r8/9/9/k+l1+R5 b B2G2SNLPbgs2nl11p 1", // mate 13
		"lnS1r4/GGs5K/2k5p/pppp5/9/PLPP5/1P+n1PS2P/4G4/g1+r3+p2 b BS2N2L6Pb 1", // mate 15
		"7s1/k2g5/n1p1p1P+LK/s2P2n2/p5pP1/2P1P4/5+p3/8L/L3+b1sN1 w NL4P2rb3gs4p 1", // mate 13
		"l7K/9/p6sp/1g1ppRpbk/7n1/1P2SP2P/P3P1P2/2+n1s2+p1/LN2+r2NL b B3GSL6P 1", // mate 9
		"1n3G1nK/5+r1R1/p2L+P3p/2p2Gp2/9/3P2B2/P1P5+n/5S1p1/L1S1L1Ggk b 2SNL6Pb3p 1", // mate 7
		"ln3+P1+PK/1r1k3+B1/3p1+L1+S1/p1p2p1+B1/3+r3s1/7s1/4p1+n+pp/+p3+n2p+p/1+p3+p+p+p+p b 2GN2L2gsp 1", // mate 15
		"lR6K/4p2+P1/1p7/p7p/2k1+b2p1/Pg1n+n3P/N5+s2/1L1P5/L4+R2L b B2G2S3Pgsn6p 1", // mate 15
		"lnp1+RS2K/1k5+P1/1pgps3p/4p4/6+Rp1/3+n5/+pP2n1P2/2+b1P4/8+p b B2G2SN2L2Pgl4p 1", // mate 23
		"lns3kn1/1r4g2/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b G2Pl4p 53", // mate 1
		"lns3kn1/1r3g3/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b G3Pl3p 51", // mate 3
		"1n+S1l3n/2s6/1pp3pg1/3p2s2/1kP4PK/4p1n1P/+l1G2+B3/1+l1G1+R3/5P3 b RL6Pbgsn3p 1", // mate 3
		"knS5+P/1g7/Pgp+B4+P/l1n1pp1+B1/7L1/pp2PPPPK/3P2+pR1/5g3/LR5S1 b GS2Ps2nl2p 1", // mate 15
		"+B+R5n1/5gk2/p1pps1gp1/4ppnsK/6pP1/1PPSP3L/PR1P1PP2/6S2/L2G1G3 w B2N2LP2p 1", // mate 3(èzä¬)
		"lnl5l/2b6/ppk6/3p1p2p/Ps2p1bP1/1NP3g1K/LP6P/9/1N6+p b R3G2SN2Prs4p 1", // mate 25
		"knS5K/llGp3G1/pp2+R2S1/2n6/9/6S2/P3+n3P/2P2P2L/6GNb b RBGSL11P 1", // mate 5
		"ln5+LK/1rk1+B2S1/p1sp5/4p1pp1/1PPP1P3/2S1P3+l/P1B2S3/1R2G2+p1/LN3G3 b 2GN5Pnp", // mate 13
		"1p+Bnn+R2K/3+R5/4ps1pp/3p2p2/1NG1s4/6kPP/P2PP4/3G1+s1G1/L8 b BSN3L6Pgp 1", // mate 11
		"lns3kn1/1r7/4pgs+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b BG3Pl3p 49", // ïsãlÇ›
		"lns4n1/1r3k3/4pgsg1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG2R1/LN1K3N+l b B3Pl3p 47", // ïsãlÇ›
		"7nl/5Psk1/1+P1+P1p1pp/K3g4/6p1B/1SP4P1/PsS3P1P/1N7/+r6NL w GLrb2gnl6p 1", // ïsãlÇ›
		"ln3+P1+PK/1rk4+B1/3p1+L1+S1/p1p2p1+B1/3+r3s1/7s1/4p1+n+pp/+p3+n2p+p/1+p3+p+p+p+p b 2GN2L2gsp 1", // ïsãlÇ›
		"l2+S1p2K/1B4G2/p4+N1p1/3+B3sk/5P1s1/P1G3p1p/2P1Pr1+n1/9/LNS5L b R2GL8Pnp 1", // ïsãlÇ›
		"+B2B1n2K/7+R1/p2p1p1ps/3g2+r1k/1p3n3/4n1P+s1/PP7/1S6p/L7L b 3GS7Pn2l2p 1", // ïsãlÇ›
		"l6GK/2p2+R1P1/p1nsp2+Sp/1p1p2s2/2+R2bk2/3P4P/P4+p1g1/2s6/L7L b B2GNL2n7p 1", // ïsãlÇ›
		"1n3G1nK/2+r2P3/p3+P1n1p/2p2Gp2/5l3/3P5/P1P3S2/6+Bpg/L1S1L3k b R2SNL5Pbg3p 1", // ïsãlÇ›
		"+B2B1n2K/7+R1/p2p1p1ps/3g2+r1k/1p3n3/4n1P+s1/PP7/1S7/L8 b 3GSL7Pn2l3p 1", // ïsãlÇ›
	};

	for (string sfen : sfens) {
		pos.set(sfen, nullptr);
		auto start = std::chrono::system_clock::now();
		bool ret = dfpn(pos);
		//bool ret = mateMoveInOddPly(pos, 11);
		auto end = std::chrono::system_clock::now();

		auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		extern int64_t searchedNode;
		cout << searchedNode << "\t";
		cout << ret << "\t";
		cout << time_ms << endl;
	}
}

void test2()
{
	Position pos;

	// é©ã ÇÃãlÇ›
	//pos.set("lns3kn1/1r3gP2/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l w G2Pl3p 52", nullptr); // mate 2
	//pos.set("l2Rp1k1l/6g2/2n+R1p1p1/p5P2/2+bpPP3/6G1K/PPPPsS1P1/9/LN2b3L b GS2NPgs4p 1", nullptr); // mate 10
	//pos.set("ln6K/1ksPg1s+bG/5s2p/ppp3p2/9/P1P3P2/4+p3P/1+r7/LN3L2L b RSb2g2n7p 1", nullptr); // mate 20
	//pos.set("l+R3g3/3+P1g2k/2+Bppp1+Bn/p1s2sspK/9/P1P+RS1pPg/3P2+n2/2+n2P+n2/L6L1 w GL5Pp 1", nullptr); // ïsãlÇ›Åiå„éËmate 1Åj
	//pos.set("l3B3k/1r7/p1n2s1G1/1p1n5/7pK/P1P2pSgr/9/6G2/L2+s5 b GSNLbnl12p 1", nullptr); // mate 10
	pos.set("l6S+P/3k5/p2p3pp/1SN1p2sn/1p7/2nBG1P1K/PP1PP3P/6l+r1/+b2G4L w RG4Pgsnlp 1", nullptr); // mate 8
	//pos.set("l1g4+RK/1k+P3p1p/1p1s3g1/p2pp1+B2/1N7/P2sP1g1P/1P7/2+r1s4/1N6L w BGS2N2L4P3p 1", nullptr); // mate 16
	//pos.set("2+R5l/6gk1/2s1p2+Pn/2pp1pP1K/6n2/1P1P4P/4Ps3/4R4/8L w 2B2G2S2N8Pg2l 1", nullptr); // ïsãlÇ›
	//pos.set("lns1S2nl/1r3kg2/4pgsp1/2pp1p2P/pp2P1b2/2P2P1P1/PPSP5/2GBG2R1/LN1K3NL w 2Pp 1", nullptr); // ïsãlÇ›

	auto start = std::chrono::system_clock::now();
	bool ret = dfpn_andnode(pos);
	//bool ret = mateMoveInEvenPly(pos, 8);
	auto end = std::chrono::system_clock::now();

	auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	extern int64_t searchedNode;
	cout << searchedNode << endl;
	cout << ret << endl;
	cout << time_ms << "ms" << endl;
}

void test3()
{
	Position pos;

	//pos.set("lns3kn1/1r3g3/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b G3Pl3p 51", nullptr);
	//Move move = makeCaptureMove(PieceType::Dragon, SQ23, SQ21, pos);
	pos.set("+B+R5n1/5gk2/p1pps1gp1/4ppnsK/6pP1/1PPSP3L/PR1P1PP2/6S2/L2G1G3 w B2N2LP2p 1", nullptr);
	Move move = makeCaptureMove(PieceType::Silver, SQ24, SQ25, pos);
	cout << move.toUSI() << ":" << pos.getBoardKeyAfter(move) << "," << pos.hand(oppositeColor(pos.turn())).value() << endl;

	StateInfo state_info;
	pos.doMove(move, state_info);
	cout << move.toUSI() << ":" << pos.getBoardKey() << "," << pos.hand(pos.turn()).value() << endl;
	cout << pos.toSFEN() << endl;
	pos.undoMove(move);
}

int main(int argc, char *argv[])
{
	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();
	dfpn_init();

	//test1();
	//test2();
	//test3();
	//return 0;

	if (argc < 3) {
		std::cout << "extract_mated_hcp input_hcp not_mated_hcp mated_hcp" << std::endl;
		return 0;
	}

	char* hcpFile = argv[1];
	char* notMatedHcp = argv[2];
	char* matedHcp = argv[3];

	std::ifstream ifs(hcpFile, std::ifstream::in | std::ifstream::binary | std::ios::ate);
	if (!ifs) {
		std::cerr << "Error: cannot open " << hcpFile << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNum = ifs.tellg() / sizeof(HuffmanCodedPos);
	ifs.seekg(0);
	std::cout << "input num = " << entryNum << std::endl;

	const s64 num_per_file = 1024 * 1024 * 1024 / sizeof(HuffmanCodedPos); // 1GB
	HuffmanCodedPos *inhcpvec = new HuffmanCodedPos[num_per_file];

	std::ofstream ofs(notMatedHcp, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << notMatedHcp << std::endl;
		exit(EXIT_FAILURE);
	}

	std::ofstream ofsMated(matedHcp, std::ios::binary);
	if (!ofsMated) {
		std::cerr << "Error: cannot open " << matedHcp << std::endl;
		exit(EXIT_FAILURE);
	}

	auto start_time = std::chrono::system_clock::now();

	Position pos;
	s64 outNum = 0;
	s64 outNumMated = 0;
	s64 d = 0;
	for (s64 i = 0; i < entryNum; i++, d++) {
		if (i % num_per_file == 0) {
			s64 num = num_per_file;
			if (i >= num_per_file * (entryNum / num_per_file))
				num = entryNum - num_per_file * (entryNum / num_per_file);
			ifs.read(reinterpret_cast<char*>(inhcpvec), sizeof(HuffmanCodedPos) * num_per_file);
			d = 0;
		}
		HuffmanCodedPos& hcp = inhcpvec[d];
		pos.set(hcp, nullptr);

		cout << pos.toSFEN() << endl;
		auto start = std::chrono::system_clock::now();
		bool mated = false;
		if (pos.inCheck()) {
			mated = dfpn_andnode(pos);
		}
		else {
			mated = dfpn(pos);
		}
		auto end = std::chrono::system_clock::now();
		cout << mated << " " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << endl;

		if (mated) {
			ofsMated.write(reinterpret_cast<char*>(&hcp), sizeof(HuffmanCodedPos));
			outNumMated++;
		}
		else {
			ofs.write(reinterpret_cast<char*>(&hcp), sizeof(HuffmanCodedPos));
			outNum++;
		}

		if (i % 1000 == 1000 - 1) {
			auto end_time = std::chrono::system_clock::now();
			auto time_sec = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
			std::cout << "nomate=" << outNum << ", mate=" << outNumMated << ", elapse=" << time_sec << "[sec]" << std::endl;
		}
	}

	std::cout << "output not mated num = " << outNum << std::endl;
	std::cout << "output mated num     = " << outNumMated << std::endl;
}