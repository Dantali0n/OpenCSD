#!/usr/bin/env python3

"""
This is a simple script to compute the Roofline Model
(https://en.wikipedia.org/wiki/Roofline_model) of given HW platforms
running given apps

Peak bandwidth must be specified in GB/s
Peak performance must be specified in GFLOP/s
Arithemtic intensity is specified in FLOP/byte
Performance is specified in GFLOP/s

Script is adapted an modified by Corne Lukken to better fit the
requirements of the performance engineering course at the UvA.

Copyright 2018, Mohamed A. Bamakhrama
Licensed under BSD license shown in LICENSE
"""


import csv
import sys
import argparse
import numpy
import matplotlib.pyplot
import matplotlib
from os.path import dirname
matplotlib.rc('font', family='Arial')


# Constants
# The following constants define the span of the intensity axis
START = -6
STOP = 5
N = abs(STOP - START + 1)

# reference implementation gflops
ref_arc     = 1.09e9
ref_ash     = 0.74e9
ref_g6      = 0.70e9
ref_tols    = 0.62e9
ref_bcsstk  = 0.22e9

# reference
# arc130xarc130
# y1 = []
# with open('../measurements/assignment1/matmul_reference_arc130xarc130.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# y1.append((sum(zt)/len(zt)) / 1e9)
# # ash958xash958
# with open('../measurements/assignment1/matmul_reference_ash958xash958.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# y1.append((sum(zt)/len(zt)) / 1e9)
# # g6xg6
# with open('../measurements/assignment1/matmul_reference_G6xG6.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# y1.append((sum(zt)/len(zt)) / 1e9)
# # tols1090xtols1090
# with open('../measurements/assignment1/matmul_reference_tols1090xtols1090.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# y1.append((sum(zt)/len(zt)) / 1e9)
# # bcsstk13xbcsstk13
# with open('../measurements/assignment1/matmul_reference_bcsstk13xbcsstk13.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# y1.append((sum(zt)/len(zt)) / 1e9)
#
# # loose
# # arc130xarc130
# z1 = []
# with open('../measurements/assignment1/matmul_loose_arc130xarc130.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# z1.append((sum(zt)/len(zt)) / 1e9)
# # ash958xash958
# with open('../measurements/assignment1/matmul_loose_ash958xash958.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# z1.append((sum(zt)/len(zt)) / 1e9)
# # g6xg6
# with open('../measurements/assignment1/matmul_loose_G6xG6.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# z1.append((sum(zt)/len(zt)) / 1e9)
# # tols1090xtols1090
# with open('../measurements/assignment1/matmul_loose_tols1090xtols1090.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# z1.append((sum(zt)/len(zt)) / 1e9)
# # bcsstk13xbcsstk13
# with open('../measurements/assignment1/matmul_loose_bcsstk13xbcsstk13.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# z1.append((sum(zt)/len(zt)) / 1e9)
#
# # inner
# # arc130xarc130
# w1 = []
# with open('../measurements/assignment1/matmul_inner_arc130xarc130.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# w1.append((sum(zt)/len(zt)) / 1e9)
# # ash958xash958
# with open('../measurements/assignment1/matmul_inner_ash958xash958.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# w1.append((sum(zt)/len(zt)) / 1e9)
# # g6xg6
# with open('../measurements/assignment1/matmul_inner_G6xG6.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# w1.append((sum(zt)/len(zt)) / 1e9)
# # tols1090xtols1090
# with open('../measurements/assignment1/matmul_inner_tols1090xtols1090.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# w1.append((sum(zt)/len(zt)) / 1e9)
# # bcsstk13xbcsstk13
# with open('../measurements/assignment1/matmul_inner_bcsstk13xbcsstk13.csv') as data:
#     zt = [float(num.split(" ")[-1]) for num in data.readlines()]
# w1.append((sum(zt)/len(zt)) / 1e9)


def roofline(num_platforms, peak_performance, peak_bandwidth, intensity):
    """
    Computes the roofline model for the given platforms.
    Returns The achievable performance
    """

    assert isinstance(num_platforms, int) and num_platforms > 0
    assert isinstance(peak_performance, numpy.ndarray)
    assert isinstance(peak_bandwidth, numpy.ndarray)
    assert isinstance(intensity, numpy.ndarray)
    assert (num_platforms == peak_performance.shape[0] and
            num_platforms == peak_bandwidth.shape[0])

    achievable_performance = numpy.zeros((num_platforms, len(intensity)))
    for i in range(num_platforms):
        achievable_performance[i:] = numpy.minimum(peak_performance[i],
                                                   peak_bandwidth[i] * intensity)
    return achievable_performance


