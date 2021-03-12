#ifndef QEMU_CSD_ARGUMENTS_HPP
#define QEMU_CSD_ARGUMENTS_HPP

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <spdk/stdinc.h>
#include <spdk/env.h>

#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace qemucsd::arguments {

	// Enum to Specify desired method to initialize NVMe device using SPDK
	enum DeviceInitMode {
		DEV_INIT_PRESERVE, // Preserve current device state
		DEV_INIT_RESET, // Reset all zones of the device upon initialization
	};

	static const std::string DEFAULT_SPDK_NAME = "";
	static const DeviceInitMode DEFAULT_DEV_INIT_MODE = DEV_INIT_RESET;
	static constexpr uint64_t DEFAULT_UBPF_MEM_SIZE = 1024*512;

	/**
	 * Program options structure
	 */
	struct options {
		/** values */
		DeviceInitMode dev_init_mode;
		uint64_t ubpf_mem_size;

		/** owned / referenced counted */
//		std::shared_ptr<std::string> settings;

		/** SPDK environment options */
		struct spdk_env_opts spdk;

		/** Containers to prevent data going out of scope */
		std::shared_ptr<std::string> _name;
	};

	/**
	 * Parse the arguments using boost::program_options and display helpful
	 * messages if necessary.
	 */
	void parse_args(int argc, char *argv[], struct options *options);
}

#endif // QEMU_CSD_ARGUMENTS_HPP
