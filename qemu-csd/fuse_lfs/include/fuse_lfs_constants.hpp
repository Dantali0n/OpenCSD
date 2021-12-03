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

    #define fuse_lfs_min(x, y) ((x) < (y) ? (x) : (y))

    /**
     * Non dependent constants that should be accessible by all header files in
     * fuse_lfs
     */

    static const uint32_t SECTOR_SIZE = 512;
    static const uint64_t MAGIC_COOKIE = 0x10ADEDB00BDEC0DE;

    /**
     * Non dependent structs that should only be used for constant data
     */

    struct data_position {
        uint64_t zone;
        uint64_t sector;
        uint64_t offset;
        uint64_t size;
    };

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
}

#endif // QEMU_CSD_FUSE_LFS_CONSTANTS_HPP