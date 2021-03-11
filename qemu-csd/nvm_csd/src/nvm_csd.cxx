#include "nvm_csd.hpp"

namespace qemucsd::nvm_csd {

	static bool probe_cb_attach_all(
		void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		struct spdk_nvme_ctrlr_opts *opts)
	{
		return true;
	}

	static void error_complete(
		void *arg, const struct spdk_nvme_cpl *completion)
	{
		struct ns_entry *entry = (ns_entry*) arg;

		if (spdk_nvme_cpl_is_error(completion)) {
			spdk_nvme_qpair_print_completion(
					entry->qpair, (struct spdk_nvme_cpl *) completion);
			fprintf(stderr, "I/O error status: %s\n",
					spdk_nvme_cpl_get_status_string(&completion->status));
			fprintf(stderr, "I/O failed, aborting run\n");
			spdk_nvme_detach(entry->ctrlr);
			exit(1);
		}
	}

	NvmCsd::NvmCsd(struct arguments::options *options) {
		this->options = *options;
		int rc;

		/** SPDK Initialization */

		rc = spdk_env_init(&options->spdk);
		if (rc < 0) {
			std::cerr << "Unable to initialize SPDK env: " << strerror(rc) <<
				std::endl;
			return;
		}

		rc = spdk_nvme_probe(NULL, this, probe_cb_attach_all, attach_cb, NULL);
		if (rc < 0) {
			std::cerr << "spdk_nvme_probe() failed: " << strerror(rc) <<
		  		std::endl;
			return;
		}

		const struct spdk_nvme_ns_data *ref_ns_data =
			spdk_nvme_ns_get_data(entry.ns);
		uint32_t lba_size = ref_ns_data->nsze;

		entry.buffer = spdk_zmalloc(lba_size, lba_size, NULL,
	  		SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
		entry.buffer_size = lba_size;

		// Reset all zones in the namespace.
		if(options->dev_init_mode == arguments::DEV_INIT_RESET)
			spdk_nvme_zns_reset_zone(entry.ns, entry.qpair, 0, true,
				error_complete, &entry);

		// Wait for I/O operation to complete
		spin_complete(&entry);

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

	void NvmCsd::attach_cb(
		void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
	{
		NvmCsd *self = (NvmCsd*)cb_ctx;
		int nsid, num_ns;
		struct spdk_nvme_ns *ns;

		// Already found a controller with ZNS namespace, detach and return
		if(self->entry.ctrlr != nullptr) {
			spdk_nvme_detach(ctrlr);
			return;
		}

		// Determine total amount of namespaces on this controller
		num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
		// Loop through all active namespaces trying to find a ZNS namespace
		for (nsid = 1; nsid <= num_ns; nsid++) {
			ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);

			// Check that namespace exists
			if(ns == NULL) continue;
			// Check that namespace is active
			if(!spdk_nvme_ns_is_active(ns)) continue;
			// Check that namespace supports ZNS command set
			if(spdk_nvme_ns_get_csi(ns) != SPDK_NVME_CSI_ZNS) continue;

			// Namespace is activate and supports ZNS
//			printf("Found ZNS supporting namespace: %u on device: %s\n",
//				   spdk_nvme_ns_get_id(ns), trid->traddr);

			// Assign variables to global state
			self->entry.ctrlr = ctrlr;
			self->entry.ns = ns;

			// Create qpair for I/O operations
			self->entry.qpair = spdk_nvme_ctrlr_alloc_io_qpair(
				self->entry.ctrlr, NULL, 0);

			// Only want first ZNS supporting namespace
			break;
		}

		// Did not find ZNS supporting namespace on this controller, detaching
		if(self->entry.ctrlr == nullptr) spdk_nvme_detach(ctrlr);
	}

	inline void NvmCsd::spin_complete(struct ns_entry *entry) {
		while(spdk_nvme_qpair_process_completions(entry->qpair, 0) == 0) {
			;
		}
	}

}