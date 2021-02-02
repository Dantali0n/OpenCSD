#ifndef QEMUCSD_ARGUMENTS_HPP
#define QEMUCSD_ARGUMENTS_HPP

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/**
 * In shared libraries some things are hard to put into a namespace,
 * everything that is notoriously so will be excluded. This includes
 * functions, templates, classes...
 */
namespace qemucsd {
	namespace arguments {

		// Enum to Specify desired window modes
		enum WindowMode {
			WINDOW_BORDERLESS,
			WINDOW_WINDOWED,
			WINDOW_FULLSCREEN
		};

		/**
		 * Program options structure
		 */
		struct options {
			/** values */
			WindowMode window_mode;

			/** owned / referenced counted */
			std::shared_ptr<std::string> settings;
		};

		/**
		 * Parse the arguments using boost::program_options and display helpful
		 * messages if necessary.
		 */
		void parse_args(int argc, char *argv[], struct options *options);
	}
}

#endif // QEMUCSD_ARGUMENTS_HPP
