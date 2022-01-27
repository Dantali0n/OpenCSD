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

#ifndef FLFS_BPF_HELPERS_H
#define FLFS_BPF_HELPERS_H

/**
 * FluffleFS I/O operations as exposed to BPF kernels
 */
enum flfs_operations {
    FLFS_READ  = 0,
    FLFS_WRITE = 1,
};

/**
 * FluffleFS I/O request parameters as exposed to BPF kernels
 */
struct dimensions {
    uint64_t size;
    uint64_t offset;
};

/**
 * FluffleFS file types as exposed to BPF kernels
 */
enum flfs_types {
    FLFS_FILE = 0,
    FLFS_DIR  = 1,
};

/**
 * FluffleFS inode information as exposed to the BPF kernels
 */
struct inode {
    uint64_t inode;
    uint64_t parent;
    enum flfs_types type;
    uint64_t size;
};

/**
 * Information to determine the FluffleFS I/O operation, relevant inode and
 * location of data blocks for that inode as exposed to BPF kernels. This is
 * the datastructure passed to bpf_get_call_info
 */
struct __attribute__((packed)) flfs_call {
    enum flfs_operations op;
    struct dimensions dims;
    struct inode ino;
    uint64_t *initial_data_lba;
};

/**
 * Increment the cur_data_lba pointer and indicate if next data block exists
 * @param cur_data_lba the current location of valid data for the given inode
 * @return 1 if has a next data block, 0 if no next data block
 */
static int next_data_lba(uint64_t *&cur_data_lba) {
    if(*(cur_data_lba + 1) != 0) {
        cur_data_lba += 1;
        return 1;
    }
    return 0;
}

#endif