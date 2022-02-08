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
f = open("test/test", "rb")
print(f.read())

# Reset read back to beginning of file
f.seek(0)

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/flfs_bpf_read.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read",
               bytes(f"{kern_ino}", "utf-8"))

# Verify the BPF kernel is set
print(xattr.getxattr("test/test", "user.process.csd_read"))

# Read the file again, this time the kernel will be executed
print(f.read())

# Close the file will release the kernel
f.close()
