/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "spdk/stdinc.h"

#include "spdk/nvme.h"
#include "spdk/vmd.h"
#include "spdk/nvme_zns.h"
#include "spdk/env.h"

struct ctrlr_entry {
	struct spdk_nvme_ctrlr		*ctrlr;
	TAILQ_ENTRY(ctrlr_entry)	link;
	char						name[1024];
};

struct ns_entry {
	struct spdk_nvme_ctrlr	*ctrlr;
	struct spdk_nvme_ns		*ns;
	TAILQ_ENTRY(ns_entry)	link;
	struct spdk_nvme_qpair	*qpair;
};

static TAILQ_HEAD(, ctrlr_entry) g_controllers = TAILQ_HEAD_INITIALIZER(g_controllers);
static TAILQ_HEAD(, ns_entry) g_namespaces = TAILQ_HEAD_INITIALIZER(g_namespaces);

static bool g_vmd = false;

static void register_ns(
	struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns)
{
	struct ns_entry *entry;

	if (!spdk_nvme_ns_is_active(ns)) {
		return;
	}

	entry = (ns_entry*) malloc(sizeof(struct ns_entry));
	if (entry == NULL) {
		perror("ns_entry malloc");
		exit(1);
	}

	entry->ctrlr = ctrlr;
	entry->ns = ns;
	TAILQ_INSERT_TAIL(&g_namespaces, entry, link);

	printf("  Namespace ID: %d size: %juGB\n", spdk_nvme_ns_get_id(ns),
	   spdk_nvme_ns_get_size(ns) / 1000000000);
}

struct hello_world_sequence {
	struct ns_entry	*ns_entry;
	char			*buf;
	unsigned        using_cmb_io;
	int				is_completed;
};

