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

#ifndef QEMU_CSD_NVME_ZNS_HPP
#define QEMU_CSD_NVME_ZNS_HPP

#include "nvme_zns_backend.hpp"
#include "nvme_zns_info.hpp"

namespace qemucsd::nvme_zns {

    /**
     * Special class to abstract away concrete backends that provided NVMe ZNS
     * drivers. Prevents excessive linkage of dependencies for FUSE_LFS while
     * also allowing to switch backends on the fly (decoupling).
     *
     * Method documentation written from the perspective of the caller, for
     * callee required behavior and expectations see NvmeZnsBackend instead.
     *
     * @tparam nvme_zns_backend The concrete backend implementation implementing
     *         NvmeZnsBackend
     */
    template<class nvme_zns_backend>
    class NvmeZns {
        static_assert(
            std::is_base_of<NvmeZnsBackend, nvme_zns_backend>::value,
            "template argument nvme_zns_backend must inherent NvmeZnsBackend");
    protected:
        nvme_zns_backend* backend;
    public:
        NvmeZns(nvme_zns_backend* backend);

        /**
         * Fill the nvme_zns_info struct with details about the underlying
         * drive.
         * @param info nvme_zns_info pointer
         */
        void get_nvme_zns_info(struct nvme_zns_info* info);

        /**
         * Perform a read operation on the drive at the specified _zone_ and
         * _sector_. All parameters are zero-indexed. _buffer_ must be of at
         * least size _size_. _offset_ + _size_ must be equal to
         * nvme_zns_info.sector_size. The combination of all size and location
         * parameters must be within the bounds of the underlying drive.
         * @return 0 upon success, < 0 upon failure
         */
        int read(uint64_t zone, uint64_t sector, uint64_t offset, void* buffer,
                 uint64_t size);

        /**
         * Perform a write append operation on the drive at the specified
         * _zone_. _sector_ will indicate the location of the appended data. All
         * parameters are zero-indexed. _buffer_ must be of at least size
         * _size_. _offset_ + _size_ must be equal to nvme_zns_info.sector_size.
         * The combination of all size and location parameters must be within
         * the bounds of the underlying drive.
         * @return 0 upon success, < 0 upon failure
         */
        int append(uint64_t zone, uint64_t& sector, uint64_t offset,
                   void* buffer, uint64_t size);

        /**
         * Resets the specified zone, all data within the zone will be lost.
         * _zone_ parameter is zero indexed.
         * @return 0 upon success, < 0 upon failure
         */
        int reset(uint64_t zone);
    };

    #include "nvme_zns.tpp"

}

#endif // QEMU_CSD_NVME_ZNS_HPP