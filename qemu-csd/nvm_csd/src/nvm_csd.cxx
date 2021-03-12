#include <nvm_csd.hpp>
#include "nvm_csd.hpp"

namespace qemucsd::nvm_csd {

	NvmCsd::NvmCsd(struct arguments::options *options,
		struct spdk_init::ns_entry *entry)
	{
		this->options = *options;
		this->entry = *entry;
		int rc;

		/** uBPF Initialization */
	}

	NvmCsd::~NvmCsd() {
		if(entry.ctrlr != nullptr) spdk_nvme_detach(entry.ctrlr);
		if(entry.buffer != nullptr) spdk_free(entry.buffer);
	}

	uint64_t NvmCsd::nvm_cmd_bpf_run(void *bpf_elf, uint64_t bpf_elf_size) {
//		int err;
//		err = ubpf_load_elf(this->vm, bpf_elf, bpf_elf_size, &message);
//
//		result =  ubpf_exec(this->vm, memory, mem_size);
	}

	void NvmCsd::nvm_cmd_bpf_result(void *data) {

	}

}