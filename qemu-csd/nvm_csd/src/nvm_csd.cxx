#include "nvm_csd.hpp"

static qemucsd::nvm_csd::NvmCsd *nvm_instance = nullptr;
static void *return_data = nullptr;
static uint64_t return_size = 0;

namespace qemucsd::nvm_csd {

	NvmCsd::NvmCsd(struct arguments::options *options,
		struct spdk_init::ns_entry *entry)
	{
		this->options = *options;
		this->entry = *entry;

		/** uBPF Initialization */
		this->vm = ubpf_create();
		this->vm_mem = malloc(this->options.ubpf_mem_size);

		nvm_instance = this;

		ubpf_register(vm, 1, "bpf_return_data", (void*)bpf_return_data);
		ubpf_register(vm, 2, "bpf_read", (void*)bpf_read);
		ubpf_register(vm, 3, "bpf_get_lba_siza", (void*)bpf_get_lba_siza);
	}

	NvmCsd::~NvmCsd() {
		if(entry.ctrlr != nullptr) spdk_nvme_detach(entry.ctrlr);
		if(entry.buffer != nullptr) spdk_free(entry.buffer);
		if(vm != nullptr) ubpf_destroy(vm);
		if(vm_mem != nullptr) free(vm_mem);
	}

	uint64_t NvmCsd::nvm_cmd_bpf_run(void *bpf_elf, uint64_t bpf_elf_size) {
		char *msg_buf = (char*) malloc(256);
		if(ubpf_load_elf(this->vm, bpf_elf, bpf_elf_size, &msg_buf) < 0) {
			std::cerr << msg_buf << std::endl;
			free(msg_buf);
			return -1;
		}

		free(msg_buf);
		if(ubpf_exec(this->vm, this->vm_mem, this->options.ubpf_mem_size) < 0)
			return -1;

		return return_size;
	}

	void NvmCsd::nvm_cmd_bpf_result(void *data) {
		if(return_data == nullptr) return;

		memcpy(return_data, data, return_size);

		free(return_data);
		return_data = nullptr;
		return_size = 0;
	}

	void NvmCsd::bpf_return_data(void *data, size_t size) {
		if(return_data != nullptr) free(return_data);

		return_data = malloc(size);
		memcpy(return_data, data, size);
		return_size = size;
	}

	void NvmCsd::bpf_read(uint64_t lba, void *data) {
		auto *self = nvm_instance;
		spdk_nvme_ns_cmd_read(self->entry.ns, self->entry.qpair, data, lba,
							  1, spdk_init::error_print, &self->entry, 0);

		spdk_init::spin_complete(&self->entry);
	}

	size_t NvmCsd::bpf_get_lba_siza() {
		return nvm_instance->entry.buffer_size;
	}
}