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

#ifndef QEMU_CSD_FLFS_DISC_HPP
#define QEMU_CSD_FLFS_DISC_HPP

#include <cstddef>
#include <cstdint>

#include <type_traits>

#include "flfs_constants.hpp"

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
    static_assert(std::is_trivially_copyable<super_block>::value);

    /**
     * Written once when filesystem opened, removed from device once closed.
     * Always stored at zone 1, sector 0.
     */
     struct dirty_block {
         uint8_t is_dirty; // Always set to 1 when written to drive.
         uint8_t  padding[SECTOR_SIZE-1];  // Pad out the rest.
     };
    static_assert(sizeof(dirty_block) == SECTOR_SIZE);
    static_assert(std::is_trivially_copyable<dirty_block>::value);

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
         uint64_t logz_lba;
         uint8_t  padding[SECTOR_SIZE-16];  // Pad out the rest
     };
    static_assert(sizeof(checkpoint_block) == SECTOR_SIZE);
    static_assert(std::is_trivially_copyable<checkpoint_block>::value);


    /**
     * RANDOM WRITE ZONE, the following data structures are written to the
     * random write zone on drive.
     */

    typedef enum random_zone_block_types {
        RANDZ_NON_BLK = 0,
        RANDZ_NAT_BLK = 1,
        RANDZ_SIT_BLK = 2
    } randz_blk_types;

    struct __attribute__((packed)) rand_block_base {
        uint64_t type;
    };

    /**
     * The point of this datastructure is to be able to safely cast and
     * determine type before doing anything else.
     */
    struct __attribute__((packed)) none_block : rand_block_base {
        // Set type to to RANDZ_NON_BLK
        uint8_t padding[SECTOR_SIZE-8];
    };
    static_assert(sizeof(none_block) == SECTOR_SIZE);
    static_assert(std::is_trivially_copyable<none_block>::value);

    static constexpr uint32_t NAT_BLK_INO_LBA_NUM = (SECTOR_SIZE-8)/16;

    /**
     * The highest lba occurrence of a particular inode identifies the valid
     * data. The start LBA is determined from the checkpoint block randz_lba.
     */
    struct __attribute__((packed)) nat_block : rand_block_base {
        // Set type to to RANDZ_NAT_BLK
        //std::pair<uint64_t, uint64_t> inode_lba[NAT_BLK_INO_LBA_NUM];
        uint64_t inode[NAT_BLK_INO_LBA_NUM];
        uint64_t lba[NAT_BLK_INO_LBA_NUM];
        uint8_t padding[8];
    };
    static_assert(sizeof(nat_block) == SECTOR_SIZE);
    static_assert(std::is_trivially_copyable<nat_block>::value);

    static constexpr uint32_t SIT_BLK_INO_LBA_NUM = SECTOR_SIZE-16;

    /**
     * The highest lba occurrence of a particular start_lba identifies the
     * valid data. The start LBA is determined from the checkpoint block
     * randz_lba. Therefor; start_lba values must be deterministic otherwise
     * overlapping regions might both be considered valid!
     */
    struct __attribute__((packed)) sit_block : rand_block_base {
        // Set type to to RANDZ_SIT_BLK
        uint64_t start_lba;
        bool bitmap[SIT_BLK_INO_LBA_NUM];
    };
    static_assert(sizeof(sit_block) == SECTOR_SIZE);
    static_assert(std::is_trivially_copyable<sit_block>::value);

    /**
     * LOG WRITE ZONE, the following datastructures will be linearly written to
     * the drive.
     */

    enum inode_type {
        INO_T_NONE = 0,
        INO_T_FILE = 1,
        INO_T_DIR = 2,
    };

    /**
     * inode block on drive layout, remaining space is zero filled no partial
     * entries written across sector boundaries. names must be null terminated.
     *
     *     33 bytes     x bytes           33 bytes     y bytes
     * | inode_entry | file/dir name | inode entry | file/dir name | ...
     */

    struct __attribute__((packed)) inode_block {
        uint8_t data[SECTOR_SIZE];
    };
    static_assert(sizeof(inode_block) == SECTOR_SIZE);
    static_assert(std::is_trivially_copyable<inode_block>::value);

    static constexpr size_t INODE_BLOCK_SIZE = sizeof(inode_block);

    struct __attribute__((packed)) inode_entry {
        uint64_t parent;   // Parent inode, can be 1 for root. only root has 0
                           // but root inode is never stored on drive.
        uint64_t inode;    // This inode
        uint8_t  type;     // Inode type see enum inode_type
        uint64_t size;     // Size of inode on drive in bytes
        uint64_t data_lba; // LBA of first data block
        // Followed by a null terminated file/dir name
    };

    static constexpr size_t INODE_ENTRY_SIZE = sizeof(inode_entry);
    static constexpr size_t INO_BLK_READ_LIM =
        INODE_BLOCK_SIZE - INODE_ENTRY_SIZE;

    static constexpr size_t MAX_NAME_SIZE = SECTOR_SIZE - INODE_ENTRY_SIZE;

    static constexpr size_t DATA_BLK_LBA_NUM = (SECTOR_SIZE-8)/8;

    struct __attribute__((packed)) data_block {
        uint64_t data_lbas[DATA_BLK_LBA_NUM]; // LBAs of data blocks linearly
        uint64_t next_block;    // LBA for next data block or zero if none
        // next_block is only stored on drive, in-memory datastructures do not
        // set it
    };
    static_assert(sizeof(data_block) == SECTOR_SIZE);
    static_assert(std::is_trivially_copyable<data_block>::value);

}

#endif // QEMU_CSD_FLFS_DISC_HPP
