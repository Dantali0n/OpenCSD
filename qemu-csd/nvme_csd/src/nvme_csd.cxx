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

#include "nvme_csd.hpp"

static qemucsd::nvme_csd::NvmeCsd *nvme_instance = nullptr;
static void *return_data = nullptr;
static uint64_t return_size = 0;

namespace qemucsd::nvme_csd {

	NvmeCsd::NvmeCsd(size_t vm_mem_size, bool vm_jit,
        nvme_zns::NvmeZnsBackend *nvme)
	{
        this->vm_mem_size = vm_mem_size;
        this->vm_jit = vm_jit;

		this->nvme = nvme;

		nvme_instance = this;
	}

	NvmeCsd::~NvmeCsd() {
		if(vm != nullptr) ubpf_destroy(vm);
		if(vm_mem != nullptr) free(vm_mem);
	}

    void NvmeCsd::initialize() {
        /** uBPF Initialization */
        this->vm = ubpf_create();
        this->vm_mem = malloc(this->vm_mem_size);

        ubpf_register(vm, 1, "bpf_return_data", (void*)bpf_return_data);
        ubpf_register(vm, 2, "bpf_read", (void*)bpf_read);
        ubpf_register(vm, 3, "bpf_write", (void*)bpf_write);
        ubpf_register(vm, 4, "bpf_get_sector_size", (void*)bpf_get_sector_size);
        ubpf_register(vm, 5, "bpf_get_zone_capacity", (void*)bpf_get_zone_capacity);
        ubpf_register(vm, 6, "bpf_get_mem_info", (void*)bpf_get_mem_info);
    }

	uint64_t NvmeCsd::nvm_cmd_bpf_run(void *bpf_elf, uint64_t bpf_elf_size) {
		char *msg_buf = nullptr;
		if(ubpf_load_elf(this->vm, bpf_elf, bpf_elf_size, &msg_buf) < 0) {
			std::cerr << msg_buf << std::endl;
			free(msg_buf);
			return -1;
		}

		free(msg_buf);

		// Jit compilation path
		if(this->vm_jit) {
		    // Measure jit compilation time
            auto start = std::chrono::high_resolution_clock::now();
            ubpf_jit_fn exec = ubpf_compile(this->vm, &msg_buf);
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            std::cout << "Jit compilation: " << duration.count() << "us." << std::endl;
            if ((int)exec(this->vm_mem, this->vm_mem_size) < 0)
                return -1;

            return return_size;
        }

		// Non jit path
		uint64_t result;
		if(ubpf_exec(this->vm, this->vm_mem, this->vm_mem_size, &result) < 0)
			return -1;

		return return_size;
	}

	void NvmeCsd::nvm_cmd_bpf_result(void *data) {
		if(return_data == nullptr) return;

		memcpy(data, return_data, return_size);

		free(return_data);
		return_data = nullptr;
		return_size = 0;
	}

	void NvmeCsd::bpf_return_data(void *data, uint64_t size) {
		if(return_data != nullptr) free(return_data);

		return_data = malloc(size);
		memcpy(return_data, data, size);
		return_size = size;
	}

	int NvmeCsd::bpf_read(uint64_t zone, uint64_t sector, uint64_t offset,
        uint64_t size, void *data)
    {
        return nvme_instance->nvme->read(zone, sector, offset, data, size);
	}

    int NvmeCsd::bpf_write(uint64_t zone, uint64_t *sector, uint64_t offset,
        uint64_t size, void *data)
    {
        return nvme_instance->nvme->append(zone, *sector, offset, data, size);
    }

	uint64_t NvmeCsd::bpf_get_sector_size() {
        nvme_zns::nvme_zns_info info = {0};
		nvme_instance->nvme->get_nvme_zns_info(&info);

        return info.sector_size;
	}

    uint64_t NvmeCsd::bpf_get_zone_capacity() {
        nvme_zns::nvme_zns_info info = {0};
        nvme_instance->nvme->get_nvme_zns_info(&info);

        return info.zone_capacity;
    }

	void NvmeCsd::bpf_get_mem_info(void **mem_ptr, uint64_t *mem_size) {
		auto *self = nvme_instance;
		*mem_ptr = self->vm_mem;
		*mem_size = self->vm_mem_size;
	}
}