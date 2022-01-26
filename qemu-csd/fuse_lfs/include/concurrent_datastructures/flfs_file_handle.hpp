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

#ifndef QEMU_CSD_FLFS_FILE_HANDLE_HPP
#define QEMU_CSD_FLFS_FILE_HANDLE_HPP

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
     * Interface for open_inode_vect file handle methods
     */
    class FuseLFSFileHandle {
    protected:
        // File handle pointer for open files
        uint64_t fh_ptr = 0;

        // Keep track of open files and directories using unique handles for
        // respective inodes and caller pids.
        open_inode_vect_t open_inode_vect;

        // Concurrency management for open_inode_vect
        pthread_rwlock_t open_inode_lck = {};
        pthread_rwlockattr_t open_inode_attr = {};

        void find_file_handle(uint64_t fh, open_inode_vect_t::iterator *it);
    public:
        FuseLFSFileHandle();
        virtual ~FuseLFSFileHandle();

        void create_file_handle(fuse_req_t req, fuse_ino_t ino,
            struct fuse_file_info *fi);

        int get_file_handle(csd_unique_t *uni_t, struct open_file_entry *entry);

        int get_file_handle(uint64_t fh, struct open_file_entry *entry);

        int update_file_handle(uint64_t fh, struct open_file_entry *entry);

        int find_file_handle_unsafe(csd_unique_t *uni_t);

        int find_file_handle(csd_unique_t *uni_t);

        int find_file_handle(uint64_t fh);

        void release_file_handle(uint64_t fh);
    };

}

#endif // QEMU_CSD_FLFS_FILE_HANDLE_HPP