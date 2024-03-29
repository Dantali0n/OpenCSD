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

#ifndef QEMU_CSD_BPF_HELPERS_PROG_H
#define QEMU_CSD_BPF_HELPERS_PROG_H

/**
 * These BPF helper definitions are to be used by the BPF program NOT by the VM
 * executing the BPF program.
 */

// Called by BPF to allow copying return data out of running BPF program.
static void (*bpf_return_data)(void *data, uint64_t size) = (void *) 1;

// Called by BPF to perform an 'on device' read at a specific position.
static int (*bpf_read)(uint64_t zone, uint64_t sector, uint64_t offset,
    uint64_t size, void *data) = (void *) 2;

// Called by BPF to perform an 'on device' write at a specific position.
static int (*bpf_write)(uint64_t zone, uint64_t *sector, uint64_t offset,
    uint64_t size, void *data) = (void *) 3;

// Called by BPF to get the size in bytes per sector.
static uint64_t (*bpf_get_sector_size)(void) = (void *) 4;

// Called by BPF to get the number of useable sectors per zone.
static uint64_t (*bpf_get_zone_capacity)(void) = (void *) 5;

// Called by BPF to get the total number of sectors per zone.
static uint64_t (*bpf_get_zone_size)(void) = (void *) 6;

// Called by BPF to determine the region of memory allowed to use.
static void (*bpf_get_mem_info)(void **mem_ptr, uint64_t *mem_size) = (void *) 7;

// Called by BPF to determine the filesystem operation to perform if any
// This call is filesystem agnostic and the contents of the pointer depend on
// the filesystem the kernel is being run on. If no fs is being used to drive
// the kernel the pointer will be null.
static void (*bpf_get_call_info)(void **call) = (void *) 8;

static void (*bpf_debug)(const char *string) = (void *) 9;

#endif //QEMU_CSD_BPF_HELPERS_PROG_H
