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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** Static data for llvm to be able to perform analysis */
static const uint32_t sector_size = 4096;
static const uint32_t zone_size = 128;
static const uint32_t zone_byte_size = zone_size * sector_size;
void *magic_file_data = NULL;

int main(int argc, char *argv[]) {
    /** Static data for llvm to be able to perform analysis */
    uint32_t file = sector_size * 1024 * 512;
    uint64_t cur_data_lba = 1302;
    magic_file_data = malloc(file);

    void *buffer = malloc(file);

    uint64_t data_limit = file;

    uint64_t buffer_offset = 0;
    uint64_t zone, sector = 0;
    uint64_t bytes_per_it = sector_size / sizeof(uint8_t);
    uint8_t *byte_buf = (uint8_t*)buffer;

    uint32_t *bins = (uint32_t*)(byte_buf + sector_size);
    for(uint16_t i = 0; i < 256; i++) {
        bins[i] = 0;
    }

    while(buffer_offset < data_limit) {
        __asm volatile("# LLVM-MCA-BEGIN stopwatchedAccumulate");
        zone = cur_data_lba / zone_size;
        sector = cur_data_lba % zone_size;

        uintptr_t address;
        address = (zone * zone_byte_size) + (sector * sector_size) + 0;
        memcpy(buffer, magic_file_data + address, sector_size);

        for(uint64_t j = 0; j < bytes_per_it; j++) {
            bins[*(byte_buf + j)] += 1;
        }

        buffer_offset = buffer_offset + sector_size;
        cur_data_lba += 1;
        __asm volatile("# LLVM-MCA-END");
    }
}