import shogi
from ElmoTeacherDecoder import *
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('num', type=int)
args = parser.parse_args()

with open(args.file, 'rb') as f:
    for i in range(args.num):
        hcpe = HuffmanCodedPosAndEval()
        f.readinto(hcpe)
        board, eval, bestMove, gameResult = decode_hcpe(hcpe)

        print(board.sfen())
        print(eval, bestMove.usi(), gameResult)