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

#ifndef QEMU_CSD_FLFS_SNAPSHOT_HPP
#define QEMU_CSD_FLFS_SNAPSHOT_HPP

extern "C" {
    #include "fuse3/fuse_lowlevel.h"
    #include <pthread.h>
}

#include "flfs_constants.hpp"
#include "flfs_memory.hpp"
#include "synchronization/flfs_rwlock.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * Interface for snapshot methods
     */
    class FuseLFSSnapShot {
    protected:
        std::map<csd_unique_t, struct csd_snapshot> snapshots;

        // Concurrency management for snapshots
        pthread_rwlock_t snapshot_lck = {};
        pthread_rwlockattr_t snapshot_attr = {};
    public:
        FuseLFSSnapShot();
        virtual ~FuseLFSSnapShot();

        virtual int create_snapshot(fuse_ino_t kernel, struct snapshot *snap) = 0;

        virtual int update_snapshot(csd_unique_t *context, fuse_ino_t kernel,
            bool write) = 0;
        virtual int update_snapshot(csd_unique_t *context,
            struct snapshot *snap, enum snapshot_store_type snap_t) = 0;

        virtual int has_snapshot(csd_unique_t *context,
            enum snapshot_store_type snap_t) = 0;

        virtual int get_snapshot(csd_unique_t *context,
            csd_snapshot *snaps) = 0;
        virtual int get_snapshot(csd_unique_t *context,
            struct snapshot *snap, enum snapshot_store_type snap_t) = 0;

        virtual int delete_snapshot(csd_unique_t *context) = 0;
    };

}

#endif // QEMU_CSD_FLFS_SNAPSHOT_HPP