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

        // Map filenames and their respective parent to inodes
        static path_inode_map_t path_inode_map;

        // Map inodes to the lba they are stored at
        static inode_lba_map_t inode_lba_map;

        static const std::string FUSE_LFS_NAME_PREFIX;
        static const std::string FUSE_SEQUENTIAL_PARAM;

        static const struct fuse_lowlevel_ops operations;

        static void path_to_inode(
            fuse_ino_t parent, const char* path, fuse_ino_t &ino);

        static void inode_to_lba(fuse_ino_t ino, uint64_t &lba);

        static void lba_to_position(
            uint64_t lba, struct data_position &position);

        static void position_to_lba(
            struct data_position position, uint64_t &lba);

        static void advance_position(struct data_position &position);

        static int ino_stat(fuse_ino_t ino, struct stat *stbuf);

        static int reply_buf_limited(fuse_req_t req, const char *buf,
                                     size_t bufsize, off_t off, size_t maxsize);
        static void dir_buf_add(fuse_req_t req, struct dir_buf* buf,
                                const char *name, fuse_ino_t ino);

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

        static struct data_position random_pos;
        static struct data_position random_ptr;

        static int determine_random_ptr();

        static int fill_nat_block(nat_update_set_t *nat_set,
                                  struct nat_block &nt_blk);

        static int append_random_block(struct rand_block_base &block);

        static int compute_nat_blocks(nat_update_set_t *nat_set,
                                      uint64_t &num_blocks);

        static int update_nat_blocks(nat_update_set_t *nat_set);

        static int buffer_random_blocks(const uint64_t zones[2]);
        static int erase_random_buffer();

        static int rewrite_random_blocks();
    public:
        FuseLFS() = delete;
        ~FuseLFS() = delete;

        static int initialize(
            int argc, char* argv[], nvme_zns::NvmeZnsBackend* nvme);

        static void init(void *userdata, struct fuse_conn_info *conn);
        static void destroy(void *userdata);
        static void lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
        static void getattr(fuse_req_t req, fuse_ino_t ino,
                           struct fuse_file_info *fi);
        static void readdir(
            fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
            struct fuse_file_info *fi);
        static void open(fuse_req_t req, fuse_ino_t ino,
                         struct fuse_file_info *fi);
        static void create(fuse_req_t req, fuse_ino_t parent, const char *name,
                          mode_t mode, struct fuse_file_info *fi);
        static void read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                        struct fuse_file_info *fi);
        static void write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                         size_t size, off_t off, struct fuse_file_info *fi);
        static void unlink(fuse_req_t req, fuse_ino_t parent, const char *name);
    };
}

#endif //QEMU_CSD_FUSE_LFS_HPP