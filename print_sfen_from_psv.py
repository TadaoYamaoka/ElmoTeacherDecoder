import numpy as np
import psv_decoder
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

PackedSfenValue = np.dtype([
    ('sfen', np.uint8, 32),
    ('score', np.int16),
    ('move', np.uint16),
    ('gamePly', np.uint16),
    ('game_result', np.uint8),
    ('padding', np.uint8),
    ])

psvvec = np.fromfile(args.file, dtype=PackedSfenValue)

print('data len =', len(psvvec))

if args.num is None:
    len = len(psvvec)
else:
    len = args.num

psv_decoder.print_sfen_from_psv(psvvec[0:len])
