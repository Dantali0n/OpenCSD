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
#include <stdbool.h>

#ifndef FLFS_BPF_HELPERS_H
#define FLFS_BPF_HELPERS_H

/**
 * FluffleFS I/O operations as exposed to BPF kernels
 */
enum flfs_operations {
    FLFS_READ_STREAM  = 0,
    FLFS_WRITE_EVENT = 1,
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
    // After this struct is initial_data_lba and the subsequent flattened data
    // blocks (for reads)
    // Or flfs_write_call + initial_data_lba and the subsequent flattened data
    // blocks (for writes)
};

/**
 * Additional datastructure presented to kernels after flfs_call
 */
struct __attribute__((packed)) flfs_write_call {
    /**
     * pre and post pad ensure start and stop of actual write data can be found
     * even when data inside the buffer is padded to be sector aligned. Pre and
     * post pad will only be non zero if there is pre-existing data inside the
     * section of the file to be written.
     *
     * This can prevent situations such as accidentally compressing small
     * ( < 1 sector ) sections of data twice.
     *
     *           | Actual written request data   |
     * | ............ |      |      |      | .............. |
     * ^         ^                               ^          ^
     * | pre_pad |                               | post_pad |
     */
    // TODO(Dantali0n): Support (partially) overwriting kernels, until then
    //                  post pad will always be zero. This requires shifting
    //                  data_block LBAs, an extensive feature.
    uint64_t pre_pad;
    uint64_t post_pad;
    // TODO(Dantali0n): Kernels running on the device should be able to
    //                  intrinsically know written sectors in zones.
    //                  Requires changes to nvme_zns interface.
    /**
     * | RANDOM ZONE | LOG ZONE                |
     *               ^          ^              ^
     *               |          |              |
     *           start_ptr    wp_ptr        end_ptr
     */
    uint64_t start_ptr;
    uint64_t wp_ptr;
    uint64_t end_ptr;
};

/**
 * Status indicator of write kernel
 */
enum write_return_status {
    WRITE_SUCCESS = 0,
    WRITE_FAIL = 1,
    // Must return full even if last append is successful!
    WRITE_DEVICE_FULL_SUCCESS  = 2,
    WRITE_DEVICE_FULL_FAIL  = 3,
};

/**
 * This data struct should be returned by write kernels
 */
struct __attribute__((packed)) flfs_write_return {
    enum write_return_status status;
    // Size of the write request after kernel processing, can be more or less.
    uint64_t size;
    // After this are the flattened LBas of kernel written data
};

/**
 * Find the flattened data blocks FluffleFS provides in filesystem specific
 * kernel data.
 * @param call_info parameter with filesystem specific data as can be retrieved
 *        from bpf_get_call_info defined in bpf_helpers_prog.h
 */
static void find_data_lba(uint64_t **call_info, bool write) {
    *call_info = (uint64_t*)((uint8_t*)*call_info + sizeof(struct flfs_call));

    if(write)
        *call_info = (uint64_t*)((uint8_t*)*call_info +
            sizeof(struct flfs_write_call));
}

/**
 * Increment the cur_data_lba pointer and indicate if next data block exists.
 * @param cur_data_lba the current location of valid data for the given inode
 */
static void next_data_lba(uint64_t **cur_data_lba) {
    *cur_data_lba += 1;
}

/**
 * Convert the lba to the zone and sector required to perform read / write
 * operations.
 */
static void lba_to_position(uint64_t lba, uint64_t zone_size, uint64_t *zone,
                            uint64_t *sector)
{
    *zone = lba / zone_size;
    *sector = lba % zone_size;
}

#endif