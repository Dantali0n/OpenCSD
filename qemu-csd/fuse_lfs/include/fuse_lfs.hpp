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

#ifndef QEMU_CSD_FUSE_LFS_HPP
#define QEMU_CSD_FUSE_LFS_HPP

#define FUSE_USE_VERSION	36

extern "C" {
    #include <assert.h>
    #include <fuse3/fuse_lowlevel.h>
    #include <string.h>
}

#include <map>
#include <iostream>
#include <sstream>
#include <string>

#include "output.hpp"
#include "fuse_lfs_constants.hpp"
#include "fuse_lfs_disc.hpp"
#include "fuse_lfs_memory.hpp"
#include "nvme_zns_backend.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * Static wrapper class around FUSE LFS filesystem.
     */
    class FuseLFS {
    protected:
        static output::Output output;

        static struct fuse_conn_info* connection;

        static struct nvme_zns::nvme_zns_info nvme_info;
        static nvme_zns::NvmeZnsBackend* nvme;

        // Keep track of nlookup count per inode
        static inode_nlookup_map_t inode_nlookup_map;

        // Map filenames and their respective parent to inodes
        static path_inode_map_t path_inode_map;

        // Map inodes to the lba they are stored at
        static inode_lba_map_t inode_lba_map;

        static const std::string FUSE_LFS_NAME_PREFIX;
        static const std::string FUSE_SEQUENTIAL_PARAM;

        static const struct fuse_lowlevel_ops operations;

        /** nlookup helpers */

        // TODO(Dantali0n): Use nlookup count to drive path_inode_map caching
        //                  and response to memory pressure

        static void inode_nlookup_increment(fuse_ino_t ino);
        static void inode_nlookup_decrement(fuse_ino_t ino, uint64_t count);

        static void fuse_reply_entry_nlookup(
            fuse_req_t req, struct fuse_entry_param *e);

        static void fuse_reply_create_nlookup(
            fuse_req_t req, struct fuse_entry_param *e,
            const struct fuse_file_info *f);

        /** Inode, path and data position helper functions */

        static void inode_to_lba(fuse_ino_t ino, uint64_t &lba);

        static void lba_to_position(
            uint64_t lba, struct data_position &position);

        static void position_to_lba(
            struct data_position position, uint64_t &lba);

        static void advance_position(struct data_position &position);

        /** FUSE helper functions */

        static int ino_stat(fuse_ino_t ino, struct stat *stbuf);

        static int reply_buf_limited(fuse_req_t req, const char *buf,
                                     size_t bufsize, off_t off, size_t maxsize);
        static void dir_buf_add(fuse_req_t req, struct dir_buf* buf,
                                const char *name, fuse_ino_t ino);

        // TODO(Dantali0n): Move filesystem creation methods to separate
        //                  interface

        static int mkfs();

        // TODO(Dantali0n): Move superblock methods to separate interface

        static int verify_superblock();

        static int write_superblock();

        // TODO(Dantali0n): Move dirtyblock methods to separate interface

        static int verify_dirtyblock();

        static int write_dirtyblock();

        static int remove_dirtyblock();

        // TODO(Dantali0n): Move checkpointblock methods to separate interface
        static struct data_position cblock_pos;

        static int update_checkpointblock(uint64_t randz_lba);

        static int get_checkpointblock(struct checkpoint_block &cblock);

        static int get_checkpointblock_locate(struct checkpoint_block &cblock);

        // TODO(Dantali0n): Move random block methods to separate interface

        static nat_update_set_t nat_update_set;

        // random_pos indicates the beginning of the random zone.
        // The reading should continue if RANDZ_BUFF_POS is reached
        // wrapping back around to RANDZ_POS with the exception if random_pos
        // is equal to RANDZ_POS. All locations within the random zone are valid
        // for random_pos with the exception of RANDZ_BUFF_POS. random_pos must
        // be aligned to zone boundaries. Thus the last valid location for
        // random_pos is RANDZ_BUFF_POS.zone - 1;
        static struct data_position random_pos;

        // random_ptr is the write pointer into the random zone and indicates
        // the next writeable sector. The location overflows from RANDZ_BUFF_POS
        // back to RANDZ_POS. If random_ptr is invalid the random zone is full.
        //
        static struct data_position random_ptr;

        static int determine_random_ptr();

        static void random_zone_distance(
            struct data_position lhs, struct data_position rhs,
            uint32_t &distance);

        static void read_random_zone(inode_lba_map_t *inode_map);

        static void fill_nat_block(nat_update_set_t *nat_set,
                                  struct nat_block &nt_blk);

        static int append_random_block(struct rand_block_base &block);

        static void compute_nat_blocks(nat_update_set_t *nat_set,
                                      uint64_t &num_blocks);

        static int update_nat_blocks(nat_update_set_t *nat_set);

        static int buffer_random_blocks(const uint64_t zones[2],
            struct data_position limit);
        static int process_random_buffer(
            nat_update_set_t *nat_set, inode_lba_map_t *inodes);
        static int erase_random_buffer();

        static int rewrite_random_blocks();
        static int rewrite_random_blocks_partial();
        static int rewrite_random_blocks_full();

        // TODO(Dantali0n): Move inode block methods to separate interface

        static inode_entries_t inode_entries;

        // Keep track of the highest observed ino and increment it for new
        // files and directories. The ino_ptr indicates the next possible ino
        // for new files and directories (similar to write pointers)
        static fuse_ino_t ino_ptr;

        // Write pointer within the log zone
        static struct data_position log_ptr;

        static int advance_log_ptr(struct data_position *log_ptr);

        static void determine_log_ptr();

        static int build_path_inode_map();

        static int create_inode(fuse_entry_param *e, fuse_ino_t parent,
                                const char *name, enum inode_type type);

        static int append_inode_block(inode_block *entries);

        static int append_data_block(data_block *data);

        static int append_data(void *data, size_t size, uint64_t &lba);

    public:
        FuseLFS() = delete;
        ~FuseLFS() = delete;

        static int initialize(
            int argc, char* argv[], nvme_zns::NvmeZnsBackend* nvme);

        static void init(void *userdata, struct fuse_conn_info *conn);
        static void destroy(void *userdata);
        static void lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
        static void forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup);
        static void getattr(fuse_req_t req, fuse_ino_t ino,
                           struct fuse_file_info *fi);
        static void readdir(
            fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
            struct fuse_file_info *fi);
        static void open(fuse_req_t req, fuse_ino_t ino,
                         struct fuse_file_info *fi);
        static void create(fuse_req_t req, fuse_ino_t parent, const char *name,
                          mode_t mode, struct fuse_file_info *fi);
        static void mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
                          mode_t mode);
        static void read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                        struct fuse_file_info *fi);
        static void write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                         size_t size, off_t off, struct fuse_file_info *fi);
        static void unlink(fuse_req_t req, fuse_ino_t parent, const char *name);
    };
}

#endif //QEMU_CSD_FUSE_LFS_HPP