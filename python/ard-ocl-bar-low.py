# -*- coding: utf-8 -*-
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

x = [("8", "128", "512",  "2048", "8192")]

yhost = []
ywin = []
yreverse = []
yfft = []
ymag = []
ydevice = []

for f in x[0]:
    try:
        data = pd.read_csv("../measurements/ard-ocl/ard-ocl_dvb-t_{0}.csv".format(f))
        yhost.append(sum(data['host'].values) / len(data['host']) / 1000000)
        ywin.append(sum(data['window'].values) / len(data['window']) / 1000000)
        yreverse.append(sum(data['reverse'].values) / len(data['reverse']) / 1000000)
        yfft.append(sum(data['fft'].values) / len(data['fft']) / 1000000)
        ymag.append(sum(data['magnitude'].values) / len(data['magnitude']) / 1000000)
        ydevice.append(sum(data['device'].values) / len(data['device']) / 1000000)
    except:
        print("File ard-seq_jorn_{0}.csv does not exist".format(f))

zwin = []
zfft = []
zmag = []

for f in x[0]:
    try:
        data = pd.read_csv("../measurements/fftw/fftw_dvb-t_{0}.csv".format(f))
    except:
        print("File ard-seq_jorn_{0}.csv does not exist".format(f))
    zwin.append(sum(data['window'].values) / len(data['window']) / 1000000)
    zfft.append(sum(data['fft'].values) / len(data['fft']) / 1000000)
    zmag.append(sum(data['magnitude'].values) / len(data['magnitude']) / 1000000)

fig, ax = plt.subplots()
index = np.arange(5)
num_bars = 5
bar_width = 0.9 / num_bars
opacity = 0.8

current_bottom = 0
plt.bar(index, yhost, bar_width,
        alpha=opacity, color='#cccc00', label='host-to-device-ocl')
plt.bar(index, ywin, bar_width,
        alpha=opacity, color='#cc0000', label='window-ocl', bottom=yhost)
current_bottom = [x + y for x, y in zip(yhost, ywin)]
plt.bar(index, yreverse, bar_width,
        alpha=opacity, color='#00cccc', label='reverse-ocl', bottom=current_bottom)
current_bottom = [x + y for x, y in zip(current_bottom, yreverse)]
plt.bar(index, yfft, bar_width,
        alpha=opacity, color='#00cc00', label='fft-ocl', bottom=current_bottom)
current_bottom = [x + y for x, y in zip(current_bottom, yfft)]
plt.bar(index, ymag, bar_width,
        alpha=opacity, color='#0000cc', label='magnitude-ocl', bottom=current_bottom)
current_bottom = [x + y for x, y in zip(current_bottom, ymag)]
plt.bar(index, ydevice, bar_width,
        alpha=opacity, color='#cc00cc', label='device-to-host-ocl', bottom=current_bottom)

current_bottom = 0
plt.bar(index + (bar_width * 1.1), zwin, bar_width,
        alpha=opacity, color='#880000', label='window-fftw')
plt.bar(index + (bar_width * 1.1), zfft, bar_width,
        alpha=opacity, color='#008800', label='fft-fftw', bottom=zwin)
current_bottom = [x + y for x, y in zip(zfft, zwin)]
plt.bar(index + (bar_width * 1.1), zmag, bar_width,
        alpha=opacity, color='#000088', label='magnitude-fftw', bottom=current_bottom)

plt.xlabel('Input size')
plt.ylabel('Mean execution time in seconds')
plt.title('Performance of basic OpenCL compared to sequential CPU')
plt.xticks(index + ((0.5 / num_bars) * (num_bars / 5)), ("8", "128", "512",  "2048", "8192"))
plt.legend()

#ax.set_ylim(ymin=0, ymax=1100

plt.tight_layout()
plt.show()