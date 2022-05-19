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

"""
    Use the example bpf_flfs_read_average kernel to interpret the entire
    contents of a file as 32 bits integers. Determines the amount of integers
    above RAND_MAX / 2 which should be around 67% of the entire file if filled
    with random data.

    This example only works for small files which result in a single read
    request.

    All kernels compiled along FluffleFS can be found in the qemu-csd/bin folder
    of the build directory.
"""

# 1M / 128k - 1048576
read_stride = 524288 # 1048528 # 131072

# import pdb; pdb.set_trace()

# Open the file and read it normally
fd = os.open("test/test", os.O_RDWR)
fsize = os.stat("test/test").st_size

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/bpf_flfs_read_average.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read_stream",
               bytes(f"{kern_ino}", "utf-8"))

# Check that the extended attribute is set
print(xattr.getxattr("test/test", "user.process.csd_read_stream"))

# Split the file reading into steps equal to the maximum stride for a single I/O
# request.
steps = int(fsize / read_stride)
if steps % read_stride is not 0:
    steps += 1

# Accumulate the total from each request
total = 0
for i in range(0, steps):
    total += int.from_bytes(
        os.pread(fd, read_stride, i * read_stride), "little")

# Print the total number of counted integers
print(total)

# Close the file, will release the kernel
os.close(fd)
