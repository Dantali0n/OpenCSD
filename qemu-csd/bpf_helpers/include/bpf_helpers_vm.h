#ifndef QEMU_CSD_BPF_HELPERS_VM_H
#define QEMU_CSD_BPF_HELPERS_VM_H

/**
 * These BPF helper declarations are to be used by the VM NOT inside the BPF
 * program itself. Actual definitions are to be implemented by the application
 * including this header.
 */

static void bpf_return_data(void *data, uint64_t size);

static void bpf_read(uint64_t lba, void *data);

static uint64_t bpf_get_lba_siza(void);

#endif //QEMU_CSD_BPF_HELPERS_VM_H
