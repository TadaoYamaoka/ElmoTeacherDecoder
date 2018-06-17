import shogi
import shogi.CSA
import numpy as np
import hcp_decoder
import os
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('csa_dir')
parser.add_argument('hcpe_file')
parser.add_argument('--turn', default=256, type=int)
parser.add_argument('--eval', type=int)
parser.add_argument('--rate', type=int)
args = parser.parse_args()

HuffmanCodedPosAndEval = np.dtype([
    ('hcp', np.uint8, 32),
    ('eval', np.int16),
    ('bestMove16', np.uint16),
    ('gameResult', np.uint8),
    ('dummy', np.uint8),
    ])

def fild_all_files(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            yield os.path.join(root, file)

hcpevec = np.empty(100000000, dtype=HuffmanCodedPosAndEval)
file_num = 0
cnt = 0

ptn_rate = re.compile(r"^'(black|white)_rate:.*:(.*)")
ptn_move = re.compile(r"^[+-]\d")
ptn_eval = re.compile(r"^'\*\* (-?\d+)")

for file in fild_all_files(args.csa_dir):
    rate = {}
    s = ""
    turn = 0
    eval = []
    eval_exist = False
    noeval = True
    try:
        for line in open(file, 'r'):
            if turn == 0:
                m = ptn_rate.search(line)
                if m:
                    rate[m.group(1)] = float(m.group(2))

            m = ptn_move.search(line)
            if m:
                if turn > 0 and eval_exist == False:
                    eval.append(None)
                turn += 1
                eval_exist = False
            else:
                m = ptn_eval.search(line)
                if m:
                    eval.append(int(m.group(1)))
                    eval_exist = True
                    noeval = False
            s += line
    except:
        print(file)
        continue

    if turn > 0 and eval_exist == False:
        eval.append(None)

    if turn == 0:
        continue

    if noeval:
        continue

    if args.rate is not None and (len(rate) < 2 or min(rate.values()) < args.rate):
        continue

    try:
        kif = shogi.CSA.Parser.parse_str(s)[0]
    except:
        print(file)
        continue

    if kif['win'] not in ('b', 'w'):
        continue

    file_num += 1
    board = shogi.Board(kif['sfen'])
    for i, move in enumerate(kif['moves']):
        if board.move_number > args.turn or (args.eval is not None and eval[i] is not None and abs(eval[i]) > args.eval):
            break

        if eval[i] is not None:
            if board.turn == shogi.WHITE:
                eval[i] *= -1
            hcp_decoder.sfen_to_hcpe(board.sfen(), eval[i], move, kif['win'], hcpevec[cnt:cnt+1])
            cnt += 1

        board.push_usi(move)

print('file num =', file_num)
print('position num =', cnt)
hcpe_uniq = np.unique(hcpevec[:cnt])
print('uniq position num =', len(hcpe_uniq))
hcpe_uniq.tofile(args.hcpe_file)