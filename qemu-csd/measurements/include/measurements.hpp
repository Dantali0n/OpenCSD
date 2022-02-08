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

#ifndef QEMU_CSD_MEASUREMENTS_HPP
#define QEMU_CSD_MEASUREMENTS_HPP

#include <xenium/ramalhete_queue.hpp>
#include <xenium/michael_scott_queue.hpp>
#include <xenium/reclamation/generic_epoch_based.hpp>

#include <atomic>
#include <chrono>
#include <vector>
#include <string>

#include "output.hpp"

namespace qemucsd::measurements {

    static output::Output output = output::Output(
        "[MEASUREMENTS] ",
        #ifdef QEMUCSD_DEBUG
            output::DEBUG
        #else
            output::INFO
        #endif
    );

    /**
     * Insert into the lockless queue so results processing can deal with it.
     */
    struct measurement {
        bool stop;
        size_t identifier;
        size_t marker;
        std::chrono::system_clock::time_point time;
    };

    /**
     * Temporary datastructure to deal with results being processed.
     */
    struct result {
        size_t count;
        size_t lowest;
        size_t highest;
        size_t total;
    };

    extern std::atomic<size_t> marker_count;
    extern std::vector<std::string> namespaces;
    extern xenium::michael_scott_queue<
        std::unique_ptr<measurement>,
        xenium::policy::reclaimer<xenium::reclamation::new_epoch_based<>>
    > queue;

    /**
     * Register a namespace that can be used for timing measurements, use the
     * retrieved identifier for associating the specified name to the
     * measurement.
     * @threadsafety: Single threaded
     * @return 0 upon success, -1 if the namespace is already registed.
     */
    int register_namespace(const char* name, size_t &identifier);

    /**
     * Reset all datastructures
     * @threadsafety: Single threaded
     */
    void reset();

    /**
     * Take a measurement for the namespace as identified by the identifier.
     * This returns a marker to be used in stop_measurement.
     * @threadsafety: Thread safe
     */
    void start_measurement(size_t identifier, size_t &marker);

    /**
     * Indicate the stop of a measurement using the supplied marker and the
     * namespace identifier.
     * @threadsafety: Thread safe
     */
    void stop_measurement(size_t identifier, size_t marker);

    /**
     * Generate the results and fill the data into the results argument
     * @threadsafety: Single threaded
     */
    void generate_results(std::vector<result> *results);

    class measure_guard {
    protected:
        size_t _marker;
        size_t _identifier;
    public:
        explicit measure_guard(size_t identifier) : _identifier(identifier) {
            start_measurement(identifier, _marker);
        }
        virtual ~measure_guard() {
            stop_measurement(_identifier, _marker);
        }
    };
}

#endif // QEMU_CSD_MEASUREMENTS_HPP
