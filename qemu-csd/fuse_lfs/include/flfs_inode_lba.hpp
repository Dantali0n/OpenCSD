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

#ifndef QEMU_CSD_FLFS_INODE_LBA_HPP
#define QEMU_CSD_FLFS_INODE_LBA_HPP

extern "C" {
    #include <fuse3/fuse_lowlevel.h>
    #include <pthread.h>
}

#include <cstddef>
#include <vector>

#include "flfs_memory.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * Interface for inode_lba_map methods
     */
    class FuseLFSInodeLba {
    protected:
        // Keep track of nlookup count per inode
        inode_lba_map_t inode_lba_map;

        // Concurrency management for inode_lba_map
        pthread_rwlock_t inode_map_lck;
        pthread_rwlockattr_t inode_map_attr;
    public:
        FuseLFSInodeLba();
        virtual ~FuseLFSInodeLba();

        uint64_t inode_lba_size();

        int get_inode_lba(fuse_ino_t ino, struct lba_inode *data);

        int lock_inode(fuse_ino_t ino);
        int unlock_inode(fuse_ino_t ino);

        void update_inode_lba(fuse_ino_t ino, struct lba_inode *data);

        void update_inode_lba_map(std::vector<fuse_ino_t> *inodes,
            uint64_t lba);
    };

}

#endif // QEMU_CSD_FLFS_INODE_LBA_HPP