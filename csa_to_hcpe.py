import numpy as np
from cshogi import *
import glob
import os.path

MAX_MOVE_COUNT = 512

# process csa
def process_csa(f, csa_file_list, filter_moves, filter_rating):
    board = Board()
    parser = Parser()
    num_games = 0
    num_positions = 0
    hcpes = np.zeros(MAX_MOVE_COUNT, dtype=HuffmanCodedPosAndEval)
    for filepath in csa_file_list:
        parser.parse_csa_file(filepath)
        if parser.endgame not in ('%TORYO', '%SENNICHITE', '%KACHI', '%HIKIWAKE') or len(parser.moves) < filter_moves:
            continue
        if filter_rating > 0 and (parser.ratings[0] < filter_rating or parser.ratings[1] < filter_rating):
            continue
        board.set_sfen(parser.sfen)
        assert board.is_ok(), "{}:{}".format(filepath, parser.sfen)
        # gameResult
        try:
            for i, (move, score) in enumerate(zip(parser.moves, parser.scores)):
                assert board.is_legal(move)

                # hcp
                board.to_hcp(hcpes[i]['hcp'])
                # eval
                assert abs(score) <= 100000
                score = min(32767, max(score, -32767))
                hcpes[i]['eval'] = score if board.turn == BLACK else -score
                # move
                hcpes[i]['bestMove16'] = move16(move)
                # result
                hcpes[i]['gameResult'] = parser.win

                board.push(move)
        except:
            print("skip {}:{}:{}:{}".format(filepath, i, move_to_usi(move), score))
            continue

        # write data
        hcpes[0:i+1].tofile(f)

        num_positions += i + 1
        num_games += 1

    return num_games, num_positions

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('csa_dir', help='directory stored CSA file')
    parser.add_argument('hcpe', help='hcpe file')
    parser.add_argument('--filter_moves', type=int, default=50, help='filter by move count')
    parser.add_argument('--filter_rating', type=int, default=3000, help='filter by rating')
    parser.add_argument('--recursive', '-r', action='store_true')

    args = parser.parse_args()

    if args.recursive:
        dir = os.path.join(args.csa_dir, '**')
    else:
        dir = args.csa_dir
    csa_file_list = glob.glob(os.path.join(dir, '*.csa'), recursive=args.recursive)

    with open(args.hcpe, 'wb') as f:
        num_games, num_positions = process_csa(f, csa_file_list, args.filter_moves, args.filter_rating)
        print(f"games : {num_games}")
        print(f"positions : {num_positions}")
