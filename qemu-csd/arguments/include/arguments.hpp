/**
 * MIT License
 *
 * Copyright (c) 2021 Dantali0n
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
    static const std::string DEFAULT_INPUT_FILE = "integers.dat";
	static const DeviceInitMode DEFAULT_DEV_INIT_MODE = DEV_INIT_RESET;
	static constexpr uint64_t DEFAULT_UBPF_MEM_SIZE = 1024*512;
    static constexpr bool DEFAULT_UBPF_JIT = false;

	/**
	 * Program options structure
	 */
	struct options {
		/** values */
		DeviceInitMode dev_init_mode;
		uint64_t ubpf_mem_size;
		bool ubpf_jit;

		/** owned / referenced counted */
		std::shared_ptr<std::string> input_file;

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
