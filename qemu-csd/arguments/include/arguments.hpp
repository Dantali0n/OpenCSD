#ifndef QEMUCSD_ARGUMENTS_HPP
#define QEMUCSD_ARGUMENTS_HPP

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include "spdk/stdinc.h"
#include <spdk/env.h>

#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace qemucsd {
	namespace arguments {

		// Enum to Specify desired method to initialize NVMe device
		enum DeviceInitMode {
			DEV_INIT_PRESERVE, // Preserve current device state
			DEV_INIT_RESET, // Reset all zones of the device upon initialization
		};

		/**
		 * Program options structure
		 */
		struct options {
			/** values */
			DeviceInitMode dev_init_mode;

			/** owned / referenced counted */
			std::shared_ptr<std::string> settings;

			/** SPDK environment options */
			struct spdk_env_opts spdk;

			/** Containers to prevent SPDK data going out of scope */
			std::shared_ptr<std::string> _name;
		};

		/**
		 * Parse the arguments using boost::program_options and display helpful
		 * messages if necessary.
		 */
		void parse_args(int argc, char *argv[], struct options *options);
	}
}

#endif // QEMUCSD_ARGUMENTS_HPP
