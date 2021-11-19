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

#include "nvme_zns_spdk.hpp"

using qemucsd::spdk_init::ns_entry;

namespace qemucsd::nvme_zns {

    NvmeZnsMemorySpdk::NvmeZnsMemorySpdk(struct ns_entry* entry) :
        NvmeZnsBackend(entry->device_size, entry->zone_size, entry->lba_size,
                       entry->max_open)
    {
        if(entry->qpair == nullptr || entry->ns == nullptr ||
           entry->ctrlr == nullptr)
        {
           std::cerr << "SPDK ns_entry not properly initialized" << std::endl;
           exit(1);
        }

        this->entry = entry;
    }

    int NvmeZnsMemorySpdk::compute_sector(
        uint64_t zone, uint64_t sector, uint64_t offset,
        uint64_t size, uint64_t& result_sector)
    {

    }

    void NvmeZnsMemorySpdk::get_nvme_zns_info(struct nvme_zns_info* info) {
        NvmeZnsBackend::get_nvme_zns_info(info);
    }

    int NvmeZnsMemorySpdk::read(
        uint64_t zone, uint64_t sector, uint64_t offset, void *buffer,
        uint64_t size)
    {
        uint64_t res_sector;
        struct ns_entry entry = *this->entry;

        if(compute_sector(zone, sector, offset, size, res_sector) == false)
            return -1;

        // Only support reading a single sector for now
        // TODO(Dantali0n): Allow to read multiple sectors in one call
        if(offset + size != info.sector_size)
            return -1;

        spdk_nvme_ns_cmd_read(entry.ns, entry.qpair,
                              entry.buffer, res_sector, 1,
                              spdk_init::error_print, &entry,0);
        spdk_init::spin_complete(&entry);

        memcpy(buffer, (uint8_t*)entry.buffer + offset, size);
    }

    int NvmeZnsMemorySpdk::append(
        uint64_t zone, uint64_t &sector, uint64_t offset, void *buffer,
        uint64_t size)
    {
        uint64_t res_sector;
        struct ns_entry entry = *this->entry;

        if(compute_sector(zone, 0, offset, size, res_sector) == false)
            return -1;

        spdk_nvme_zns_zone_append(entry.ns, entry.qpair, entry.buffer,
            res_sector, 1, spdk_init::error_print, &entry, 0);

    }

    int NvmeZnsMemorySpdk::reset(uint64_t zone) {
        uint64_t res_sector;
        struct ns_entry entry = *this->entry;

        if(zone >= info.num_zones)
            return -1;

        res_sector = info.zone_size * zone;

        spdk_nvme_zns_reset_zone(entry.ns, entry.qpair, res_sector, false,
                                 spdk_init::error_print, &entry);

        spdk_init::spin_complete(&entry);
    }
}