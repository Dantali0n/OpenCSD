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
        rwlock_init(&open_inode_lck, &open_inode_attr, "open_inode_vect");
    }

    FuseLFSFileHandle::~FuseLFSFileHandle() {
        rwlock_destroy(&open_inode_lck, &open_inode_attr, "open_inode_vect");
    }

    /**
     * Create a uniquely identifying file handle for the inode and calling pid.
     * These are necessary for state management such as CSD operations and in
     * memory snapshots.
     * @threadsafety: thread safe
     */
    void FuseLFSFileHandle::create_file_handle(csd_unique_t *context,
        struct fuse_file_info *fi)
    {
        lock_guard<pthread_rwlock_t> guard(open_inode_lck, true);
        struct open_file_entry file_entry = {0};

        #ifdef QEMUCSD_DEBUG
        // Check for opening the same file multiple times within the same
        // process.
        if(find_file_handle_unsafe(context) != 0) {
            output.warning("Opening the same file multiple times within the ",
               "same process can lead to undefined behavior regarding CSD ",
               "operations!");
        }
        #endif

        fi->fh = fh_ptr;

        file_entry.fh = fi->fh;
        file_entry.ino = context->first;
        file_entry.pid = context->second;
        file_entry.flags = fi->flags;
        open_inode_vect.push_back(file_entry);

        if(fh_ptr == UINT64_MAX)
            output.fatal("Exhausted all possible file handles!");
        fh_ptr += 1;
    }

    /**
     * Use a csd_unique_t pair of inode and pid to find and return the
     * open_file_entry to the caller.
     * @threadsafety: thread safe
      * @return FLFS_RET_NONE upon success, FLFS_RET_ENOENT if not found
     */
    int FuseLFSFileHandle::get_file_handle(csd_unique_t *uni_t,
        struct open_file_entry *entry)
    {
        lock_guard<pthread_rwlock_t> guard(open_inode_lck);

        for(auto &f_entry : open_inode_vect) {
            if(uni_t->first == f_entry.ino && uni_t->second == f_entry.pid) {
                *entry = f_entry;
                return FLFS_RET_NONE;
            }
        }

        return FLFS_RET_ENOENT;
    }

    /**
     * Use a file handle to find and return the open_file_entry to the caller.
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ENOENT if not found
     */
    int FuseLFSFileHandle::get_file_handle(uint64_t fh,
        struct open_file_entry *entry)
    {
        lock_guard<pthread_rwlock_t> guard(open_inode_lck);

        for(auto &f_entry : open_inode_vect) {
            if(fh == f_entry.fh) {
                *entry = f_entry;
                return FLFS_RET_NONE;
            }
        }

        return FLFS_RET_ENOENT;
    }

    /**
     * Update an existing file handle with new information
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFSFileHandle::update_file_handle(uint64_t fh,
        struct open_file_entry *entry)
    {
        lock_guard<pthread_rwlock_t> guard(open_inode_lck, true);

        open_inode_vect_t::iterator it;
        find_file_handle_unsafe(fh, &it);
        if(it == open_inode_vect.end()) {
            return FLFS_RET_ERR;
        }

        open_inode_vect.at(std::distance(open_inode_vect.begin(), it)) = *entry;

        return FLFS_RET_NONE;
    }

    /**
     * Use a csd_unique_t pair of inode and pid to find the iterator index in
     * the open_inode_vect and return this to the caller.
     * @threadsafety: single threaded
     * @return 1 if the file handle was found, 0 otherwise
     */
    int FuseLFSFileHandle::find_file_handle_unsafe(csd_unique_t *uni_t) {
        auto it = std::find_if(open_inode_vect.begin(), open_inode_vect.end(),
           [&uni_t](const struct open_file_entry& entry)
        {
            return uni_t->first == entry.ino && uni_t->second == entry.pid;
        });

        return it == open_inode_vect.end() ? 0 : 1;
    }

    /**
     * Use a csd_unique_t pair of inode and pid to find the iterator index in
     * the open_inode_vect and return this to the caller.
     * @threadsafety: thread safe
     * @return 1 if the file handle was found, 0 otherwise
     */
    int FuseLFSFileHandle::find_file_handle(csd_unique_t *uni_t)
    {
        lock_guard<pthread_rwlock_t> guard(open_inode_lck);

        int result = find_file_handle_unsafe(uni_t);

        return result;
    }

    /**
     * Use an open file handle to find the iterator index in the
     * open_inode_Vect and return this to the caller.
     * @threadsafety: single threaded
     */
    void FuseLFSFileHandle::find_file_handle_unsafe(uint64_t fh,
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
        lock_guard<pthread_rwlock_t> guard(open_inode_lck);

        open_inode_vect_t::iterator it;
        find_file_handle_unsafe(fh, &it);

        return it == open_inode_vect.end() ? 0 : 1;
    }

    /**
     * Called be release to remove the session for a file handle
     * @threadsafety: thread safe
     */
    void FuseLFSFileHandle::release_file_handle(uint64_t fh) {
        lock_guard<pthread_rwlock_t> guard(open_inode_lck, true);

        open_inode_vect_t::iterator del_key;
        find_file_handle_unsafe(fh, &del_key);

        if(del_key != open_inode_vect.end())
            open_inode_vect.erase(del_key);
    }
}