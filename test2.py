import numpy as np
import shogi
from ElmoTeacherDecoder2 import *
import os
import argparse
import time

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

start = time.time()

with open(args.file, 'rb') as f:
    hcpevec = np.fromfile(args.file, dtype=HuffmanCodedPosAndEval)
    for hcpe in hcpevec:
        board, eval, bestMove, gameResult = decode_hcpe(hcpe)

        print(board.sfen())
        print(eval, bestMove.usi(), gameResult)

elapsed_time = time.time() - start
print("elapsed_time:{0}".format(elapsed_time) + "[sec]")