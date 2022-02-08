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

#ifndef QEMU_CSD_NVME_CSD_HPP
#define QEMU_CSD_NVME_CSD_HPP

#include "output.hpp"
#include "measurements.hpp"
#include "nvme_zns_backend.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

extern "C" {
	#include <ubpf.h>
}


namespace qemucsd::nvme_csd {

    static output::Output output = output::Output(
        "[NVME CSD] ",
        #ifdef QEMUCSD_DEBUG
            output::DEBUG
        #else
            output::INFO
        #endif
    );

    /**
     * Statistics about the kernel that has run. Used to verify the behavior
     * of BPF kernels or reject changes if they misbehaved.
     */
    struct bpf_stats {
        std::vector<uint64_t> read_lbas;
        std::vector<uint64_t> written_lbas;
    };

    /**
     * Emulated additions to the NVMe command set to enable BPF CSD
     * functionality. These commands take extensive liberty in what would be
     * really possible using such a bus interface, such as not taking into
     * consideration any command chunking (due to fix command size) that must
     * occur. In addition, no completions queues and completion commands are
     * used. Finally, the commands pass arbitrary pointers as argument such that
     * the callee can fill the datastructure with information for the caller,
     * this is something that is only possible with shared memory which is not
     * feasible with actual NVMe commands.
     *
     * The limitations of such interfaces are well understood as well as the
     * methodologies to circumvent them. However, they greatly increase the
     * required programming effort and thus are left out to safe on development
     * time.
     */
	class NvmeCsd {
    protected:
        // Measurement instrumentation
        static size_t msr[10];
        enum measure_index {
            MSRI_VM_INIT = 0, MSRI_VM_DESTROY = 1, MSRI_BPF_RUN = 2,
            MSRI_BPF_RESULT = 3, MSRI_BPF_RETURN_DATA = 4,
            MSRI_BPF_SECTOR_SIZE = 5, MSRI_BPF_ZONE_CAPACITY = 6,
            MSRI_BPF_ZONE_SIZE = 7, MSRI_BPF_MEM_INFO = 8,
            MSRI_BPF_CALL_INFO = 9,
        };
        static void register_namespaces();
	public:
		NvmeCsd(size_t vm_mem_size, bool vm_jit,
            nvme_zns::NvmeZnsBackend *nvme);

        // Destructor must always be virtual otherwise won't be called in
        // superclasses!
		virtual ~NvmeCsd();

		/**
		 * Emulated NVMe command to pass a BPF program to a Computational
		 * Storage Device. This command is blocking as it needs to determine
		 * the amount of bytes in the result.
		 *
		 * @return below 0 for errors, otherwise number of bytes of result
		 * data.
		 */
        int64_t nvm_cmd_bpf_run(void *bpf_elf, uint64_t bpf_elf_size);

        /**
         * Emulated NVMe command to pass a BPF program tied to a running
         * filesystem to a Computational Storage Device. This command is
         * blocking as it needs to determine the amount of bytes in the result.
         * @param call data with filesystem specific information to perform the
         *             requested I/O operation.
         * @param call_size size of the filesystem specific data.
         * @return below 0 for errors, otherwise number of bytes of result
		 * data.
         */
        int64_t nvm_cmd_bpf_run_fs(
            void *bpf_elf, uint64_t bpf_elf_size, void *call,
            uint64_t call_size);

		/**
		 * Emulated NVMe command to retrieve BPF return data.
		 * @param data The buffer the data will be placed into.
		 */
		void nvm_cmd_bpf_result(void *data);

        /**
		 * Emulated NVMe command to retrieve statistics of read / written device
         * areas.
		 * @param stats buffer to fill with read and write stats
		 */
        void nvm_cmd_bpf_stats(struct bpf_stats *stats);
	protected:
        nvme_zns::NvmeZnsBackend *nvme;
		struct ubpf_vm *vm = nullptr;

        bool vm_jit = false;
        size_t vm_mem_size = 0;
		void *vm_mem = nullptr;

        void vm_init();
        void vm_destroy();

        int64_t _nvm_cmd_bpf_run(void *bpf_elf, uint64_t bpf_elf_size);

		static void bpf_return_data(void *data, uint64_t size);

		static int bpf_read(uint64_t zone, uint64_t sector, uint64_t offset,
            uint64_t size, void *data);

        static int bpf_write(uint64_t zone, uint64_t *sector, uint64_t offset,
            uint64_t size, void *data);

		static uint64_t bpf_get_sector_size(void);

        static uint64_t bpf_get_zone_capacity(void);

        static uint64_t bpf_get_zone_size(void);

		static void bpf_get_mem_info(void **mem_ptr, uint64_t *mem_size);

        static void bpf_get_call_info(void **call);

        static void bpf_debug(const char *string);
	};
}

#endif //QEMU_CSD_NVME_CSD_HPP
