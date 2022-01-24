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

    FuseLFSNlookup::FuseLFSNlookup() {
        rwlock_init(&inode_nlookup_lck, &inode_nlookup_attr,
                    "inode_nlookup_map");
    }

    FuseLFSNlookup::~FuseLFSNlookup() {
        rwlock_destroy(&inode_nlookup_lck, &inode_nlookup_attr,
                       "inode_nlookup_map");
    }

    /**
     * Increment nlookup for the given inode by either creating the map
     * entry or incrementing the existing one.
     */
    void FuseLFS::inode_nlookup_increment(fuse_ino_t ino) {
        // Get read lock
        pthread_rwlock_rdlock(&inode_nlookup_lck);

        auto it = inode_nlookup_map.find(ino);
        if(it == inode_nlookup_map.end()) {

            // Elevate lock to write
            pthread_rwlock_unlock(&inode_nlookup_lck);
            pthread_rwlock_wrlock(&inode_nlookup_lck);

            // Verify inode still not inserted, now x waits for infinity...
            it = inode_nlookup_map.find(ino);
            if(it == inode_nlookup_map.end())
                inode_nlookup_map.insert(std::make_pair(ino, 1));
            else
                // Increments are safe regardless of lock as they are atomics
                it->second += 1;
        }
        else {
            // Increments are safe regardless of lock as they are atomics
            it->second += 1;
        }

        // Always release lock
        pthread_rwlock_unlock(&inode_nlookup_lck);
    }

    /**
     * Decrement nlookup for the given inode by either destroying the map
     * entry or decrementing the existing one.
     */
    void FuseLFS::inode_nlookup_decrement(fuse_ino_t ino, uint64_t count) {
        // Get read lock
        pthread_rwlock_rdlock(&inode_nlookup_lck);
        auto it = inode_nlookup_map.find(ino);

        if(it == inode_nlookup_map.end()) {
            output.error("Requested to decrease nlookup count of inode that ",
                          "has already reached count zero!");
            pthread_rwlock_unlock(&inode_nlookup_lck);
            return;
        }

        #ifdef QEMUCSD_DEBUG
        if(count > it->second) {
            // Elevate lock to write
            pthread_rwlock_unlock(&inode_nlookup_lck);
            pthread_rwlock_wrlock(&inode_nlookup_lck);

            if(count > it->second) {
                output.error("Attempting to decrease nlookup count more than ",
                              "current value!");
                // Can only subtract the current count maximum
                count = it->second;
            }
        }
        #endif

        it->second -= count;

        // TODO(Dantali0n): Implement forget callbacks (unlink, rename, rmdir)
        //                  and fire these once this count reaches zero.
        if(it->second == 0) {
            // Elevate lock to write
            pthread_rwlock_unlock(&inode_nlookup_lck);
            pthread_rwlock_wrlock(&inode_nlookup_lck);

            if(it->second == 0)
                inode_nlookup_map.erase(it);
        }

        pthread_rwlock_unlock(&inode_nlookup_lck);
    }

    /**
     * Perform the necessary cache count management for inodes by tracking
     * the nlookup value by wrapping fuse_reply_entry.
     */
    void FuseLFS::fuse_reply_entry_nlookup(
            fuse_req_t req, struct fuse_entry_param *e)
    {
        inode_nlookup_increment(e->ino);
        //e->attr.st_nlink = inode_nlookup_map.find(e->ino)->second;
        fuse_reply_entry(req, e);
    }

    /**
     * Perform the necessary cache count management for inodes by tracking
     * the nlookup value by wrapping fuse_reply_create.
     */
    void FuseLFS::fuse_reply_create_nlookup(
            fuse_req_t req, struct fuse_entry_param *e,
            const struct fuse_file_info *f)
    {
        inode_nlookup_increment(e->ino);
        //e->attr.st_nlink = inode_nlookup_map.find(e->ino)->second;
        fuse_reply_create(req, e, f);
    }
}