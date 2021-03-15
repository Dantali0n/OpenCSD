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

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "tests.hpp"

#include "arguments.hpp"

BOOST_AUTO_TEST_SUITE(Test_Arguments)

	BOOST_AUTO_TEST_CASE(Test_Arguments_Window_Mode) {
		int argc = 3;
		char *argv[3] = {(char*)"test", (char*)"-m", (char*)"preserve"};
		qemucsd::arguments::options opts;
		qemucsd::arguments::parse_args(argc, argv, &opts);

		BOOST_CHECK(
			opts.dev_init_mode == qemucsd::arguments::DEV_INIT_PRESERVE);
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

		BOOST_CHECK(reference.compare(opts.spdk.name) == 0);
	}

BOOST_AUTO_TEST_SUITE_END()