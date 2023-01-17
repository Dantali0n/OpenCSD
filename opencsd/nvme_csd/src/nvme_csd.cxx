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
static uint64_t fs_call_size = 0;
static void *return_data = nullptr;
static int64_t return_size = 0;

namespace qemucsd::nvme_csd {

    size_t NvmeCsd::msr[10] = {0};

	NvmeCsd::NvmeCsd(size_t vm_mem_size, bool vm_jit,
        nvme_zns::NvmeZnsBackend *nvme)
	{
        this->vm_mem_size = vm_mem_size;
        this->vm_jit = vm_jit;

		this->nvme = nvme;

		nvme_instance = this;

        register_msr_nvmecsd_namespaces();
	}

	NvmeCsd::~NvmeCsd() {
		if(vm != nullptr) ubpf_destroy(vm);
		if(vm_mem != nullptr) free(vm_mem);
	}

    void NvmeCsd::register_msr_nvmecsd_namespaces() {
        measurements::register_namespace(
            "NVME_CSD][vm_init", msr[MSRI_VM_INIT]);
        measurements::register_namespace(
            "NVME_CSD][vm_destroy", msr[MSRI_VM_DESTROY]);
        measurements::register_namespace(
            "NVME_CSD][bpf_run", msr[MSRI_BPF_RUN]);
        measurements::register_namespace(
            "NVME_CSD][bpf_result", msr[MSRI_BPF_RESULT]);
        measurements::register_namespace(
            "NVME_CSD][bpf_return_data", msr[MSRI_BPF_RETURN_DATA]);
        measurements::register_namespace(
            "NVME_CSD][bpf_sector_size", msr[MSRI_BPF_SECTOR_SIZE]);
        measurements::register_namespace(
            "NVME_CSD][bpf_zone_capacity", msr[MSRI_BPF_ZONE_CAPACITY]);
        measurements::register_namespace(
            "NVME_CSD][bpf_zone_size", msr[MSRI_BPF_ZONE_SIZE]);
        measurements::register_namespace(
            "NVME_CSD][bpf_mem_info", msr[MSRI_BPF_MEM_INFO]);
        measurements::register_namespace(
            "NVME_CSD][bpf_call_info", msr[MSRI_BPF_CALL_INFO]);
    }

    void NvmeCsd::vm_init() {
        measurements::measure_guard msr_guard(msr[MSRI_VM_INIT]);

        /** uBPF Initialization */
        this->vm = ubpf_create();
        this->vm_mem = malloc(this->vm_mem_size);

        ubpf_register(vm, 1, "bpf_return_data", (void*)bpf_return_data);
        ubpf_register(vm, 2, "bpf_read", (void*)bpf_read);
        ubpf_register(vm, 3, "bpf_write", (void*)bpf_write);
        ubpf_register(vm, 4, "bpf_get_sector_size", (void*)bpf_get_sector_size);
        ubpf_register(vm, 5, "bpf_get_zone_capacity", (void*)bpf_get_zone_capacity);
        ubpf_register(vm, 6, "bpf_get_zone_size", (void*)bpf_get_zone_size);
        ubpf_register(vm, 7, "bpf_get_mem_info", (void*)bpf_get_mem_info);
        ubpf_register(vm, 8, "bpf_get_call_info", (void*)bpf_get_call_info);
    }

    void NvmeCsd::vm_destroy() {
        measurements::measure_guard msr_guard(msr[MSRI_VM_DESTROY]);

        ubpf_destroy(this->vm);
        free(vm_mem);

        this->vm = nullptr;
        this->vm_mem = nullptr;
    }

    int64_t NvmeCsd::_nvm_cmd_bpf_run(void *bpf_elf, uint64_t bpf_elf_size) {
        measurements::measure_guard msr_guard(msr[MSRI_BPF_RUN]);

        char *msg_buf = nullptr;
        if(ubpf_load_elf(this->vm, bpf_elf, bpf_elf_size, &msg_buf) < 0) {
            output.error(msg_buf);
            free(msg_buf);
            return -1;
        }

        // Jit compilation path
        if(this->vm_jit) {
            // Measure jit compilation time
            auto start = std::chrono::high_resolution_clock::now();
            ubpf_jit_fn exec = ubpf_compile(this->vm, &msg_buf);
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            output.debug("Jit compilation: ", duration.count(), "us.");
            if ((int)exec(this->vm_mem, this->vm_mem_size) < 0)
                return -1;

            return return_size;
        }

        // Non jit path
        // TODO(Dantali0n): uint64_t is never going to be negative
        uint64_t result;
        auto start = std::chrono::high_resolution_clock::now();
        if(ubpf_exec(this->vm, this->vm_mem, this->vm_mem_size, &result) < 0)
            return -1;
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        output.debug("Kernel wall time: ", duration.count(), "us.");

        if((int)result < 0)
            return int(result);

        return return_size;
    }

