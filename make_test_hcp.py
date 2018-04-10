import shogi
import numpy as np
import hcp_decoder
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('hcp_file')
parser.add_argument('--sfen', default='lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1')
args = parser.parse_args()

HuffmanCodedPos = np.dtype([
    ('hcp', np.uint8, 32),
    ])

hcp = np.empty(1, dtype=HuffmanCodedPos)

hcp_decoder.sfen_to_hcp(args.sfen, hcp)
hcp.tofile(args.hcp_file)
