import numpy as np
import shogi
import hcp_decoder
import os
import argparse
import time

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

# see: position.hpp:178: struct HuffmanCodedPosAndEval
HuffmanCodedPosAndEval = np.dtype([
    ('hcp', np.uint8, 32),
    ('eval', np.int16),
    ('bestMove16', np.uint16),
    ('gameResult', np.uint8),
    ('dummy', np.uint8),
    ])

start = time.time()

with open(args.file, 'rb') as f:
    hcpevec = np.fromfile(args.file, dtype=HuffmanCodedPosAndEval)
    result = np.empty(len(hcpevec), dtype=np.int32)
    hcp_decoder.decode(hcpevec, result)

elapsed_time = time.time() - start
print("elapsed_time:{0}".format(elapsed_time) + "[sec]")

print(result.shape)
print(result)
