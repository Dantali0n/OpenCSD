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

#ifndef QEMU_CSD_BPF_HELPERS_VM_H
#define QEMU_CSD_BPF_HELPERS_VM_H

/**
 * These BPF helper declarations are to be used by the VM NOT inside the BPF
 * program itself. Actual definitions are to be implemented by the application
 * including this header.
 */

static void bpf_return_data(void *data, uint64_t size);

static int bpf_read(uint64_t zone, uint64_t sector, uint64_t offset, uint64_t
    limit, void *data);

static int bpf_write(uint64_t zone, uint64_t *sector, uint64_t offset, uint64_t
    limit, void *data);

static uint64_t bpf_get_sector_size(void);

static uint64_t bpf_get_zone_capacity(void);

static uint64_t bpf_get_zone_size(void);

static void bpf_get_mem_info(void **mem_ptr, uint64_t *mem_size);

static void bpf_get_call_info(void **call);

static void bpf_debug(const char *string);

#endif //QEMU_CSD_BPF_HELPERS_VM_H
