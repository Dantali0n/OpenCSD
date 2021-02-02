#include <iostream>

#include "arguments.hpp"

int main(int argc, char* argv[]) {
	airglow::arguments::options opts{};
	airglow::arguments::parse_args(argc, argv, &opts);
	std::cout << "Has AirGlow file: " << opts.settings->c_str() << std::endl;
	std::cout << "Has AirGlow output: " << opts.window_mode << std::endl;
}