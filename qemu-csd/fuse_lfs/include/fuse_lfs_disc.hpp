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
     * Below is documentation on drive layout
     */

    /**
     * DRIVE LAYOUT:
     *
     *        0             1           2         3       4  ... X
     * | SUPER BLOCK | DIRTY BLOCK | CHECKPOINT BLOCK | RANDOM ZONE | ...
     *
     *        N  ... N + 2    Y ... Z   G ... G + 4
     * ... | RANDOM BUFFER | LOG ZONE | LOG BUFFER |
     */

    /**
     * STATIC WRITE ZONE, the following data structures are written to the
     * static location write zone on drive.
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
        uint8_t  padding[SECTOR_SIZE-32];  // Pad out the rest
    };
    static_assert(sizeof(super_block) == SECTOR_SIZE);

    /**
     * Written once when filesystem opened, removed from device once closed.
     * Always stored at zone 1, sector 0.
     */
     struct dirty_block {
         uint8_t is_dirty; // Always set to 1 when written to drive.
         uint8_t  padding[SECTOR_SIZE-1];  // Pad out the rest.
     };
    static_assert(sizeof(dirty_block) == SECTOR_SIZE);

     /**
      * Written linearly from zone 2, sector 0 till, not including zone 4.
      * Once zone 3, sector 0 is written than zone 2 must be reset. If zone 3 is
      * full write to zone 2 sector 0 and afterwards reset zone 3.
      *
      * The last checkpoint that can be read starting from zone 2 sector 0
      * is valid. Reading should not continue after holes in the rare case zone
      * 3 is not reset yet after writing zone 2 sector 0 again.
      */
     struct checkpoint_block {
         uint64_t randz_lba;
         uint8_t  padding[SECTOR_SIZE-8];  // Pad out the rest
     };
    static_assert(sizeof(checkpoint_block) == SECTOR_SIZE);


    /**
     * RANDOM WRITE ZONE, the following data structures are written to the
     * random write zone on drive.
     */

    typedef enum random_zone_block_types {
        RANDZ_NON_BLK = 0,
        RANDZ_NAT_BLK = 1,
        RANDZ_SIT_BLK = 2
    } randz_blk_types;

    /**
     * A nat_block, ids are unique but not on disc. The highest lba occurrence
     * of a particular id identifies the valid nat_block.
     */
    struct nat_block {
        uint64_t id;
        uint64_t type;
        std::pair<uint64_t, uint64_t> inode_lba[31];
    };
    static_assert(sizeof(nat_block) == SECTOR_SIZE);

    /**
     * LOG WRITE ZONE, the following datastructures will be linearly written to
     * the drive.
     */

    struct inode_block {
        uint8_t padding[SECTOR_SIZE];  // Pad out the rest
    };
    static_assert(sizeof(inode_block) == SECTOR_SIZE);

    struct inode_entry {
        uint64_t parent;   // Parent inode
        uint64_t inode;    // This inode
        uint64_t size;     // Size of the inode
        uint8_t  type;     // Inode type
        uint64_t data_lba; // LBA of data block
    };

    struct data_block {
        uint64_t data_lbas[63]; // LBAs of data blocks linearly for size
        uint64_t next_block;    // LBA for next data block or zero if none
    };
    static_assert(sizeof(data_block) == SECTOR_SIZE);

}

#endif // QEMU_CSD_FUSE_LFS_DISC_HPP
