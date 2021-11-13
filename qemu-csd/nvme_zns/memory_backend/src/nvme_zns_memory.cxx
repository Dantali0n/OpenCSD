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

#include "nvme_zns_memory.hpp"

namespace qemucsd::nvme_zns {

    NvmeZnsMemoryBackend::NvmeZnsMemoryBackend(
        size_t num_zones, size_t zone_size, size_t sector_size)
    {
        info.num_zones = num_zones;
        info.zone_size = zone_size;
        info.sector_size = sector_size;

        info.max_open = 0;

        uint64_t size = num_zones * zone_size * sector_size * sizeof(*data);
        data = (unsigned char*) malloc(size);

        if(!data) {
            std::cerr << "nvm_zns_memory_backend memory allocation for " <<
                "size: " << size << " failed." << std::endl;
            exit(1);
        }

        zone_byte_size = info.zone_size * info.sector_size;
        memory_limit = (uintptr_t) (void*)data + size;

        write_pointers.resize(num_zones);
        for(auto& write_pointer : write_pointers) {
            write_pointer = 0;
        }
    }

    NvmeZnsMemoryBackend::~NvmeZnsMemoryBackend() {
        free(data);
    }

    bool NvmeZnsMemoryBackend::in_range(
        uint64_t zone, uint64_t sector, size_t offset)
    {
        // Ranges are zero indexed so equal is already out of range
        if(zone >= info.num_zones ||
            sector >= info.zone_size ||
            offset >= info.sector_size) {
            return false;
        }

        return true;
    }

    int NvmeZnsMemoryBackend::compute_address(
        uint64_t zone, uint64_t sector, size_t offset, size_t size, uintptr_t& address)
    {
        if(in_range(zone, sector, offset) == false) return -1;

        // Address = modifies external variable before guaranteed success
        // this is okaj because compute_address is a protected member function.
        address = (zone * zone_byte_size) + (sector * info.sector_size) + offset;
        if(memory_limit < (uintptr_t) data + address + size) return -1;

        return 0;
    }

    void NvmeZnsMemoryBackend::get_nvme_zns_info(struct nvme_zns_info* info) {
        *info = this->info;
    }

    int NvmeZnsMemoryBackend::read(
        uint64_t zone, uint64_t sector, size_t offset, void* buffer,
        size_t size)
    {
        uintptr_t address;
        // Determine address offset and verify in range
        if(compute_address(zone, sector, offset, size, address) != 0)
            return -1;

        // Refuse to read unwritten sectors
        if(write_pointers.at(zone) < sector) return -1;

        memcpy(buffer, data + address, size);

        return 0;
    }

    int NvmeZnsMemoryBackend::append(
        uint64_t zone, uint64_t& sector, size_t offset, void* buffer,
        size_t size)
    {
        size_t remainder = (offset + size) % info.sector_size;
        uintptr_t address;
        // Determine address offset and verify in range
        if(compute_address(zone, write_pointers.at(zone), offset, size,
                           address) != 0)
            return -1;

        // Write pointer advancements
        uint64_t temp_write_pointer = write_pointers.at(zone);
        temp_write_pointer += (offset + size) / info.sector_size;
        if(remainder != 0) temp_write_pointer += 1;

        // Write pointer should never advance into next zone
        if(temp_write_pointer > info.zone_size) return -1;

        // All is well, update the write pointer
        write_pointers.at(zone) = temp_write_pointer;

        // Only modify external variable after guaranteeing success
        sector = write_pointers.at(zone);

        // Zero offset into the sector if the offset is non zeros
        if(offset != 0) memset(data + address - offset, 0, offset);

        memcpy(data + address, buffer, size);

        // Zero remainder of last sector
        if(remainder != 0) memset(data + address + size, 0, remainder);

        return 0;
    }

    int NvmeZnsMemoryBackend::reset(uint64_t zone) {
        uintptr_t address = zone * info.zone_size * info.sector_size;
        if(memory_limit < (uintptr_t) data + address + zone_byte_size)
            return -1;

        write_pointers.at(zone) = 0;

        memset(data + address, 0, zone_byte_size);

        return 0;
    };
}