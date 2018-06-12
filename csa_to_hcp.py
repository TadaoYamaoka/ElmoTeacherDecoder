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
parser.add_argument('--rate', type=int)
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

ptn_rate = re.compile(r"^'(black|white)_rate:.*:(.*)")
ptn_move = re.compile(r"^[+-]\d")
ptn_eval = re.compile(r"^'\*\* (-?\d+)")

for file in fild_all_files(args.csa_dir):
    rate = {}
    s = ""
    turn = 0
    turnEval = args.turn
    try:
        for line in open(file, 'r'):
            if turn == 0:
                m = ptn_rate.search(line)
                if m:
                    rate[m.group(1)] = float(m.group(2))

            if turnEval == args.turn:
                m = ptn_move.search(line)
                if m:
                    turn += 1
                else:
                    m = ptn_eval.search(line)
                    if m:
                        if abs(int(m.group(1))) > args.eval:
                            turnEval = turn
            s += line
    except:
        print(file)
        continue

    if turn == 0:
        continue

    if args.rate is not None and (len(rate) < 2 or min(rate.values()) < args.rate):
        continue

    try:
        kif = shogi.CSA.Parser.parse_str(s)[0]
    except:
        print(file)
        continue

    file_num += 1
    board = shogi.Board(kif['sfen'])
    for move in kif['moves']:
        if board.move_number > args.turn or board.move_number >= turnEval:
            break
        board.push_usi(move)

        hcp_decoder.sfen_to_hcp(board.sfen(), hcp[cnt:cnt+1])
        cnt += 1

print('file num =', file_num)
print('position num =', cnt)
hcp_uniq = np.unique(hcp[:cnt])
print('uniq position num =', len(hcp_uniq))
hcp_uniq.tofile(args.hcp_file)