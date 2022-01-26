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

    /**
     * Write a single sector of data taking into account any pre-existing data
     * if it exists.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure and
     *         FLFS_RET_LOGZ_FULL if the log zone is full.
     */
    int FuseLFS::write_sector(size_t size, off_t offset, uint64_t cur_lba,
        const char *data, uint64_t &result_lba)
    {
        if(offset + size > SECTOR_SIZE) return FLFS_RET_ERR;
        auto buffer = (uint8_t*) malloc(SECTOR_SIZE);

        // Fetch current sector data if it exists and sector won't be completely
        // rewritten
        if(cur_lba != 0 && (size != SECTOR_SIZE && offset != 0)) {
            struct data_position cur_data_pos = {0};
            lba_to_position(cur_lba, cur_data_pos);
            if(nvme->read(cur_data_pos.zone, cur_data_pos.sector,
                          cur_data_pos.offset, buffer, SECTOR_SIZE) != 0) {
                free(buffer);
                return FLFS_RET_ERR;
            }
        }
        #ifdef QEMUCSD_DEBUG
        else if(offset != 0) {
            output.warning("[write_sector] No pre-existing data but offset ",
                            "is non zero. In debug this buffer will be zerosd...");
            memset(buffer, 0, offset);
        }
        #endif

        memcpy(buffer + offset, data, size);
        int result = log_append(buffer, SECTOR_SIZE, result_lba);

        free(buffer);
        return result;
    }

    void FuseLFS::write_regular(fuse_req_t req, fuse_ino_t ino,
        const char *buffer, size_t size, off_t off,
        struct write_context *wr_context, struct fuse_file_info *fi)
    {
        inode_entry_t entry;
        if(get_inode(ino, &entry) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        // Create data_block and fetch existing data_block info if it exists
        struct data_block cur_db_blk = {0};
        if(entry.first.size > 0 && get_data_block(entry.first,
            wr_context->cur_db_blk_num, &cur_db_blk) != FLFS_RET_NONE)
        {
            output.error("Failed to get data_block ",
                wr_context->cur_db_blk_num, " for inode", ino);
            fuse_reply_err(req, EIO);
            return;
        }

        uint64_t write_res_lba;
        uint64_t b_off = 0;
        uint64_t s_size = size;
        uint64_t s_off = off % SECTOR_SIZE;
        for(uint64_t i = 0; i < wr_context->num_sectors; i++) {
            if(write_sector(
                s_size + s_off > SECTOR_SIZE ? SECTOR_SIZE - s_off : s_size,
                s_off, cur_db_blk.data_lbas[wr_context->cur_db_lba_index],
                buffer + b_off, write_res_lba) != FLFS_RET_NONE)
            {
                fuse_reply_err(req, EIO);
                return;
            }

            // Update location of data for current sector
            cur_db_blk.data_lbas[wr_context->cur_db_lba_index] = write_res_lba;

            // Increment data index in current data_block
            wr_context->cur_db_lba_index += 1;

            // Handle overflow to next data block
            if(wr_context->cur_db_lba_index >= DATA_BLK_LBA_NUM) {
                assign_data_block(ino, wr_context->cur_db_blk_num, &cur_db_blk);

                wr_context->cur_db_lba_index = 0;
                wr_context->cur_db_blk_num += 1;

                memset(&cur_db_blk, 0, sizeof(data_block));
            }

            // Get new data_block if file sufficiently sized such that it should
            // exist.
            if(wr_context->cur_db_lba_index == 0 && entry.first.size >
                DATA_BLK_LBA_NUM * SECTOR_SIZE * wr_context->cur_db_blk_num)
            {
                if(get_data_block(entry.first, wr_context->cur_db_blk_num,
                                  &cur_db_blk) != FLFS_RET_NONE)
                {
                    output.error("Failed to get data_block ",
                        wr_context->cur_db_blk_num, " for inode", ino);
                    fuse_reply_err(req, EIO);
                    return;
                }
            }

            // Compute offset into provided data buffer
            b_off += SECTOR_SIZE - s_off;
            // Reduce remaining size to be written
            s_size -= SECTOR_SIZE - s_off;
            // Only first write has sector offset.
            if(i == 0) s_off = 0;
        }

        if(wr_context->cur_db_lba_index != 0)
            assign_data_block(ino, wr_context->cur_db_blk_num, &cur_db_blk);
        entry.first.size = entry.first.size > off + size ?
            entry.first.size : off + size;
        update_inode_entry(&entry);
        fuse_reply_write(req, size);
    }

    void FuseLFS::write_snapshot(fuse_req_t req, csd_unique_t *context,
                                 const char *buffer, size_t size, off_t off,
                                 struct write_context *wr_context, struct fuse_file_info *fi)
    {
        struct snapshot snap;

        if(get_snapshot(context, &snap, SNAP_FILE) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        // Create data_block and fetch existing data_block info if it exists
        struct data_block cur_db_blk = {0};
        if(snap.inode_data.first.size > 0)
            cur_db_blk = snap.data_blocks.at(wr_context->cur_db_blk_num);

        uint64_t write_res_lba;
        uint64_t b_off = 0;
        uint64_t s_size = size;
        uint64_t s_off = off % SECTOR_SIZE;
        for(uint64_t i = 0; i < wr_context->num_sectors; i++) {
            if(write_sector(
                s_size + s_off > SECTOR_SIZE ? SECTOR_SIZE - s_off : s_size,
                s_off, cur_db_blk.data_lbas[wr_context->cur_db_lba_index],
                buffer + b_off, write_res_lba) != FLFS_RET_NONE)
            {
                fuse_reply_err(req, EIO);
                return;
            }

            // Update location of data for current sector
            cur_db_blk.data_lbas[wr_context->cur_db_lba_index] = write_res_lba;

            // Increment data index in current data_block
            wr_context->cur_db_lba_index += 1;

            // Handle overflow to next data block
            if(wr_context->cur_db_lba_index >= DATA_BLK_LBA_NUM) {
                snap.data_blocks.insert_or_assign(wr_context->cur_db_blk_num,
                    cur_db_blk);

                wr_context->cur_db_lba_index = 0;
                wr_context->cur_db_blk_num += 1;

                memset(&cur_db_blk, 0, sizeof(data_block));
            }

            // Get new data_block if file sufficiently sized such that it should
            // exist.
            if(wr_context->cur_db_lba_index == 0 && snap.inode_data.first.size >
                DATA_BLK_LBA_NUM * SECTOR_SIZE * wr_context->cur_db_blk_num)
            {
                cur_db_blk = snap.data_blocks.at(wr_context->cur_db_blk_num);
            }

            // Compute offset into provided data buffer
            b_off += SECTOR_SIZE - s_off;
            // Reduce remaining size to be written
            s_size -= SECTOR_SIZE - s_off;
            // Only first write has sector offset.
            if(i == 0) s_off = 0;
        }

        if(wr_context->cur_db_lba_index != 0)
            snap.data_blocks.insert_or_assign(wr_context->cur_db_blk_num,
                cur_db_blk);
        snap.inode_data.first.size = snap.inode_data.first.size > off + size ?
            snap.inode_data.first.size : off + size;

        update_snapshot(context, &snap, SNAP_FILE);
        fuse_reply_write(req, size);
    }

}