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

#ifndef QEMU_CSD_FLFS_CSD_HPP
#define QEMU_CSD_FLFS_CSD_HPP

extern "C" {
    #include <fuse3/fuse_lowlevel.h>
}

#include <cstddef>

#include "flfs_memory.hpp"
#include "flfs_write.hpp"
#include "nvme_csd.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * Interface for CSD methods
     */
    class FuseLFSCSD {
    protected:
        nvme_csd::NvmeCsd *csd_instance;
    public:
        FuseLFSCSD(arguments::options *options, nvme_zns::NvmeZnsBackend *nvme);
        virtual ~FuseLFSCSD();

        virtual int read_csd(fuse_req_t req, csd_unique_t *context, size_t size,
            off_t off, struct fuse_file_info *fi) = 0;

        virtual int write_csd(fuse_req_t req, csd_unique_t *context,
            const char *buf, size_t size, off_t off,
            struct write_context *wr_context, struct fuse_file_info *fi) = 0;
    };

}

#endif // QEMU_CSD_FLFS_CSD_HPP