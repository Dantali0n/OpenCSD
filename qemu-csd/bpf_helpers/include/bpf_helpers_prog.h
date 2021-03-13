#ifndef QEMU_CSD_BPF_HELPERS_PROG_H
#define QEMU_CSD_BPF_HELPERS_PROG_H

/**
 * These BPF helper definitions are to be used by the BPF program NOT by the VM
 * executing the BPF program.
 */

// Called by BPF to allow copying return data out of running BPF program.
static void (*bpf_return_data)(void *data, uint64_t size) = (void *) 1;

// Called by BPF to perform an 'on device' read at a specific LBA.
static void (*bpf_read)(uint64_t lba, uint64_t offset, uint16_t limit, void *data) = (void *) 2;

// Called by BPF to get the size in bytes per LBA.
static uint64_t (*bpf_get_lba_siza)(void) = (void *) 3;

// Called by BPF to determine the region of memory allowed to use.
static void (*bpf_get_mem_info)(void **mem_ptr, uint64_t *mem_size) = (void *) 4;

#endif //QEMU_CSD_BPF_HELPERS_PROG_H
