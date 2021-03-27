#include "position.hpp"
#include "init.hpp"
#include "move.hpp"
#include "search.hpp"
#include <string>
#include <iostream>
#include <filesystem>

constexpr int max_moves = 310;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "hcpe_to_hcpe2 hcpeFile hcpe2File" << std::endl;
		return 0;
	}

	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();

	char* hcpeFile = argv[1];
	char* hcpe2File = argv[2];

	std::ifstream ifs(hcpeFile, std::ifstream::in | std::ifstream::binary);
	if (!ifs) {
		std::cerr << "Error: cannot open " << hcpeFile << std::endl;
		exit(EXIT_FAILURE);
	}
	const s64 entryNum = std::filesystem::file_size(hcpeFile) / sizeof(HuffmanCodedPosAndEval);
	std::cout << "input num = " << entryNum << std::endl;

	auto hcpe = new HuffmanCodedPosAndEval[entryNum];
	ifs.read(reinterpret_cast<char*>(hcpe), sizeof(HuffmanCodedPosAndEval) * entryNum);

	std::ofstream ofs(hcpe2File, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: cannot open " << hcpe2File << std::endl;
		exit(EXIT_FAILURE);
	}

	s64 outNum = 0;
	Position pos;
	pos.set(hcpe[0].hcp, nullptr);
	Bitboard pre_bb = pos.occupiedBB();
	s64 start_p = 0;
	GameResult pre_result = hcpe[0].gameResult;
	//bool is_maxmove = false;
	//s64 draw_cnt = 0;
	for (s64 i = 0; i < entryNum; i++) {
		pos.set(hcpe[i].hcp, nullptr);
		auto bb = pos.occupiedBB();
		const auto diff_cnt = (pre_bb ^ bb).popCount();
		if (pre_result != hcpe[i].gameResult || diff_cnt > 7 || i + 1 == entryNum) {
			//std::cout << i << "\t" << diff_cnt << "\t" << i - start_p << "\t" << (int)hcpe[i].gameResult << std::endl;

			bool is_out = false;

			//if (is_maxmove) {
			//	// o—Í‚µ‚È‚¢
			//	is_out = false;
			//	for (s64 j = start_p; j < i; ++j) {
			//		(int8_t&)hcpe[j].gameResult |= 16; // max_move
			//	}
			//}

			if (pre_result == Draw) {
				//std::cout << i << "\t" << i - start_p << std::endl;
				//if (i - start_p >= 309 || hcpe[i - 1].eval == 30000 || hcpe[i - 1].eval == -30000) {
				//	for (s64 j = i - 16; j < i; ++j) {
				//		pos.set(hcpe[j].hcp, nullptr);
				//		auto move = move16toMove(Move(hcpe[j].bestMove16), pos);
				//		std::cout << pos.toSFEN() << "\t" << move.toUSI() << "\t" << hcpe[j].eval << "\t" << (int)hcpe[j].gameResult << std::endl;
				//	}
				//	Position pos_copy = pos;
				//	auto move = move16toMove(Move(hcpe[i - 1].bestMove16), pos);
				//	StateInfo st;
				//	pos_copy.doMove(move, st);
				//	std::cout << pos_copy.toSFEN() << "\t" << hcpe[i - 1].eval << "\t" << (int)hcpe[i - 1].gameResult << "\t" << i - start_p << std::endl;
				//}

				//if (hcpe[i - 1].eval == 0) {
					// ç“úŽè
					for (s64 j = start_p; j < i; ++j) {
						(int8_t&)hcpe[j].gameResult |= 4; // sennichite
					}
					is_out = true;
				//}
			}
			else {
				is_out = true;

				// “ü‹ÊéŒ¾Ÿ‚¿
				Position pos2;
				pos2.set(hcpe[i - 1].hcp, nullptr);
				auto move = move16toMove(Move(hcpe[i - 1].bestMove16), pos2);
				StateInfo st;
				pos2.doMove(move, st);
				if (nyugyoku(pos2)) {
					//std::cout << pos2.toSFEN() << "\t" << hcpe[i - 1].eval << "\t" << (int)hcpe[i - 1].gameResult << "\t" << i - start_p << std::endl;
					for (s64 j = start_p; j < i; ++j) {
						(int8_t&)hcpe[j].gameResult |= 8; // nyugyoku
					}
				}
			}

			if (is_out) {
				const auto num = i - start_p;
				ofs.write(reinterpret_cast<char*>(hcpe + start_p), sizeof(HuffmanCodedPosAndEval) * num);
				outNum += num;
			}

			start_p = i;
			//draw_cnt = 0;
			//is_maxmove = false;
		}
	
		//if (hcpe[i].gameResult == Draw) {
		//	++draw_cnt;
		//	if (draw_cnt >= max_moves)
		//		is_maxmove = true;
		//}
		//else {
		//	draw_cnt = 0;
		//}

		pre_bb = bb;
		pre_result = hcpe[i].gameResult;
	}

	std::cout << "output num = " << outNum << std::endl;

	return 0;
}