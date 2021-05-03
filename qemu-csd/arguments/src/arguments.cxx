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

#include "arguments.hpp"

namespace po = boost::program_options;

namespace qemucsd::arguments {
	std::istream &operator>>(std::istream &in, DeviceInitMode &dev_init_mode) {
		std::string token;
		in >> token;

		// Make token detection resilient to use of capitals.
		boost::to_upper(token);

		if (token == "PRESERVE") {
			dev_init_mode = DEV_INIT_PRESERVE;
		} else if (token == "RESET") {
			dev_init_mode = DEV_INIT_RESET;
		} else {
			throw po::invalid_option_value("");
		}

		return in;
	}

	std::ostream &operator<<(std::ostream &out, DeviceInitMode &dev_init_mode)
	{
		if (dev_init_mode == DEV_INIT_PRESERVE) {
			return out << "preserve";
		} else if (dev_init_mode == DEV_INIT_RESET) {
			return out << "reset";
		} else {
			throw po::invalid_option_value("");
		}
	}

	void parse_args(int argc, char* argv[], struct options *options) {
		po::options_description desc("Allowed options");
		desc.add_options()
				("help,h", "Produce help message")
//				("settings,s", po::value<std::string>(), "Alternative path to settings file")
				(
					"mode,m", po::value<DeviceInitMode>(&options->dev_init_mode)->default_value(DEFAULT_DEV_INIT_MODE),
					R"(NVMe SPDK Device initialization mode: "preserve", "reset")"
				)
				("vmmem", po::value<uint64_t>(), "uBPF vm memory size in bytes")
				// SPDK env opts
				("name", po::value<std::string>(), "Name for SPDK environment");
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << std::endl;
		}

		if(vm.count("vmmem")) {
			options->ubpf_mem_size = vm["vmmem"].as<uint64_t>();
		} else {
			options->ubpf_mem_size = DEFAULT_UBPF_MEM_SIZE;
		}

//		if(vm.count("settings")) {
//			options->settings = std::make_shared<std::string>(vm["settings"].as<std::string>());
//		} else {
//			options->settings = std::make_shared<std::string>("");
//		}

		struct spdk_env_opts *opts = &options->spdk;
		spdk_env_opts_init(opts);
		// Same default value as identify example
		opts->shm_id = -1;

		if(vm.count("name")) {
			options->_name = std::make_shared<std::string>(vm["name"].as<std::string>());
			opts->name = options->_name->c_str();
		} else {
			options->_name = std::make_shared<std::string>(DEFAULT_SPDK_NAME);
			opts->name = options->_name->c_str();
		}
	}
}