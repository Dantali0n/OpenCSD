#include <iostream>
#include <stdexcept>

#ifdef QEMUCSD_DEBUG
	#include <backward.hpp>
	using namespace backward;
#endif

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
		if (qemucsd::spdk_init::initialize_zns_spdk(&opts, &entry) < 0)
			return EXIT_FAILURE;

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