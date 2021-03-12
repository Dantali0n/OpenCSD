#ifndef QEMU_CSD_SPDK_INIT_HPP
#define QEMU_CSD_SPDK_INIT_HPP

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

		void *buffer;
		uint32_t buffer_size;
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
	inline static void spin_complete(struct ns_entry *entry);

	/**
	 * Completion callback to print errors extracted from struct ns_entry
	 * @param void_entry a pointer to an instance of ns_entry
	 */
	static void error_print(void *void_entry,
 		const struct spdk_nvme_cpl *completion);

	int initialize_zns_spdk(struct arguments::options *options,
		struct ns_entry *entry);
}

#endif //QEMU_CSD_SPDK_INIT_HPP
