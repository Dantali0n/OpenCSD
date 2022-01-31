/**
 * Emulated NVMe command to pass a BPF program to a
 * Computational Storage Device. This command is
 * blocking as it needs to determine the amount of
 * bytes of the result.
 *
 * @return below 0 for errors, otherwise number of
 * bytes of result data.
 */
uint64_t nvm_cmd_bpf_run(void *bpf_elf,
	uint64_t bpf_elf_size);

/**
 * Emulated NVMe command to retrieve BPF return
 * data.
 * @param data The buffer the data will be placed
 * into.
 */
void nvm_cmd_bpf_result(void *data);