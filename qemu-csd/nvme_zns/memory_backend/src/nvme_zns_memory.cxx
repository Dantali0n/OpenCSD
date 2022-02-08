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

    size_t NvmeZnsMemoryBackend::msr_read_identifier = 0;
    size_t NvmeZnsMemoryBackend::msr_append_identifier = 0;
    size_t NvmeZnsMemoryBackend::msr_reset_identifier = 0;

    NvmeZnsMemoryBackend::NvmeZnsMemoryBackend(
        uint64_t num_zones, uint64_t zone_size, uint64_t sector_size) :
        // TODO(Dantali0n): Remove halving of zone capacity
        NvmeZnsBackend(num_zones, zone_size, zone_size / 2, sector_size, 0)
    {

        measurements::register_namespace(
            "NVME_ZNS_MEMORY][read", msr_read_identifier);
        measurements::register_namespace(
            "NVME_ZNS_MEMORY][append", msr_append_identifier);
        measurements::register_namespace(
            "NVME_ZNS_MEMORY][reset", msr_reset_identifier);

        info.max_open = 0;

        uint64_t size = num_zones * info.zone_capacity * sector_size * sizeof(*data);
        data = (unsigned char*) malloc(size);

        if(!data) {
            output(std::cerr, "nvm_zns_memory_backend memory allocation for ",
                "size: ", size, " failed.");
            exit(1);
        }

        zone_byte_size = info.zone_capacity * info.sector_size;
        memory_limit = (uintptr_t) (void*)data + size;

        write_pointers.resize(num_zones);
        for(auto& write_pointer : write_pointers) {
            write_pointer = 0;
        }
    }

    NvmeZnsMemoryBackend::~NvmeZnsMemoryBackend() {
        free(data);
    }

    /**
     * Determine if the requested zone, sector, offset and size fit within the
     * allocated memory. Puts the resulting address in the address parameter.
     * @param address the resulting memory address is copied into this variable
     * @return 0 upon success, < 0 upon failure
     */
    int NvmeZnsMemoryBackend::compute_address(
        uint64_t zone, uint64_t sector, uint64_t offset, uint64_t size, uintptr_t& address)
    {
        if(in_range(zone, sector, offset, size) != 0) return -1;

        address = (zone * zone_byte_size) + (sector * info.sector_size) + offset;

        return 0;
    }

    void NvmeZnsMemoryBackend::get_nvme_zns_info(struct nvme_zns_info* info) {
        NvmeZnsBackend::get_nvme_zns_info(info);
    }

    int NvmeZnsMemoryBackend::read(
        uint64_t zone, uint64_t sector, uint64_t offset, void* buffer,
        uint64_t size)
    {
        measurements::measure_guard msr_guard(msr_read_identifier);
        std::lock_guard<std::mutex> guard(gl);

        uintptr_t address;
        // Determine address offset and verify in range
        if(compute_address(zone, sector, offset, size, address) != 0)
            return -1;

        // Refuse to read unwritten sectors
        if(write_pointers.at(zone) <= sector) return -1;

        output(std::cout, output::DEBUG,"read: [", zone, "][", sector, "][",
               offset, "][", size, "]");

        memcpy(buffer, data + address, size);

        return 0;
    }

    int NvmeZnsMemoryBackend::append(
        uint64_t zone, uint64_t& sector, uint64_t offset, void* buffer,
        uint64_t size)
    {
        measurements::measure_guard msr_guard(msr_append_identifier);
        std::lock_guard<std::mutex> guard(gl);

        uint64_t remainder = (offset + size) % info.sector_size;
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
        if(temp_write_pointer > info.zone_capacity) return -1;

        // Only modify external variable after guaranteeing success
        sector = write_pointers.at(zone);

        output(std::cout, output::DEBUG, "append: [", zone, "][", sector,
               "][", offset, "][", size, "]");

        // All is well, update the write pointer
        write_pointers.at(zone) = temp_write_pointer;

        // Zero offset into the sector if the offset is non zeros
        if(offset != 0) memset(data + address - offset, 0, offset);

        memcpy(data + address, buffer, size);

        // Zero remainder of last sector
        if(remainder != 0) memset(data + address + size, 0, remainder);

        return 0;
    }

    int NvmeZnsMemoryBackend::reset(uint64_t zone) {
        measurements::measure_guard msr_guard(msr_reset_identifier);
        std::lock_guard<std::mutex> guard(gl);

        uintptr_t address = zone * info.zone_capacity * info.sector_size;
        if(memory_limit < (uintptr_t) data + address + zone_byte_size)
            return -1;

        output(std::cout, output::DEBUG, "reset: [", zone, "]");

        write_pointers.at(zone) = 0;

        memset(data + address, 0, zone_byte_size);

        return 0;
    };
}