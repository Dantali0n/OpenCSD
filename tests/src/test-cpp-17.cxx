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