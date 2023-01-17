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

#ifndef QEMU_CSD_NVME_ZNS_MEMORY_HPP
#define QEMU_CSD_NVME_ZNS_MEMORY_HPP

#include <cstddef>
#include <cstring>
#include <iostream>
#include <vector>
#include <mutex>

#include "output.hpp"
#include "measurements.hpp"
#include "nvme_zns_backend.hpp"

namespace qemucsd::nvme_zns {

    static output::Output output = output::Output(
        "[NVME_ZNS_MEMORY] ", output::INFO);

    class NvmeZnsMemoryBackend : public NvmeZnsBackend {
    protected:

        std::vector<uint64_t> write_pointers;

        std::mutex gl;

        uintptr_t memory_limit;

        unsigned char* data;

        // Measurement variables
        static size_t msr_read_identifier;
        static size_t msr_append_identifier;
        static size_t msr_reset_identifier;

        int compute_address(uint64_t zone, uint64_t sector, uint64_t offset,
                            uint64_t size, uintptr_t& address);

    public:
        NvmeZnsMemoryBackend(
            uint64_t num_zones,  uint64_t zone_size, uint64_t sector_size);

        // Virtual required to enforce destructor is called in super classes
        virtual ~NvmeZnsMemoryBackend();

        void get_nvme_zns_info(struct nvme_zns_info* info) override;

        int read(uint64_t zone, uint64_t sector, uint64_t offset, void* buffer,
                 uint64_t size) override;

        int append(uint64_t zone, uint64_t& sector, uint64_t offset,
                   void* buffer, uint64_t size) override;

        int reset(uint64_t zone) override;
    };

}

#endif // QEMU_CSD_NVME_ZNS_MEMORY_HPP