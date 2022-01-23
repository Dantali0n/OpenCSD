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

#include "flfs.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * Read the super block for the filesystem and verify the parameters to
     * prevent overwritten a drive configured for other filesystems.
     * @threadsafety: single threaded, only called during initialization
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR upon failure
     */
    int FuseLFS::verify_superblock() {
        struct super_block sblock;
        if(nvme->read(SBLOCK_POS.zone, SBLOCK_POS.sector, SBLOCK_POS.offset,
                      &sblock, sizeof(super_block)) != 0)
            return FLFS_RET_ERR;
        if(sblock.magic_cookie != MAGIC_COOKIE)
            return FLFS_RET_ERR;
        if(sblock.zones != nvme_info.num_zones)
            return FLFS_RET_ERR;
        if(sblock.sectors != nvme_info.zone_size)
            return FLFS_RET_ERR;
        if(sblock.sector_size != nvme_info.sector_size)
            return FLFS_RET_ERR;

        return FLFS_RET_NONE;
    }

    /**
     * Write the super block so it can be recognized on subsequent
     * initializations.
     * @threadsafety: single threaded, only called during initialization
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR upon failure
     */
    int FuseLFS::write_superblock() {
        struct super_block sblock;
        sblock.magic_cookie = MAGIC_COOKIE;
        sblock.zones = nvme_info.num_zones;
        sblock.sectors = nvme_info.zone_size;
        sblock.sector_size = nvme_info.sector_size;

        uint64_t sector;
        if(nvme->append(SBLOCK_POS.zone, sector, SBLOCK_POS.offset, &sblock,
                        sizeof(super_block)) != 0)
            return FLFS_RET_ERR;

        if(sector != SBLOCK_POS.sector)
            return FLFS_RET_ERR;

        return FLFS_RET_NONE;
    }

};