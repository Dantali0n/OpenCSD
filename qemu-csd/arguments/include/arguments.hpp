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

#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace qemucsd::arguments {

    // When generics get out of hand
    typedef std::pair<int, std::vector<char*>> t_strip_args;
    typedef std::vector<t_strip_args> t_auto_strip_args;

	// Enum to Specify desired method to initialize NVMe device using SPDK
	enum DeviceInitMode {
		DEV_INIT_PRESERVE, // Preserve current device state
		DEV_INIT_RESET, // Reset all zones of the device upon initialization
	};

    static const char *ARG_SEPARATOR = "--";

	static const char *DEFAULT_SPDK_NAME = "";
    static const char *DEFAULT_INPUT_FILE = "integers.dat";
	static const DeviceInitMode DEFAULT_DEV_INIT_MODE = DEV_INIT_PRESERVE;
	static constexpr uint64_t DEFAULT_UBPF_MEM_SIZE = 1024*128*8;
    static constexpr bool DEFAULT_UBPF_JIT = false;

	/**
	 * Program options structure
	 */
	struct options {
		/** values */
		DeviceInitMode dev_init_mode;

		uint64_t ubpf_mem_size;
		bool ubpf_jit;

		/** owned / reference counted */
		std::shared_ptr<std::string> input_file;

		/** Containers to prevent data going out of scope */
		std::shared_ptr<std::string> _name;
	};

	/**
	 * Parse the arguments using boost::program_options and display helpful
	 * messages if necessary.
	 */
	void parse_args(int argc, char *argv[], struct options *options);

    /**
     * Create vector of argc, argv[] pairs separated by -- arguments. argv[0]
     * is added to each pair and argc is atleast 1.
     */
    void auto_strip_args(
        int argc, char *argv[], t_auto_strip_args* stripped_args);

    /**
     * Reduce the arguments to the first subset before finding an occurrence of
     * -- as argument.
     */
    void strip_args(int argc, char *argv[], t_strip_args* strip_args);
}

#endif // QEMU_CSD_ARGUMENTS_HPP
