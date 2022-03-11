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

    FuseLFSSnapShot::FuseLFSSnapShot() {
        rwlock_init(&snapshot_lck, &snapshot_attr, "snapshot");
    }

    FuseLFSSnapShot::~FuseLFSSnapShot() {
        rwlock_destroy(&snapshot_lck, &snapshot_attr, "snapshot");
    }

    /**
     * Create a new csd_snapshot identified by the csd context. This overarching
     * snapshot keeps track of both the inode activated for CSD functionality
     * but also its read and / or write kernels
     * @threadsafety: thread safe
     * @param snap_t Specify which type of kernel to update
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR upon failure
     */
    int FuseLFS::update_snapshot(csd_unique_t *context, fuse_ino_t kernel,
        enum snapshot_store_type snap_t)
    {
        struct csd_snapshot snap = {inode_entry_t(), data_map_t()};

        // Either it fails or we get everything that already exists
        // does not influence behavior / correctness.
        if(get_snapshot(context, &snap) == FLFS_RET_NONE)
            output.warning("Updating existing snapshot for ",
                "ino ", context->first, " with pid ", context->second);
        // Only stream based kernels trigger a file snapshot
        else if(snap_t == SNAP_READ_STREAM || snap_t == SNAP_WRITE_STREAM)
            if(create_snapshot(context->first, &snap.file) != FLFS_RET_NONE)
                return FLFS_RET_ERR;

        // Create the snapshot for the provided inode and operation
        switch (snap_t) {
            case SNAP_READ_STREAM:
                if(create_snapshot(kernel, &snap.read_stream_kernel) !=
                    FLFS_RET_NONE)
                    return FLFS_RET_ERR;
                break;
            case SNAP_WRITE_STREAM:
                if(create_snapshot(kernel, &snap.write_stream_kernel) !=
                    FLFS_RET_NONE)
                    return FLFS_RET_ERR;
                break;
            case SNAP_READ_EVENT:
                if(create_snapshot(kernel, &snap.read_event_kernel) !=
                    FLFS_RET_NONE)
                    return FLFS_RET_ERR;
                break;
            case SNAP_WRITE_EVENT:
                if(create_snapshot(kernel, &snap.write_event_kernel) !=
                    FLFS_RET_NONE)
                    return FLFS_RET_ERR;
                break;
            default:
                return FLFS_RET_ERR;
        }


        lock_guard<pthread_rwlock_t> guard(snapshot_lck, true);

        // Update the snapshot
        snapshots.insert_or_assign(*context, snap);

        return FLFS_RET_NONE;
    }

    /**
     * Update the csd_snapshot its file, read or write kernel using the provided
     * snapshot object. This method can not fail as it will create an empty
     * csd_snapshot context if no current snapshot can be found.
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE
     */
    int FuseLFS::update_snapshot(csd_unique_t *context, struct snapshot *snap,
        enum snapshot_store_type snap_t)
    {
        struct csd_snapshot temp_snap;

        // Either it exists or we create it so return value irrelevant.
        get_snapshot(context, &temp_snap);

        if(snap_t == SNAP_FILE)
            temp_snap.file = *snap;
        else if(snap_t == SNAP_READ_STREAM)
            temp_snap.read_stream_kernel = *snap;
        else if(snap_t == SNAP_WRITE_STREAM)
            temp_snap.write_stream_kernel = *snap;
        else if(snap_t == SNAP_READ_EVENT)
            temp_snap.read_event_kernel = *snap;
        else if(snap_t == SNAP_WRITE_EVENT)
            temp_snap.write_event_kernel = *snap;

        lock_guard<pthread_rwlock_t> guard(snapshot_lck, true);

        snapshots.insert_or_assign(*context, temp_snap);

        // Return statement necessary because of C++ limitation:
        // all virtual methods with the same name must share the same return
        // type.
        return FLFS_RET_NONE;
    }

    /**
     *
     * @threadsafety: thread safe
     * TODO(Dantali0n): Make get_data_block thread safe
     * @return FLFS_RET_NONE
     */
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
     * @threadsafety: thread safe
     * @return 1 if has snapshot, 0 if not
     */
    int FuseLFS::has_snapshot(csd_unique_t *context,
        enum snapshot_store_type snap_t)
    {
        lock_guard<pthread_rwlock_t> guard(snapshot_lck);

        auto it = snapshots.find(*context);
        if(it == snapshots.end())
            return 0;

        switch(snap_t) {
            case SNAP_FILE:
                return it->second.file.inode_data.first.inode != 0 ? 1 : 0;
            case SNAP_READ_STREAM:
                return it->second.read_stream_kernel.inode_data.first.inode != 0 ? 1 : 0;
            case SNAP_WRITE_STREAM:
                return it->second.write_stream_kernel.inode_data.first.inode != 0 ? 1 : 0;
            case SNAP_READ_EVENT:
                return it->second.read_event_kernel.inode_data.first.inode != 0 ? 1 : 0;
            case SNAP_WRITE_EVENT:
                return it->second.write_event_kernel.inode_data.first.inode != 0 ? 1 : 0;
            default:
                return 0;
        }

        return 0;
    }

    /**
     * Get csd_snapshot contain all three snapshots for the kernels and the file
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR if not found
     */
    int FuseLFS::get_snapshot(csd_unique_t *context, csd_snapshot *snaps) {
        lock_guard<pthread_rwlock_t> guard(snapshot_lck);

        auto it = snapshots.find(*context);

        if(it == snapshots.end())
            return FLFS_RET_ERR;

        *snaps = it->second;

        return FLFS_RET_NONE;
    }

    /**
     * Get a specific snapshot for the context selection the file, read or
     * write kernel
     * @threadsafety: thread safe
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
        else if(snap_t == SNAP_READ_STREAM)
            *snap = snaps.read_stream_kernel;
        else if(snap_t == SNAP_WRITE_STREAM)
            *snap = snaps.write_stream_kernel;
        else if(snap_t == SNAP_READ_EVENT)
            *snap = snaps.read_event_kernel;
        else if(snap_t == SNAP_WRITE_EVENT)
            *snap = snaps.write_event_kernel;

        return FLFS_RET_NONE;
    }

    /**
     * Remove the snapshot belonging to the supplied context
     * @threadsafety: thread safe
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR if not found
     */
    int FuseLFS::delete_snapshot(csd_unique_t *context) {
        lock_guard<pthread_rwlock_t> guard(snapshot_lck, true);

        auto it = snapshots.find(*context);

        if(it == snapshots.end())
            return FLFS_RET_ERR;

        snapshots.erase(it);

        return FLFS_RET_NONE;
    }

}