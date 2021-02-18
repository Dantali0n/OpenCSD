#include <cerrno>

#include <spdk/env.h>
#include <spdk/nvme.h>
#include <spdk/nvme_zns.h>

struct ns_entry {
	struct spdk_nvme_ctrlr	*ctrlr;
	struct spdk_nvme_ns		*ns;
	struct spdk_nvme_qpair	*qpair;
};

static bool probe_cb(
		void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		struct spdk_nvme_ctrlr_opts *opts)
{

	if(false) printf("Attaching to %s\n", trid->traddr);
	return false;
}

static void attach_cb(
		void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
	int nsid, num_ns;
	struct ctrlr_entry *entry;
	struct spdk_nvme_ns *ns;
	const struct spdk_nvme_ctrlr_data *cdata;
}

int main(int argc, char **argv) {
	struct spdk_env_opts opts;
	int rc;

	/*
	 * SPDK relies on an abstraction around the local environment
	 * named env that handles memory allocation and PCI device operations.
	 * This library must be initialized first.
	 */
	spdk_env_opts_init(&opts);
	opts.name = "find-fill-filter";
	opts.shm_id = 0;

	rc = spdk_env_init(&opts);
	if (rc < 0) {
		fprintf(stderr, "Unable to initialize SPDK env: %s\n",
			strerror(rc));
		return 1;
	}

	printf("Initializing first NVMe Controller containing ZNS namespaces \n");

	/*
	 * Start the SPDK NVMe enumeration process.  probe_cb will be called
	 *  for each NVMe controller found, giving our application a choice on
	 *  whether to attach to each controller.  attach_cb will then be
	 *  called for each controller after the SPDK NVMe driver has completed
	 *  initializing the controller we chose to attach.
	 */
	rc = spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL);
	if (rc != 0) {
		fprintf(stderr, "spdk_nvme_probe() failed: %s\n",
			strerror(rc));
		return 1;
	}

	printf("Initialization complete.\n");

	return 0;
}