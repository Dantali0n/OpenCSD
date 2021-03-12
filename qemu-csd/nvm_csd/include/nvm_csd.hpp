#ifndef QEMU_CSD_NVM_CSD_HPP
#define QEMU_CSD_NVM_CSD_HPP

#include "arguments.hpp"
#include "spdk_init.hpp"

#include <spdk/env.h>
#include <spdk/nvme.h>
#include <spdk/nvme_zns.h>

namespace qemucsd::nvm_csd {

	class NvmCsd {
	public:
		NvmCsd(struct arguments::options *options,
			struct spdk_init::ns_entry *entry);
		~NvmCsd();

		/**
		 * Emulated NVMe command to pass a BPF program to a Computational
		 * Storage Device. This command is blocking as it needs to determine
		 * the amount of bytes if the result.
		 *
		 * @return below 0 for errors, otherwise number of bytes of result
		 * data.
		 */
		uint64_t nvm_cmd_bpf_run(void *bpf_elf, uint64_t bpf_elf_size);

		/**
		 * Emulated NVMe command to retrieve BPF return data.
		 * @param data The buffer the data will be placed into.
		 */
		void nvm_cmd_bpf_result(void *data);
	protected:
		struct arguments::options options;
		struct spdk_init::ns_entry entry;
		struct ubpf_vm *vm;
	};

}

#endif //QEMU_CSD_NVM_CSD_HPP
