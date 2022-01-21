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
				("mode,m", po::value<DeviceInitMode>(&options->dev_init_mode)->default_value(DEFAULT_DEV_INIT_MODE),
                 R"(NVMe SPDK Device initialization mode: "preserve", "reset")")
                ("input-file,f", po::value<bool>(),
                 "Name of file to write to ZNS SSD (Ignored in OpenCSD / FUSE)")
				("vmmem", po::value<uint64_t>(), "uBPF vm memory size in bytes")
                ("jit,j", po::value<bool>(), "uBPF jit compilation")
				// SPDK env opts
				("name", po::value<std::string>(), "Name for SPDK environment");
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << std::endl;
            exit(0);
		}

		if(vm.count("vmmem")) {
			options->ubpf_mem_size = vm["vmmem"].as<uint64_t>();
		} else {
			options->ubpf_mem_size = DEFAULT_UBPF_MEM_SIZE;
		}

        if(vm.count("jit")) {
            options->ubpf_jit = vm["jit"].as<bool>();
        } else {
            options->ubpf_jit = DEFAULT_UBPF_JIT;
        }

		if(vm.count("input-file")) {
			options->input_file = std::make_shared<std::string>(vm["input-file"].as<std::string>());
		} else {
			options->input_file = std::make_shared<std::string>(DEFAULT_INPUT_FILE);
		}

		if(vm.count("name")) {
			options->_name = std::make_shared<std::string>(vm["name"].as<std::string>());
		} else {
			options->_name = std::make_shared<std::string>(DEFAULT_SPDK_NAME);
		}
	}

    void auto_strip_args(
        int argc, char *argv[], t_auto_strip_args* stripped_args)
    {
        // First argument is always file, strip separately
        stripped_args->resize(1);
        stripped_args->at(0).first = 1;
        stripped_args->at(0).second.push_back(argv[0]);
        argc = argc - 1;
        argv = argv + 1;

        // Start stripping at argument 1
        for(int i = 1; argc > 0; i++) {
            stripped_args->resize(i + 1);
            // Always add filename back as first argument for each set
            stripped_args->at(i).second.push_back(stripped_args->at(0).second.at(0));
            strip_args(argc, argv, &stripped_args->at(i));

            // + 1 for adding file argument back, -1 in offset for removing --
            argc = argc - (stripped_args->at(i).first);
            argv = argv + (stripped_args->at(i).first);
        }
    }

    void strip_args(
        int argc, char *argv[], t_strip_args* strip_args)
    {
        for(int i = 0; i < argc; i++) {
            // If argument not -- add it args and continue
            if(strcmp(ARG_SEPARATOR, argv[i]) != 0) {
                strip_args->second.push_back(argv[i]);
                continue;
            }
            // Stop immediately upon encountering --
            break;
        }

        strip_args->first = strip_args->second.size();
    }
}