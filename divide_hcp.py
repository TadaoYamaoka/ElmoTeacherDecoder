import numpy as np
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('num', type=int)
parser.add_argument('outfile')
args = parser.parse_args()

HuffmanCodedPos = np.dtype([
    ('hcp', np.uint8, 32),
    ])

hcpvec = np.fromfile(args.file, dtype=HuffmanCodedPos)

print('data len =', len(hcpvec))

size = int(len(hcpvec) / args.num) + 1
for i in range(args.num):
    hcpvec[size * i:size * (i + 1)].tofile(args.outfile + str(i))
