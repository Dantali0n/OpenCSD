#include <iostream>
#include <stdexcept>

#include "arguments.hpp"
#include "spdk_init.hpp"
#include "nvm_csd.hpp"

extern "C" {
	#include "bpf_zone_int_filter.h"
}

int main(int argc, char* argv[]) {
	struct bpf_zone_int_filter *skel;
	qemucsd::arguments::options opts;
	struct qemucsd::spdk_init::ns_entry entry;

	// Parse commandline arguments
	qemucsd::arguments::parse_args(argc, argv, &opts);

//	std::cout << "uBPF vm mem size: " << opts.ubpf_mem_size << std::endl;

	// Initialize SPDK with the first ZNS supporting zone found
//	if(qemucsd::spdk_init::initialize_zns_spdk(&opts, &entry) < 0)
//		return EXIT_FAILURE;

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

	if(return_size < 0) {
		fprintf(stderr, "Error while executing BPF program on device\n");
		return EXIT_FAILURE;
	}

	void *data = malloc(return_size);
	nvm_csd.nvm_cmd_bpf_result(data);

	std::cout << "BPF device result: " << *(uint64_t*)data << std::endl;

	return EXIT_SUCCESS;
}