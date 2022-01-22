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

#ifndef QEMU_CSD_FLFS_HPP
#define QEMU_CSD_FLFS_HPP

#define FUSE_USE_VERSION	36

extern "C" {
    #include <fuse3/fuse_lowlevel.h>
    #include <sys/xattr.h>
}

#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
#include <sstream>
#include <string>

#include "output.hpp"
#include "arguments.hpp"
#include "nvme_csd.hpp"
#include "flfs_constants.hpp"
#include "flfs_csd.hpp"
#include "flfs_dirtyblock.hpp"
#include "flfs_disc.hpp"
#include "flfs_memory.hpp"
#include "flfs_snapshot.hpp"
#include "flfs_superblock.hpp"
#include "flfs_write.hpp"

#include "nvme_zns_backend.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * FUSE LFS filesystem for Zoned Namespaces SSDs (FluffleFS).
     */
    class FuseLFS : public FuseLFSCSD, FuseLFSDirtyBlock, FuseLFSSnapShot,
        FuseLFSSuperBlock, FuseLFSWrite
    {
    protected:
        output::Output *output;

        arguments::options *options;

        struct fuse_conn_info *connection;

        struct nvme_zns::nvme_zns_info nvme_info;
        nvme_zns::NvmeZnsBackend *nvme;

        // Map filenames and their respective parent to inodes
        path_inode_map_t *path_inode_map;

        // Map inodes to the lba they are stored at
        inode_lba_map_t *inode_lba_map;

        static const char *FUSE_LFS_NAME_PREFIX;
        static const char *FUSE_SEQUENTIAL_PARAM;

        /** nlookup helpers */

        // Keep track of nlookup count per inode
        inode_nlookup_map_t *inode_nlookup_map;

        // TODO(Dantali0n): Use nlookup count to drive path_inode_map caching
        //                  and response to memory pressure

        void inode_nlookup_increment(fuse_ino_t ino);
        void inode_nlookup_decrement(fuse_ino_t ino, uint64_t count);

        void fuse_reply_entry_nlookup(
            fuse_req_t req, struct fuse_entry_param *e);

        void fuse_reply_create_nlookup(
            fuse_req_t req, struct fuse_entry_param *e,
            const struct fuse_file_info *f);

        /** Inode, path and data position helper functions */

        void lba_to_position(
            uint64_t lba, struct data_position &position) const;

        void position_to_lba(
            struct data_position position, uint64_t &lba);

        void update_inode_lba_map(
            std::vector<fuse_ino_t> *inodes, uint64_t lba,
            inode_lba_map_t *lba_map);

        /** Debug helper functions */

        void output_fi(const char *name, struct fuse_file_info *fi);

        /** FUSE helper functions */

        int ino_stat(fuse_ino_t ino, struct stat *stbuf);

        static int reply_buf_limited(fuse_req_t req, const char *buf,
                                     size_t bufsize, off_t off, size_t maxsize);
        static void dir_buf_add(fuse_req_t req, struct dir_buf* buf,
                                const char *name, fuse_ino_t ino);

        // TODO(Dantali0n): Move filesystem initialization methods to separate
        //                  interface



        // TODO(Dantali0n): Move filesystem creation methods to separate
        //                  interface

        int mkfs();

        /** Super block interface methods */

        int verify_superblock() override;

        int write_superblock() override;

        /** Dirty block interface methods */

        int verify_dirtyblock() override;

        int write_dirtyblock() override;

        int remove_dirtyblock() override;

        // TODO(Dantali0n): Move checkpointblock methods to separate interface

        struct data_position cblock_pos;

        int update_checkpointblock(uint64_t randz_lba,
                                          uint64_t logz_lba);

        int get_checkpointblock(struct checkpoint_block &cblock);

        int get_checkpointblock_locate(struct checkpoint_block &cblock);

        // TODO(Dantali0n): Move random block methods to separate interface

        nat_update_set_t *nat_update_set;

        // random_pos indicates the beginning of the random zone.
        // The reading should continue if RANDZ_BUFF_POS is reached
        // wrapping back around to RANDZ_POS with the exception if random_pos
        // is equal to RANDZ_POS. All locations within the random zone are valid
        // for random_pos with the exception of RANDZ_BUFF_POS. random_pos must
        // be aligned to zone boundaries. Thus the last valid location for
        // random_pos is RANDZ_BUFF_POS.zone - 1;
        struct data_position random_pos;

        // random_ptr is the write pointer into the random zone and indicates
        // the next writeable sector. The location overflows from RANDZ_BUFF_POS
        // back to RANDZ_POS. If random_ptr is invalid the random zone is full.
        //
        struct data_position random_ptr;

        void add_nat_update_set_entries(std::vector<fuse_ino_t> *inodes);

        int determine_random_ptr();

        static void random_zone_distance(
            struct data_position lhs, struct data_position rhs,
            uint32_t &distance);

        void read_random_zone(inode_lba_map_t *inode_map);

        void fill_nat_block(nat_update_set_t *nat_set,
                                  struct nat_block &nt_blk);

        int append_random_block(struct rand_block_base &block);

        static void compute_nat_blocks(nat_update_set_t *nat_set,
                                      uint64_t &num_blocks);

        int update_nat_blocks(nat_update_set_t *nat_set);

        int buffer_random_blocks(const uint64_t zones[2],
            struct data_position limit);
        int process_random_buffer(
            nat_update_set_t *nat_set, inode_lba_map_t *inodes);
        int erase_random_buffer();

        int rewrite_random_blocks();
        int rewrite_random_blocks_partial();
        int rewrite_random_blocks_full();

        // TODO(Dantali0n): Move log management methods to separate interface

        // Current start of the log zone
        struct data_position log_pos;

        // Write pointer within the log zone
        struct data_position log_ptr;

        int advance_log_ptr(struct data_position *log_ptr) const;

        void determine_log_ptr();

        int log_append(void *data, size_t size, uint64_t &lba);

        // TODO(Dantali0n): Move data block methods to separate interface

        data_blocks_t *data_blocks;

        static void compute_data_block_num(uint64_t num_lbas, uint64_t &blocks);

        void assign_data_blocks(fuse_ino_t ino, data_map_t *blocks); //, std::vector<data_block> *blocks);

        void assign_data_block(fuse_ino_t ino, uint64_t block_num,
            struct data_block *blk); //, struct data_block *blk);

        int get_data_block(inode_entry entry, uint64_t block_num,
            struct data_block *blk);

        int get_data_block_immediate(
            struct data_position pos, struct data_block *blk);

        int get_data_block_linked(
            struct data_position pos, uint64_t block_num,
            struct data_block *blk);

        // TODO(Dantali0n): Move inode block methods to separate interface

        inode_entries_t *inode_entries;

        // Keep track of the highest observed ino and increment it for new
        // files and directories. The ino_ptr indicates the next possible ino
        // for new files and directories (similar to write pointers)
        fuse_ino_t ino_ptr;

        // TODO(Dantali0n): Create and keep track of ino_pos for log zone linear
        //                  continuity.

//        static int build_path_inode_map();

        static int fill_inode_block(
            struct inode_block *blck, std::vector<fuse_ino_t> *ino_remove,
            inode_entries_t *entries);

        static void erase_inode_entries(
            std::vector<fuse_ino_t> *ino_remove, inode_entries_t *entries);

        int get_inode_entry_t(fuse_ino_t ino, inode_entry_t *entry);

        int create_inode(fuse_ino_t parent, const char *name,
                                enum inode_type type, fuse_ino_t &ino);

        int update_inode_entry_t(inode_entry_t *entry);

        // TODO(Dantali0n): Move synchronization / flush methods to separate
        //                  interface. (These exclude those of the NAT / SIT
        //                  RANDOM ZONE).

        /** NOTICE; Flush methods are high level functions they will have side
         *          effects such as calling add_nat_update_set_entries or
         *          update_inode_entry
         */

        int flush_inodes(bool only_if_full);

        int flush_inodes_always();

        int flush_inodes_if_full();

        int flush_data_blocks();

        // TODO(Dantali0n): Move CSD / snapshot methods to separate interface

        int update_snapshot(csd_unique_t *context, fuse_ino_t kernel,
            bool write) override;
        int update_snapshot(csd_unique_t *context, struct snapshot *snap,
            enum snapshot_store_type snap_t) override;
        int create_snapshot(fuse_ino_t ino, struct snapshot *snap) override;
        int has_snapshot(csd_unique_t *context,
            enum snapshot_store_type snap_t) override;
        int get_snapshot(csd_unique_t *context, csd_snapshot *snaps) override;
        int get_snapshot(csd_unique_t *context, struct snapshot *snap,
            enum snapshot_store_type snap_t) override;
        int delete_snapshot(csd_unique_t *context) override;

        // TODO(Dantali0n): Move open file handle methods to separate interface

        // File handle pointer for open files
        uint64_t fh_ptr;

        // Keep track of open files and directories using unique handles for
        // respective inodes and caller pids.
        open_inode_vect_t *open_inode_vect;

        void create_file_handle(
            fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

        int get_file_handle(csd_unique_t *uni_t, struct open_file_entry *entry);

        int get_file_handle(uint64_t fh, struct open_file_entry *entry);

        int update_file_handle(uint64_t fh, struct open_file_entry *entry);

        int find_file_handle(csd_unique_t *uni_t,
                             open_inode_vect_t::iterator *it);

        int find_file_handle(uint64_t fh, open_inode_vect_t::iterator *it);

        void release_file_handle(uint64_t fh);

        // TODO(Dantali0n): Move Garbage Collection methods to separate
        //                  interface

        int log_garbage_collect();

        // TODO(Dantali0n): Move FUSE internal wrapper functions to separate
        //                  interface

        static int flfs_ret_to_fuse_reply(int flfs_ret);

        static int check_flags(int flags);

        static void ino_fake_permissions(fuse_req_t req, struct stat *stbuf);

        int ftruncate(fuse_ino_t ino, size_t size);

        void lookup_regular(fuse_req_t req, fuse_ino_t ino);

        void lookup_csd(fuse_req_t req, csd_unique_t *context);

        void getattr_regular(fuse_req_t req, fuse_ino_t ino,
            struct fuse_file_info *fi);

        void getattr_csd(fuse_req_t req, csd_unique_t *context,
            struct fuse_file_info *fi);

        void setattr_regular(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
            int to_set, struct fuse_file_info *fi);

        void setattr_csd(fuse_req_t req, csd_unique_t *context,
            struct stat *attr, int to_set, struct fuse_file_info *fi);

        void read_regular(fuse_req_t req, struct stat *stbuf,
            size_t size, off_t off, struct fuse_file_info *fi);

        int read_snapshot(csd_unique_t *context, size_t size,
            off_t off, void *buffer, struct snapshot *snap);

        /** CSD interface method */

        void read_csd(fuse_req_t req, csd_unique_t *context, size_t size,
            off_t off, struct fuse_file_info *fi) override;

        /** Write interface methods */

        int write_sector(size_t size, off_t offset, uint64_t cur_lba,
             const char *data, uint64_t &result_lba) override;

        void write_regular(fuse_req_t req, fuse_ino_t ino, const char *buf,
            size_t size, off_t off, struct write_context *wr_context,
            struct fuse_file_info *fi) override;
        void write_snapshot(fuse_req_t req, csd_unique_t *context,
            const char *buf, size_t size, off_t off,
            struct write_context *wr_context,
            struct fuse_file_info *fi) override;

        /** CSD interface method */

        void write_csd(fuse_req_t req, csd_unique_t *context,
            const char *buf, size_t size, off_t off,
            struct write_context *wr_context,
            struct fuse_file_info *fi) override;

        // TODO(Dantali0n): Move xattr methods to separate interface

        void get_csd_xattr(fuse_req_t req, fuse_ino_t ino, size_t size);

        void set_csd_xattr(fuse_req_t req, struct open_file_entry *entry,
            const char *value, size_t size, int flags, bool write);

        void xattr(fuse_req_t req, fuse_ino_t ino, const char *name,
            const char *value, size_t size, int flags, bool set);

    public:
        explicit FuseLFS(arguments::options *options,
            nvme_zns::NvmeZnsBackend* nvme);
        virtual ~FuseLFS();

        int run(int argc, char* argv[],
            const struct fuse_lowlevel_ops *operations);

        void init(void *userdata, struct fuse_conn_info *conn);
        void destroy(void *userdata);
        void lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
        void forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup);
        void getattr(fuse_req_t req, fuse_ino_t ino,
            struct fuse_file_info *fi);
        void setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
            int to_set, struct fuse_file_info *fi);
        void readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
            struct fuse_file_info *fi);
        void open(fuse_req_t req, fuse_ino_t ino,
            struct fuse_file_info *fi);
        void release(fuse_req_t req, fuse_ino_t ino,
            struct fuse_file_info *fi);
        void create(fuse_req_t req, fuse_ino_t parent, const char *name,
            mode_t mode, struct fuse_file_info *fi);
        void mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
            mode_t mode);
        void read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
            struct fuse_file_info *fi);
        void write(fuse_req_t req, fuse_ino_t ino, const char *buf,
            size_t size, off_t off, struct fuse_file_info *fi);
        void statfs(fuse_req_t req, fuse_ino_t ino);
        void fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
            struct fuse_file_info *fi);
        void rename(fuse_req_t req, fuse_ino_t parent, const char *name,
            fuse_ino_t newparent, const char *newname, unsigned int flags);
        void unlink(fuse_req_t req, fuse_ino_t parent, const char *name);
        void rmdir(fuse_req_t req, fuse_ino_t parent, const char *name);
        void getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
            size_t size);
        void setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
            const char *value, size_t size, int flags);
        void listxattr(fuse_req_t req, fuse_ino_t ino, size_t size);
        void removexattr(fuse_req_t req, fuse_ino_t ino,
            const char *name);
    };
}

#endif //QEMU_CSD_FLFS_HPP