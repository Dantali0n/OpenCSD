#include <iostream>
#include <stdexcept>

#include "entry.hpp"

int main() {
	airglow::entry::EntryTriangle app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}