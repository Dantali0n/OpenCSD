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

#include "spdk_init.hpp"

namespace qemucsd::spdk_init {

    using std::ios_base;

	int initialize_zns_spdk(struct arguments::options *options,
		struct ns_entry *entry)
	{
		int rc;

		// Set the pointers in the container to null, just in case
		entry->buffer = nullptr;
		entry->ctrlr = nullptr;
		entry->qpair = nullptr;
		entry->ns = nullptr;

		rc = spdk_env_init(&options->spdk);
		if(rc < 0) {
			std::cerr << "Unable to initialize SPDK env: " << strerror(rc) <<
		  		std::endl;
			return -1;
		}

		rc = spdk_nvme_probe(NULL, entry, probe_cb_attach_all,
	   		attach_cb_ns_entry,NULL);
		if(rc < 0) {
			std::cerr << "spdk_nvme_probe() failed: " << strerror(rc) <<
		  		std::endl;
			return -1;
		}

		// Reset all zones in the namespace.
		if(options->dev_init_mode == arguments::DEV_INIT_RESET) {
            rc = reset_zones(entry);
            if(rc < 0) {
                std::cerr << "spdk_nvme_zns_reset_zone() failed: " <<
                          strerror(rc) << std::endl;
                return -1;
            }
        }

		return 0;
	}

    int reset_zones(struct ns_entry *entry) {
        int result = spdk_nvme_zns_reset_zone(
            entry->ns, entry->qpair, 0, true, error_print, &entry);

        // Wait for I/O operation to complete
        spin_complete(entry);

        return result;
	}

	static bool probe_cb_attach_all(
		void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		struct spdk_nvme_ctrlr_opts *opts)
	{
		return true;
	}

	static void attach_cb_ns_entry(
		void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
	{
		ns_entry *entry = (ns_entry*)cb_ctx;
		int nsid, num_ns;
		struct spdk_nvme_ns *ns;

		// Already found a controller with ZNS namespace, detach and return
		if(entry->ctrlr != nullptr) {
			spdk_nvme_detach(ctrlr);
			return;
		}

		// Determine total amount of namespaces on this controller
		num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
		// Loop through all active namespaces trying to find a ZNS namespace
		for(nsid = 1; nsid <= num_ns; nsid++) {
			ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);

			// Check that namespace exists
			if(ns == NULL) continue;
			// Check that namespace is active
			if(!spdk_nvme_ns_is_active(ns)) continue;
			// Check that namespace supports ZNS command set
			if(spdk_nvme_ns_get_csi(ns) != SPDK_NVME_CSI_ZNS) continue;

            const struct spdk_nvme_zns_ns_data* ns_data =
                spdk_nvme_zns_ns_get_data(ns);

            // Can't support variable zone capacity as we use a single variable
            // to differentiate between size and capacity difference.
            if(ns_data->zoc.variable_zone_capacity) {

            }

            // Device must be capable of reading across zone boundaries.
            if(ns_data->ozcs.read_across_zone_boundaries == 0) {

            }


			// Namespace is activate and supports ZNS
			printf("Found ZNS supporting namespace: %u on device: %s\n",
				   spdk_nvme_ns_get_id(ns), trid->traddr);

			// Assign variables to global state
			entry->ctrlr = ctrlr;
			entry->ns = ns;

			// Create qpair for I/O operations
			entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(
				entry->ctrlr, NULL, 0);

			// Determine size of DMA IO buffer
            entry->lba_size = spdk_nvme_ns_get_sector_size(entry->ns);
			entry->buffer_size = entry->lba_size;

			// Construct DMA buffer
			entry->buffer = spdk_zmalloc(entry->buffer_size, entry->buffer_size,
				NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

			// Add zone size to entry
            uint32_t zone_size = spdk_nvme_zns_ns_get_zone_size(entry->ns);
            entry->zone_size = zone_size / entry->lba_size;

            // Add size of device to entry
            entry->device_size = spdk_nvme_zns_ns_get_num_zones(entry->ns);

            // Determine maximum number of open zones
            entry->max_open = spdk_nvme_zns_ns_get_max_open_zones(entry->ns);

			// Only want first ZNS supporting namespace
			break;
		}

		// Did not find ZNS supporting namespace on this controller, detaching
		if(entry->ctrlr == nullptr) spdk_nvme_detach(ctrlr);
	}

	void error_print(void *void_entry,
		const struct spdk_nvme_cpl *completion)
	{
		struct ns_entry *entry = (ns_entry*) void_entry;

		if(spdk_nvme_cpl_is_error(completion)) {
			spdk_nvme_qpair_print_completion(
					entry->qpair, (struct spdk_nvme_cpl *) completion);
			fprintf(stderr, "I/O error status: %s\n",
					spdk_nvme_cpl_get_status_string(&completion->status));
			fprintf(stderr, "I/O failed, aborting run\n");
			spdk_nvme_detach(entry->ctrlr);
		}
	}

    int fill_first_zone(struct qemucsd::spdk_init::ns_entry *entry,
        struct qemucsd::arguments::options *opts)
    {
        int rc = 0;

        std::ifstream in(*opts->input_file, ios_base::in | ios_base::binary);

        // Determine length of input file, Huge performance impact that will make
        // the performance incomparable to nvme cli.
        //    in.seekg(0, ios_base::end);
        //    std::streamsize file_length = in.tellg();
        //    in.seekg(0, ios_base::beg);

        in.seekg(0, ios_base::beg);
        std::streamsize file_length = in.tellg();

        // Check that the file exists
        if(file_length < 0) {
            std::cerr << "File " << *opts->input_file << " does not exist in" <<
                      "current directory" << std::endl;
            exit(1);
        }
        uint64_t zone_size = entry->lba_size * entry->zone_size;

        // Determine if length of file is sufficient
        in.seekg(zone_size, ios_base::beg);
        file_length = in.tellg();
        in.seekg(0, ios_base::beg);

        // Ensure the input file has sufficient data to write the whole zone
        assert(file_length >= zone_size);

        // Create buffer to store file contents into
        char* file_buffer = new char[file_length];
        in.read(file_buffer, file_length);

        uint32_t int_lba = entry->lba_size / sizeof(uint32_t);
        assert(entry->lba_size % sizeof(uint32_t)== 0);

        uint32_t *data = (uint32_t*) spdk_zmalloc(
            entry->lba_size, entry->lba_size, NULL, SPDK_ENV_SOCKET_ID_ANY,
            SPDK_MALLOC_DMA);

        if(data == nullptr) return -1;

        // Create a copy of the pointer we can safely advance
        char* file_buffer_alias = file_buffer;
        for(uint32_t i = 0; i < entry->zone_size; i++) {

            // Copy file contents into SPDK buffer
            memcpy(data, file_buffer_alias, entry->lba_size);

            // Zone append automatically tracks write pointer within block, so the
            // zslba argument remains 0 for the entire zone.
            rc = spdk_nvme_zns_zone_append(entry->ns, entry->qpair, data, 0,
                                      1, qemucsd::spdk_init::error_print,
                                      entry, 0);

            if(rc < 0) return -1;

            spin_complete(entry);

            // Advance buffer pointer.
            file_buffer_alias += entry->lba_size;
        }

        spdk_free(data);

        // This is why alias is needed
        delete[] file_buffer;

        return 0;
    }
}