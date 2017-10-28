import numpy as np
import hcp_decoder
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

HuffmanCodedPosWithHistoryAndEval = np.dtype([
    ('hcp', np.uint8, 32),
    ('historyMove16', np.uint16, 7),
    ('eval', np.int16),
    ('bestMove16', np.uint16),
    ('gameResult', np.uint8),
    ('dummy', np.uint8),
    ])

hcphevec = np.fromfile(args.file, dtype=HuffmanCodedPosWithHistoryAndEval)

print('data len =', len(hcphevec))

if args.num is None:
    len = len(hcphevec)
else:
    len = args.num

hcp_decoder.print_sfen_from_hcphe(hcphevec[0:len])
