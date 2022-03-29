/**
 * MIT License
 *
 * Copyright (c) 2022 Dantali0n
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
#define BOOST_TEST_MODULE TestMeasurements

#include <boost/test/unit_test.hpp>

#include <thread>
#include <future>

#include "tests.hpp"

#include "measurements.hpp"

/**
 * BACKGROUND INFO:
 *
*/

BOOST_AUTO_TEST_SUITE(Test_Measurements)

    void measure_start(size_t identifier) {
        size_t marker;
        qemucsd::measurements::start_measurement(identifier, marker);
    }

    void measure(size_t identifier, size_t time) {
        size_t marker;
        qemucsd::measurements::start_measurement(identifier, marker);
        std::this_thread::sleep_for(std::chrono::nanoseconds(time));
        qemucsd::measurements::stop_measurement(identifier, marker);
    }

    void __attribute__ ((noinline)) performance_raw(uint32_t num_measures) {
        for(uint32_t k = 0; k < num_measures; k++) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
//            for(uint16_t i = 0; i < UINT16_MAX - 1; i++) {
//                __asm__ volatile("" : "+g" (i) : :);
//            }
        }
    }

    void __attribute__ ((noinline)) performance_instrumented(
        uint32_t num_measures, size_t identifier)
    {
        size_t marker;
        for(uint32_t k = 0; k < num_measures; k++) {
            qemucsd::measurements::start_measurement(identifier, marker);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
//            for(uint16_t i = 0; i < UINT16_MAX-1; i++) {
//                __asm__ volatile("" : "+g" (i) : :);
//            }
            qemucsd::measurements::stop_measurement(identifier, marker);
        }
    }

    BOOST_AUTO_TEST_CASE(Test_Measurements_register,
        * boost::unit_test::timeout(5))
    {
        size_t identifier;
        BOOST_CHECK(qemucsd::measurements::register_namespace(
            "test1", identifier) == 0);
        BOOST_CHECK(identifier == 0);
        BOOST_CHECK(qemucsd::measurements::namespaces.at(identifier) ==
            "test1");

        BOOST_CHECK(qemucsd::measurements::register_namespace(
            "test2", identifier) == 0);
        BOOST_CHECK(identifier == 1);
        BOOST_CHECK(qemucsd::measurements::namespaces.at(identifier) ==
            "test2");

        BOOST_CHECK(qemucsd::measurements::register_namespace(
            "test2", identifier) == -1);

        qemucsd::measurements::reset();
    }

    BOOST_AUTO_TEST_CASE(Test_Measurements_start,
        * boost::unit_test::timeout(5))
    {
        size_t identifier, marker;
        BOOST_CHECK(qemucsd::measurements::register_namespace(
            "test1", identifier) == 0);

        qemucsd::measurements::start_measurement(identifier, marker);

        std::unique_ptr<qemucsd::measurements::measurement> measure;
        BOOST_CHECK(qemucsd::measurements::queue.try_pop(measure));

        qemucsd::measurements::reset();
    }

    BOOST_AUTO_TEST_CASE(Test_Measurements_push_concurrent,
         * boost::unit_test::timeout(5))
    {
        size_t identifier;
        BOOST_CHECK(qemucsd::measurements::register_namespace(
            "test1", identifier) == 0);

        std::thread thread1(measure_start, identifier);
        std::thread thread2(measure_start, identifier);
        std::thread thread3(measure_start, identifier);
        std::thread thread4(measure_start, identifier);

        thread1.join();
        thread2.join();
        thread3.join();
        thread4.join();

        std::unique_ptr<qemucsd::measurements::measurement> measure;
        for(uint8_t i = 0; i < 4; i++)
            BOOST_CHECK(qemucsd::measurements::queue.try_pop(measure));

        BOOST_CHECK(qemucsd::measurements::queue.try_pop(measure) == false);

        qemucsd::measurements::reset();
    }

    BOOST_AUTO_TEST_CASE(Test_Measurements_results_concurrent,
        * boost::unit_test::timeout(5))
    {
        size_t identifier;
        BOOST_CHECK(qemucsd::measurements::register_namespace(
            "test1", identifier) == 0);

        std::thread thread1(measure, identifier,  10000000);
        std::thread thread2(measure, identifier,  40000000);
        std::thread thread3(measure, identifier,  80000000);
        std::thread thread4(measure, identifier, 160000000);

        thread1.join();
        thread2.join();
        thread3.join();
        thread4.join();

        std::vector<qemucsd::measurements::result> results;
        qemucsd::measurements::generate_results(&results);

        BOOST_CHECK(results.size() == 1);
        BOOST_CHECK(results.at(0).count == 4);

        qemucsd::measurements::reset();
    }

    BOOST_AUTO_TEST_CASE(Test_Measurements_overhead,
        * boost::unit_test::timeout(30))
    {
        size_t identifier;
        BOOST_CHECK(qemucsd::measurements::register_namespace(
            "performance", identifier) == 0);

        static constexpr uint16_t NUM_MEASURES = 256;
        int64_t raw_total = 0;
        int64_t instrument_total = 0;

        // Raw performance measurement
        auto start = std::chrono::high_resolution_clock::now();
        std::thread thread1(performance_raw, NUM_MEASURES);
        std::thread thread2(performance_raw, NUM_MEASURES);
        std::thread thread3(performance_raw, NUM_MEASURES);
        std::thread thread4(performance_raw, NUM_MEASURES);

        thread1.join();
        thread2.join();
        thread3.join();
        thread4.join();
        auto stop = std::chrono::high_resolution_clock::now();
        auto nanoseconds = std::chrono::duration_cast<
            std::chrono::nanoseconds>(stop - start).count();
        raw_total = nanoseconds;

        // Performance instrumented
        start = std::chrono::high_resolution_clock::now();
        std::thread thread5(performance_instrumented, NUM_MEASURES, identifier);
        std::thread thread6(performance_instrumented, NUM_MEASURES, identifier);
        std::thread thread7(performance_instrumented, NUM_MEASURES, identifier);
        std::thread thread8(performance_instrumented, NUM_MEASURES, identifier);

        thread5.join();
        thread6.join();
        thread7.join();
        thread8.join();
        stop = std::chrono::high_resolution_clock::now();
        nanoseconds = std::chrono::duration_cast<
            std::chrono::nanoseconds>(stop - start).count();
        instrument_total = nanoseconds;

        std::cout << "Raw median " << raw_total / NUM_MEASURES / 1.e9 <<
            " seconds" << std::endl;
        std::cout << "Instrument median " << instrument_total / NUM_MEASURES /
            1.e9 << " seconds" << std::endl;
        std::cout << "Normalized overhead of single instrumentation " <<
            (instrument_total - raw_total) / NUM_MEASURES / 4 <<
            " nanoseconds" << std::endl;
    }

BOOST_AUTO_TEST_SUITE_END()