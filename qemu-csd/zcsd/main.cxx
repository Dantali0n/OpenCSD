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
        auto start = std::chrono::high_resolution_clock::now();
		if(qemucsd::spdk_init::initialize_zns_spdk(&opts, &entry) < 0)
			return EXIT_FAILURE;
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << "Initialization and reset: " << duration.count() << "us." << std::endl;

        start = std::chrono::high_resolution_clock::now();
        if(qemucsd::spdk_init::fill_first_zone(&entry, &opts) < 0)
            return EXIT_FAILURE;
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << "Fill first zone: " << duration.count() << "us." << std::endl;

		// Initialize simulator for NVMe BPF command set
        start = std::chrono::high_resolution_clock::now();
		qemucsd::nvm_csd::NvmCsd nvm_csd(&opts, &entry);

		skel = bpf_zone_int_filter__open();
		if (!skel) {
			fprintf(stderr, "Failed to open BPF skeleton\n");
			return EXIT_FAILURE;
		}

		// Run bpf program on 'device'
		uint64_t return_size = nvm_csd.nvm_cmd_bpf_run(
		        (void*)skel->skeleton->data, skel->skeleton->data_sz);

		if (return_size < 0) {
			fprintf(stderr, "Error while executing BPF program on device\n");
			return EXIT_FAILURE;
		}

		void *data = malloc(return_size);
		nvm_csd.nvm_cmd_bpf_result(data);
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << "BPF execution time: " << duration.count() << "us." << std::endl;

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