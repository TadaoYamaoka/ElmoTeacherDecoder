import shogi
import shogi.CSA
import numpy as np
import hcp_decoder
import os
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('csa_dir')
parser.add_argument('hcp_file')
parser.add_argument('--turn', default=150, type=int)
parser.add_argument('--eval', default=1000, type=int)
args = parser.parse_args()

HuffmanCodedPos = np.dtype([
    ('hcp', np.uint8, 32),
    ])

def fild_all_files(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            yield os.path.join(root, file)

hcp = np.empty(100000000, dtype=HuffmanCodedPos)
file_num = 0
cnt = 0

ptn = re.compile(r"^'\*\* (\d+)")

for file in fild_all_files(args.csa_dir):
    s = ""
    turn = 0
    turnEval = args.turn
    for line in open(file, 'r'):
        if turnEval == args.turn:
            m = ptn.search(line)
            if m:
                if abs(int(m.group(1))) > args.eval:
                    turnEval = turn
                turn += 1
        s += line

    if turn == 0:
        continue

    try:
        kif = shogi.CSA.Parser.parse_str(s)[0]
    except:
        print(file)
        continue

    file_num += 1
    board = shogi.Board(kif['sfen'])
    for move in kif['moves']:
        if board.move_number > args.turn or board.move_number > turnEval:
            break
        board.push_usi(move)

        hcp_decoder.sfen_to_hcp(board.sfen(), hcp[cnt:cnt+1])
        cnt += 1

print('file num =', file_num)
print('position num =', cnt)
hcp_uniq = np.unique(hcp[:cnt])
print('uniq position num =', len(hcp_uniq))
hcp_uniq.tofile(args.hcp_file)