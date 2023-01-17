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
     * Perform request checks for read_regular and read_csd. This method
     * handles FUSE replies on the fuse_req_t object.
     * @threadsafety: Ensure the inode is locked before these checks are
     *                executed.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFSRead::read_precheck(fuse_req_t req, struct inode_entry entry,
        size_t &size, off_t &offset)
    {
        // Do not read beyond file size
        if(entry.size < offset) {
            fuse_reply_buf(req, nullptr, 0);
            return FLFS_RET_ERR;
        }

        // Do not return data from buffers past end of file (EOF)
        if(entry.size < offset + size) {
            size = entry.size - offset;
        }

        // Don't actually perform 0 sized reads
        if(size == 0) {
            fuse_reply_buf(req, nullptr, 0);
            return FLFS_RET_ERR;
        }

        return FLFS_RET_NONE;
    }

    /**
     *
     * @return
     */
    void FuseLFS::read_regular(fuse_req_t req, struct stat *stbuf,
        size_t size, off_t offset, struct fuse_file_info *fi)
    {
        measurements::measure_guard msr_guard(msr_reg[MSRI_REG_READ]);

        // Actual read starts here
        inode_entry_t entry;
        get_inode(stbuf->st_ino, &entry);

        // Precheck handles pre read checks and manages FUSE reply, only
        // continue if there are no errors.
        if(read_precheck(req, entry.first, size, offset) != FLFS_RET_NONE)
            return;

        // Variables corresponding to initial data_block, sector aligned
        uint64_t db_block_num;
        uint64_t db_num_lbas = offset / SECTOR_SIZE;
        db_block_num = db_num_lbas / DATA_BLK_LBA_NUM;
//        compute_data_block_num(db_num_lbas, db_block_num);

        auto *blk = (struct data_block *) malloc(sizeof(data_block));
        if(get_data_block(entry.first, db_block_num, blk) != FLFS_RET_NONE) {
            uint64_t error_lba = entry.first.data_lba;
            output.error("Failed to get data_block at lba ", error_lba,
                " for inode ", stbuf->st_ino, " in read");
            free(blk);
            return;
        }

        // Amount of data to return to the caller.
        uint64_t data_limit = flfs_min(size, stbuf->st_size);
        // Initial lba index in the data_block
        uint64_t db_lba_index = db_num_lbas % DATA_BLK_LBA_NUM;

        // Round buffer size to account for offset of first and last sector
        // as well as sector alignment
        auto buffer = (uint8_t*) malloc(
            data_limit + (offset % SECTOR_SIZE) + (size % SECTOR_SIZE)
            + (SECTOR_SIZE-1) & (-SECTOR_SIZE));

        // Loop through the data until the buffer is filled to the required size
        uint64_t buffer_offset = 0;
        struct data_position data_pos = {0};

        // Read limit to account for first sector offset.
        uint64_t  read_limit = data_limit + (offset % SECTOR_SIZE);
        while(buffer_offset < read_limit) {
            // Detect db_lba_index overflow and fetch next data_block
            if(db_lba_index >= DATA_BLK_LBA_NUM) {
                db_lba_index = 0;
                db_block_num += 1;
                if(get_data_block(entry.first, db_block_num, blk) !=
                   FLFS_RET_NONE)
                {
                    uint64_t error_lba = entry.first.data_lba;
                    output.error(
                        "Failed to get data_block at lba ", error_lba,
                        " for inode ", stbuf->st_ino, " in read");
                    free(buffer);
                    free(blk);
                    return;
                }
            }

            if(blk->data_lbas[db_lba_index] == 0) {
                output.info(
                    "File sized sufficiently but no actual data at block ",
                    db_block_num, " with index ", db_lba_index);
                break;
            }

            // Convert the data_block lba at the current index to a position
            lba_to_position(blk->data_lbas[db_lba_index], data_pos);

            // Read the data from this position into the buffer given the
            // currently accumulated offset
            if(nvme->read(data_pos.zone, data_pos.sector, data_pos.offset,
                buffer + buffer_offset, SECTOR_SIZE) != 0)
            {
                uint64_t data_lba = blk->data_lbas[db_lba_index];
                output.error("Failed to retrieve data at at data_block index ",
                    db_lba_index, " with lba ", data_lba, " for inode ",
                    stbuf->st_ino);
                fuse_reply_err(req, EIO);
                free(buffer);
                free(blk);
                return;
            }

            buffer_offset += SECTOR_SIZE;
            db_lba_index += 1;
        }

        fuse_reply_buf(req, (const char*)buffer + (offset % SECTOR_SIZE),
            flfs_min(data_limit, size));

        free(buffer);
        free(blk);
    }

    /**
     *
     * @return
     */
    int FuseLFS::read_snapshot(csd_unique_t *context, size_t size, off_t off,
                               void *buffer, struct snapshot *snap)
    {
        // Inode is of size 0
        if(snap->inode_data.first.size == 0) {
            return FLFS_RET_NONE;
        }

        // Variables corresponding to initial data_block
        uint64_t db_block_num;
        uint64_t db_num_lbas = off / SECTOR_SIZE;
        compute_data_block_num(db_num_lbas, db_block_num);

        struct data_block *blk = &snap->data_blocks.at(db_block_num);

        uint64_t data_limit = flfs_min(size, snap->inode_data.first.size);
        // Initial lba index in the data_block
        uint64_t db_lba_index = db_num_lbas % DATA_BLK_LBA_NUM;

        // Round buffer size to nearest higher multiple of SECTOR_SIZE
        auto internal_buffer = (uint8_t*) malloc(
            data_limit + (SECTOR_SIZE-1) & (-SECTOR_SIZE));

        // Loop through the data until the buffer is filled to the required size
        uint64_t buffer_offset = 0;
        struct data_position data_pos = {0};
        while(buffer_offset < data_limit) {

            // Detect db_lba_index overflow and fetch next data_block
            if(db_lba_index >= DATA_BLK_LBA_NUM) {
                db_lba_index = 0;
                db_block_num += 1;
                blk = &snap->data_blocks.at(db_block_num);
            }

            // Convert the data_block lba at the current index to a position
            lba_to_position(blk->data_lbas[db_lba_index], data_pos);

            // Read the data from this position into the buffer given the
            // currently accumulated offset
            if(nvme->read(data_pos.zone, data_pos.sector, data_pos.offset,
                internal_buffer + buffer_offset, SECTOR_SIZE) != 0)
            {
                fuse_ino_t inode = snap->inode_data.first.inode;
                uint64_t data_lba = blk->data_lbas[db_lba_index];
                output.error("Failed to retrieve data at at data_block index ",
                    db_lba_index, " with lba ", data_lba, " for inode ",
                    inode);
                free(buffer);
                free(blk);
                return FLFS_RET_ERR;
            }

            buffer_offset += SECTOR_SIZE;
            db_lba_index += 1;
        }

        memcpy(buffer, internal_buffer, flfs_min(data_limit, size));
        free(internal_buffer);

        return FLFS_RET_NONE;
    }
}