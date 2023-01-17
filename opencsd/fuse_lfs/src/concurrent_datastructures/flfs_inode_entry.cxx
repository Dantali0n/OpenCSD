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

    FuseLFSInodeEntry::FuseLFSInodeEntry() {
        rwlock_init(&inode_entries_lck, &inode_entries_attr, "inode_entries");
    }

    FuseLFSInodeEntry::~FuseLFSInodeEntry() {
        rwlock_destroy(&inode_entries_lck, &inode_entries_attr, "inode_entries");
    }

    /**
     * Retrieve any inode if present in the inode_entries datastructure
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, FLFS_RET_ENOENT if not found
     */
    int FuseLFSInodeEntry::get_inode_entry(fuse_ino_t ino,
        inode_entry_t *entry)
    {
        lock_guard<pthread_rwlock_t> guard(inode_entries_lck);

        auto it = inode_entries.find(ino);
        if(it != inode_entries.end()) {
            *entry = it->second;
            return FLFS_RET_NONE;
        }

        return FLFS_RET_ENOENT;
    }

    /**
     * Update any given inode by adding or overriding the new data to
     * inode_entries
     * @threadsafety: thread safe
     */
    void FuseLFSInodeEntry::update_inode_entry(inode_entry_t *entry) {
        lock_guard<pthread_rwlock_t> guard(inode_entries_lck, true);

        // This overrides the inode_entry in case the previous one hadn't
        // flushed to drive yet.
        inode_entries.insert_or_assign(entry->first.inode,
            std::make_pair(entry->first, entry->second));
    }

    /**
     * Remove the inode_entries present in ino_remove
     * @threadsafety: thread safe
     */
    void FuseLFSInodeEntry::erase_inode_entries(
        std::vector<fuse_ino_t> *ino_remove)
    {
        lock_guard<pthread_rwlock_t> guard(inode_entries_lck, true);

        for(auto &inode : *ino_remove)
            inode_entries.erase(inode);
    }

    /**
     * Fill an inode_block for entries and remove every entry added to block
     * @threadsafety: thread safe, the read lock should not be necessary as this
     *                function is only called from GC / Fsync.
     * TODO(Dantali0n): Verify read lock is redundant
     * @return FLFS_RET_NONE if block not full, FLFS_RET_INO_BLK_FULL if the
     *         entire block is filled.
     */
    int FuseLFSInodeEntry::fill_inode_block(
        std::vector<fuse_ino_t> *ino_remove, struct inode_block *blk)
    {
        lock_guard<pthread_rwlock_t> guard(inode_entries_lck);

        ino_remove->reserve(INODE_BLOCK_SIZE / INODE_ENTRY_SIZE);

        // Keep track of the occupied size by the current inode_entries
        uint32_t occupied_size = 0;

        // Store inode_entries and names in temporary larger buffer incase of
        // overshoot.
        auto *buffer = (uint8_t*) malloc(INODE_BLOCK_SIZE * 2);

        for(auto &entry : inode_entries) {
            // If no more space return that the block is full
            if(entry.second.second.size() + 1 + occupied_size +
               INODE_ENTRY_SIZE > INODE_BLOCK_SIZE)
                goto compute_inode_block_full;

            memcpy(buffer + occupied_size,
               &entry.second.first, INODE_ENTRY_SIZE);
            occupied_size += INODE_ENTRY_SIZE;

            memcpy(buffer + occupied_size,
               entry.second.second.c_str(), entry.second.second.size() + 1);
            occupied_size += entry.second.second.size() + 1;

            ino_remove->push_back(entry.first);
        }

        // If remaining space insufficient return full
        if(occupied_size > INODE_BLOCK_SIZE - INODE_ENTRY_SIZE)
            goto compute_inode_block_full;

        // Block is not full so return none
        free(buffer);
        return FLFS_RET_NONE;

        compute_inode_block_full:
        memcpy(blk, buffer, INODE_BLOCK_SIZE);
        free(buffer);
        return FLFS_RET_INO_BLK_FULL;
    }

}