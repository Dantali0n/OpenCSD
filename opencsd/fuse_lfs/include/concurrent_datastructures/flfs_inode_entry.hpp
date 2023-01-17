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

#ifndef QEMU_CSD_FLFS_INODE_ENTRY_HPP
#define QEMU_CSD_FLFS_INODE_ENTRY_HPP

extern "C" {
    #include "fuse3/fuse_lowlevel.h"
    #include <pthread.h>
}

#include <cstddef>
#include <cstdint>
#include <vector>

#include "synchronization/flfs_rwlock.hpp"
#include "flfs_memory.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * Interface for inode_entries unflushed inode methods
     */
    class FuseLFSInodeEntry {
    protected:
        inode_entries_t inode_entries;

        // Concurrency management for inode_lba_map
        pthread_rwlock_t inode_entries_lck = {};
        pthread_rwlockattr_t inode_entries_attr = {};
    public:
        FuseLFSInodeEntry();
        virtual ~FuseLFSInodeEntry();

        int get_inode_entry(fuse_ino_t ino, inode_entry_t *entry);

        void update_inode_entry(inode_entry_t *entry);

        void erase_inode_entries(std::vector<fuse_ino_t> *ino_remove);

        int fill_inode_block(std::vector<fuse_ino_t> *ino_remove,
            struct inode_block *blk);
    };

}

#endif // QEMU_CSD_FLFS_INODE_ENTRY_HPP