    int64_t NvmeCsd::nvm_cmd_bpf_run(void *bpf_elf, uint64_t bpf_elf_size) {
        vm_init();

        int64_t result = _nvm_cmd_bpf_run(bpf_elf, bpf_elf_size);

        vm_destroy();

        return result;
	}

    int64_t NvmeCsd::nvm_cmd_bpf_run_fs(void *bpf_elf, uint64_t bpf_elf_size,
        void *call, uint64_t call_size)
    {
        vm_init();

        // Copy call data into memory context accessible by uBPF vm.
        fs_call_size = call_size;
        if(call_size > vm_mem_size)
            return -ENOMEM;
        memcpy(vm_mem, call, call_size);

        int64_t result = _nvm_cmd_bpf_run(bpf_elf, bpf_elf_size);

        fs_call_size = 0;

        vm_destroy();

        return result;
    }

	void NvmeCsd::nvm_cmd_bpf_result(void *data) {
        measurements::measure_guard msr_guard(msr[MSRI_BPF_RESULT]);

		if(return_data == nullptr) return;

		memcpy(data, return_data, return_size);

		free(return_data);
		return_data = nullptr;
		return_size = 0;
	}

    /**
     * Stats reports the lbas of read and written sectors as executed by the
     * BPF kernel. The VM is in complete control over these operations so the
     * kernel has no potential mechanism to lie about these stats.
     * TODO(Dantali0n): Implement this call
     */
    void NvmeCsd::nvm_cmd_bpf_stats(struct bpf_stats *stats) {

    }

	void NvmeCsd::bpf_return_data(void *data, uint64_t size) {
        measurements::measure_guard msr_guard(msr[MSRI_BPF_RETURN_DATA]);

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
        measurements::measure_guard msr_guard(msr[MSRI_BPF_SECTOR_SIZE]);

        nvme_zns::nvme_zns_info info = {0};
		nvme_instance->nvme->get_nvme_zns_info(&info);

        return info.sector_size;
	}

    uint64_t NvmeCsd::bpf_get_zone_capacity() {
        measurements::measure_guard msr_guard(msr[MSRI_BPF_ZONE_CAPACITY]);

        nvme_zns::nvme_zns_info info = {0};
        nvme_instance->nvme->get_nvme_zns_info(&info);

        return info.zone_capacity;
    }

    uint64_t NvmeCsd::bpf_get_zone_size() {
        measurements::measure_guard msr_guard(msr[MSRI_BPF_ZONE_SIZE]);

        nvme_zns::nvme_zns_info info = {0};
        nvme_instance->nvme->get_nvme_zns_info(&info);

        return info.zone_size;
    }

    /**
     * Appoint the heap area to the running BPF kernel taking into account the
     * potential filesystem context which is stored on top of the actual heap.
     */
	void NvmeCsd::bpf_get_mem_info(void **mem_ptr, uint64_t *mem_size) {
        measurements::measure_guard msr_guard(msr[MSRI_BPF_MEM_INFO]);

		auto *self = nvme_instance;
		*mem_ptr = (uint8_t*)self->vm_mem + fs_call_size;
		*mem_size = self->vm_mem_size - fs_call_size;
	}

    /**
     * Present the filesystem context to the running BPF kernel if any
     */
    void NvmeCsd::bpf_get_call_info(void **call) {
        measurements::measure_guard msr_guard(msr[MSRI_BPF_CALL_INFO]);

        auto *self = nvme_instance;
        if(fs_call_size)
            *call = self->vm_mem;
    }

    void NvmeCsd::bpf_debug(const char *string) {
        output.info(string);
    }
}