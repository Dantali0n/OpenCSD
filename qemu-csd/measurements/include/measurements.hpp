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
     * Insert into the lockless queue so the results thread can deal with it
     */
    struct measurement {
        bool stop;
        size_t identifier;
        size_t marker;
        std::chrono::system_clock::time_point time;
    };

    struct result {
        size_t count;
        size_t lowest;
        size_t highest;
        size_t total;
    };

    static std::atomic<size_t> marker_count = 0;
    static std::vector<std::string> namespaces = std::vector<std::string>();

//    static xenium::ramalhete_queue<
//        std::unique_ptr<measurement>,
//        xenium::policy::reclaimer<xenium::reclamation::new_epoch_based<>>,
//        xenium::policy::entries_per_node<2048>
//    > queue;

    static xenium::michael_scott_queue<
        std::unique_ptr<measurement>,
        xenium::policy::reclaimer<xenium::reclamation::new_epoch_based<>>
    > queue;


    /**
     *
     * @threadsafety: Single threaded
     */
    static int register_namespace(const char* name, size_t &identifier) {
        auto it = std::find_if(namespaces.begin(), namespaces.end(),
           [&name](const auto& x) {
               return x == name;
           }
        );
        if(it != namespaces.end()) return -1;

        namespaces.emplace_back(name);

        identifier = namespaces.size() - 1;
        return 0;
    }

    /**
     *
     * @threadsafety: Single threaded
     */
    static void reset() {
        namespaces.clear();
        marker_count = 0;
    }

    /**
     *
     * @threadsafety: Thread safe
     */
    static void start_measurement(size_t identifier, size_t &marker) {
        marker = marker_count.fetch_add(1, std::memory_order_relaxed);
        measurement measure = {
            false, identifier, marker, std::chrono::high_resolution_clock::now()
        };
        queue.push(std::make_unique<measurement>(measure));
    }

    /**
     *
     * @threadsafety: Thread safe
     */
    static void stop_measurement(size_t identifier, size_t marker) {
        measurement measure = {
            true, identifier, marker, std::chrono::high_resolution_clock::now()
        };
        queue.push(std::make_unique<measurement>(measure));
    }

    /**
     *
     * @threadsafety: Single threaded
     */
    static void generate_results(std::vector<result> *results) {
        results->resize(namespaces.size());

        for(auto &result : *results) {
            result.lowest = SIZE_MAX;
        }

        std::vector<measurement> temp_starts;
        std::vector<measurement> temp_stops;

        std::unique_ptr<qemucsd::measurements::measurement> measure;
        while(queue.try_pop(measure)) {
            if(!measure->stop) temp_starts.push_back(*measure);
            else temp_stops.push_back(*measure);
        }

        for(auto &starts : temp_starts) {
            auto it = std::find_if(temp_stops.begin(), temp_stops.end(),
               [&starts](const auto& x) {
                   return x.marker == starts.marker;
               }
            );
            if(it == temp_stops.end()) {
                output.error("Missing stop for measurement with identifier",
                    starts.identifier);
                continue;
            }

            auto result = results->at(starts.identifier);
            result.count += 1;
            auto nanoseconds = std::chrono::duration_cast<
                std::chrono::nanoseconds>(it->time - starts.time).count();
            if(nanoseconds < result.lowest)
                result.lowest = nanoseconds;
            if(nanoseconds > result.highest)
                result.highest = nanoseconds;
            result.total += nanoseconds;

            results->at(starts.identifier) = result;

            std::destroy(it, it);
        }

        for(size_t i = 0; i < results->size(); i++) {
            auto result = results->at(i);
            if(i >= namespaces.size())
                output.info("[", i, "] count:", result.count, " median ",
                    result.total / result.count / 1.e9, " lowest ",
                    result.lowest / 1.e9, " highest ", result.highest / 1.e9);
            else
                output.info("[", namespaces.at(i), "] count:", result.count,
                    " median ", result.total / result.count / 1.e9, " lowest ",
                    result.lowest / 1.e9, " highest ", result.highest / 1.e9);
        }
    }
}

#endif // QEMU_CSD_MEASUREMENTS_HPP
