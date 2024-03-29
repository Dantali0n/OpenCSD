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
#include <mutex>
#include <iostream>
#include <sstream>
#include <string>

#include "output.hpp"
#include "arguments.hpp"
#include "concurrent_datastructures/flfs_file_handle.hpp"
#include "concurrent_datastructures/flfs_inode_entry.hpp"
#include "concurrent_datastructures/flfs_inode_lba.hpp"
#include "concurrent_datastructures/flfs_nlookup.hpp"
#include "concurrent_datastructures/flfs_snapshot.hpp"
#include "nvme_csd.hpp"
#include "flfs_constants.hpp"
#include "flfs_csd.hpp"
#include "flfs_dirtyblock.hpp"
#include "flfs_disc.hpp"
#include "flfs_init.hpp"
#include "flfs_read.hpp"
#include "flfs_memory.hpp"
#include "flfs_superblock.hpp"
#include "flfs_write.hpp"

#include "nvme_zns_backend.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * FUSE LFS filesystem for Zoned Namespaces SSDs (FluffleFS).
     */
    class FuseLFS : public FuseLFSCSD, public FuseLFSDirtyBlock,
        public FuseLFSInit, public FuseLFSRead, public FuseLFSFileHandle,
        public FuseLFSInodeEntry, public FuseLFSInodeLba, public FuseLFSNlookup,
        public FuseLFSSnapShot, public FuseLFSSuperBlock, public FuseLFSWrite
    {
    protected:
        /** Measurement Instrumentation */
        static size_t msr_reg[5];
        static const char* msr_reg_names[5];
        enum measure_reg_index {
            MSRI_REG_LOOKUP = 0, MSRI_REG_READ = 1, MSRI_REG_WRITE = 2,
            MSRI_REG_GETATTR= 3, MSRI_REG_SETATTR = 4,
        };
        static void register_msr_reg_namespaces();
    protected:
        arguments::options *options;

        // Concurrency management for global lock
        pthread_rwlock_t gl;
        pthread_rwlockattr_t gl_attr;

        struct fuse_conn_info *connection;

        struct nvme_zns::nvme_zns_info nvme_info;
        nvme_zns::NvmeZnsBackend *nvme;

        // Map filenames and their respective parent to inodes
        path_inode_map_t *path_inode_map;

        /** Initialization / Run methods */

        int run_init() override;

        /** Nlookup interface methods */

        // TODO(Dantali0n): Use nlookup count to drive path_inode_map caching
        //                  and response to memory pressure

        void inode_nlookup_increment(fuse_ino_t ino) override;
        void inode_nlookup_decrement(fuse_ino_t ino, uint64_t count) override;

        void fuse_reply_entry_nlookup(
            fuse_req_t req, struct fuse_entry_param *e) override;

        void fuse_reply_create_nlookup(
            fuse_req_t req, struct fuse_entry_param *e,
            const struct fuse_file_info *f) override;

        /** Inode, path and data position helper functions */

        void lba_to_position(
            uint64_t lba, struct data_position &position) const;

        void position_to_lba(
            struct data_position position, uint64_t &lba);

        /** Debug helper functions */

        void output_fi(const char *name, struct fuse_file_info *fi);

        /** FUSE helper functions */

        static int reply_buf_limited(fuse_req_t req, const char *buf,
                                     size_t bufsize, off_t off, size_t maxsize);
        static void dir_buf_add(fuse_req_t req, struct dir_buf* buf,
                                const char *name, fuse_ino_t ino);

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

        // TODO(Dantali0n): Get rid of this error prone method
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

        /** Inode methods */

        // Keep track of the highest observed ino and increment it for new
        // files and directories. The ino_ptr indicates the next possible ino
        // for new files and directories (similar to write pointers)
        fuse_ino_t ino_ptr;

        // TODO(Dantali0n): Create and keep track of ino_pos for log zone linear
        //                  continuity.

//        static int build_path_inode_map();

        int inode_stat(fuse_ino_t ino, struct stat *stbuf);

        int get_inode(fuse_ino_t ino, inode_entry_t *entry);

        int create_inode(fuse_ino_t parent, const char *name,
            enum inode_type type, fuse_ino_t &ino);

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
            enum snapshot_store_type snap_t) override;
        int update_snapshot(csd_unique_t *context, struct snapshot *snap,
            enum snapshot_store_type snap_t) override;
        int create_snapshot(fuse_ino_t ino, struct snapshot *snap) override;
        int has_snapshot(csd_unique_t *context,
            enum snapshot_store_type snap_t) override;
        int get_snapshot(csd_unique_t *context, csd_snapshot *snaps) override;
        int get_snapshot(csd_unique_t *context, struct snapshot *snap,
            enum snapshot_store_type snap_t) override;
        int delete_snapshot(csd_unique_t *context) override;

        // TODO(Dantali0n): Move Garbage Collection methods to separate
        //                  interface

        int log_garbage_collect();

        // TODO(Dantali0n): Move FUSE internal wrapper functions to separate
        //                  interface

        static int flfs_ret_to_fuse_reply(int flfs_ret);

        static int check_flags(int flags);

        static void ino_fake_permissions(fuse_req_t req, struct stat *stbuf);

        static void ino_fake_mtime(struct stat *stbuf);

        int ftruncate(fuse_ino_t ino, size_t size);

        void lookup_regular(fuse_req_t req, fuse_ino_t ino);

        void getattr_regular(fuse_req_t req, fuse_ino_t ino,
            struct fuse_file_info *fi);

        void setattr_regular(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
            int to_set, struct fuse_file_info *fi);

        /** CSD interface method */

        void create_csd_context(struct snapshot *snap, size_t size,
            off_t off, enum flfs_operations op, void *&call,
            uint64_t &call_size) override;

        void lookup_csd(fuse_req_t req, csd_unique_t *context) override;

        void read_csd(fuse_req_t req, csd_unique_t *context, size_t size,
            off_t off, struct fuse_file_info *fi) override;

        void write_csd(fuse_req_t req, csd_unique_t *context,
           const char *buf, size_t size, off_t off,
           struct write_context *wr_context,
           struct fuse_file_info *fi) override;

        void getattr_csd(fuse_req_t req, csd_unique_t *context,
            struct fuse_file_info *fi) override;

        void setattr_csd(fuse_req_t req, csd_unique_t *context,
            struct stat *attr, int to_set, struct fuse_file_info *fi) override;

        /** Read interface methods */

        void read_regular(fuse_req_t req, struct stat *stbuf,
            size_t size, off_t off, struct fuse_file_info *fi) override;

        int read_snapshot(csd_unique_t *context, size_t size,
            off_t off, void *buffer, struct snapshot *snap) override;

        /** Write interface methods */

        int write_sector(size_t size, off_t offset, uint64_t cur_lba,
             const char *data, uint64_t &result_lba) override;

        void write_regular(fuse_req_t req, fuse_ino_t ino, const char *buf,
            size_t size, off_t off, struct write_context *wr_context,
            struct fuse_file_info *fi) override;
        // Implemented but not used, commented out to remove clutter
//        void write_snapshot(fuse_req_t req, csd_unique_t *context,
//            const char *buf, size_t size, off_t off,
//            struct write_context *wr_context,
//            struct fuse_file_info *fi) override;

        // TODO(Dantali0n): Move xattr methods to separate interface

        void get_csd_xattr(fuse_req_t req, fuse_ino_t ino, size_t size);

        void set_csd_xattr(fuse_req_t req, struct open_file_entry *entry,
            const char *value, size_t size, int flags,
            enum snapshot_store_type snap_t);

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