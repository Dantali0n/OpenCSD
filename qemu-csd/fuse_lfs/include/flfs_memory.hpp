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

#ifndef QEMU_CSD_FLFS_MEMORY_HPP
#define QEMU_CSD_FLFS_MEMORY_HPP

#include <set>

#include "flfs_constants.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * This header contains data structures to be stored in memory by fuse_lfs
     */

    /**
     * Directory buffer used to fill directory information before it is
     * returned to the caller. See dir_buf_add
     */
    struct dir_buf {
        char* p;
        size_t size;
    };


    /**
     * Keep track of open files and their state, including CSD state such as
     * if kernels are enabled and which ones.
     */
    struct open_file_entry {
        uint64_t fh;
        fuse_ino_t ino;
        pid_t pid;
        int flags;
        fuse_ino_t csd_read_kernel;
        fuse_ino_t csd_write_kernel;
    };

    // Keep track of the number of nlookups per inode.
    // Increases by one for every call to fuse_reply_entry & fuse_reply_create
    // Decreases by calls to forget.
    // Scheduled unlink, rmdir or rename operations can only be flushed when
    // count reaches 0.
    typedef std::map<fuse_ino_t, uint64_t> inode_nlookup_map_t;

    // Names and their inodes for use in path_inode_map_t
    // TODO(Dantali0n): Investigate if memory consumption will be beyond control
    typedef std::map<std::string, fuse_ino_t> path_map_t;

    // Map path sections to their corresponding inode
    typedef std::map<fuse_ino_t, path_map_t*> path_inode_map_t;

    // Map corresponding inodes to the lba storing the inode_block
    typedef std::map<fuse_ino_t, uint64_t> inode_lba_map_t;

    // Pair of data that is interpreted as unique for the CSD context
    typedef std::pair<fuse_ino_t, pid_t> csd_unique_t;

    // Unique handles corresponding to open file entries
    // TODO(Dantali0n): Extend this to prevent additional lookups for open files
    typedef std::vector<struct open_file_entry> open_inode_vect_t;

    /**
     * In memory datastructures for synchronizing between memory and drive. That
     * means any data in these pending datastructures are pending and are not on
     * the drive yet or need to be updated.
     */

    // A set of inodes that have been updated since flush and must be written
    typedef std::set<fuse_ino_t> nat_update_set_t;

    // An inode_entry combined with its name into a pair as inode_entry_t
    typedef std::pair<inode_entry, std::string> inode_entry_t;

    // A map of inode_entry and name that must be flushed to drive
    typedef std::map<fuse_ino_t, inode_entry_t> inode_entries_t;

    // A map of the block number and its corresponding data_block
    typedef std::map<uint64_t, struct data_block> data_map_t;

    // A map of data_blocks that must be flushed to drive
    typedef std::map<fuse_ino_t, data_map_t*> data_blocks_t;
}

#endif //QEMU_CSD_FLFS_MEMORY_HPP
