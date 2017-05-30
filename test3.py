import numpy as np
import hcp_decoder
import os
import argparse
import time

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

MAX_HPAWN_NUM = 8 # 歩の持ち駒の上限
MAX_HLANCE_NUM = 4
MAX_HKNIGHT_NUM = 4
MAX_HSILVER_NUM = 4
MAX_HGOLD_NUM = 4
MAX_HBISHOP_NUM = 2
MAX_HROOK_NUM = 2
MAX_PIECES_IN_HAND_SUM = MAX_HPAWN_NUM + MAX_HLANCE_NUM + MAX_HKNIGHT_NUM + MAX_HSILVER_NUM + MAX_HGOLD_NUM + MAX_HBISHOP_NUM + MAX_HROOK_NUM

# see: position.hpp:178: struct HuffmanCodedPosAndEval
HuffmanCodedPosAndEval = np.dtype([
    ('hcp', np.uint8, 32),
    ('eval', np.int16),
    ('bestMove16', np.uint16),
    ('gameResult', np.uint8),
    ('dummy', np.uint8),
    ])

start = time.time()

hcpevec = np.fromfile(args.file, dtype=HuffmanCodedPosAndEval)
features1 = np.empty((len(hcpevec), 2, 14, 81), dtype=np.float32)
features2 = np.empty((len(hcpevec), 2 * MAX_PIECES_IN_HAND_SUM + 1, 81), dtype=np.float32)

result = np.empty(len(hcpevec), dtype=np.float32)
move = np.empty(len(hcpevec), dtype=np.int32)
value = np.empty(len(hcpevec), dtype=np.float32)

#hcp_decoder.decode_with_result(hcpevec, features1, features2, result)
#hcp_decoder.decode_with_move(hcpevec, features1, features2, move)
hcp_decoder.decode_with_value(hcpevec, features1, features2, value, move, result)

elapsed_time = time.time() - start
print("elapsed_time:{0}".format(elapsed_time) + "[sec]")

#np.set_printoptions(threshold=np.inf)
print(features1[0])
print(features2[0])
print(result.shape)
print(result)
print(move.shape)
print(move)
print(value.shape)
print(value)
