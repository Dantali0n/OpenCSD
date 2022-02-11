import os
import xattr

""" 
    Use the bpf_read kernel to let the filesystem read be performed by BPF and
    return the data as is. Useful to verify behavior of filesystem itself.

    This example only works for small files which result in a single read
    request.

"""

import pdb; pdb.set_trace()

# Open the file and read it normally.
# this file will be
fd = os.open("test/test", os.O_RDWR)
fsize = os.stat("test/test").st_size
print(os.read(fd, fsize))

# Reset read back to beginning of file
# os.lseek(fd, 0)

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/bpf_flfs_read.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read_stream",
               bytes(f"{kern_ino}", "utf-8"))

# Verify the BPF kernel is set
print(xattr.getxattr("test/test", "user.process.csd_read_stream"))

# Read the file again, this time the kernel will be executed
print(os.read(fd, fsize))

# Close the file will release the kernel
os.close(fd)
