void bpf_return_data(void *data,
    uint64_t size);
void bpf_read(uint64_t lba,
     uint64_t offset, uint64_t limit,
     void *data);
uint64_t bpf_get_lba_size(void);
uint64_t bpf_get_zone_size(void);
void bpf_get_mem_info(void **mem_ptr,
     uint64_t *mem_size);