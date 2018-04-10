import numpy as np
import hcp_decoder
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

HuffmanCodedPosAndEval = np.dtype([
    ('hcp', np.uint8, 32),
    ('eval', np.int16),
    ('bestMove16', np.uint16),
    ('gameResult', np.uint8),
    ('dummy', np.uint8),
    ])

hcpevec = np.fromfile(args.file, dtype=HuffmanCodedPosAndEval)

print('data len =', len(hcpevec))

if args.num is None:
    len = len(hcpevec)
else:
    len = args.num

hcp_decoder.print_sfen_from_hcpe(hcpevec[0:len])
