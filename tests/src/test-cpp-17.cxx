#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestDFTSeq

#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <set>
#include <string>
#include <tuple>

#include "tests.hpp"

/**
 * Reference: https://www.codingame.com/playgrounds/2205/7-features-of-c17-that-will-simplify-your-code/structured-bindings
 */
struct S {
	int n;
	std::string s;
	float d;
	bool operator<(const S& rhs) const {
		return std::tie(n, s, d) < std::tie(rhs.n, rhs.s, rhs.d);
	}
};

BOOST_AUTO_TEST_SUITE(Test_CPP17)

	BOOST_AUTO_TEST_CASE(Test_CPP17_Structured_Binding) {

		std::set<S> mySet;
		S value{42, "Test", 3.14};
		auto [iter, inserted] = mySet.insert(value);

		BOOST_CHECK(inserted == true);

		double myArray[3] = {1.0, 2.0, 3.0};
		auto [a, b, c] = myArray;

		double myPair[2] = {1.0, 2.0};
		auto [d, e] = myPair;

		const std::map<int, int> myMap {
			{1, 2},
			{3, 4},
			{5, 6}
		};

		int count = 0;
		for (const auto &[k,v] : myMap)
		{
			count++;
			BOOST_CHECK(v == k+1);
		}

		BOOST_CHECK(count == 3);
	}

	BOOST_AUTO_TEST_CASE(Test_CPP17_If_Switch_init) {
		if (auto val = 1; true) {
			BOOST_CHECK(val == 1);
		}
	}

BOOST_AUTO_TEST_SUITE_END()