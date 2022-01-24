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

#include "flfs.hpp"

namespace qemucsd::fuse_lfs {

    FuseLFSInodeLba::FuseLFSInodeLba() {
        // Initialize the lock attributes
        if(pthread_rwlockattr_init(&inode_map_attr) != 0) {
            output.error("Failed to initialize inode_lba_map lock"
                         "attributes");
        }

        // Disable support for recursive reads but enable writer preference
        if(pthread_rwlockattr_setkind_np(&inode_map_attr,
             PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP) != 0)
        {
            output.error("Invalid arguments for configuring inode_lba_map ",
                         "reader writer lock");
        }

        // Initialize the reader writer lock
        if(pthread_rwlock_init(&inode_map_lck, &inode_map_attr) != 0) {
            output.error("Failed to initialize inode_nlookup_map reader ",
                         "writer lock");
        }
    }

    FuseLFSInodeLba::~FuseLFSInodeLba() {
        if(pthread_rwlockattr_destroy(&inode_map_attr) != 0) {
            output.error("Failed to destroy inode_map_attr");
        }

        if(pthread_rwlock_destroy(&inode_map_lck)!= 0) {
            output.error("Failed to destroy inode_map_lck");
        }
    }

    uint64_t FuseLFSInodeLba::inode_lba_size() {
        pthread_rwlock_rdlock(&inode_map_lck);
        uint64_t size = inode_lba_map.size();
        pthread_rwlock_unlock(&inode_map_lck);
        return size;
    }

    /**
     * Get the inode_lba_map information of an inode and populate the callers
     * field with it.
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ENOENT if not found
     */
    int FuseLFSInodeLba::get_inode_lba(
        fuse_ino_t ino, struct lba_inode *data)
    {
        pthread_rwlock_rdlock(&inode_map_lck);

        auto it = inode_lba_map.find(ino);
        if(it == inode_lba_map.end()) {
            pthread_rwlock_unlock(&inode_map_lck);
            return FLFS_RET_ENOENT;
        }

        *data = it->second;

        pthread_rwlock_unlock(&inode_map_lck);
        return FLFS_RET_NONE;
    }

    /**
     * Lock a given inode
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ENOENT if not found
     */
    int FuseLFSInodeLba::lock_inode(fuse_ino_t ino) {
        struct lba_inode cur_lba;
        if(get_inode_lba(ino, &cur_lba) != FLFS_RET_NONE)
            return FLFS_RET_ENOENT;

        cur_lba.l->lock();
        return FLFS_RET_NONE;
    }

    /**
     * Unlock a given inode
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ENOENT if not found
     */
    int FuseLFSInodeLba::unlock_inode(fuse_ino_t ino) {
        struct lba_inode cur_lba;
        if(get_inode_lba(ino, &cur_lba) != FLFS_RET_NONE)
            return FLFS_RET_ENOENT;

        cur_lba.l->unlock();
        return FLFS_RET_NONE;
    }

    /**
     * Update an lba_inode in the inode_lba_map. the shared_ptr mutex is always
     * retained and subsequently exposed to the caller.
     * @threadsafety: thread safe, course grained concurrency
     */
    void FuseLFSInodeLba::update_inode_lba(fuse_ino_t ino,
        struct lba_inode *data)
    {
        // Caller might expect std::mutex to be valid lock object after
        // insertion
        struct lba_inode cur_lba = {0, 0, data->l};

        pthread_rwlock_wrlock(&inode_map_lck);

        auto it = inode_lba_map.find(ino);
        if(it != inode_lba_map.end())
            cur_lba = it->second;

        cur_lba.lba = data->lba;
        cur_lba.parent = data->parent;
        inode_lba_map.insert_or_assign(ino, cur_lba);

        // If object already existed replace the non copyable lock shared_ptr so
        // it is actually valid
        data->l = cur_lba.l;

        pthread_rwlock_unlock(&inode_map_lck);
    }

    /**
     * Update an inode_lba_map_t with the provided lba for the vector of inodes
     * @threadsafety: thread safe, course grained concurrency
     */
    void FuseLFSInodeLba::update_inode_lba_map(std::vector<fuse_ino_t> *inodes,
        uint64_t lba)
    {
        pthread_rwlock_wrlock(&inode_map_lck);

        for(auto &ino : *inodes) {
            #ifdef QEMUCSD_DEBUG
            // Inode 0 is regarded as invalid
            // Inode 1 is hardcoded for root and should not be updated
            if(ino < 2) {
                output.error("Invalid attempt to insert inode ", ino, " into ",
                             "a inode_lba_map_t");
                continue;
            }
            #endif

            // It is absolutely critical that the mutex be constructed inside
            // the loop. Otherwise, multiple inodes would share the same mutex!
            struct lba_inode cur_lba = {0, 0, std::make_shared<std::mutex>()};

            auto it = inode_lba_map.find(ino);
            if(it != inode_lba_map.end())
                cur_lba = it->second;

            cur_lba.lba = lba;
            inode_lba_map.insert_or_assign(ino, cur_lba);
        }

        pthread_rwlock_unlock(&inode_map_lck);
    }
}