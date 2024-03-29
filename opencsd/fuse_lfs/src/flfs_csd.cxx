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

    FuseLFSCSD::FuseLFSCSD(qemucsd::arguments::options *options,
        nvme_zns::NvmeZnsBackend *nvme)
    {
        register_msr_csd_namespaces();

        csd_instance = new nvme_csd::NvmeCsd(options->ubpf_mem_size,
            options->ubpf_jit, nvme);
    }

    FuseLFSCSD::~FuseLFSCSD() {
        delete csd_instance;
    }

    size_t FuseLFSCSD::msr_csd[5] = {0};
    const char *FuseLFSCSD::msr_csd_names[5] = {
        "FUSE_LFS][CSD][lookup", "FUSE_LFS][CSD][read", "FUSE_LFS][CSD][write",
        "FUSE_LFS][CSD][getattr", "FUSE_LFS][CSD][setattr",
    };

    void FuseLFSCSD::register_msr_csd_namespaces() {
        for(uint32_t i = 0; i < 5; i++) {
            measurements::register_namespace(msr_csd_names[i], msr_csd[i]);
        }
    }

    /**
     * Determine the selection data blocks that need to be passed to the CSD
     * kernel. These are flattened one after another and only include the exact
     * blocks required for the kernel to run.
     *
     * Kernels can include bpf_helpers_flfs.h to operate on these flattened
     * blocks.
     */
    void FuseLFSCSD::flatten_data_blocks(uint64_t size, uint64_t off,
        data_map_t *blocks, std::vector<uint64_t> *flat_blocks)
    {
        uint64_t start_lba = off / SECTOR_SIZE;
        uint64_t end_lba = (size + off) / SECTOR_SIZE;
        if((size + off) % SECTOR_SIZE != 0) end_lba += 1;

        uint64_t start_block = start_lba / DATA_BLK_LBA_NUM;

        flat_blocks->reserve(end_lba - start_lba);
        struct data_block *cur_blk = &blocks->at(start_block);
        for(uint64_t i = start_lba; i < end_lba; i++) {
            flat_blocks->push_back(cur_blk->data_lbas[i % DATA_BLK_LBA_NUM]);

            if((i + 1) % DATA_BLK_LBA_NUM == 0) {
                start_block += 1;
                cur_blk = &blocks->at(start_block);
            }
        }
    }

    /**
     * Create the filesystem specific data structure to pass to CSD kernels.
     * Datastructure is different for read_stream vs write_event kernels.
     */
    void FuseLFS::create_csd_context(struct snapshot *snap, size_t size,
        off_t off, enum flfs_operations op, void *&call, uint64_t &call_size)
    {
        uint64_t fcall_size = sizeof(struct flfs_call);
        struct flfs_call context = {};

        context.dims.size = size;
        context.dims.offset = off;

        context.ino.inode = snap->inode_data.first.inode;
        context.ino.parent = snap->inode_data.first.parent;
        context.ino.size = snap->inode_data.first.size;
        context.ino.type = snap->inode_data.first.type == INO_T_FILE ?
            FLFS_FILE : FLFS_DIR;

        context.op = op;

        // Provide sector aligned data to be written, ensure any pre-existing
        // data in first and last sector are padded with result from drive.
        if(op == FLFS_WRITE_EVENT) {
            fcall_size += sizeof(struct flfs_write_call);

            // First sector (pre) pad if data exists and not sector aligned
            if(off < context.ino.size && off % SECTOR_SIZE != 0) {
            }


            // Last sector (post) pad if data exists and not sector aligned
            if(off + size < context.ino.size && off + size % SECTOR_SIZE != 0) {
            }
        }

        // Provide flattened data blocks to kernel.
        std::vector<uint64_t> flat_blocks;
        flatten_data_blocks(size, off, &snap->data_blocks,
                            &flat_blocks);

        uint64_t lbas_size = flat_blocks.size() * sizeof(uint64_t);

        call_size = lbas_size + fcall_size;
        call = malloc(call_size);
        memcpy(call, &context, fcall_size);
        memcpy((uint8_t*)call + fcall_size, flat_blocks.data(), lbas_size);
    }

    void FuseLFS::lookup_csd(fuse_req_t req, csd_unique_t *context) {
        measurements::measure_guard msr_guard(msr_csd[MSRI_CSD_LOOKUP]);
        struct fuse_entry_param e = {0};
        struct snapshot snap;

        if(get_snapshot(context, &snap, SNAP_FILE) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        e.ino = context->first;
        e.attr_timeout = 0.0;
        e.entry_timeout = 0.0;
        if(inode_stat(context->first, &e.attr) == FLFS_RET_ENOENT) {
            fuse_reply_err(req, ENOENT);
            return;
        }

        #ifdef FLFS_FAKE_PERMS
        ino_fake_permissions(req, &e.attr);
        #endif

        e.attr.st_size = snap.inode_data.first.size;

        // Do not increment nlookup for snapshot lookups
        fuse_reply_entry(req, &e);
    }

    /**
     * Run the read operation through the offloaded CSD functionality.
     * @attention read_csd fastly different from write_csd stream vs event
     *            kernel.
     */
    void FuseLFS::read_csd(fuse_req_t req, csd_unique_t *context, size_t size,
        off_t off, struct fuse_file_info *fi)
    {

        measurements::measure_guard msr_guard(msr_csd[MSRI_CSD_READ]);
        struct snapshot kernel_snap;

        /** Get the snapshot information for the read kernel */
        if(get_snapshot(context, &kernel_snap, SNAP_READ_STREAM) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        /** Allocate a buffer sufficient for the contents of the read kernel */
        void *kernel_data = malloc(kernel_snap.inode_data.first.size);
        if(!kernel_data) {
            fuse_reply_err(req, ENOMEM);
            return;
        }

        /** Read the read kernel contents from the drive */
        if(read_snapshot(context, kernel_snap.inode_data.first.size, 0,
            kernel_data, &kernel_snap) != FLFS_RET_NONE)
        {
            fuse_reply_err(req, EIO);
            return;
        }

        /** Get the snapshot for the file itself */
        struct snapshot file_snap;
        if(get_snapshot(context, &file_snap, SNAP_FILE) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        /** Check that size and offset match file snapshot dimensions,
         * return early if out of range (return FLFS_RET_ERR) */
        if(read_precheck(req, file_snap.inode_data.first, size, off) !=
            FLFS_RET_NONE)
        {
            return;
        }

        /** Verify CSD read strides */
        if(size != CSD_READ_STRIDE && off + size <
            file_snap.inode_data.first.size)
        {
            uint64_t t_ino = file_snap.inode_data.first.inode;
            output.error("Strided read not aligned for ", t_ino, " with size ",
                size);
        }

        /** Create the filesystem context to present to the kernel during
         * execution. */
        void *call = nullptr;
        uint64_t call_size;
        create_csd_context(&file_snap, size, off, FLFS_READ_STREAM, call,
            call_size);

        /** Launch the uBPF vm with the read kernel and filesystem context */
        int64_t result_size = csd_instance->nvm_cmd_bpf_run_fs(kernel_data,
            kernel_snap.inode_data.first.size, call, call_size);
        free(kernel_data);
        free(call);

        /** Use results of less than 0 for errors */
        if(result_size < 0) {
            output.error("Kernel encountered error ", result_size);
            fuse_reply_err(req, EIO);
            return;
        }

        /** Fetch the result data from the kernel */
        void *result_data = malloc(result_size);
        csd_instance->nvm_cmd_bpf_result(result_data);

        fuse_reply_buf(req, (const char*)result_data, flfs_min(result_size, size));
        free(result_data);
    }

    /**
     * Run the write operation through the offloaded CSD functionality.
     * @attention write_csd fastly different from read_csd event vs stream
     *            kernel.
     */
    void FuseLFS::write_csd(fuse_req_t req, csd_unique_t *context,
        const char *buf, size_t size, off_t off,
        struct write_context *wr_context, struct fuse_file_info *fi)
    {
        measurements::measure_guard msr_guard(msr_csd[MSRI_CSD_WRITE]);
        struct snapshot snap;

        if(get_snapshot(context, &snap, SNAP_WRITE_STREAM) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        void *kernel_data = malloc(snap.inode_data.first.size);
        if(!kernel_data) {
            fuse_reply_err(req, ENOMEM);
            return;
        }
    }

    void FuseLFS::getattr_csd(fuse_req_t req, csd_unique_t *context,
        struct fuse_file_info *fi)
    {
        measurements::measure_guard msr_guard(msr_csd[MSRI_CSD_GETATTR]);
        struct stat stbuf = {0};
        struct snapshot snap;

        if(get_snapshot(context, &snap, SNAP_FILE) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        if (inode_stat(context->first, &stbuf) == FLFS_RET_ENOENT)
            fuse_reply_err(req, ENOENT);

        #ifdef FLFS_FAKE_PERMS
        ino_fake_permissions(req, &stbuf);
        #endif

        stbuf.st_size = snap.inode_data.first.size;

        fuse_reply_attr(req, &stbuf, 90.0);
    }

    void FuseLFS::setattr_csd(fuse_req_t req, csd_unique_t *context,
        struct stat *attr, int to_set, struct fuse_file_info *fi)
    {/*
*/        measurements::measure_guard msr_guard(msr_csd[MSRI_CSD_SETATTR]);
        struct snapshot snap;
        if(get_snapshot(context, &snap, SNAP_FILE) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        // Change permissions (chmod)
        if(to_set & FUSE_SET_ATTR_MODE) {

        }
        // Change owner (chown)
        if(to_set & FUSE_SET_ATTR_UID) {

        }
        // Change group (chgrp)
        if(to_set & FUSE_SET_ATTR_GID) {

        }
        // Change size of file (truncate / ftruncate)
        if(to_set & FUSE_SET_ATTR_SIZE) {
            snap.inode_data.first.size = attr->st_size;
        }

        update_snapshot(context, &snap, SNAP_FILE);

        getattr_csd(req, context, fi);
    }
}