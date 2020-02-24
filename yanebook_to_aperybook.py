import argparse
import numpy as np
from cshogi import *

parser = argparse.ArgumentParser()
parser.add_argument('yanebook')
parser.add_argument('out')
args = parser.parse_args()

board = Board()

bookdic = {}
with open(args.yanebook) as f:
    for line in f:
        if line[:1] == '#':
            continue
        line = line.strip()
        if line[:4] == 'sfen':
            board.set_sfen(line[5:])
            key = board.book_key()
            bookdic[key] = []
        else:
            vals = line.split(' ')
            bookdic[key].append((move16(board.move_from_usi(vals[0])), vals[4], vals[2]))

num_entries = 0
for entries in bookdic.values():
    num_entries += len(entries)

# output
book_entries = np.empty(num_entries, BookEntry)
i = 0
for key in sorted(bookdic.keys()):
    entries = bookdic[key]
    for entry in entries:
        book_entries[i] = (key, entry[0], entry[1], entry[2])
        i += 1
book_entries.tofile(args.out)

print(f"positions : {len(bookdic)}")
print(f"entries : {num_entries}")
