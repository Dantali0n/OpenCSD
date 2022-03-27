import os
import xattr

""" 
    Use the bpf_read kernel to let the filesystem read be performed by BPF and
    return the data as is. Useful to verify behavior of filesystem itself.

    This example only works for small files which result in a single read
    request.

"""

# 1M / 128k - 1048576
read_stride = 524288 # 1048528 # 131072

# import pdb; pdb.set_trace()

# Open the file
fd = os.open("test/test", os.O_RDWR)
fsize = os.stat("test/test").st_size

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/bpf_flfs_read.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read_stream",
               bytes(f"{kern_ino}", "utf-8"))

# Verify the BPF kernel is set
print(xattr.getxattr("test/test", "user.process.csd_read_stream"))

# Split the file reading into steps equal to the maximum stride for a single I/O
# request.
steps = int(fsize / read_stride)
if steps % read_stride is not 0:
    steps += 1

# Read the file
total = 0
for i in range(0, steps):
    print(os.pread(fd, read_stride, i * read_stride))

# Close the file will release the kernel
os.close(fd)
