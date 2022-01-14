import os
import xattr

import pdb; pdb.set_trace()

# Open the file and read it normally
f = open("test/test", "rb")
print(f.read())

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/kernel").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read", f"{kern_ino}")

# Read the file again, this time the kernel will be executed
print(f.read())

# Close the file will release the kernel
f.close()
