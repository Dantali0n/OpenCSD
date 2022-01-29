/**
 * MIT License
 *
 * Copyright (c) 2022 Dantali0n
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

//#include <linux/bpf.h>
#include <stdint.h>

#include <bpf_helpers_prog.h>
#include <bpf_helpers_flfs.h>

int main() {
    // Keep datastructures on the heap due to very limited stack space
    struct flfs_call *call = 0;
    bpf_get_call_info((void**)&call);

    uint64_t *cur_data_lba = (uint64_t*)call;
    find_data_lba((uint64_t**)&cur_data_lba);

    uint64_t zone_capacity = bpf_get_zone_capacity();
    uint64_t zone_size = bpf_get_zone_size();
    uint64_t sector_size = bpf_get_sector_size();

    if(call == 0) return -1;
    if(*cur_data_lba == 0) return -2;

    // Ensure the read kernel is being used for a read operation
    if(call->op == FLFS_WRITE) return -3;

    // Advance offset blocks until the right data lba is found
    // TODO(Dantali0n): Improve this by only sending relevant data lbas to
    //                  kernel
    if(call->dims.offset > 0) {
        uint64_t offset_lbas = call->dims.offset / sector_size;
        for (uint64_t i = 0; i < offset_lbas; i++) {
            next_data_lba(&cur_data_lba);
        }

        // Validate advanced data_lba has not become 0
        if(*cur_data_lba == 0) return -4;
    }

    uint64_t buffer_size;
    void *buffer;
    bpf_get_mem_info(&buffer, &buffer_size);

    uint64_t data_limit = call->dims.size < call->ino.size ?
        call->dims.size : call->ino.size;
    uint64_t buffer_offset = 0;
    uint64_t zone, sector;
    while(buffer_offset < data_limit) {
        lba_to_position(*cur_data_lba, zone_size, &zone, &sector);
        bpf_read(zone, sector, 0, sector_size, buffer + buffer_offset);
        buffer_offset = buffer_offset + sector_size;
        next_data_lba(&cur_data_lba);
    }

    bpf_return_data(buffer, data_limit);

    return 0;
}