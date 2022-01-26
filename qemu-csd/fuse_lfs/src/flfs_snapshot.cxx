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
     * @param write False to update the read kernel, true to update the write
     *              kernel.
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR upon failure
     */
    int FuseLFS::update_snapshot(csd_unique_t *context, fuse_ino_t kernel,
        bool write)
    {
        struct csd_snapshot snap = {inode_entry_t(), data_map_t()};

        // Check if snapshot for context file already exists
        if(snapshots.find(*context) != snapshots.end()) {
            output.warning("Updating existing snapshot for ",
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

    /**
     * Update the csd_snapshot its file, read or write kernel using the provided
     * snapshot object. This method can not fail as it will create an empty
     * csd_snapshot context if no current snapshot can be found.
     */
    int FuseLFS::update_snapshot(csd_unique_t *context, struct snapshot *snap,
        enum snapshot_store_type snap_t)
    {
        struct csd_snapshot temp_snap;

        // Either it exists or we create it so return value irrelevant.
        get_snapshot(context, &temp_snap);

        if(snap_t == SNAP_FILE)
            temp_snap.file = *snap;
        else if(snap_t == SNAP_READ)
            temp_snap.read_kernel = *snap;
        else if(snap_t == SNAP_WRITE)
            temp_snap.write_kernel = *snap;

        snapshots.insert_or_assign(*context, temp_snap);

        // Return statement necessary because of C++ limitation:
        // all virtual methods with the same name must share the same return
        // type..
        return FLFS_RET_NONE;
    }

    int FuseLFS::create_snapshot(fuse_ino_t ino, struct snapshot *snap) {
        if(get_inode(ino, &snap->inode_data) != FLFS_RET_NONE)
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
     * @return 1 if has snapshot, 0 if not
     */
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
     * Get csd_snapshot contain all three snapshots for the kernels and the file
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR if not found
     */
    int FuseLFS::get_snapshot(csd_unique_t *context, csd_snapshot *snaps) {
        auto it = snapshots.find(*context);

        if(it == snapshots.end())
            return FLFS_RET_ERR;

        *snaps = it->second;

        return FLFS_RET_NONE;
    }

    /**
     * Get a specific snapshot for the context selection the file, read or
     * write kernel
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR if not found
     */
    int FuseLFS::get_snapshot(csd_unique_t *context, struct snapshot *snap,
        enum snapshot_store_type snap_t)
    {
        csd_snapshot snaps;
        if(get_snapshot(context, &snaps) != FLFS_RET_NONE)
            return FLFS_RET_ERR;

        if(snap_t == SNAP_FILE)
            *snap = snaps.file;
        else if(snap_t == SNAP_READ)
            *snap = snaps.read_kernel;
        else if(snap_t == SNAP_WRITE)
            *snap = snaps.write_kernel;

        return FLFS_RET_NONE;
    }

    /**
     * Remove the snapshot belonging to the supplied context
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR if not found
     */
    int FuseLFS::delete_snapshot(csd_unique_t *context) {
        auto it = snapshots.find(*context);

        if(it == snapshots.end())
            return FLFS_RET_ERR;

        snapshots.erase(it);

        return FLFS_RET_NONE;
    }

}