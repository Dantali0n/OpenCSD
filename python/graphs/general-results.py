# -*- coding: utf-8 -*-

"""
  MIT License

  Copyright (c) 2022 Dantali0n

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
"""

import matplotlib.pyplot as plt
import matplotlib.cm as cm
import pandas as pd
import numpy as np

operation = "write"  # "read"

mode = "seq"  # "rand"

# x = [("64k", "256k", "1024k", "4096k", "16384k", "65536k", "262144k", "1048576k")]

# Sizes for seq mode
x = [("64", "256", "1024", "4096", "16384", "65536", "262144", "1048576")]
k = "k"
# Sizes for rand mode
if mode == "rand":
    k = ""
    x = [("64", "256", "1024", "4096", "16384", "65536", "524288")]

def adj_lightness(color, amount=0.5):
    import matplotlib.colors as mc
    import colorsys
    try:
        c = mc.cnames[color]
    except:
        c = color
    c = colorsys.rgb_to_hls(*mc.to_rgb(c))
    return colorsys.hls_to_rgb(c[0], max(0, min(1, amount * c[1])), c[2])

bandwidth_f2fs = []
base_std_f2fs = []

bandwidth_flufflefs = []
base_std_flufflefs = []

for f in x[0]:
    if f == "0":
        bandwidth_f2fs.append(0)
        base_std_f2fs.append(([0],[0]))
        continue
    try:
        data = pd.read_csv(f"../measurements/fio/fio-{mode}-{operation}-f2fs-1-{f}{k}.csv")
        bandwidth = sum(data['bandwidth'].values) / 1048576 / len(data['bandwidth'])# MiB/S
        bandwidth_f2fs.append(bandwidth)
        base_std = ([],[])
        base_std[0].append(
            abs(min((x[0] / 1048576) - bandwidth for x in zip(data['bandwidth'].values)))
        )
        base_std[1].append(
            abs(max((x[0] / 1048576) - bandwidth for x in zip(data['bandwidth'].values)))
        )
        base_std_f2fs.append(base_std)
    except:
        print(f"File ../measurements/fio/fio-{mode}-{operation}-f2fs-1-{f}{k}.csv does not exist")

for f in x[0]:
    if f == "0":
        bandwidth_flufflefs.append(0)
        base_std_flufflefs.append(([0],[0]))
        continue
    try:
        data = pd.read_csv(f"../measurements/fio/fio-{mode}-{operation}-fluffle-1-{f}{k}.csv")
        bandwidth = sum(data['bandwidth'].values) / 1048576 / len(data['bandwidth']) # MiB/S
        bandwidth_flufflefs.append(bandwidth)
        base_std = ([],[])
        base_std[0].append(
            abs(min((x[0] / 1048576) - bandwidth for x in zip(data['bandwidth'].values)))
        )
        base_std[1].append(
            abs(max((x[0] / 1048576) - bandwidth for x in zip(data['bandwidth'].values)))
        )
        base_std_flufflefs.append(base_std)
    except:
        print(f"File ../measurements/fio/fio-{mode}-{operation}-fluffle-1-{f}{k}.csv does not exist")

from matplotlib import rcParams
rcParams['font.family'] = 'Times New Roman'
# rcParams['font.sans-serif'] = ['times-new-roman']

fontsize = 18
plt.rc('font', size=fontsize) #controls default text size
plt.rc('axes', titlesize=fontsize) #fontsize of the title
plt.rc('axes', labelsize=fontsize) #fontsize of the x and y labels
plt.rc('xtick', labelsize=fontsize) #fontsize of the x tick labels
plt.rc('ytick', labelsize=fontsize) #fontsize of the y tick labels
plt.rc('legend', fontsize=fontsize) #fontsize of the legend

colors = cm.rainbow(np.linspace(0, 1, 2))

plt.xscale('log', base=4, nonpositive='mask')
xvalues = [int(value) for value in x[0]]
plt.xticks([int(value) for value in x[0]], labels=x[0])

plt.grid(which='both', zorder=1, axis='both')


if mode == "seq":
    plt.xlabel('File Size in KibiBytes')
else:
    plt.xlabel('Request Size in Bytes')

plt.ylabel('Throughput (MiB/S)')

modestring = "Sequential"
if mode == "rand":
    modestring = "Random"

plt.title(f"{modestring} {operation} Performance F2FS vs FluffleFS")

plt.plot(xvalues, bandwidth_f2fs, color=colors[0], label='F2FS')
plt.plot(xvalues, bandwidth_flufflefs, color=colors[1], label='FluffleFS')

for (i,y) in enumerate(bandwidth_f2fs):
    plt.errorbar(xvalues[i], bandwidth_f2fs[i], yerr=base_std_f2fs[i], color=colors[0], capsize=10, alpha=0.90)
    plt.errorbar(xvalues[i], bandwidth_flufflefs[i], yerr=base_std_flufflefs[i], color=colors[1], capsize=10, alpha=0.90)

# plt.axhline(y=1, color='#000', alpha=0.5, label='Identical performance')
# plt.xticks(index + ((0.05 / num_bars) * (num_bars / 2)), x[0])
plt.legend()

# ax.set_yscale('log')
# ax.set_ylim(ymin=0, ymax=16)

plt.tight_layout()
plt.show()