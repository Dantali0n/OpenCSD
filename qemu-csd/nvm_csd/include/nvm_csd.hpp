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

#ifndef QEMU_CSD_NVM_CSD_HPP
#define QEMU_CSD_NVM_CSD_HPP

#include "arguments.hpp"
#include "spdk_init.hpp"

extern "C" {
	#include <ubpf.h>
}

// SPDK headers already have if __cplusplus extern "C" wrapper
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
		struct ubpf_vm *vm = nullptr;
		void *vm_mem = nullptr;

		static void bpf_return_data(void *data, uint64_t size);

		static void bpf_read(uint64_t lba, uint64_t offset, uint16_t limit, void *data);

		static uint64_t bpf_get_lba_size(void);

        static uint64_t bpf_get_zone_size(void);

		static void bpf_get_mem_info(void **mem_ptr, uint64_t *mem_size);
	};
}

#endif //QEMU_CSD_NVM_CSD_HPP
