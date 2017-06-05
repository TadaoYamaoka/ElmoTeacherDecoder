import numpy as np
import hcp_decoder
import os
import argparse
import time

parser = argparse.ArgumentParser()
parser.add_argument('file')
args = parser.parse_args()

# see: position.hpp:178: struct HuffmanCodedPosAndEval
HuffmanCodedPosAndEval = np.dtype([
    ('hcp', np.uint8, 32),
    ('eval', np.int16),
    ('bestMove16', np.uint16),
    ('gameResult', np.uint8),
    ('dummy', np.uint8),
    ])

hcpevec = np.fromfile(args.file, dtype=HuffmanCodedPosAndEval)
hcp_decoder.check_duplicates(hcpevec)
