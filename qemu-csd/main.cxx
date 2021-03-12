#include <iostream>
#include <stdexcept>

#include "arguments.hpp"
#include "nvm_csd.hpp"
#include "spdk_init.hpp"

int main(int argc, char* argv[]) {
	qemucsd::arguments::options opts;
	struct qemucsd::spdk_init::ns_entry entry;

	// Parse commandline arguments
	qemucsd::arguments::parse_args(argc, argv, &opts);

	// Initialize SPDK with the first ZNS supporting zone found
	if(qemucsd::spdk_init::initialize_zns_spdk(&opts, &entry) < 0)
		return EXIT_FAILURE;

	// Initialize simulator for NVMe BPF command set
	qemucsd::nvm_csd::NvmCsd nvm_csd(&opts, &entry);

//	std::cout << "Name: " << opts.spdk.name << std::endl;

	return EXIT_SUCCESS;
}