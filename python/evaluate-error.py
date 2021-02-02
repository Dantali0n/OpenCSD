# -*- coding: utf-8 -*-
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
from scipy.interpolate import CubicSpline
import numpy as np

import fileinput

reference = []
target = []

mode = 0

for line in fileinput.input():
    line = line.rstrip()
    if "ref" in line:
        mode = 0
    elif "tar" in line:
        mode = 1
    elif "eof" in line:
        break
    elif mode is 0:
        try:
            reference.append(float(line))
        except:
            print(line)
    elif mode is 1:
        target.append(float(line))

x = [x for x in range(len(reference))]

fig, axs = plt.subplots(1, 2)

x = np.array(x, dtype=float)
reference = np.array(reference, dtype=float)
target = np.array(target, dtype=float)


offset = (max(reference) * 0.0)
target = [x + offset for x in target]

p1 = CubicSpline(x, reference)
p2 = CubicSpline(x, target)

# p1 = np.polynomial.chebyshev.chebfit(x, reference, 3)
# p2 = np.polynomial.chebyshev.chebfit(x, target, 3)

axs[0].set_title("Evaluate FFT",fontsize=16)
axs[0].set_xlabel('bin', fontsize=18)
axs[0].set_ylabel('amplitude', fontsize=16)
axs[0].plot(x, reference, 'ro',label="reference", color='red')
axs[0].plot(x, target, 'ro',label="target", color='blue')
axs[0].plot(x, p2(x), color='cyan') # plot first order polynomial
axs[0].plot(x, p1(x), color='pink') # plot first order polynomial
axs[0].legend(loc='upper left')
# axs[0].set_xscale('log')
#axs[0].set_yscale('log')

difference = [abs(j - target[i]) for i, j in enumerate(reference)]

p3 = CubicSpline(x, difference)

axs[1].set_title("Error",fontsize=16)
axs[1].set_xlabel('bin', fontsize=18)
axs[1].set_ylabel('error', fontsize=16)
axs[1].plot(x, difference, 'ro',label="difference", color='red')
axs[1].plot(x, p3(x), color='pink') # plot first order polynomial
axs[1].legend(loc='upper left')
axs[1].yaxis.set_major_formatter(mtick.FormatStrFormatter('%.2e'))

plt.show()