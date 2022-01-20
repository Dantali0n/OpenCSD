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

    NvmeZnsSpdkBackend::NvmeZnsSpdkBackend(struct ns_entry* entry) :
        NvmeZnsBackend(entry->device_size, entry->zone_size,
                       entry->zone_capacity, entry->sector_size, entry->max_open)
    {
        if(entry->qpair == nullptr || entry->ns == nullptr ||
           entry->ctrlr == nullptr)
        {
           std::cerr << "SPDK ns_entry not properly initialized" << std::endl;
           exit(1);
        }

        this->entry = entry;

        write_pointers.resize(info.num_zones);

        /**
         * Update the write pointers to their current location.
         * Its inexcusable how difficult and ugly this is in SPDK.
         */
        uint32_t report_bufsize =
            spdk_nvme_ns_get_max_io_xfer_size(entry->ns);
        auto *report_buf = (spdk_nvme_zns_zone_report *)
            malloc(report_bufsize);
        uint64_t zones = 0;
        while(zones < info.num_zones) {
            uint64_t lba = zones * info.zone_size;

            spdk_nvme_zns_report_zones(
                entry->ns, entry->qpair, report_buf, report_bufsize, lba,
                SPDK_NVME_ZRA_LIST_ALL, true, spdk_init::error_print, &entry);
            spdk_init::spin_complete(entry);

            for(uint64_t i = 0; i < report_buf->nr_zones; i++) {
                uint64_t normalized_wp = report_buf->descs[i].wp -
                    report_buf->descs[i].zslba;

                if(normalized_wp > report_buf->descs[i].zcap)
                    normalized_wp = report_buf->descs[i].zcap;

                write_pointers.at(zones + i) = normalized_wp;
            }

            zones += report_buf->nr_zones;
        }

        free(report_buf);
    }

    NvmeZnsSpdkBackend::~NvmeZnsSpdkBackend() {
        if(entry->ctrlr != nullptr) spdk_nvme_detach(entry->ctrlr);
        if(entry->buffer != nullptr) spdk_free(entry->buffer);
    }

    void NvmeZnsSpdkBackend::get_nvme_zns_info(struct nvme_zns_info* info) {
        NvmeZnsBackend::get_nvme_zns_info(info);
    }

    int NvmeZnsSpdkBackend::read(
        uint64_t zone, uint64_t sector, uint64_t offset, void *buffer,
        uint64_t size)
    {
        uint64_t lba;
        struct ns_entry entry = *this->entry;

        if(in_range(zone, sector, offset, size) != 0)
            return -1;

        position_to_lba(zone, sector, lba);

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

    int NvmeZnsSpdkBackend::append(
        uint64_t zone, uint64_t &sector, uint64_t offset, void *buffer,
        uint64_t size)
    {
        uint64_t lba;
        struct ns_entry entry = *this->entry;

        if(in_range(zone, 0, offset, size) != 0)
            return -1;

        position_to_lba(zone, 0, lba);

        // Refuse to append to full zone
        if(write_pointers.at(zone) >= info.zone_size)
            return -1;

        // Only support appending a single sector for now
        // TODO(Dantali0n): Allow to append multiple sectors in one call
        if(offset + size != info.sector_size)
            return -1;

        if(offset != 0) memset(entry.buffer, 0, offset);
        uint64_t remainder = (offset + size) % info.sector_size;
        if(remainder != 0) memset((uint8_t*)entry.buffer + offset + size, 0,
                                  remainder);
        memcpy(entry.buffer, (uint8_t*)buffer + offset, size);

        spdk_nvme_zns_zone_append(entry.ns, entry.qpair, entry.buffer,
                                  lba, 1, spdk_init::error_print, &entry, 0);

        spdk_init::spin_complete(&entry);

        // Indicate location of written data
        sector = write_pointers.at(zone);

        // Advance write pointer by one
        write_pointers.at(zone) = write_pointers.at(zone) + 1;

        return 0;
    }

    int NvmeZnsSpdkBackend::reset(uint64_t zone) {
        uint64_t lba;
        struct ns_entry entry = *this->entry;

        if(in_range(zone, 0, 0, 0) != 0)
            return -1;

        position_to_lba(zone, 0, lba);

        spdk_nvme_zns_reset_zone(entry.ns, entry.qpair, lba, false,
                                 spdk_init::error_print, &entry);

        spdk_init::spin_complete(&entry);

        write_pointers.at(zone) = 0;

        return 0;
    }
}