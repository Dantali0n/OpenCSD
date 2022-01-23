/**
 * MIT License
 *
 * Copyright (c) 2021 Dantali0n
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef QEMU_CSD_FLFS_WRAPPER_HPP
#define QEMU_CSD_FLFS_WRAPPER_HPP

#define FUSE_USE_VERSION	36

extern "C" {
    #include <fuse3/fuse_lowlevel.h>
}

#include "arguments.hpp"
#include "flfs.hpp"

namespace qemucsd::fuse_lfs {

    class FuseLFSWrapper {
    protected:
        static const struct fuse_lowlevel_ops operations;
        static FuseLFS* flfs_w;
    public:
        static int initialize(int argc, char* argv[],
            arguments::options *options, nvme_zns::NvmeZnsBackend* nvme);
        static void init(void *userdata, struct fuse_conn_info *conn);
        static void destroy(void *userdata);
        static void lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
        static void forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup);
        static void getattr(fuse_req_t req, fuse_ino_t ino,
                            struct fuse_file_info *fi);
        static void setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                            int to_set, struct fuse_file_info *fi);
        static void readdir(
                fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                struct fuse_file_info *fi);
        static void open(fuse_req_t req, fuse_ino_t ino,
                         struct fuse_file_info *fi);
        static void release(fuse_req_t req, fuse_ino_t ino,
                            struct fuse_file_info *fi);
        static void create(fuse_req_t req, fuse_ino_t parent, const char *name,
                           mode_t mode, struct fuse_file_info *fi);
        static void mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
                          mode_t mode);
        static void read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                         struct fuse_file_info *fi);
        static void write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                          size_t size, off_t off, struct fuse_file_info *fi);
        static void statfs(fuse_req_t req, fuse_ino_t ino);
        static void fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
                          struct fuse_file_info *fi);
        static void rename(fuse_req_t req, fuse_ino_t parent, const char *name,
                           fuse_ino_t newparent, const char *newname,
                           unsigned int flags);
        static void unlink(fuse_req_t req, fuse_ino_t parent, const char *name);
        static void rmdir(fuse_req_t req, fuse_ino_t parent, const char *name);
        static void getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                             size_t size);
        static void setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                             const char *value, size_t size, int flags);
        static void listxattr(fuse_req_t req, fuse_ino_t ino, size_t size);
        static void removexattr(fuse_req_t req, fuse_ino_t ino,
                                const char *name);
    };
}

#endif // QEMU_CSD_FLFS_WRAPPER_HPP