/**
* MIT License
*
* Copyright (c) 2022 Dantali0n
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

#ifndef QEMU_CSD_FLFS_READ_HPP
#define QEMU_CSD_FLFS_READ_HPP

extern "C" {
    #include <fuse3/fuse_lowlevel.h>
}

#include <cstdio>
#include <cstddef>
#include <cstdint>

#include "flfs_memory.hpp"

namespace qemucsd::fuse_lfs {

    class FuseLFSRead {
    public:
        virtual int read_precheck(fuse_req_t req, struct inode_entry entry,
            size_t &size, off_t &offset);

        virtual void read_regular(fuse_req_t req, struct stat *stbuf,
            size_t size, off_t off, struct fuse_file_info *fi) = 0;

        virtual int read_snapshot(csd_unique_t *context, size_t size,
            off_t off, void *buffer, struct snapshot *snap) = 0;
    };

}

#endif //QEMU_CSD_FLFS_READ_HPP