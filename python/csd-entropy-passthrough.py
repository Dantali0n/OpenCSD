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

import pdb; pdb.set_trace()

# Open the file we are going to compute shannon entropy on
fr = open("test/test", "rb+")

# Get the inode number for the BPF kernel
kern_ino = os.stat("test/bpf_flfs_read_entropy.o").st_ino

# Enable the BPF kernel on the open file for read operations
xattr.setxattr("test/test", "user.process.csd_read",
               bytes(f"{kern_ino}", "utf-8"))

print(xattr.getxattr("test/test", "user.process.csd_read"))

fr.seek(0)

# Determine the size of the file
data_size = os.stat("test/test").st_size
# Read the result from the kernel which will be 256 unsigned integers.
data = struct.unpack('<256i', fr.read(data_size))

ent = 0.0
for f in data:
    if f > 0:
        freq = float(f) / data_size
        ent = ent + freq * math.log(freq, 2)
ent = -ent

print('Shannon entropy (min bits per byte-character):', ent)
print('Min possible file size assuming max theoretical compression efficiency:')
print((ent * data_size) / 8, 'bytes')

# Close the file will release the kernel
fr.close()
