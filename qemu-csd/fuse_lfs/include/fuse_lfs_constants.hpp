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

#ifndef QEMU_CSD_FUSE_LFS_CONSTANTS_HPP
#define QEMU_CSD_FUSE_LFS_CONSTANTS_HPP

#define FUSE_USE_VERSION	36

extern "C" {
#include "fuse3/fuse_lowlevel.h"
}

#include <map>
#include <iostream>
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
     * the random zone after each copy to the random buffer, the memory map it
     * has to access remains larger for a longer period of time.
     */
//    #define FLFS_RANDOM_RW_STRICT

    #define flfs_min(x, y) ((x) < (y) ? (x) : (y))

    enum FLFS_RETURN_CODES {
        FLFS_RET_ERR = -1,
        FLFS_RET_NONE = 0,
        FLFS_RET_RANDZ_FULL = 1,
        FLFS_RET_RANDZ_INSUFFICIENT = 2,
    };

    /**
     * Non dependent constants that should be accessible by all header files in
     * fuse_lfs
     */

    static const uint32_t SECTOR_SIZE = 512;
    static const uint64_t MAGIC_COOKIE = 0x10ADEDB00BDEC0DE;

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
//            // Invalid position can never equal anything
//            if(this->_valid == false) return 0;
//            if(cmp._valid == false) return 0;

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

//        void validate() {
//            this->_valid = 1;
//        }
//
//        void invalidate() {
//            this->_valid = 0;
//        }
    };
    static_assert(sizeof(data_position) == sizeof(uint64_t) * 4);
    static_assert(std::is_trivially_copyable<data_position>::value);

    int dpos_valid(struct data_position dpos);

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
    static constexpr struct data_position LOG_POS = {
        12,0, 0, SECTOR_SIZE
    };
    // random buffer must be exactly two zones large
    static_assert((LOG_POS.zone - RANDZ_BUFF_POS.zone) == 2);

    /**
     * No need to store LOG_BUFF_POS as it is num_zones - 2;
     */
}

#endif // QEMU_CSD_FUSE_LFS_CONSTANTS_HPP