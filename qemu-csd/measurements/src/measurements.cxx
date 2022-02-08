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

#include "measurements.hpp"

namespace qemucsd::measurements {

    std::atomic<size_t> marker_count = 0;
    std::vector<std::string> namespaces = std::vector<std::string>();

//    static xenium::ramalhete_queue<
//        std::unique_ptr<measurement>,
//        xenium::policy::reclaimer<xenium::reclamation::new_epoch_based<>>,
//        xenium::policy::entries_per_node<2048>
//    > queue;

    xenium::michael_scott_queue<
        std::unique_ptr<measurement>,
        xenium::policy::reclaimer<xenium::reclamation::new_epoch_based<>>
    > queue;

    int register_namespace(const char* name, size_t &identifier)  {
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

    void reset() {
        namespaces.clear();
        marker_count = 0;
        std::unique_ptr<measurement> measure;
        while(queue.try_pop(measure)) {

        }
    }

    void start_measurement(size_t identifier, size_t &marker) {
        marker = marker_count.fetch_add(1, std::memory_order_relaxed);
        measurement measure = {
                false, identifier, marker, std::chrono::high_resolution_clock::now()
        };
        queue.push(std::make_unique<measurement>(measure));
    }

    void stop_measurement(size_t identifier, size_t marker) {
        measurement measure = {
                true, identifier, marker, std::chrono::high_resolution_clock::now()
        };
        queue.push(std::make_unique<measurement>(measure));
    }

    void generate_results(std::vector<result> *results) {
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
            if(result.total == 0 || result.count == 0) continue;
            if(i >= namespaces.size())
                output.info("[", i, "] count ", result.count, " median ",
                    result.total / result.count / 1.e9, " lowest ",
                    result.lowest / 1.e9, " highest ", result.highest / 1.e9);
            else
                output.info("[", namespaces.at(i), "] count ", result.count,
                    " median ", result.total / result.count / 1.e9, " lowest ",
                    result.lowest / 1.e9, " highest ", result.highest / 1.e9);
        }
    }

}