import xattr

def enable_csd_kernel():
    xattr.setxattr("test/test", "user.process.csd_read", "yes")

import pdb; pdb.set_trace()
f = open("test/test", "rb")
print(f.read())
f.close()
