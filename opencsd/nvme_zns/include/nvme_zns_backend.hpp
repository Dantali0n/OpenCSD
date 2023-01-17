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

        /**
         * Check that each of the individual params does not exceed the
         * underlying device hierarchy. In addition check that the combination
         * of parameters does not exceed the total device size.
         * @return 0 upon success, < 0 upon failure
         */
        int in_range(uint64_t zone, uint64_t sector, uint64_t offset,
            uint64_t size);
    public:
        explicit NvmeZnsBackend(struct nvme_zns_info* info);

        /**
         * Initializes nvme_zns_info _info_ member variable and computes
         * _device_byte_size_ and _zone_byte_size_
         */
        NvmeZnsBackend(uint64_t num_zones,  uint64_t zone_size,
            uint64_t zone_capacity, uint64_t sector_size, uint64_t max_open);

        /**
         * Convert zone and requested sector to a Logical Block Address (LBA).
         * Takes into account gaps created by zone capacity and zone size
         * differences.
         */
        void position_to_lba(uint64_t zone, uint64_t sector, uint64_t &lba);

        /**
         * Provide nvme_zns_info to caller, can be implemented by calling
         * NvmeZnsBackend base implementation in almost all cases.
         */
        virtual void get_nvme_zns_info(struct nvme_zns_info* info) = 0;

        /**
         * Perform a read operation on the requested zone and sector for size
         * _size_. Put this data in _buffer_ starting from _offset_. Make sure
         * to use _in_range_ to determine if the request is possible.
         * @return 0 upon success, < 0 upon failure
         */
        virtual int read(
            uint64_t zone, uint64_t sector, uint64_t offset, void *buffer,
            uint64_t size) = 0;

        /**
         * Perform a write append to the requested zone for size _size_.
         * Write the data from _buffer to the device starting from _offset_.
         * Update _sector_ to indicate start location of written data. Make sure
         * to use _in_range_ to determine if the request is possible.
         * @return 0 upon success, < 0 upon failure
         */
        virtual int append(
            uint64_t zone, uint64_t &sector, uint64_t offset, void *buffer,
            uint64_t size) = 0;

        /**
         * Reset the indicated zone and ensure the requested zones is below
         * < num_zones from nvme_zns_info.
         * @return  0 upon success, < 0 upon failure
         */
        virtual int reset(uint64_t zone) = 0;
    };

}

#endif // QEMU_CSD_NVME_ZNS_BACKEND_HPP