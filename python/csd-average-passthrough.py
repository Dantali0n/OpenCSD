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

import pdb; pdb.set_trace()

# Open the file and read it normally
fr = open("test/test", "rb+")
print(fr.read())

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/bpf_flfs_read_average.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read",
               bytes(f"{kern_ino}", "utf-8"))

# Check that the extended attribute is set
print(xattr.getxattr("test/test", "user.process.csd_read"))

# Seek back to beginning of file
fr.seek(0)

# Read the file, executing the kernel and interpret the result as an integer
print(int.from_bytes(fr.read(), "little"))

# Close the file will release the kernel
fr.close()
