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

#ifndef QEMU_CSD_FUSE_LFS_DICS_HPP
#define QEMU_CSD_FUSE_LFS_DISC_HPP

#include "fuse_lfs_constants.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * This header contains data structures to be stored on disc by fuse_lfs.
     */

    /**
     * One time write, read only information. Always stored at zone 0, sector 0.
     */
    struct super_block {
        uint64_t magic_cookie;  // Used to ensure partition is configured for
                                // fuse_lfs
        uint64_t zones;         // Verifies given partition matches expectations
        uint64_t sectors;       // Verifies given partition matches expectations
        uint64_t sector_size;   // Verifies given partition matches expectations
        uint8_t  padding[480];  // Pad out the rest
    };
    static_assert(sizeof(super_block) == SECTOR_SIZE);

    /**
     * RANDOM WRITE ZONE, the following data structures are written to the
     * random write zone on disc.
     */

    /**
     * A nat_block, ids are unique but not on disc. The highest lba occurrence
     * of a particular id identifies the valid nat_block.
     */
    struct nat_block {
        uint64_t id;
        uint64_t size;
        std::pair<uint64_t, uint64_t> inode_lba[31];
    };
    static_assert(sizeof(nat_block) == SECTOR_SIZE);
}

#endif // QEMU_CSD_FUSE_LFS_DISC_HPP
