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
		char *argv[3] = {(char*)"test", (char*)"-s", (char*)"test.xml"};
		qemucsd::arguments::options opts;
		qemucsd::arguments::parse_args(argc, argv, &opts);

		BOOST_CHECK(
			opts.settings->compare("test.xml") == 0);
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