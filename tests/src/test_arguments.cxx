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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestArguments

#include <boost/test/unit_test.hpp>

#include "tests.hpp"

#include "arguments.hpp"

BOOST_AUTO_TEST_SUITE(Test_Arguments)

    BOOST_AUTO_TEST_CASE(Test_Arguments_Defaults) {
        int argc = 1;
        char *argv[1] = {(char*)"test"};
        qemucsd::arguments::options opts;
        qemucsd::arguments::parse_args(argc, argv, &opts);

        BOOST_CHECK(
            opts.dev_init_mode == qemucsd::arguments::DEFAULT_DEV_INIT_MODE);
        BOOST_CHECK(
            opts.ubpf_mem_size == qemucsd::arguments::DEFAULT_UBPF_MEM_SIZE);
        BOOST_CHECK(opts.ubpf_jit == qemucsd::arguments::DEFAULT_UBPF_JIT);
    }

	BOOST_AUTO_TEST_CASE(Test_Arguments_Device_Mode) {
		int argc = 3;
		char *argv[3] = {(char*)"test", (char*)"-m", (char*)"preserve"};
		qemucsd::arguments::options opts;
		qemucsd::arguments::parse_args(argc, argv, &opts);

		BOOST_CHECK(
			opts.dev_init_mode == qemucsd::arguments::DEV_INIT_PRESERVE);
	}

    BOOST_AUTO_TEST_CASE(Test_Arguments_Device_Mode_reset) {
        int argc = 3;
        char *argv[3] = {(char*)"test", (char*)"-m", (char*)"reset"};
        qemucsd::arguments::options opts;
        qemucsd::arguments::parse_args(argc, argv, &opts);

        BOOST_CHECK(
                opts.dev_init_mode == qemucsd::arguments::DEV_INIT_RESET);
    }

	BOOST_AUTO_TEST_CASE(Test_Arguments_Settings) {
		int argc = 3;
		char *argv[3] = {(char*)"test", (char*)"--vmmem", (char*)"12"};
		qemucsd::arguments::options opts;
		qemucsd::arguments::parse_args(argc, argv, &opts);

		BOOST_CHECK(opts.ubpf_mem_size == 12);
	}

	BOOST_AUTO_TEST_CASE(Test_Arguments_Name) {
		int argc = 3;
		char *argv[3] = {(char*)"test", (char*)"--name", (char*)"spdk_env_opts"};
		qemucsd::arguments::options opts;
		qemucsd::arguments::parse_args(argc, argv, &opts);

		std::string reference = "spdk_env_opts";

		BOOST_CHECK(reference.compare(opts._name->c_str()) == 0);
	}

    BOOST_AUTO_TEST_CASE(Test_Arguments_Jit_true) {
        int argc = 3;
        char *argv[3] = {(char*)"test", (char*)"--jit", (char*)"true"};
        qemucsd::arguments::options opts;
        qemucsd::arguments::parse_args(argc, argv, &opts);

        BOOST_CHECK(opts.ubpf_jit == true);
    }

    BOOST_AUTO_TEST_CASE(Test_Arguments_Jit_false) {
        int argc = 3;
        char *argv[3] = {(char*)"test", (char*)"--jit", (char*)"false"};
        qemucsd::arguments::options opts;
        qemucsd::arguments::parse_args(argc, argv, &opts);

        BOOST_CHECK(opts.ubpf_jit == false);
    }

    BOOST_AUTO_TEST_CASE(Test_Arguments_auto_strip_first) {
        int argc = 3;
        char *argv[3] = {(char*)"test", (char*)"--jit", (char*)"false"};

        qemucsd::arguments::t_auto_strip_args parsed_args;
        qemucsd::arguments::auto_strip_args(argc, argv, &parsed_args);

        // No -- so should be size 2, one for isolated first arg
        BOOST_CHECK(parsed_args.size() == 2);

        BOOST_CHECK(strcmp(parsed_args.at(0).second.at(0), argv[0]) == 0);
        BOOST_CHECK(strcmp(parsed_args.at(1).second.at(0), argv[0]) == 0);

        BOOST_CHECK(strcmp(parsed_args.at(1).second.at(1), argv[1]) == 0);
        BOOST_CHECK(strcmp(parsed_args.at(1).second.at(2), argv[2]) == 0);
    }

    BOOST_AUTO_TEST_CASE(Test_Arguments_auto_strip_second) {
        int argc = 3;
        char *argv[3] = {(char*)"test", (char*)"--", (char*)"false"};

        qemucsd::arguments::t_auto_strip_args parsed_args;
        qemucsd::arguments::auto_strip_args(argc, argv, &parsed_args);

        // One -- so should be size 3, one for isolated first arg
        BOOST_CHECK(parsed_args.size() == 3);

        BOOST_CHECK(strcmp(parsed_args.at(0).second.at(0), argv[0]) == 0);
        BOOST_CHECK(strcmp(parsed_args.at(1).second.at(0), argv[0]) == 0);
        BOOST_CHECK(strcmp(parsed_args.at(2).second.at(0), argv[0]) == 0);

        BOOST_CHECK(strcmp(parsed_args.at(2).second.at(1), argv[2]) == 0);
    }

BOOST_AUTO_TEST_SUITE_END()