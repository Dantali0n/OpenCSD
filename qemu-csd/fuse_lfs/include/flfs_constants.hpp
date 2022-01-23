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

#ifndef QEMU_CSD_FLFS_CONSTANTS_HPP
#define QEMU_CSD_FLFS_CONSTANTS_HPP

#define FUSE_USE_VERSION	36

extern "C" {
    #include "fuse3/fuse_lowlevel.h"
}

#include <map>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "nvme_zns_info.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * Random zone rewrite strict mode, only flushes found inodes if the lba
     * matches the current lba in the memory map.
     *
     * If strict mode is disabled aggressive mode is used. This flushes inodes
     * (with the current lba) as soon as the inode is encountered.
     *
     * While strict mode has substantially less data to write while rewriting
     * upon each copy to the random buffer, the memory map it has to access
     * remains larger for a longer period of time.
     */
//    #define FLFS_RANDOM_RW_STRICT

    /**
     * Flush the nat_update_set as soon as entries occupy one nat_block instead
     * of waiting for periodic flushes.
     */
//    #define FLFS_NAT_FLUSH_IMMEDIATE

    /**
     * Flush the sit_update_set as soon as entries occupy one sit_block instead
     * of waiting for periodic flushes.
     */
//    #define FLFS_SIT_FLUSH_IMMEDIATE

    /**
     * Flush inode_entries as soon as they occupy one inode_block instead of
     * waiting for periodic flushes.
     */
    #define FLFS_INODE_FLUSH_IMMEDIATE

    /**
     * Decide to output fuse_file_info fi debug info to the console upon FUSE
     * calls. output level will be debug
     */
//    #define FLFS_DBG_FI

    /**
     * Fake group and owner ids so that every file and directory appears to
     * belong to the caller. This is to prevent some sensitive / complaining
     * application from trying to chown / chgrp everything before writing.
     */
    #define FLFS_FAKE_PERMS

    #define flfs_min(x, y) ((x) < (y) ? (x) : (y))

    enum flfs_return_codes {
        // Generic error not otherwise specified
        FLFS_RET_ERR                = -1,

        // Success, no problem during operation
        FLFS_RET_NONE               =  0,

        // Indicates the random zone is full
        FLFS_RET_RANDZ_FULL         =  1,

        // Indicates the random zone is of insufficient size
        FLFS_RET_RANDZ_INSUFFICIENT =  2,

        // Indicate the log zone is full
        FLFS_RET_LOGZ_FULL          =  3,

        // Maximum number of inodes used
        FLFS_RET_MAX_INO            =  4,

        // Indicate the inode_entries filled an entire inode_block
        FLFS_RET_INO_BLK_FULL       =  5,

        // Indicate the requested inode was not found
        FLFS_RET_ENOENT             =  6,

        // Indicate the requested inode is a directory
        FLFS_RET_EISDIR             =  7,
    };

    enum snapshot_store_type {
        SNAP_FILE = 1,
        SNAP_READ = 2,
        SNAP_WRITE =3
    };

    /**
     * Non dependent constants that should be accessible by all header files in
     * fuse_lfs
     */

    static constexpr uint32_t SECTOR_SIZE = 4096;
    static constexpr uint64_t MAGIC_COOKIE = 0x10ADEDB00BDEC0DE;

    static const char *FUSE_LFS_NAME_PREFIX = "[FUSE LFS] ";

    static const char *CSD_READ_KEY = "user.process.csd_read";
    static const char *CSD_WRITE_KEY = "user.process.csd_write";

    /**
     * Non dependent structs that should only be used for constant data
     */

    /** Position of data on drive, only to be used in memory */
    struct data_position {
        uint64_t zone;
        uint64_t sector;
        uint64_t offset;
        uint64_t size;
//        uint8_t _valid;

        int operator==(data_position const& cmp) const {
            if(this->zone != cmp.zone) return 0;
            if(this->sector != cmp.sector) return 0;
            if(this->offset != cmp.offset) return 0;
            if(this->size != cmp.size) return 0;

            return 1;
        }

        int operator!=(data_position const& cmp) const {
            return !(*this == cmp);
        }

        /**
         * If the data_position is valid
         * @return 1 if true, 0 if false
         */
        [[nodiscard]] int valid() const {
            if(this->size < SECTOR_SIZE) return 0;
            if(this->size % SECTOR_SIZE != 0) return 0;
            if(this->offset >= SECTOR_SIZE) return 0;
//            if(_valid) return FLFS_RET_NONE;
            return 1;
        }

        /**
         * Checks if the position is exactly at the limit with the specified
         * zone and sector.
         * @return 1 if true, 0 if false
         */
        [[nodiscard]] int meets_limit(uint64_t zone, uint64_t sector) const {
            if(!valid()) return 0;
            if(this->zone != zone || this->sector != sector) return 0;
            return 1;
        }
    };
    static_assert(sizeof(data_position) == sizeof(uint64_t) * 4);
    static_assert(std::is_trivially_copyable<data_position>::value);

    /**
     * Position of super block on device
     */
    static constexpr struct data_position SBLOCK_POS = {
        0, 0, 0, SECTOR_SIZE
    };

    /**
     * Position of dirty block on device
     */
    static constexpr struct data_position DBLOCK_POS = {
        1, 0, 0, SECTOR_SIZE
    };


    /**
     * Position of (potential) first checkpoint block on device
     */
    static constexpr struct data_position CBLOCK_POS = {
        2, 0, 0, SECTOR_SIZE
    };

    /**
     * Position of start of the RANDOM ZONE
     */
     static constexpr struct data_position RANDZ_POS = {
        4, 0, 0, SECTOR_SIZE
     };

    /**
     * Position of start of the RANDOM BUFFER
     * TODO(Dantali0n): Compute this using some function, LOG_POS -2;
     */
    static constexpr struct data_position RANDZ_BUFF_POS = {
        10, 0, 0, SECTOR_SIZE
    };
    // random zone needs to have multiple of 2 zones.
    static_assert((RANDZ_BUFF_POS.zone - RANDZ_POS.zone) % 2 == 0);

    /**
     * Position of start of the LOG ZONE
     * TODO(Dantali0n): Compute this using some function
     */
    static constexpr struct data_position LOGZ_POS = {
        12,0, 0, SECTOR_SIZE
    };

    static constexpr uint32_t N_RAND_BUFF_ZONES =
            LOGZ_POS.zone - RANDZ_BUFF_POS.zone;
    // random buffer must be exactly two zones large
    static_assert(N_RAND_BUFF_ZONES == 2);

    /**
     * No need to store LOGZ_BUFF_POS as it is num_zones - 4;
     */
    static constexpr uint32_t N_LOG_BUFF_ZONES = 4;
}

#endif // QEMU_CSD_FLFS_CONSTANTS_HPP