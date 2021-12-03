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

#include "nvme_zns_backend.hpp"

namespace qemucsd::nvme_zns {

    NvmeZnsBackend::NvmeZnsBackend(struct nvme_zns_info *info) {
        this->info = *info;

        zone_byte_size = info->sector_size * info->zone_capacity;
    }

    NvmeZnsBackend::NvmeZnsBackend(uint64_t num_zones, uint64_t zone_size,
        uint64_t zone_capacity, uint64_t sector_size, uint64_t max_open)
    {
        info.num_zones = num_zones;
        info.zone_size = zone_size;
        info.zone_capacity = zone_capacity;
        info.sector_size = sector_size;

        info.max_open = max_open;

        zone_byte_size = sector_size * zone_capacity;
        device_byte_size = zone_byte_size * num_zones;
    }

    int NvmeZnsBackend::in_range(
        uint64_t zone, uint64_t sector, uint64_t offset, uint64_t size)
    {
        // Ranges are zero indexed so equal is already out of range
        if(zone >= info.num_zones ||
           sector >= info.zone_capacity ||
           offset >= info.sector_size) {
            return -1;
        }

        // Verify that desired size does not cause operation to
        // exceed total device size limits
        if((zone_byte_size * zone) + (info.sector_size * sector) +
            offset + size >= device_byte_size)
            return -1;

        return 0;
    }

    /**
     * Concrete implementation of virtual void function, recommended to just
     * call this from within the overriding function.
     */
    void NvmeZnsBackend::get_nvme_zns_info(struct nvme_zns_info* info) {
        *info = this->info;
    }

}