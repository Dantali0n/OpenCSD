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

    FuseLFSFileHandle::FuseLFSFileHandle() {
        // Initialize the lock attributes
        if(pthread_rwlockattr_init(&open_inode_attr) != 0) {
            output.error("Failed to initialize open_inode_vect lock"
                         "attributes");
        }

        // Disable support for recursive reads but enable writer preference
        if(pthread_rwlockattr_setkind_np(&open_inode_attr,
            PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP) != 0)
        {
            output.error("Invalid arguments for configuring open_inode_vect ",
                         "lock");
        }

        // Initialize the reader writer lock
        if(pthread_rwlock_init(&open_inode_lck, &open_inode_attr) != 0) {
            output.error("Failed to initialize open_inode_vect lock");
        }
    }

    FuseLFSFileHandle::~FuseLFSFileHandle() {
        if(pthread_rwlockattr_destroy(&open_inode_attr) != 0) {
            output.error("Failed to destroy open_inode_attr");
        }

        if(pthread_rwlock_destroy(&open_inode_lck)!= 0) {
            output.error("Failed to destroy open_inode_lck");
        }
    }

    /**
     * Create a uniquely identifying file handle for the inode and calling pid.
     * These are necessary for state management such as CSD operations and in
     * memory snapshots.
     * @threadsafety: thread safe
     */
    void FuseLFSFileHandle::create_file_handle(
        fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
    {
        const fuse_ctx *context = fuse_req_ctx(req);
        struct open_file_entry file_entry = {0};

        pthread_rwlock_wrlock(&open_inode_lck);

        #ifdef QEMUCSD_DEBUG
        // Check for opening the same file multiple times within the same
        // process.
        csd_unique_t csd_uni = std::make_pair(ino, context->pid);
        if(find_file_handle(&csd_uni) != 0) {
            output.warning("Opening the same file multiple times within the ",
               "same process can lead to undefined behavior regarding CSD ",
               "operations!");
        }
        #endif

        fi->fh = fh_ptr;

        file_entry.fh = fi->fh;
        file_entry.ino = ino;
        file_entry.pid = context->pid;
        file_entry.flags = fi->flags;
        open_inode_vect.push_back(file_entry);

        if(fh_ptr == UINT64_MAX)
            output.fatal("Exhausted all possible file handles!");
        fh_ptr += 1;

        pthread_rwlock_unlock(&open_inode_lck);
    }

    /**
     * Use a csd_unique_t pair of inode and pid to find and return the
     * open_file_entry to the caller.
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFSFileHandle::get_file_handle(csd_unique_t *uni_t,
        struct open_file_entry *entry)
    {
        pthread_rwlock_rdlock(&open_inode_lck);

        for(auto &f_entry : open_inode_vect) {
            if(uni_t->first == f_entry.ino && uni_t->second == f_entry.pid) {
                *entry = f_entry;
                pthread_rwlock_unlock(&open_inode_lck);
                return FLFS_RET_NONE;
            }
        }

        pthread_rwlock_unlock(&open_inode_lck);
        return FLFS_RET_ERR;
    }

    /**
     * Use a file handle to find and return the open_file_entry to the caller.
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFSFileHandle::get_file_handle(uint64_t fh,
        struct open_file_entry *entry)
    {
        pthread_rwlock_rdlock(&open_inode_lck);

        for(auto &f_entry : open_inode_vect) {
            if(fh == f_entry.fh) {
                *entry = f_entry;
                pthread_rwlock_unlock(&open_inode_lck);
                return FLFS_RET_NONE;
            }
        }

        pthread_rwlock_unlock(&open_inode_lck);
        return FLFS_RET_ERR;
    }

    /**
     * Update an existing file handle with new information
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFSFileHandle::update_file_handle(uint64_t fh,
        struct open_file_entry *entry)
    {
        pthread_rwlock_wrlock(&open_inode_lck);

        open_inode_vect_t::iterator it;
        find_file_handle(fh, &it);
        if(it == open_inode_vect.end()) {
            pthread_rwlock_unlock(&open_inode_lck);
            return FLFS_RET_ERR;
        }

        open_inode_vect.at(std::distance(open_inode_vect.begin(), it)) = *entry;

        pthread_rwlock_unlock(&open_inode_lck);
        return FLFS_RET_NONE;
    }

    /**
     * Use a csd_unique_t pair of inode and pid to find the iterator index in
     * the open_inode_vect and return this to the caller.
     * @threadsafety: thread safe
     * @return 1 if the file handle was found, 0 otherwise
     */
    int FuseLFSFileHandle::find_file_handle(csd_unique_t *uni_t)
    {
        pthread_rwlock_rdlock(&open_inode_lck);

        auto it = std::find_if(open_inode_vect.begin(), open_inode_vect.end(),
           [&uni_t](const struct open_file_entry& entry)
        {
           return uni_t->first == entry.ino && uni_t->second == entry.pid;
        });

        pthread_rwlock_unlock(&open_inode_lck);
        return it == open_inode_vect.end() ? 0 : 1;
    }

    /**
     * Use an open file handle to find the iterator index in the
     * open_inode_Vect and return this to the caller.
     * @threadsafety: single threaded
     * @return 1 if the file handle was found, 0 otherwise
     */
    void FuseLFSFileHandle::find_file_handle(uint64_t fh,
        open_inode_vect_t::iterator *it)
    {
        *it = std::find_if(open_inode_vect.begin(), open_inode_vect.end(),
            [&fh](const struct open_file_entry& entry)
        {
            return fh == entry.fh;
        });
    }

    /**
     * Use an open file handle to find the iterator index in the
     * open_inode_Vect and return this to the caller.
     * @threadsafety: thread safe
     * @return 1 if the file handle was found, 0 otherwise
     */
    int FuseLFSFileHandle::find_file_handle(uint64_t fh) {
        pthread_rwlock_rdlock(&open_inode_lck);

        open_inode_vect_t::iterator it;
        find_file_handle(fh, &it);

        pthread_rwlock_unlock(&open_inode_lck);
        return it == open_inode_vect.end() ? 0 : 1;
    }

    /**
     * Called be release to remove the session for a file handle
     * @threadsafety: thread safe
     */
    void FuseLFSFileHandle::release_file_handle(uint64_t fh) {
        pthread_rwlock_wrlock(&open_inode_lck);

        open_inode_vect_t::iterator del_key;
        find_file_handle(fh, &del_key);

        if(del_key != open_inode_vect.end())
            open_inode_vect.erase(del_key);

        pthread_rwlock_unlock(&open_inode_lck);
    }
}