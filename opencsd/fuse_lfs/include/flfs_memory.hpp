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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

extern "C" {
    #include <fuse3/fuse_lowlevel.h>
}

#include "flfs_disc.hpp"

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
        fuse_ino_t read_stream_kernel;
        fuse_ino_t write_stream_kernel;
        fuse_ino_t read_event_kernel;
        fuse_ino_t write_event_kernel;
    };

    struct lba_inode {
        uint64_t parent;
        uint64_t lba;
        std::shared_ptr<std::mutex> l;
    };

    // Keep track of the number of nlookups per inode.
    // Increases by one for every call to fuse_reply_entry & fuse_reply_create
    // Decreases by calls to forget.
    // Scheduled unlink, rmdir or rename operations can only be flushed when
    // count reaches 0. Count must be able to go negative due to reordering /
    // non-determinism of inode locks. Call to forget could be performed before
    // all calls to lookup are finished making the count go negative temporarily
    typedef std::map<fuse_ino_t, std::atomic<int64_t>> inode_nlookup_map_t;

    // Names and their inodes for use in path_inode_map_t
    // TODO(Dantali0n): Investigate if memory consumption will be beyond control
    typedef std::map<std::string, fuse_ino_t> path_map_t;

    // Map path sections to their corresponding inode
    typedef std::map<fuse_ino_t, path_map_t*> path_inode_map_t;

    // Map corresponding inodes to the lba storing the inode_block
    typedef std::map<fuse_ino_t, struct lba_inode> inode_lba_map_t;

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
    typedef std::pair<struct inode_entry, std::string> inode_entry_t;

    // A map of inode_entry and name that must be flushed to drive
    typedef std::map<fuse_ino_t, inode_entry_t> inode_entries_t;

    // A map of the block number and its corresponding data_block
    typedef std::map<uint64_t, struct data_block> data_map_t;

    // A map of data_blocks that must be flushed to drive
    typedef std::map<fuse_ino_t, data_map_t*> data_blocks_t;

    /**
     * Datastructures for in-memory snapshots
     */

    struct snapshot {
        inode_entry_t inode_data;
        data_map_t data_blocks;
    };

    struct csd_snapshot {
        struct snapshot file;
        struct snapshot read_stream_kernel;
        struct snapshot write_stream_kernel;
        struct snapshot read_event_kernel;
        struct snapshot write_event_kernel;
    };
}

#endif //QEMU_CSD_FLFS_MEMORY_HPP
