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

    /**
     * Create a new csd_snapshot identified by the csd context. This overarching
     * snapshot keeps track of both the inode activated for CSD functionality
     * but also its read and / or write kernels
     * @param write False
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR upon failure
     */
    int FuseLFS::update_snapshot(csd_unique_t *context, fuse_ino_t kernel,
        bool write)
    {
        struct csd_snapshot snap = {inode_entry_t(), data_map_t()};

        // Check if snapshot for context file already exists
        if(snapshots.find(*context) != snapshots.end()) {
            output->warning("Updating existing snapshot for ",
               "ino ", context->first, " with pid ", context->second);

            if(get_snapshot(context, &snap.file, SNAP_FILE) != FLFS_RET_NONE)
                return FLFS_RET_ERR;
        }
        // New context, create snapshot of file
        else if(create_snapshot(context->first, &snap.file) != FLFS_RET_NONE)
            return FLFS_RET_ERR;

        // Get existing kernel snapshot data but only for type of operation
        // that won't be overriden. Return type ignored as it is not relevant.
        if(write)
            get_snapshot(context, &snap.read_kernel, SNAP_READ);
        else
            get_snapshot(context, &snap.write_kernel, SNAP_WRITE);

        // Create the snapshot for the provided inode and operation
        if(create_snapshot(kernel,
            write ? &snap.write_kernel : &snap.read_kernel) != FLFS_RET_NONE)
            return FLFS_RET_ERR;

        // Update the snapshot
        snapshots.insert_or_assign(*context, snap);

        return FLFS_RET_NONE;
    }

    int FuseLFS::create_snapshot(fuse_ino_t ino, struct snapshot *snap) {
        if(get_inode_entry_t(ino, &snap->inode_data) != FLFS_RET_NONE)
            return FLFS_RET_ERR;

        uint64_t blocks;
        uint64_t num_lbas = snap->inode_data.first.size / SECTOR_SIZE;
        num_lbas += snap->inode_data.first.size % SECTOR_SIZE != 0 ? 1 : 0;
        compute_data_block_num(num_lbas, blocks);

        struct data_block db = {0};
        for(uint64_t i = 0; i < blocks; i++) {
            if(get_data_block(snap->inode_data.first, i, &db) != FLFS_RET_NONE)
                return FLFS_RET_ERR;

            snap->data_blocks.insert(std::make_pair(i, db));
        }

        return FLFS_RET_NONE;
    }

    /**
     * Determine if the current csd_unique context still has an active snapshot
     * @return 1 of has snapshot, 0 if not
     */
    int FuseLFS::has_snapshot(csd_unique_t *context) {
        return snapshots.find(*context) != snapshots.end() ? 1 : 0;
    }

    int FuseLFS::has_snapshot(csd_unique_t *context,
        enum snapshot_store_type snap_t)
    {
        auto it = snapshots.find(*context);
        if(it == snapshots.end())
            return 0;

        switch(snap_t) {
            case SNAP_FILE:
                return 1;
            case SNAP_READ:
                return it->second.read_kernel.inode_data.first.inode != 0 ? 1 : 0;
            case SNAP_WRITE:
                return it->second.write_kernel.inode_data.first.inode != 0 ? 1 : 0;
        }

        return 0;
    }

    /**
     *
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR if not found
     */
    int FuseLFS::get_snapshot(csd_unique_t *context, struct snapshot *snap,
        enum snapshot_store_type snap_t)
    {
        auto it = snapshots.find(*context);

        if(it == snapshots.end())
            return FLFS_RET_ERR;

        if(snap_t == SNAP_FILE)
            *snap = it->second.file;
        else if(snap_t == SNAP_READ)
            *snap = it->second.read_kernel;
        else if(snap_t == SNAP_WRITE)
            *snap = it->second.write_kernel;

        return FLFS_RET_NONE;
    }

    /**
     *
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR upon failure
     */
    int FuseLFS::delete_snapshot(csd_unique_t *context) {
        auto it = snapshots.find(*context);

        if(it == snapshots.end())
            return FLFS_RET_ERR;

        snapshots.erase(it);

        return FLFS_RET_NONE;
    }

}