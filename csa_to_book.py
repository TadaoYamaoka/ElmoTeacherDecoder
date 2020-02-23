import numpy as np
from cshogi import *
import glob
import os.path
from collections import defaultdict

# process csa
def process_csa(f, csa_file_list, limit_moves, filter_rating, min_counts):
    board = Board()
    parser = Parser()
    num_games = 0
    num_positions = 0
    book = {}

    for filepath in csa_file_list:
        parser.parse_csa_file(filepath)
        if filter_rating > 0 and (parser.ratings[0] < filter_rating or parser.ratings[1] < filter_rating):
            continue
        board.set_sfen(parser.sfen)
        assert board.is_ok(), "{}:{}".format(filepath, parser.sfen)
        for i, move in enumerate(parser.moves):
            if board.move_number > limit_moves:
                break
            if not board.is_legal(move):
                print("skip {}:{}:{}".format(filepath, i, move_to_usi(move)))
                break

            key = board.book_key()
            if key not in book:
                book[key] = defaultdict(int)
            entries = book[key]

            entries[move16(move)] += 1

            board.push(move)

        num_games += 1

    # output
    book_entries = np.empty(1000000, BookEntry)
    i = 0
    for key, entries in sorted(book.items()):
        i_tmp = i
        sum = 0
        for fromToPro, count in sorted(entries.items(), key=lambda x: -x[1]):
            book_entries[i] = key, fromToPro, count, 0
            i += 1
            sum += count
        if sum < min_counts:
            i = i_tmp
        else:
            num_positions += 1
    book_entries[:i].tofile(f)

    return num_games, num_positions, i

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('csa_dir', help='directory stored CSA file')
    parser.add_argument('book', help='book file')
    parser.add_argument('--limit_moves', type=int, default=80)
    parser.add_argument('--filter_rating', type=int, default=3000)
    parser.add_argument('--min_counts', type=int, default=50)
    parser.add_argument('-r', '--recursive', '-r', action='store_true')

    args = parser.parse_args()

    if args.recursive:
        dir = os.path.join(args.csa_dir, '**')
    else:
        dir = args.csa_dir
    csa_file_list = glob.glob(os.path.join(dir, '*.csa'), recursive=args.recursive)

    with open(args.book, 'wb') as f:
        num_games, num_positions, num_entries = process_csa(f, csa_file_list, args.limit_moves, args.filter_rating, args.min_counts)
        print(f"games : {num_games}")
        print(f"positions : {num_positions}")
        print(f"entries : {num_entries}")
