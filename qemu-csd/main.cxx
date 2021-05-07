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

#include <chrono>
#include <iostream>
#include <fstream>
#include <stdexcept>

#ifdef QEMUCSD_DEBUG
	#include <backward.hpp>
	using namespace backward;
#endif

using std::ios_base;

#include "arguments.hpp"
#include "spdk_init.hpp"
#include "nvm_csd.hpp"

extern "C" {
	#include <signal.h>

	#include "bpf_zone_int_filter.h"
}

/**
 * Stack trace printer when encountering seg faults.
 */
struct sigaction glob_sigaction;
void segfault_handler(int signal, siginfo_t *si, void *arg) {
	#ifdef QEMUCSD_DEBUG
		StackTrace st; st.load_here(32);
		Printer p; p.print(st);
	#endif
	exit(1);
}

void fill_first_zone(struct qemucsd::spdk_init::ns_entry *entry,
    struct qemucsd::arguments::options *opts)
{
    std::ifstream in(*opts->input_file, ios_base::in | ios_base::binary);

    // Determine length of input file, Huge performance impact that will make
    // the performance incomparable to nvme cli.
//    in.seekg(0, ios_base::end);
//    std::streamsize file_length = in.tellg();
//    in.seekg(0, ios_base::beg);

    in.seekg(0, ios_base::beg);
    std::streamsize file_length = in.tellg();

    // Check that the file exists
    if(file_length <= 0) {
        std::cerr << "File " << *opts->input_file << " does not exist in" <<
                  "current directory" << std::endl;
    }

	const struct spdk_nvme_ns_data *ref_ns_data =
		spdk_nvme_ns_get_data(entry->ns);
	uint32_t lba_size = ref_ns_data->nsze;
	uint32_t zone_size = spdk_nvme_zns_ns_get_zone_size(entry->ns);
	uint32_t lba_zone = zone_size / lba_size;
	assert(zone_size % lba_size == 0);

	// Determine if length of file is sufficient
    in.seekg(zone_size, ios_base::beg);
    file_length = in.tellg();
    in.seekg(0, ios_base::beg);

	// Ensure the input file has sufficient data to write the whole zone
	assert(file_length >= zone_size);

	// Create buffer to store file contents into
    char* file_buffer = new char[file_length];
    in.read(file_buffer, file_length);

	uint32_t int_lba = lba_size / sizeof(uint32_t);
	assert(lba_size % sizeof(uint32_t)== 0);

	uint32_t *data = (uint32_t*) spdk_zmalloc(
		lba_size, lba_size, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	// Create a copy of the pointer we can safely advance
    char* file_buffer_alias = file_buffer;
	for(uint32_t i = 0; i < lba_zone; i++) {

	    // Copy file contents into SPDK buffer
        memcpy(data, file_buffer_alias, lba_size);

		// Zone append automatically tracks write pointer within block, so the
		// zslba argument remains 0 for the entire zone.
		spdk_nvme_zns_zone_append(entry->ns, entry->qpair, data, 0,
								  1, qemucsd::spdk_init::error_print, entry, 0);
		spin_complete(entry);

		// Advance buffer pointer.
        file_buffer_alias += lba_size;
	}

	spdk_free(data);

	// This is why alias is needed
    delete[] file_buffer;
}

int main(int argc, char* argv[]) {
	struct bpf_zone_int_filter *skel = nullptr;
	qemucsd::arguments::options opts;
	struct qemucsd::spdk_init::ns_entry entry = {0};

	// Setup segfault handler to print backward stacktraces
	sigemptyset(&glob_sigaction.sa_mask);
	glob_sigaction.sa_sigaction = segfault_handler;
	glob_sigaction.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &glob_sigaction, NULL);

	try {
		// Parse commandline arguments
		qemucsd::arguments::parse_args(argc, argv, &opts);

		// Initialize SPDK with the first ZNS supporting zone found
		if (qemucsd::spdk_init::initialize_zns_spdk(&opts, &entry) < 0)
			return EXIT_FAILURE;

		fill_first_zone(&entry, &opts);

		// Initialize simulator for NVMe BPF command set
		qemucsd::nvm_csd::NvmCsd nvm_csd(&opts, &entry);

		skel = bpf_zone_int_filter__open();
		if (!skel) {
			fprintf(stderr, "Failed to open BPF skeleton\n");
			return EXIT_FAILURE;
		}

		// Run bpf program on 'device'
		uint64_t return_size = nvm_csd.nvm_cmd_bpf_run(
			skel->skeleton->data, skel->skeleton->data_sz);

		if (return_size < 0) {
			fprintf(stderr, "Error while executing BPF program on device\n");
			return EXIT_FAILURE;
		}

		void *data = malloc(return_size);
		nvm_csd.nvm_cmd_bpf_result(data);

		std::cout << "BPF device result: " << *(uint64_t *) data << std::endl;

		free(data);
	}
	catch(...) {
		#ifdef QEMUCSD_DEBUG
			StackTrace st; st.load_here(32);
			Printer p; p.print(st);
		#endif
	}

	return EXIT_SUCCESS;
}