static void read_complete(void *arg, const struct spdk_nvme_cpl *completion) {
	struct hello_world_sequence *sequence = (hello_world_sequence*) arg;

	/* Assume the I/O was successful */
	sequence->is_completed = 1;
	/* See if an error occurred. If so, display information
	 * about it, and set completion value so that I/O
	 * caller is aware that an error occurred.
	 */
	if (spdk_nvme_cpl_is_error(completion)) {
		spdk_nvme_qpair_print_completion(
			sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n",
			spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Read I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}

	/*
	 * The read I/O has completed.  Print the contents of the
	 *  buffer, free the buffer, then mark the sequence as
	 *  completed.  This will trigger the hello_world() function
	 *  to exit its polling loop.
	 */
	printf("%s", sequence->buf);
	spdk_free(sequence->buf);
}

static void write_complete(void *arg, const struct spdk_nvme_cpl *completion) {
	struct hello_world_sequence	*sequence = (hello_world_sequence*) arg;
	struct ns_entry				*ns_entry = sequence->ns_entry;
	int							rc;

	/* See if an error occurred. If so, display information
	 * about it, and set completion value so that I/O
	 * caller is aware that an error occurred.
	 */
	if (spdk_nvme_cpl_is_error(completion)) {
		spdk_nvme_qpair_print_completion(
			sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n",
			spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Write I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}
	/*
	 * The write I/O has completed.  Free the buffer associated with
	 *  the write I/O and allocate a new zeroed buffer for reading
	 *  the data back from the NVMe namespace.
	 */
	if (sequence->using_cmb_io) {
		spdk_nvme_ctrlr_unmap_cmb(ns_entry->ctrlr);
	} else {
		spdk_free(sequence->buf);
	}
	sequence->buf = (char*) spdk_zmalloc(
		0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	rc = spdk_nvme_ns_cmd_read(
		ns_entry->ns, ns_entry->qpair, sequence->buf,
		0, /* LBA start */
		1, /* number of LBAs */
		read_complete, (void *)sequence, 0);
	if (rc != 0) {
		fprintf(stderr, "starting read I/O failed\n");
		exit(1);
	}
}

static void reset_zone_complete(
	void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence *sequence = (hello_world_sequence*) arg;

	/* Assume the I/O was successful */
	sequence->is_completed = 1;
	/* See if an error occurred. If so, display information
	 * about it, and set completion value so that I/O
	 * caller is aware that an error occurred.
	 */
	if (spdk_nvme_cpl_is_error(completion)) {
		spdk_nvme_qpair_print_completion(
			sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n",
			spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Reset zone I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}
}

static void reset_zone_and_wait_for_completion(
	struct hello_world_sequence *sequence)
{
	if (spdk_nvme_zns_reset_zone(
		sequence->ns_entry->ns, sequence->ns_entry->qpair,
		0, /* starting LBA of the zone to reset */
		false, /* don't reset all zones */
		reset_zone_complete,
		sequence))
	{
		fprintf(stderr, "starting reset zone I/O failed\n");
		exit(1);
	}
	while (!sequence->is_completed) {
		spdk_nvme_qpair_process_completions(sequence->ns_entry->qpair, 0);
	}
	sequence->is_completed = 0;
}

static void print_zns_zone(struct spdk_nvme_zns_zone_desc *desc) {
	printf("ZSLBA: 0x%016"PRIx64" ZCAP: 0x%016"PRIx64" WP: 0x%016"PRIx64" ZS: %x ZT: %x ZA: %x\n",
	   desc->zslba, desc->zcap, desc->wp, desc->zs, desc->zt, desc->za.raw);
}

static void check_complete(void *arg, const struct spdk_nvme_cpl *completion) {
	if (spdk_nvme_cpl_is_error(completion)) {
//		spdk_nvme_qpair_print_completion(
//			sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n",
			spdk_nvme_cpl_get_status_string(&completion->status));
//		fprintf(stderr, "Reset zone I/O failed, aborting run\n");
//		sequence->is_completed = 2;
		exit(1);
	}
}

static void hello_world() {
	struct ns_entry				*ns_entry;
	struct hello_world_sequence	sequence;
	int							rc;
	size_t						sz;
	int 						result = 0;

	TAILQ_FOREACH(ns_entry, &g_namespaces, link) {
		/*
		 * Allocate an I/O qpair that we can use to submit read/write requests
		 *  to namespaces on the controller.  NVMe controllers typically support
		 *  many qpairs per controller.  Any I/O qpair allocated for a controller
		 *  can submit I/O to any namespace on that controller.
		 *
		 * The SPDK NVMe driver provides no synchronization for qpair accesses -
		 *  the application must ensure only a single thread submits I/O to a
		 *  qpair, and that same thread must also check for completions on that
		 *  qpair.  This enables extremely efficient I/O processing by making all
		 *  I/O operations completely lockless.
		 */
		ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(
			ns_entry->ctrlr, NULL, 0);
		if (ns_entry->qpair == NULL) {
			printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
			return;
		}

		/*
		 * Use spdk_dma_zmalloc to allocate a 4KB zeroed buffer. This memory
		 * will be pinned, which is required for data buffers used for SPDK NVMe
		 * I/O operations.
		 */
		sequence.using_cmb_io = 1;
		sequence.buf = (char*) spdk_nvme_ctrlr_map_cmb(ns_entry->ctrlr, &sz);
		if (sequence.buf == NULL || sz < 0x1000) {
			sequence.using_cmb_io = 0;
			sequence.buf = (char*) spdk_zmalloc(
					0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
		}
		if (sequence.buf == NULL) {
			printf("ERROR: write buffer allocation failed\n");
			return;
		}
		if (sequence.using_cmb_io) {
			printf("INFO: using controller memory buffer for IO\n");
		} else {
			printf("INFO: using host memory buffer for IO\n");
		}
		sequence.is_completed = 0;
		sequence.ns_entry = ns_entry;

		/*
		 * If the namespace is a Zoned Namespace, rather than a regular
		 * NVM namespace, we need to reset the first zone, before we
		 * write to it. This not needed for regular NVM namespaces.
		 */
		if (spdk_nvme_ns_get_csi(ns_entry->ns) == SPDK_NVME_CSI_ZNS) {
			reset_zone_and_wait_for_completion(&sequence);
		}

		uint64_t num_zones = spdk_nvme_zns_ns_get_num_zones(ns_entry->ns);
		printf("NVMe namespace has %lu zones\n", num_zones);

		const struct spdk_nvme_ns_data *ref_ns_data =
			spdk_nvme_ns_get_data(ns_entry->ns);
		printf("NVMe namespace lba size: %lu\n",
		   ref_ns_data->nsze);

		const struct spdk_nvme_zns_ns_data *ref_ns_zns_data =
			spdk_nvme_zns_ns_get_data(ns_entry->ns);
		printf("NVMe namespace zone size %lu (%lu * %lu)\n",
		   spdk_nvme_zns_ns_get_zone_size(ns_entry->ns),
		   ref_ns_zns_data->lbafe->zsze, ref_ns_data->nsze);

//		const struct spdk_nvme_zns_ctrlr_data *ref_ctrl_data =
//			spdk_nvme_zns_ctrlr_get_data(ns_entry->ctrlr);
		printf("NVMe namespace zone append size limit %u\n",
		   spdk_nvme_zns_ctrlr_get_max_zone_append_size(ns_entry->ctrlr));

		// Allocate buffers for getting zone information
		uint32_t report_bufsize =
			spdk_nvme_ns_get_max_io_xfer_size(ns_entry->ns);
		auto *report_buf = (spdk_nvme_zns_zone_report *) malloc(report_bufsize);

		// Queue a command to retrieve information about all zones, this command
		// might not actually able to get information for ALL zones depending on
		// the number of zones. Given this small example a single call will be
		// sufficient.
		spdk_nvme_zns_report_zones(
			ns_entry->ns, ns_entry->qpair, report_buf, report_bufsize, 0,
			SPDK_NVME_ZRA_LIST_ALL, false,
			check_complete, NULL);

		// Wait for the I/O operation to complete
		result = 0;
		while (result == 0) {
			result = spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}

		// Print the zone information for all zones in the namespace.
		printf("NVMe zone information:\n");
		for (uint32_t i = 0; i < report_buf->nr_zones && i < num_zones; i++) {
			print_zns_zone(&report_buf->descs[i]);
		}

		// Put hello world into buffer
		snprintf(sequence.buf, 0x1000, "%s", "Hello world!\n");

		// Write to the first lba of the first zone.
		struct spdk_nvme_zns_zone_desc first_zone_info = report_buf->descs[0];
		spdk_nvme_zns_zone_append(ns_entry->ns, ns_entry->qpair, sequence.buf,
			first_zone_info.zslba, 1, check_complete, NULL, 0);

		// Wait for the I/O operation to complete
		result = 0;
		while (result == 0) {
			result = spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}

		// Free the old buffer and allocate a new one, this primarily to prove
		// that the data can not be retained between the write and read.
		if (sequence.using_cmb_io) {
			spdk_nvme_ctrlr_unmap_cmb(ns_entry->ctrlr);
		} else {
			spdk_free(sequence.buf);
		}
		sequence.buf = (char*) spdk_zmalloc(
			0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

		// Read back the first lba of the first zone
		rc = spdk_nvme_ns_cmd_read(
			ns_entry->ns, ns_entry->qpair, sequence.buf,
			first_zone_info.zslba, /* LBA start */
			1, /* number of LBAs */
			check_complete, NULL, 0);
		if (rc != 0) {
			fprintf(stderr, "starting read I/O failed\n");
			exit(1);
		}

		// Wait for the I/O operation to complete
		result = 0;
		while (result == 0) {
			result = spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}

		// Display the read back data and free the buffer
		printf("Read from first zone: %s", sequence.buf);
		spdk_free(sequence.buf);

		/*
		 * Free the I/O qpair.  This typically is done when an application exits.
		 *  But SPDK does support freeing and then reallocating qpairs during
		 *  operation.  It is the responsibility of the caller to ensure all
		 *  pending I/O are completed before trying to free the qpair.
		 */
		spdk_nvme_ctrlr_free_io_qpair(ns_entry->qpair);
	}
}

static bool probe_cb(
	void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	struct spdk_nvme_ctrlr_opts *opts)
{
	printf("Attaching to %s\n", trid->traddr);

	return true;
}

static void attach_cb(
	void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
	int nsid, num_ns;
	struct ctrlr_entry *entry;
	struct spdk_nvme_ns *ns;
	const struct spdk_nvme_ctrlr_data *cdata;

	entry = (ctrlr_entry*) malloc(sizeof(struct ctrlr_entry));
	if (entry == NULL) {
		perror("ctrlr_entry malloc");
		exit(1);
	}

	printf("Attached to %s\n", trid->traddr);

	/*
	 * spdk_nvme_ctrlr is the logical abstraction in SPDK for an NVMe
	 *  controller.  During initialization, the IDENTIFY data for the
	 *  controller is read using an NVMe admin command, and that data
	 *  can be retrieved using spdk_nvme_ctrlr_get_data() to get
	 *  detailed information on the controller.  Refer to the NVMe
	 *  specification for more details on IDENTIFY for NVMe controllers.
	 */
	cdata = spdk_nvme_ctrlr_get_data(ctrlr);

	snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)",
		cdata->mn, cdata->sn);

	entry->ctrlr = ctrlr;
	TAILQ_INSERT_TAIL(&g_controllers, entry, link);

	/*
	 * Each controller has one or more namespaces.  An NVMe namespace is basically
	 *  equivalent to a SCSI LUN.  The controller's IDENTIFY data tells us how
	 *  many namespaces exist on the controller.  For Intel(R) P3X00 controllers,
	 *  it will just be one namespace.
	 *
	 * Note that in NVMe, namespace IDs start at 1, not 0.
	 */
	num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
	printf("Using controller %s with %d namespaces.\n",
	   entry->name, num_ns);
	for (nsid = 1; nsid <= num_ns; nsid++) {
		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (ns == NULL) {
			continue;
		}
		register_ns(ctrlr, ns);
	}
}

static void cleanup() {
	struct ns_entry *ns_entry, *tmp_ns_entry;
	struct ctrlr_entry *ctrlr_entry, *tmp_ctrlr_entry;
	struct spdk_nvme_detach_ctx *detach_ctx = NULL;

	TAILQ_FOREACH_SAFE(ns_entry, &g_namespaces, link, tmp_ns_entry) {
		TAILQ_REMOVE(&g_namespaces, ns_entry, link);
		free(ns_entry);
	}

	TAILQ_FOREACH_SAFE(ctrlr_entry, &g_controllers, link, tmp_ctrlr_entry) {
		TAILQ_REMOVE(&g_controllers, ctrlr_entry, link);
		spdk_nvme_detach_async(ctrlr_entry->ctrlr, &detach_ctx);
		free(ctrlr_entry);
	}

	while (detach_ctx && spdk_nvme_detach_poll_async(detach_ctx) == -EAGAIN) {
		;
	}
}

static void usage(const char *program_name) {
	printf("%s [options]", program_name);
	printf("\n");
	printf("options:\n");
	printf(" -V         enumerate VMD\n");
}

static int parse_args(int argc, char **argv) {
	int op;

	while ((op = getopt(argc, argv, "V")) != -1) {
		switch (op) {
			case 'V':
				g_vmd = true;
				break;
			default:
				usage(argv[0]);
				return 1;
		}
	}

	return 0;
}

int main(int argc, char **argv) {
	int rc;
	struct spdk_env_opts opts;

	rc = parse_args(argc, argv);
	if (rc != 0) {
		return rc;
	}

	/*
	 * SPDK relies on an abstraction around the local environment
	 * named env that handles memory allocation and PCI device operations.
	 * This library must be initialized first.
	 *
	 */
	spdk_env_opts_init(&opts);
	opts.name = "hello_world";
	opts.shm_id = 0;
	if (spdk_env_init(&opts) < 0) {
		fprintf(stderr, "Unable to initialize SPDK env\n");
		return 1;
	}

	printf("Initializing NVMe Controllers\n");

	// if (g_vmd && spdk_vmd_init()) {
	// 	fprintf(stderr, "Failed to initialize VMD."
	// 		" Some NVMe devices can be unavailable.\n");
	// }

	/*
	 * Start the SPDK NVMe enumeration process.  probe_cb will be called
	 *  for each NVMe controller found, giving our application a choice on
	 *  whether to attach to each controller.  attach_cb will then be
	 *  called for each controller after the SPDK NVMe driver has completed
	 *  initializing the controller we chose to attach.
	 */
	rc = spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL);
	if (rc != 0) {
		fprintf(stderr, "spdk_nvme_probe() failed\n");
		cleanup();
		return 1;
	}

	if (TAILQ_EMPTY(&g_controllers)) {
		fprintf(stderr, "no NVMe controllers found\n");
		cleanup();
		return 1;
	}

	printf("Initialization complete.\n");
	hello_world();
	cleanup();
	// if (g_vmd) {
	// 	spdk_vmd_fini();
	// }

	return 0;
}