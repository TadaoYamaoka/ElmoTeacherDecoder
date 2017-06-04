import numpy as np
import hcp_decoder
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

HuffmanCodedPos = np.dtype([
    ('hcp', np.uint8, 32),
    ])

hcpvec = np.fromfile(args.file, dtype=HuffmanCodedPos)

if args.num is None:
    len = len(hcpvec)
else:
    len = args.num

hcp_decoder.print_sfen_from_hcp(hcpvec[0:len])
