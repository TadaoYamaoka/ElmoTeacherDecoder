#include "init.hpp"
#include "position.hpp"
#include "move.hpp"

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

	pos.set("1n1g3+Pl/k1p1s4/1ng5p/pSP1p1pp1/1n3p3/P1K3P1P/1P7/9/L1G5L b 2R2BG2SL5Pn 161", nullptr); // mate 15
	//pos.set("lns3kn1/1r4g2/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b G2Pl4p 53", nullptr); // mate 1
	//pos.set("lns3kn1/1r3g3/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b G3Pl3p 51", nullptr); // mate 3
	//pos.set("lns3kn1/1r7/4pgs+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l b BG3Pl3p 49", nullptr); // •s‹l‚Ý
	//pos.set("lns4n1/1r3k3/4pgsg1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG2R1/LN1K3N+l b B3Pl3p 47", nullptr); // •s‹l‚Ý
	//pos.set("7nl/5Psk1/1+P1+P1p1pp/K3g4/6p1B/1SP4P1/PsS3P1P/1N7/+r6NL w GLrb2gnl6p 1", nullptr); // •s‹l‚Ý

	auto start = std::chrono::system_clock::now();
	bool ret = dfpn(pos);
	auto end = std::chrono::system_clock::now();

	auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	cout << ret << endl;
	cout << time_ms << "ms" << endl;
}

void test2()
{
	Position pos;

	// Ž©‹Ê‚Ì‹l‚Ý
	pos.set("lns3kn1/1r3gP2/3Bp1s+R1/2pp1p3/pp2P4/2P1SP3/PPSP5/2GBG4/LN1K3N+l w G2Pl3p 52", nullptr); // mate 2
	//pos.set("lns1S2nl/1r3kg2/4pgsp1/2pp1p2P/pp2P1b2/2P2P1P1/PPSP5/2GBG2R1/LN1K3NL w 2Pp 1", nullptr); // •s‹l‚Ý
	auto start = std::chrono::system_clock::now();
	bool ret = dfpn_andnode(pos);
	auto end = std::chrono::system_clock::now();

	auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	cout << ret << endl;
	cout << time_ms << "ms" << endl;
}

int main(int argc, char *argv[])
{
	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();
	dfpn_init();

	//test1();
	//test2();
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
		//cout << pos.toSFEN() << endl;
		bool mated = false;
		if (pos.inCheck()) {
			mated = dfpn_andnode(pos);
		}
		else {
			mated = dfpn(pos);
		}

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