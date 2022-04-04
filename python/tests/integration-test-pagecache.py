import os
import xattr

"""
    Check that reading regularly followed by immediately reading offloaded
    still results in two distinct read events instead of one going to cache.
    https://www.kernel.org/doc/html/latest/filesystems/fuse-io.html
"""

# 1M / 128k - 1048576
read_stride = 524288 # 1048528 # 131072

# import pdb; pdb.set_trace()

# Open the file
fd = os.open("test/test", os.O_RDWR)
fsize = os.stat("test/test").st_size

# Read the file regularly
print(os.pread(fd, read_stride, 0))

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/bpf_flfs_read.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read_stream",
               bytes(f"{kern_ino}", "utf-8"))

# Verify the BPF kernel is set
print(xattr.getxattr("test/test", "user.process.csd_read_stream"))

# Read the file with offloading
print(os.pread(fd, read_stride, 0))

# Close the file will release the kernel
os.close(fd)
