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

import os
import xattr
import struct
import math

"""
    Use the example bpf_flfs_read_entropy kernel to compute the amount of
    entropy in a given file. This python scripts uses test as an example file.
    The FluffleFS filesystem is assumed to be mounted on the test directory
    relative from the users current directory. The result of the kernel will
    return 256 bins which will be used to compute the entropy.

    This example only works for small files which result in a single read
    request.
"""

# 1M / 128k - 1048576
read_stride = 524288 # 1048528 # 131072

# import pdb; pdb.set_trace()

# Open the file we are going to compute shannon entropy on
fd = os.open("test/test", os.O_RDWR)
# Determine the size of the file
fsize = os.stat("test/test").st_size

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/bpf_flfs_read_entropy.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read_stream",
               bytes(f"{kern_ino}", "utf-8"))

print(xattr.getxattr("test/test", "user.process.csd_read_stream"))


# Split the file reading into steps equal to the maximum stride for a single I/O
# request.
steps = int(fsize / read_stride)
if fsize % read_stride != 0:
    steps += 1

# Create totality bins for entropy calculation
final_bins = [0] * 256

# Accumulate data for each strided request into the bins
for i in range(0, steps):
    data = struct.unpack('<256i', os.pread(fd, read_stride, i * read_stride))
    for j in range(0, 256):
        final_bins[j] += data[j]

# Compute entropy
ent = 0.0
for f in final_bins:
    if f > 0:
        freq = float(f) / fsize
        ent = ent + freq * math.log(freq, 2)
ent = -ent

# Print results
print('Shannon entropy (min bits per byte-character):', ent)
print('Min possible file size assuming max theoretical compression efficiency:')
print((ent * fsize) / 8, 'bytes')

# Close the file will release the kernel
os.close(fd)
