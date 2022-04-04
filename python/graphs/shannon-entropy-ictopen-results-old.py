# -*- coding: utf-8 -*-
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

x = [("32",)]

ywin = []
yfft = []
ymag = []

for f in x[0]:
    try:
        data = pd.read_csv("../measurements/shannon/ictopen-offloaded-{0}.csv".format(f))
    except:
        print("File ictopen-regular-{0}.csv does not exist".format(f))
    import pdb; pdb.set_trace()
    ywin.append(sum(data['wait'].values / len(data['wait'])))
    yfft.append(sum(data['user'].values / len(data['user'])))
    ymag.append(sum(data['sys'].values / len(data['sys'])))

zwin = []
zfft = []
zmag = []

for f in x[0]:
    try:
        data = pd.read_csv("../measurements/shannon/ictopen-regular-{0}.csv".format(f))
    except:
        print("File ictopen-regular-{0}.csv does not exist".format(f))
    zwin.append(sum(data['wait'].values / len(data['wait'])))
    zfft.append(sum(data['user'].values / len(data['user'])))
    zmag.append(sum(data['sys'].values / len(data['sys'])))

fig, ax = plt.subplots()
index = np.arange(1)
num_bars = 1
bar_width = 0.9 / num_bars
opacity = 0.8

plt.bar(index, ywin, bar_width,
        alpha=opacity, color='#cc0000', label='csd-wait')
plt.bar(index, yfft, bar_width,
        alpha=opacity, color='#00cc00', label='csd-user', bottom=ywin)
plt.bar(index, ymag, bar_width,
        alpha=opacity, color='#0000cc', label='csd-sys', bottom=yfft)

plt.bar(index + (bar_width * 1.1), zwin, bar_width,
        alpha=opacity, color='#880000', label='regular-wait')
plt.bar(index + (bar_width * 1.1), zfft, bar_width,
        alpha=opacity, color='#008800', label='regular-user', bottom=zwin)
plt.bar(index + (bar_width * 1.1), zmag, bar_width,
        alpha=opacity, color='#000088', label='regular-sys', bottom=zfft)

plt.xlabel('Number of files')
plt.ylabel('Mean execution time in seconds')
plt.title('Performance of offloaded Shannon entropy vs regular')
plt.xticks(index + ((0.5 / num_bars) * (num_bars / 1)), ("32",))
plt.legend()

#ax.set_ylim(ymin=0, ymax=1000000)

plt.tight_layout()
plt.show()
