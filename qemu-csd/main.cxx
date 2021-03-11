#include <iostream>
#include <stdexcept>

#include "arguments.hpp"
#include "nvm_csd.hpp"

int main(int argc, char* argv[]) {

	qemucsd::arguments::options opts;
	qemucsd::arguments::parse_args(argc, argv, &opts);

	qemucsd::nvm_csd::NvmCsd nvm_csd(&opts);

//	std::cout << "Name: " << opts.spdk.name << std::endl;

	return EXIT_SUCCESS;
}