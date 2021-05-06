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
		ubpf_register(vm, 3, "bpf_get_lba_size", (void*)bpf_get_lba_size);
        ubpf_register(vm, 4, "bpf_get_zone_size", (void*)bpf_get_zone_size);
		ubpf_register(vm, 5, "bpf_get_mem_info", (void*)bpf_get_mem_info);
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

		// Jit compilation path
		if(this->options.ubpf_jit) {
		    // Measure jit compilation time
            auto start = std::chrono::high_resolution_clock::now();
            ubpf_jit_fn exec = ubpf_compile(this->vm, &msg_buf);
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            std::cout << "Jit compilation: " << duration.count() << " ms." << std::endl;
            if (exec(this->vm_mem, this->options.ubpf_mem_size) < 0)
                return -1;

            return return_size;
        }

		// Non jit path
		if(ubpf_exec(this->vm, this->vm_mem, this->options.ubpf_mem_size) < 0)
			return -1;

		return return_size;
	}

	void NvmCsd::nvm_cmd_bpf_result(void *data) {
		if(return_data == nullptr) return;

		memcpy(data, return_data, return_size);

		free(return_data);
		return_data = nullptr;
		return_size = 0;
	}

	void NvmCsd::bpf_return_data(void *data, uint64_t size) {
		if(return_data != nullptr) free(return_data);

		return_data = malloc(size);
		memcpy(return_data, data, size);
		return_size = size;
	}

	void NvmCsd::bpf_read(uint64_t lba, uint64_t offset, uint16_t limit, void *data) {
		auto *self = nvm_instance;

		// Safer, still unsafe, limit is only uint16_t
		uint64_t lba_size = bpf_get_lba_size();
		if(limit >= lba_size) limit = lba_size;

		spdk_nvme_ns_cmd_read(self->entry.ns, self->entry.qpair,
			self->entry.buffer, lba, 1, spdk_init::error_print,
			&self->entry,0);
		spdk_init::spin_complete(&self->entry);

		memcpy(data, (uint8_t*)self->entry.buffer + offset, limit);
	}

	uint64_t NvmCsd::bpf_get_lba_size() {
		return nvm_instance->entry.buffer_size;
	}

    uint64_t NvmCsd::bpf_get_zone_size() {
        return nvm_instance->entry.zone_size;
    }

	void NvmCsd::bpf_get_mem_info(void **mem_ptr, uint64_t *mem_size) {
		auto *self = nvm_instance;
		*mem_ptr = self->vm_mem;
		*mem_size = self->options.ubpf_mem_size;
	}
}