#include <time.h>

#include <cerrno>

#include <spdk/env.h>
#include <spdk/nvme.h>
#include <spdk/nvme_zns.h>

struct ns_entry {
	struct spdk_nvme_ctrlr	*ctrlr;
	struct spdk_nvme_ns		*ns;
	struct spdk_nvme_qpair	*qpair;
};

// Indicate to have found the first zns supporting namespace
bool have_zns_ns = false;
struct ns_entry zns_entry = {nullptr, nullptr, nullptr};

/**
 * Return true for every controller as we can only determine if controller has
 * zns namespaces when already attached.
 */
static bool probe_cb(
	void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	struct spdk_nvme_ctrlr_opts *opts)
{
	printf("Testing NVMe controller at: %s\n", trid->traddr);
	return true;
}

/**
 * Attach to the controller and check each namespace for zns support. Exits
 * immediately after finding first controller with ZNS namesapce.
 */
static void attach_cb(
	void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
	int nsid, num_ns;
	struct spdk_nvme_ns *ns;

	// Already found a controller with ZNS namespace, detach and return
	if(have_zns_ns) {
		spdk_nvme_detach(ctrlr);
		return;
	}

	// Determine total amount of namespaces on this controller
	num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);

	// Loop through all active namespaces trying to find a zns namespace
	for (nsid = 1; nsid <= num_ns; nsid++) {
		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);

		// Check that namespace exists
		if(ns == NULL) continue;

		// Check that namespace is active
		if(!spdk_nvme_ns_is_active(ns)) continue;

		// Check that namespace supports ZNS command set
		if(spdk_nvme_ns_get_csi(ns) != SPDK_NVME_CSI_ZNS) continue;

		printf("Found ZNS supporting namespace: %u on device: %s\n",
			spdk_nvme_ns_get_id(ns), trid->traddr);

		// Namespace is activate and supports ZNS
		have_zns_ns = true;

		// Assign variables to global state
		zns_entry.ctrlr = ctrlr;
		zns_entry.ns = ns;

		// Create qpair for I/O operations
		zns_entry.qpair = spdk_nvme_ctrlr_alloc_io_qpair(
			zns_entry.ctrlr, NULL, 0);

		// Only want first ZNS supporting namespace
		break;
	}

	// Did not find ZNS supporting namespace on this controller, detaching
	if(have_zns_ns == false) spdk_nvme_detach(ctrlr);
}

/**
 * Completion callback for write and read commands. Ensures the command executes
 * and otherwise exits the application cleanly.
 */
static void check_complete(void *arg, const struct spdk_nvme_cpl *completion) {
	struct ns_entry *entry = (ns_entry*) arg;

	if (spdk_nvme_cpl_is_error(completion)) {
		spdk_nvme_qpair_print_completion(
			entry->qpair, (struct spdk_nvme_cpl *) completion);
		fprintf(stderr, "I/O error status: %s\n",
			spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "I/O failed, aborting run\n");
		spdk_nvme_detach(zns_entry.ctrlr);
		exit(1);
	}
}

/**
 * Active busy loop that waits for the completion of a single outstanding I/O
 * operation on any qpair.
 */
inline static void spin_complete(struct ns_entry *entry) {
	while(spdk_nvme_qpair_process_completions(entry->qpair, 0) == 0) {
		;
	}
}

int main(int argc, char **argv) {
	struct spdk_nvme_detach_ctx *detach_ctx = NULL;
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

	// If global state indicates no device found, exit
	if(zns_entry.ns == nullptr) {
		printf("No NVMe device supporting zoned namespaces (ZNS) could"
			"be found, exiting.\n");
		exit(1);
	}

	// get the data of a single lba / sector
	const struct spdk_nvme_ns_data *ref_ns_data =
		spdk_nvme_ns_get_data(zns_entry.ns);
	uint32_t lba_size = ref_ns_data->nsze;

	// Get zone size
	uint32_t zone_size = spdk_nvme_zns_ns_get_zone_size(zns_entry.ns);

	// Allocate buffer for reading and writing equal to size of sector
	uint32_t *data = (uint32_t*) spdk_zmalloc(
		lba_size, lba_size, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	printf("Initialization complete.\n");

	// Reset all zones in the namespace.
	spdk_nvme_zns_reset_zone(zns_entry.ns, zns_entry.qpair, 0, true,
		check_complete, &zns_entry);

	// Wait for I/O operation to complete
	spin_complete(&zns_entry);

	printf("Reset zones complete.\n");

	uint32_t lba_zone = zone_size / lba_size;
	assert(zone_size % lba_size == 0);

	uint32_t int_lba = lba_size / sizeof(uint32_t);
	assert(lba_size % sizeof(uint32_t)== 0);

	// Measure hypothetical write speed
	struct timespec start_write, stop_write;
	clock_gettime(CLOCK_MONOTONIC, &start_write);

	// For each lba in the zone
	for(uint32_t i = 0; i < lba_zone; i++) {
		// For each unit that fits in the lba
		for(uint32_t j = 0; j < int_lba; j++) {
			*(data + j) = rand() % UINT32_MAX;
		}

		// Zone append automatically tracks write pointer within block, so the
		// zslba argument remains 0 for the entire zone.
		spdk_nvme_zns_zone_append(zns_entry.ns, zns_entry.qpair, data, 0,
			1, check_complete, &zns_entry, 0);

		spin_complete(&zns_entry);
	}
	// Write operations finished
	clock_gettime(CLOCK_MONOTONIC, &stop_write);

	printf("Filled first zone with data.\n");

	// Use different data buffer for result, proofs data was read from device
	uint32_t *result_data = (uint32_t*) spdk_zmalloc(
		lba_size, lba_size, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	// Measure hypothetical read speed
	struct timespec start_read, stop_read;
	clock_gettime(CLOCK_MONOTONIC, &start_read);

	static constexpr uint32_t UINT32_HALF = RAND_MAX / 2;
	uint64_t filter_count = 0;
	for(uint32_t i = 0; i < lba_zone; i++) {
		spdk_nvme_ns_cmd_read(zns_entry.ns, zns_entry.qpair, result_data, i,
			1, check_complete, &zns_entry, 0);

		spin_complete(&zns_entry);

		for(uint32_t j = 0; j < int_lba; j++) {
			if(*(data + j) > UINT32_HALF) filter_count++;
		}
	}
	// Finish reading
	clock_gettime(CLOCK_MONOTONIC, &stop_read);

	// Division filtered vs total should be around 50%.
	printf("Filtered %lu out of %lu integers\n", filter_count,
		(uint64_t)int_lba * lba_zone);

	double write_time = ((double) stop_write.tv_sec) +
		((double) stop_write.tv_nsec / (1000000000.0)) -
		((double) start_write.tv_sec) +
		((double) start_write.tv_nsec / (1000000000.0));
	double read_time = ((double) stop_read.tv_sec) +
		((double) stop_read.tv_nsec / (1000000000.0)) -
		((double) start_read.tv_sec) +
		((double) start_read.tv_nsec / (1000000000.0));
	printf("Hypothethical write speed: %f KB/s\n", zone_size / write_time / 1024);
	printf("Hypothethical read speed: %f KB/s\n", zone_size / read_time / 1024);

	// Teardown
	printf("Tearing down.\n");
	spdk_nvme_detach(zns_entry.ctrlr);
	spdk_free(data);
	spdk_free(result_data);

	return 0;
}