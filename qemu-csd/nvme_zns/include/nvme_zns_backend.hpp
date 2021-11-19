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

#ifndef QEMU_CSD_NVME_ZNS_BACKEND_HPP
#define QEMU_CSD_NVME_ZNS_BACKEND_HPP

#include <cstddef>
#include <cstdint>

#include "nvme_zns_info.hpp"

namespace qemucsd::nvme_zns {

    class NvmeZnsBackend {
    protected:
        uint64_t device_byte_size;
        uint64_t zone_byte_size;

        struct nvme_zns_info info;

        bool in_range(uint64_t zone, uint64_t sector, uint64_t offset,
            uint64_t size);
    public:
        explicit NvmeZnsBackend(struct nvme_zns_info* info);

        NvmeZnsBackend(uint64_t num_zones,  uint64_t zone_size,
            uint64_t sector_size, uint64_t max_open);

        virtual void get_nvme_zns_info(struct nvme_zns_info* info) = 0;

        virtual int read(
            uint64_t zone, uint64_t sector, uint64_t offset, void *buffer,
            uint64_t size) = 0;

        virtual int append(
            uint64_t zone, uint64_t &sector, uint64_t offset, void *buffer,
            uint64_t size) = 0;

        virtual int reset(uint64_t zone) = 0;
    };

}

#endif // QEMU_CSD_NVME_ZNS_BACKEND_HPP