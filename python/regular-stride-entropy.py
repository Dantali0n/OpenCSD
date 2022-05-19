import sys
import math
import os

# 1M / 128k - 1048576
# Keep this the same as CSDs
read_stride = 524288 # 1048528 # 131072

if len(sys.argv) != 2:
    print('Usage: file_entropy.py [path]filename')
    sys.exit()

fd = os.open(sys.argv[1], os.O_RDWR)
fsize = os.stat(sys.argv[1]).st_size

# Split the file reading into steps equal to the maximum stride for a single I/O
# request.
steps = int(fsize / read_stride)
if steps % read_stride != 0:
    steps += 1

freqList = [0] * 256

for i in range(0, steps):
    byteArr = bytearray(os.pread(fd, read_stride, i * read_stride))
    for b in byteArr:
        freqList[b] += 1

print('File size in bytes: ', fsize)
os.close(fd)

ent = 0.0
for f in freqList:
    if f > 0:
        freq = float(f) / fsize
        ent = ent + freq * math.log(freq, 2)
ent = -ent

print('Shannon entropy (min bits per byte-character):', ent)
print('Min possible file size assuming max theoretical compression efficiency:')
print((ent * fsize) / 8, 'bytes')