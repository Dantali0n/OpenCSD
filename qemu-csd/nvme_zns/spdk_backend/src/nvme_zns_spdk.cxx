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
        NvmeZnsBackend(entry->device_size, entry->zone_size,
                       entry->zone_capacity, entry->lba_size, entry->max_open)
    {
        if(entry->qpair == nullptr || entry->ns == nullptr ||
           entry->ctrlr == nullptr)
        {
           std::cerr << "SPDK ns_entry not properly initialized" << std::endl;
           exit(1);
        }

        this->entry = entry;

        write_pointers.resize(info.num_zones);
        for(auto& write_pointer : write_pointers) {
            write_pointer = 0;
        }
    }

    void NvmeZnsMemorySpdk::get_nvme_zns_info(struct nvme_zns_info* info) {
        NvmeZnsBackend::get_nvme_zns_info(info);
    }

    int NvmeZnsMemorySpdk::read(
        uint64_t zone, uint64_t sector, uint64_t offset, void *buffer,
        uint64_t size)
    {
        uint64_t lba;
        struct ns_entry entry = *this->entry;

        if(in_range(zone, sector, offset, size) != 0)
            return -1;

        if(position_to_lba(zone, sector, lba) != 0)
            return -1;

        // Refuse to read unwritten sectors
        if(write_pointers.at(zone) <= sector) return -1;

        // Only support reading a single sector for now
        // TODO(Dantali0n): Allow to read multiple sectors in one call
        if(offset + size != info.sector_size)
            return -1;

        spdk_nvme_ns_cmd_read(entry.ns, entry.qpair,
                              entry.buffer, lba, 1,
                              spdk_init::error_print, &entry,0);
        spdk_init::spin_complete(&entry);

        memcpy(buffer, (uint8_t*)entry.buffer + offset, size);

        return 0;
    }

    int NvmeZnsMemorySpdk::append(
        uint64_t zone, uint64_t &sector, uint64_t offset, void *buffer,
        uint64_t size)
    {
        uint64_t lba;
        struct ns_entry entry = *this->entry;

        if(in_range(zone, 0, offset, size) != 0)
            return -1;

        if(position_to_lba(zone, 0, lba) != 0)
            return -1;

        // Refuse to append to full zone
        if(write_pointers.at(zone) >= info.zone_size)
            return -1;

        // Only support appending a single sector for now
        // TODO(Dantali0n): Allow to append multiple sectors in one call
        if(offset + size != info.sector_size)
            return -1;

        spdk_nvme_zns_zone_append(entry.ns, entry.qpair, entry.buffer,
                                  lba, 1, spdk_init::error_print, &entry, 0);

        spdk_init::spin_complete(&entry);

        // Indicate location of written data
        sector = write_pointers.at(zone);

        // Advance write pointer by one
        write_pointers.at(zone) = write_pointers.at(zone) + 1;

        return 0;
    }

    int NvmeZnsMemorySpdk::reset(uint64_t zone) {
        uint64_t lba;
        struct ns_entry entry = *this->entry;

        if(in_range(zone, 0, 0, 0) != 0)
            return -1;

        if(position_to_lba(zone, 0, lba) != 0)
            return -1;

        spdk_nvme_zns_reset_zone(entry.ns, entry.qpair, lba, false,
                                 spdk_init::error_print, &entry);

        spdk_init::spin_complete(&entry);

        write_pointers.at(zone) = 0;

        return 0;
    }
}