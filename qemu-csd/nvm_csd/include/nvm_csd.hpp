#ifndef QEMU_CSD_NVM_CSD_HPP
#define QEMU_CSD_NVM_CSD_HPP

#include "arguments.hpp"

#include <spdk/env.h>
#include <spdk/nvme.h>
#include <spdk/nvme_zns.h>

namespace qemucsd {
	namespace nvm_csd {

		// Datastructure to retain basic SPDK NVMe state
		struct ns_entry {
			struct spdk_nvme_ctrlr	*ctrlr;
			struct spdk_nvme_ns		*ns;
			struct spdk_nvme_qpair	*qpair;

			void					*buffer;
			uint32_t 				buffer_size;
		};

		// SPDK probe callback to always attach
		static bool probe_cb_attach_all(
			void *cb_ctx, const struct spdk_nvme_transport_id *trid,
			struct spdk_nvme_ctrlr_opts *opts);

		// SPDK callback to exit upon encountering any error
		static void error_complete(void *arg,
			 const struct spdk_nvme_cpl *completion);

		class NvmCsd {
		public:
			NvmCsd(struct arguments::options *options);
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
			struct ns_entry entry;

			/**
			 * Attach to the first controller having a ZNS namespace and assign
			 * entry accessible through instance of NvmCsd passed by cb_ctx.
			 * @param cb_ctx An instance of NvmCsd
			 */
			static void attach_cb(
				void *cb_ctx, const struct spdk_nvme_transport_id *trid,
				struct spdk_nvme_ctrlr *ctrlr,
				const struct spdk_nvme_ctrlr_opts *opts);

			/**
			 * Busy spin to check outstanding IO operations on qpair.
			 * @param entry SPDK state information to spin on
			 */
			inline static void spin_complete(struct ns_entry *entry);
		};

	}
}

#endif //QEMU_CSD_NVM_CSD_HPP
