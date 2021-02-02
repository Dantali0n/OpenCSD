#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestArguments

#include <boost/test/unit_test.hpp>

#include "tests.hpp"

#include "arguments.hpp"

BOOST_AUTO_TEST_SUITE(Test_Arguments)

	BOOST_AUTO_TEST_CASE(Test_Arguments_Window_Mode) {
		int argc = 3;
		char *argv[3] = {(char*)"test", (char*)"-o",  (char*)"fullscreen"};
		qemucsd::arguments::options opts;
		qemucsd::arguments::parse_args(argc, argv, &opts);

		BOOST_CHECK(
			opts.window_mode == qemucsd::arguments::WINDOW_FULLSCREEN);
	}

	BOOST_AUTO_TEST_CASE(Test_Arguments_Settings) {
		int argc = 3;
		char *argv[3] = {(char*)"test", (char*)"-s",  (char*)"test.xml"};
		qemucsd::arguments::options opts;
		qemucsd::arguments::parse_args(argc, argv, &opts);

		BOOST_CHECK(
			opts.settings->compare("test.xml") == 0);
	}

BOOST_AUTO_TEST_SUITE_END()