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

#ifndef QEMU_CSD_SPDK_INIT_HPP
#define QEMU_CSD_SPDK_INIT_HPP

#include <chrono>

#include "arguments.hpp"

#include <spdk/env.h>
#include <spdk/nvme.h>
#include <spdk/nvme_zns.h>

namespace qemucsd::spdk_init {

	// Datastructure to retain basic SPDK NVMe state
	struct ns_entry {
		struct spdk_nvme_ctrlr *ctrlr;
		struct spdk_nvme_ns *ns;
		struct spdk_nvme_qpair *qpair;

		// Buffer size is equivalent to 1 LBA
		void *buffer;
		uint32_t buffer_size;

        // LBA size in bytes
        uint64_t lba_size;
		// Zone size in number of LBAs
		uint64_t zone_size;
	};

	/**
	 * Probe callback to always attach to every controller
	 * @return always returns true
	 */
	static bool probe_cb_attach_all(
		void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		struct spdk_nvme_ctrlr_opts *opts);

	/**
	 * Attach to the first controller having a ZNS namespace and assign
	 * ns_entry accessible by cb_ctx.
	 * @param cb_ctx An instance of ns_entry
	 */
	static void attach_cb_ns_entry(
		void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		struct spdk_nvme_ctrlr *ctrlr,
		const struct spdk_nvme_ctrlr_opts *opts);

	/**
	 * Busy spin to check outstanding IO operations on qpair.
	 * @param entry SPDK state information to spin on
	 */
	static inline void spin_complete(struct ns_entry *entry) {
        while(spdk_nvme_qpair_process_completions(entry->qpair, 0) == 0) {
            ;
        }
	}

	/**
	 * Completion callback to print errors extracted from struct ns_entry
	 * @param void_entry a pointer to an instance of ns_entry
	 */
	void error_print(void *void_entry,
 		const struct spdk_nvme_cpl *completion);

    /**
     * Initializes SPDK environment and fills out entry with controller and
     * namespace information. Resets the device depending on options
     * @param entry
     * @return 0 upon success, < 0 upon error
     */
	int initialize_zns_spdk(struct arguments::options *options,
		struct ns_entry *entry);

    /**
     * Small function to reset all zones of a device
     * @param entry data wrapper required to perform operations.
     * @return 0 upon success, < 0 upon failure.
     */
	int reset_zones(struct ns_entry *entry);


    int fill_first_zone(struct qemucsd::spdk_init::ns_entry *entry,
        struct qemucsd::arguments::options *opts);
}

#endif //QEMU_CSD_SPDK_INIT_HPP
