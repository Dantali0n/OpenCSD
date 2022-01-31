import os
import xattr

"""
"""

import pdb; pdb.set_trace()

# Open the file and read it normally
fr = open("test/test", "rb+")
print(fr.read())

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/bpf_flfs_read_average.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read", f"{kern_ino}")

print(xattr.getxattr("test/test", "user.process.csd_read"))

# Seek back to beginning of file
fr.seek(0)

# Read the file again
print(int.from_bytes(fr.read(), "little"))

# Close the file will release the kernel
fr.close()
