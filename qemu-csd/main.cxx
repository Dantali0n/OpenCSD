#include <iostream>
#include <stdexcept>

#include "arguments.hpp"

int main(int argc, char* argv[]) {

	qemucsd::arguments::options opts;
	qemucsd::arguments::parse_args(argc, argv, &opts);

	return EXIT_SUCCESS;
}