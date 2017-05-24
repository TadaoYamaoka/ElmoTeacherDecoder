import shogi
from ElmoTeacherDecoder import *
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

with open(args.file, 'rb') as f:
    filesize = os.fstat(f.fileno()).st_size
    if args.num:
        num = args.num
    else:
        num = int(filesize / sizeof(HuffmanCodedPosAndEval))
    for i in range(num):
        hcpe = HuffmanCodedPosAndEval()
        f.readinto(hcpe)
        board, eval, bestMove, gameResult = decode_hcpe(hcpe)

        print(board.sfen())
        print(eval, bestMove.usi(), gameResult)