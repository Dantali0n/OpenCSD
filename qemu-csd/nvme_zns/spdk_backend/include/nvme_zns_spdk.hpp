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

#ifndef QEMU_CSD_NVME_ZNS_SPDK_HPP
#define QEMU_CSD_NVME_ZNS_SPDK_HPP

#include "nvme_zns_backend.hpp"

namespace qemucsd::nvme_zns {

    class NvmeZnsMemorySpdk : NvmeZnsBackend {
    protected:
        struct nvme_zns_info info;
        struct ns_entry* entry;
    public:
        NvmeZnsMemorySpdk(struct ns_entry* entry);

        void get_nvme_zns_info(struct nvme_zns_info* info) override;

        int read(uint64_t zone, uint64_t sector, size_t offset, void *buffer,
                 size_t size) override;

        int append(uint64_t zone, uint64_t &sector, size_t offset, void *buffer,
                   size_t size) override;

        int reset(uint64_t zone) override;
    };

}

#endif // QEMU_CSD_NVME_ZNS_SPDK_HPP