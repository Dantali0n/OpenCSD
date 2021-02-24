#include <iostream>

#include "arguments.hpp"

int main(int argc, char* argv[]) {
	qemucsd::arguments::options opts{};
	qemucsd::arguments::parse_args(argc, argv, &opts);
	std::cout << "qemucsd spdk name: " << opts.spdk.name << std::endl;
	std::cout << "qemucsd init mode: " << opts.dev_init_mode << std::endl;
}