import shogi
import shogi.KIF
import numpy as np
import hcp_decoder
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('kif_dir')
parser.add_argument('hcp_file')
parser.add_argument('--turn', default=85, type=int)
args = parser.parse_args()

HuffmanCodedPos = np.dtype([
    ('hcp', np.uint8, 32),
    ])

def fild_all_files(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            yield os.path.join(root, file)

hcp = np.empty(100000000, dtype=HuffmanCodedPos)
cnt = 0

for file in fild_all_files(args.kif_dir):
    try:
        kif = shogi.KIF.Parser.parse_file(file)[0]
    except:
        print(file)
        continue

    board = shogi.Board(kif['sfen'])
    for move in kif['moves']:
        if board.move_number > args.turn:
            break
        board.push_usi(move)

        hcp_decoder.sfen_to_hcp(board.sfen(), hcp[cnt:cnt+1])
        cnt += 1

print(cnt)
hcp_uniq = np.unique(hcp[:cnt])
print(len(hcp_uniq))
hcp_uniq.tofile(args.hcp_file)