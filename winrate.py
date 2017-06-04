import shogi
from ElmoTeacherDecoder import *
import os
import argparse
from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np

from chainer import Variable
from chainer import optimizers
import chainer.links as L
import chainer.functions as F

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('--num', type=int)
args = parser.parse_args()

sum_win = defaultdict(lambda: 0)
num_win = defaultdict(lambda: 0)
sum_win_all = 0
with open(args.file, 'rb') as f:
    filesize = os.fstat(f.fileno()).st_size
    if args.num:
        num = args.num
    else:
        num = int(filesize / sizeof(HuffmanCodedPosAndEval))
    for i in range(num):
        hcpe = HuffmanCodedPosAndEval()
        f.readinto(hcpe)
        board, eval, bestMove, gameResult = decode_hcpe(hcpe)

        if gameResult == 2:
            win = -1
        else:
            win = gameResult
        if board.turn == shogi.WHITE:
            win *= -1

        eval100 = int(eval/100)
        sum_win[eval100] += win
        num_win[eval100] += 1
        if win > 0:
            sum_win_all += 1

print('win avr.', float(sum_win_all) / num)

avr_win = {}
for eval100 in sum_win.keys():
    avr_win[eval100] = sum_win[eval100] / num_win[eval100]

arv_win_keys = sorted(avr_win.keys())
x = np.array(list(arv_win_keys), dtype=np.float32)
x *= 100
t = np.array([avr_win[_] for _ in arv_win_keys], dtype=np.float32)

# tanh
optimizer = optimizers.AdaGrad(lr=0.00001)

f = L.Linear(1, 1, initialW=0.001, nobias = True)
optimizer.setup(f)

val_x = Variable(x.reshape(len(x), 1))
val_t = Variable(t.reshape(len(t), 1))
for i in range(1000):
    y = F.tanh(f(val_x))

    f.cleargrads()
    loss = F.mean_squared_error(y, val_t)
    loss.backward()
    optimizer.update()

print(f.W.data)

plt.plot(x, t, '.')
plt.plot(x, F.tanh(f(val_x)).data)
plt.xlim(-3500, 3500)
plt.show()

# sigmoid
optimizer = optimizers.AdaGrad(lr=0.00001)

f = L.Linear(1, 1, initialW=0.001, nobias = True)
optimizer.setup(f)

t = (t + 1) / 2.0
val_t = Variable(t.reshape(len(t), 1))
for i in range(1000):
    y = F.sigmoid(f(val_x))

    f.cleargrads()
    loss = F.mean_squared_error(y, val_t)
    loss.backward()
    optimizer.update()

print(f.W.data)

plt.plot(x, t, '.')
plt.plot(x, F.sigmoid(f(val_x)).data)
plt.xlim(-3500, 3500)
plt.show()