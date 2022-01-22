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
        csd_instance = new nvme_csd::NvmeCsd(options->ubpf_mem_size,
            options->ubpf_jit, nvme);
    }

    FuseLFSCSD::~FuseLFSCSD() {
        delete csd_instance;
    }

    void FuseLFS::read_csd(fuse_req_t req, csd_unique_t *context, size_t size,
        off_t off, struct fuse_file_info *fi)
    {
        struct snapshot snap;

        if(get_snapshot(context, &snap, SNAP_READ) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        void *kernel_data = malloc(snap.inode_data.first.size);
        if(!kernel_data) {
            fuse_reply_err(req, ENOMEM);
            return;
        }

        if(read_snapshot(context, snap.inode_data.first.size, 0, kernel_data,
                         &snap) != FLFS_RET_NONE)
        {
            fuse_reply_err(req, EIO);
            return;
        }

        uint64_t result_size = csd_instance->nvm_cmd_bpf_run(kernel_data,
            snap.inode_data.first.size);
        free(kernel_data);

        void *result_data = malloc(result_size);
        csd_instance->nvm_cmd_bpf_result(result_data);

        reply_buf_limited(req, (const char*) result_data, size, off,
                          result_size);
        free(result_data);
    }

    void FuseLFS::write_csd(fuse_req_t req, csd_unique_t *context,
        const char *buf, size_t size, off_t off,
        struct write_context *wr_context,
        struct fuse_file_info *fi)
    {
        struct snapshot snap;

        if(get_snapshot(context, &snap, SNAP_WRITE) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        void *kernel_data = malloc(snap.inode_data.first.size);
        if(!kernel_data) {
            fuse_reply_err(req, ENOMEM);
            return;
        }
    }
}