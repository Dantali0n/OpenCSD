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

//#include <linux/bpf.h>
#include <stdint.h>

#include "bpf_helpers_prog.h"

#define RAND_MAX 2147483647

/** Limitation examples, globals
 * int test; -> Clang won't compile
 * int test = 12; -> uBPF won't run, BPF relocation type 1
 */

// Global vars must be static const,
// uBPF does not support BPF relocation instructions!
static const uint16_t BUFFER_SIZE = 64;

int main() {
    /** Reading from device using stack based buffers */
//	uint64_t sector_size = bpf_get_sector_size();
//	uint8_t buffer[BUFFER_SIZE] = {0};
//
//	uint16_t its_per_lba = sector_size / BUFFER_SIZE;
//	uint32_t ints_per_it = BUFFER_SIZE / sizeof(uint32_t);
//
//	uint64_t count = 0;
//
//	for(uint16_t i = 0; i < its_per_lba; i++) {
//		bpf_read(0, i*BUFFER_SIZE, BUFFER_SIZE, &buffer);
//
//		uint32_t *int_buf = (uint32_t*)buffer;
//		for(uint32_t j = 0; j < ints_per_it; j++) {
//			if(*(int_buf + j) > RAND_MAX / 2) count++;
//		}
//	}
//
//	bpf_return_data(&count, sizeof(uint64_t));



    /** Reading from device using 'heap' based buffers */
    uint64_t sector_size = bpf_get_sector_size();
    uint64_t buffer_size;
    void *buffer;

    bpf_get_mem_info(&buffer, &buffer_size);

    if(buffer_size < sector_size) return -1;

    uint32_t ints_per_it = lba_size / sizeof(uint32_t);
    uint64_t count = 0;

    bpf_read(0, 0, sector_size, buffer);

    uint32_t *int_buf = (uint32_t*)buffer;
    for(uint32_t j = 0; j < ints_per_it; j++) {
        if(*(int_buf + j) > RAND_MAX / 2) count++;
    }

    bpf_return_data(&count, sizeof(uint64_t));


//	__u16 lba_size = 8;
//	__u8 buffer[8] = {0};
//
//	for(__u8 i = 0; i < sizeof(__u64); i++) {
//		buffer[i] = 1;
//	}
//
//	bpf_return_data(&buffer, test);



    return 0;
}