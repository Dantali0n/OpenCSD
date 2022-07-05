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

x = [("regular", "offloaded")]

def adj_lightness(color, amount=0.5):
    import matplotlib.colors as mc
    import colorsys
    try:
        c = mc.cnames[color]
    except:
        c = color
    c = colorsys.rgb_to_hls(*mc.to_rgb(c))
    return colorsys.hls_to_rgb(c[0], max(0, min(1, amount * c[1])), c[2])

init_reset = []
fill_zone = []
execute = []
base_std = ([],[])

for f in x[0]:
    try:
        data = pd.read_csv("../measurements/ictopen/ictopen-{0}-32.csv".format(f))
        init_reset_result = sum(data['wait'].values) #  / 1e6
        fill_zone_result = sum(data['user'].values)  # / 1e6
        execute_result = sum(data['sys'].values)  # / 1e6
        result = (init_reset_result + fill_zone_result + execute_result) / len(data['wait'])
        init_reset.append(init_reset_result / len(data['wait']))
        fill_zone.append(fill_zone_result / len(data['user']))
        execute.append(execute_result / len(data['sys']))
        base_std[0].append(
            abs(min(((x+y+z)) - result for x,y,z in zip(data['wait'].values, data['user'].values, data['sys'].values)))
        )
        base_std[1].append(
            abs(max(((x+y+z)) - result for x,y,z in zip(data['wait'].values, data['user'].values, data['sys'].values)))
        )
    except:
        print("File ../measurements/ictopen/ictopen-{0}-32.csv does not exist".format(f))

import pdb; pdb.set_trace()
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

fig, ax = plt.subplots()
index = np.arange(2)
num_bars = 2
# bar_width = 2.6 / num_bars
bar_width = 0.6 / num_bars
opacity = 0.95

colors = cm.rainbow(np.linspace(0, 1, 5))

plt.grid(which='both', zorder=1, axis='y')

current_bottom = 0
plt.bar(index + (bar_width * 0.1), init_reset, bar_width,
        alpha=opacity, color=adj_lightness(colors[0], 1), label='wait', zorder=2)
plt.bar(index + (bar_width * 0.1), fill_zone, bar_width,
        alpha=opacity, color=adj_lightness(colors[1], 1), label='user', zorder=2, bottom=init_reset)
current_bottom = [x + y for x, y in zip(init_reset, fill_zone)]
plt.bar(index + (bar_width * 0.1), execute, bar_width, yerr=base_std, capsize=5,
        alpha=opacity, color=adj_lightness(colors[2], 1), label='sys', zorder=2, bottom=current_bottom)

# plt.title('')

plt.xlabel('Implementation')
plt.ylabel('Total wall execution time (seconds)')
plt.title('Shannon entropy workload (compression)')
# plt.axhline(y=1, color='#000', alpha=0.5, label='Identical performance')
plt.xticks(index + ((0.05 / num_bars) * (num_bars / 2)), ("regular", "offloaded"))
plt.legend()

# ax.set_yscale('log')
# ax.set_ylim(ymin=0, ymax=16)

plt.tight_layout()
plt.show()