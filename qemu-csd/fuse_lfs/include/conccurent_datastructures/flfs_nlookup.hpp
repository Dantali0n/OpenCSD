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

#ifndef QEMU_CSD_FLFS_NLOOKUP_HPP
#define QEMU_CSD_FLFS_NLOOKUP_HPP

extern "C" {
    #include "fuse3/fuse_lowlevel.h"
    #include <pthread.h>
}

#include <cstddef>

#include "flfs_memory.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * Interface for nlookup methods
     */
    class FuseLFSNlookup {
    protected:
        // Keep track of nlookup count per inode
        inode_nlookup_map_t *inode_nlookup_map;

        // Concurrency management for inode_lnookup_map
        pthread_rwlock_t inode_nlookup_lck;
        pthread_rwlockattr_t inode_nlookup_attr;
    public:
        FuseLFSNlookup();
        virtual ~FuseLFSNlookup();

        virtual void inode_nlookup_increment(fuse_ino_t ino) = 0;

        virtual void inode_nlookup_decrement(fuse_ino_t ino, uint64_t count) = 0;

        virtual void fuse_reply_entry_nlookup(
            fuse_req_t req, struct fuse_entry_param *e) = 0;

        virtual void fuse_reply_create_nlookup(
            fuse_req_t req, struct fuse_entry_param *e,
            const struct fuse_file_info *f) = 0;
    };

}

#endif // QEMU_CSD_FLFS_NLOOKUP_HPP