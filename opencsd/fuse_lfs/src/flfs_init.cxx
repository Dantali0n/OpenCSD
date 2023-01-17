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

    int FuseLFS::run_init() {
        struct checkpoint_block cblock = {0};

        /** Fill nvme_info struct */
        nvme->get_nvme_zns_info(&nvme_info);

        /** Check compiled sector size matches device sector size */
        if(SECTOR_SIZE != nvme_info.sector_size) {
            output.error("Compiled sector size ", SECTOR_SIZE, " does not ",
                         "match device sector size ", nvme_info.sector_size);
            return FLFS_RET_ERR;
        }

        // TODO(Dantali0n): Only create filesystem when a certain command line
        //                  argument is supplied. See fuse hello_ll for example.
        output.info("Creating filesystem..");
        if(mkfs() != FLFS_RET_NONE) {
            return FLFS_RET_ERR;
        }

        output.info("Checking super block..");
        if(verify_superblock() != FLFS_RET_NONE) {
            output.error("Failed to verify super block, are you ",
                         "sure the partition does not contain another filesystem?");
            return FLFS_RET_ERR;
        }

        // TODO(Dantali0n): Filesystem cleanup / recovery from dirty state
        output.info("Checking dirty block..");
        if(verify_dirtyblock() != FLFS_RET_NONE) {
            output.error("Filesystem dirty, no recovery methods yet",
                         " unable to continue :(");
            return FLFS_RET_ERR;
        }

        output.info("Writing dirty block..");
        if(write_dirtyblock() != FLFS_RET_NONE) {
            output.error("Unable to write dirty block to drive, "
                         "check that drive is writeable");
            return FLFS_RET_ERR;
        }

        /** Set the random_pos and log_pos to the correct position */
        get_checkpointblock(cblock);
        lba_to_position(cblock.randz_lba, random_pos);
        lba_to_position(cblock.logz_lba, log_pos);

        /** Determine random_ptr now that random_pos is known */
        if(determine_random_ptr() == FLFS_RET_RANDZ_FULL) {
            output.warning("Filesystem initialized while random zone still ",
                           "full, unclean shutdown attempting recovery..");
            if(rewrite_random_blocks() != FLFS_RET_NONE) {
                return FLFS_RET_ERR;
            }

            if(determine_random_ptr() != FLFS_RET_NONE) {
                return FLFS_RET_ERR;
            }
        }

        // Root inode
        output.info("Creating root inode..");
        path_inode_map->insert(std::make_pair(1, new path_map_t()));

        /** Now that random_pos is known reconstruct inode lba map */
        // TODO(Dantali0n): Make this also reconstruct SIT blocks
        read_random_zone(&inode_lba_map);

        /** Find highest free inode number and keep track */
        ino_ptr = 2;
        for(auto &ino : inode_lba_map) {
            if(ino.first > ino_ptr) ino_ptr = ino.first + 1;
        }

        /** Determine the log write pointer */
        determine_log_ptr();

        /** With inodes available build path_inode_map now */
        // TODO(Dantali0n): Build path_inode_map
//        if(build_path_inode_map() != FLFS_RET_NONE) {
//            ret = 1;
//            goto err_out1;
//        }

        return FLFS_RET_NONE;
    }

}