def process(hw_platforms, sw_apps, xkcd):
    """
    Processes the hw_platforms and sw_apps to plot the Roofline.
    """
    assert isinstance(hw_platforms, list)
    assert isinstance(sw_apps, list)
    assert isinstance(xkcd, bool)

    # arithmetic intensity
    arithmetic_intensity = numpy.logspace(START, STOP, num=N, base=2)

    # Hardware platforms
    platforms = [p[0] for p in hw_platforms]

    # Compute the rooflines
    achievable_performance = roofline(len(platforms),
                                      numpy.array([p[1] for p in hw_platforms]),
                                      numpy.array([p[2] for p in hw_platforms]),
                                      arithmetic_intensity)
    # Apps
    if sw_apps != []:
        apps = [a[0] for a in sw_apps]
        apps_intensity = numpy.array([a[1] for a in sw_apps])

    # Plot the graphs
    if xkcd:
        matplotlib.pyplot.xkcd()
    fig, axis = matplotlib.pyplot.subplots(1, 1)
    #for axis in axes:
    axis.set_xscale('log', basex=2)
    axis.set_yscale('log', basey=2)
    axis.set_xlabel('Arithmetic Intensity (FLOPS/byte)', fontsize=12)
    axis.grid(True, which='major')

    matplotlib.pyplot.setp(axis, xticks=arithmetic_intensity,
                           yticks=numpy.logspace(1, 20, num=20, base=2))

    axis.set_xticklabels(arithmetic_intensity)
    axis.set_yticklabels(numpy.logspace(1, 20, num=20, base=2))

    axis.set_ylabel("Achieveable Performance (GFLOPS/s)", fontsize=12)

    axis.set_title('Roofline Model GPU', fontsize=14)

    color = matplotlib.pyplot.cm.get_cmap("plasma", len(platforms)).colors
    for idx, val in enumerate(platforms):
        axis.plot(arithmetic_intensity, achievable_performance[idx, 0:],
                  label=val, marker='o', color=color[idx])

    # reset color map for vertical lines
    # number of apps must be less than or equal to number of roofs :(
    color = matplotlib.pyplot.cm.get_cmap("plasma", len(sw_apps)).colors
    if sw_apps != []:
        for idx, val in enumerate(apps):
            axis.axvline(apps_intensity[idx], label=val,
                         linestyle='-.', marker='x', color=color[idx])

    # this static inserting of scatters will hopefully be improved
    # axis.scatter([0.166 for x in y1], y1, c=color[0], label='matmul_flags', s=192)
    # axis.scatter([0.166 for x in z1], z1, c=color[4], label='matmul_loose', s=192)
    # axis.scatter([0.166 for x in w1], w1, c=color[8], label='matmul_inner', s=192)

    axis.legend()
    fig.tight_layout()
    matplotlib.pyplot.show()


def read_file(filename, row_len, csv_name):
    """
    Reads CSV file and returns a list of row_len-ary tuples
    """
    assert isinstance(row_len, int)
    elements = list()
    try:
        in_file = open(filename, 'r') if filename is not None else sys.stdin
        reader = csv.reader(in_file, dialect='excel')
        for row in reader:
            if len(row) != row_len:
                print("Error: Each row in %s must be contain exactly %d entries!"
                      % (csv_name, row_len), file=sys.stderr)
                sys.exit(1)
            element = tuple([row[0]] + [float(r) for r in row[1:]])
            elements.append(element)
        if filename is not None:
            in_file.close()
    except IOError as ex:
        print(ex, file=sys.stderr)
        sys.exit(1)
    return elements


def main():
    """
    main function
    """
    hw_platforms = list()
    apps = list()
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", metavar="hw_csv", help="HW platforms CSV file", type=str)
    parser.add_argument("-a", metavar="apps_csv", help="applications CSV file", type=str)
    parser.add_argument("--hw-only", action='store_true', default=False)
    parser.add_argument("--xkcd", action='store_true', default=False)

    args = parser.parse_args()
    # HW
    print("Reading HW characteristics...")
    hw_platforms = read_file(args.i, 3, "HW CSV")
    # apps
    if args.hw_only:
        print("Plotting only HW characteristics without any applications...")
        apps = list()
    else:
        print("Reading applications intensities...")
        apps = read_file(args.a, 2, "SW CSV")

    print(hw_platforms)
    print("Plotting using XKCD plot style is set to %s" % (args.xkcd))
    if apps != []:
        print(apps)
    process(hw_platforms, apps, args.xkcd)
    sys.exit(0)


if __name__ == "__main__":
    main()
