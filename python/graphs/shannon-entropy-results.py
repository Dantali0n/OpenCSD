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

x = [("1024", "4096", "16384", "65536", "262144", "1048576")]
x = [("1024", "4096", "16384")]
# x = [("65536", "262144", "1048576")]

def adj_lightness(color, amount=0.5):
    import matplotlib.colors as mc
    import colorsys
    try:
        c = mc.cnames[color]
    except:
        c = color
    c = colorsys.rgb_to_hls(*mc.to_rgb(c))
    return colorsys.hls_to_rgb(c[0], max(0, min(1, amount * c[1])), c[2])

kwait_results = []
kuser_results = []
ksys_results = []
kbase_std = ([],[])

def time_string_to_float(string: str):
    input = string.split('m')
    return (float(input[0]) * 60) + float(input[1].split('s')[0])

for f in x[0]:
    try:
        data = pd.read_csv(f"../measurements/entropy/entropy-kernel-{f}k.csv")
        results = [time_string_to_float(real) for real in data['real'].values]
        real_result = sum(results) #  / 1e6
        user_result = sum([time_string_to_float(real) for real in data['user'].values])  # / 1e6
        sys_result = sum([time_string_to_float(real) for real in data['sys'].values])  # / 1e6

        kuser_results.append(user_result / len(data['user']))
        ksys_results.append(sys_result / len(data['sys']))
        kwait_results.append((real_result - user_result - sys_result) / len(data['real']))
        # base_std.append([min(results), max(results)])
        kbase_std[0].append(
            abs(min(results) - (real_result / len(data['real'])))
            # abs(min(((x+y+z)) - result for x,y,z in zip(data['real'].values, data['user'].values, data['sys'].values)))
        )
        kbase_std[1].append(
            # abs(max(((x+y+z)) - result for x,y,z in zip(data['real'].values, data['user'].values, data['sys'].values)))
            abs(max(results) - (real_result / len(data['real'])))
        )
    except:
        print(f"../measurements/entropy/entropy-kernel-{f}k.csv does not exist")

rwait_results = []
ruser_results = []
rsys_results = []
rbase_std = ([],[])

for f in x[0]:
    try:
        data = pd.read_csv(f"../measurements/entropy/entropy-regular-{f}k.csv")
        results = [time_string_to_float(real) for real in data['real'].values]
        real_result = sum(results) #  / 1e6
        user_result = sum([time_string_to_float(real) for real in data['user'].values])  # / 1e6
        sys_result = sum([time_string_to_float(real) for real in data['sys'].values])  # / 1e6

        ruser_results.append(user_result / len(data['user']))
        rsys_results.append(sys_result / len(data['sys']))
        rwait_results.append((real_result - user_result - sys_result) / len(data['real']))
        # base_std.append([min(results), max(results)])
        rbase_std[0].append(
            abs(min(results) - (real_result / len(data['real'])))
            # abs(min(((x+y+z)) - result for x,y,z in zip(data['real'].values, data['user'].values, data['sys'].values)))
        )
        rbase_std[1].append(
            # abs(max(((x+y+z)) - result for x,y,z in zip(data['real'].values, data['user'].values, data['sys'].values)))
            abs(max(results) - (real_result / len(data['real'])))
        )
    except:
        print(f"../measurements/entropy/entropy-regular-{f}k.csv does not exist")

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
index = np.arange(len(x[0]))
num_bars = 6
# bar_width = 2.6 / num_bars
bar_width = 0.6 / num_bars
opacity = 0.95

colors = cm.rainbow(np.linspace(0, 1, 5))

plt.grid(which='both', zorder=1, axis='y')

import pdb; pdb.set_trace()

current_bottom = 0
plt.bar(index + (bar_width * 0.1), rwait_results, bar_width,
        alpha=opacity, color=adj_lightness(colors[0], 1), label='wait', zorder=2)
plt.bar(index + (bar_width * 0.1), ruser_results, bar_width,
        alpha=opacity, color=adj_lightness(colors[1], 1), label='user', zorder=2, bottom=rwait_results)
current_bottom = [x + y for x, y in zip(rwait_results, ruser_results)]
plt.bar(index + (bar_width * 0.1), rsys_results, bar_width, yerr=rbase_std, capsize=5,
        alpha=opacity, color=adj_lightness(colors[2], 1), label='sys', zorder=2, bottom=current_bottom)

current_bottom = 0
wait_bar = plt.bar(index + (bar_width * 1.3), kwait_results, bar_width,
        alpha=opacity, color=adj_lightness(colors[0], 1), label='wait', zorder=2)
user_bar = plt.bar(index + (bar_width * 1.3), kuser_results, bar_width,
        alpha=opacity, color=adj_lightness(colors[1], 1), label='user', zorder=2, bottom=kwait_results)
current_bottom = [x + y for x, y in zip(kwait_results, kuser_results)]
sys_bar = plt.bar(index + (bar_width * 1.3), ksys_results, bar_width, yerr=kbase_std, capsize=5,
        alpha=opacity, color=adj_lightness(colors[2], 1), label='sys', zorder=2, bottom=current_bottom)

# plt.title('')

plt.xlabel('File size in KibiBytes')
plt.ylabel('Total wall execution time (seconds)')
plt.title('Shannon entropy workload (compression)')
# plt.axhline(y=1, color='#000', alpha=0.5, label='Identical performance')
plt.xticks(index + ((0.065 / num_bars) * (num_bars / 1)), x[0])
plt.legend(handles=[wait_bar, user_bar, sys_bar])

# ax.set_yscale('log')
# ax.set_ylim(ymin=0, ymax=250)

plt.tight_layout()
plt.show()