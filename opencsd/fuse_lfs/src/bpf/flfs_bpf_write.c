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
    // Set find_data_lba write=true
    find_data_lba((uint64_t**)&cur_data_lba, true);

    uint64_t zone_capacity = bpf_get_zone_capacity();
    uint64_t zone_size = bpf_get_zone_size();
    uint64_t sector_size = bpf_get_sector_size();

    if(call == 0) return -1;
    if(*cur_data_lba == 0) return -2;

    // Ensure the write kernel is being used for a read operation
    if(call->op != FLFS_WRITE_EVENT) return -3;

    uint64_t buffer_size;
    void *buffer;
    bpf_get_mem_info(&buffer, &buffer_size);

    bpf_return_data(buffer, data_limit);

    return 0;
}