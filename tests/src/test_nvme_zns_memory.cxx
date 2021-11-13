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

#include "nvme_zns_memory.hpp"

using qemucsd::nvme_zns::NvmeZnsMemoryBackend;

BOOST_AUTO_TEST_SUITE(Test_NvmeZnsMemoryBackend)

    BOOST_AUTO_TEST_CASE(Test_NvmeZnsMemoryBackend_Defaults) {
        uint32_t num_zones = 10;
        uint32_t zone_size = 16;
        uint32_t sector_size = 512;
        NvmeZnsMemoryBackend backend(10,  16, 512);

        struct qemucsd::nvme_zns::nvme_zns_info info;
        backend.get_nvme_zns_info(&info);

        BOOST_CHECK(num_zones == info.num_zones);
        BOOST_CHECK(zone_size == info.zone_size);
        BOOST_CHECK(sector_size == info.sector_size);
    }

    BOOST_AUTO_TEST_CASE(Test_NvmeZnsMemoryBackend_read) {
        constexpr uint32_t sector_size = 512;
        constexpr uint32_t buffer_size = sector_size + 2;

        NvmeZnsMemoryBackend backend(10,  16, sector_size);

        // Reset the zone so it is zeroed
        backend.reset(0);

        // Create buffer with pre and post fence space
        unsigned char buffer[buffer_size];

        // Set the entire buffer to non zero
        for(uint32_t i = 0; i < buffer_size; i++) {
            buffer[i] = 0xff;
        }

        // Parse buffer to function past the pre fence
        backend.read(0, 0, 0, buffer + 1, sector_size);

        // check contents of buffer is zeroed
        for(uint32_t i = 1; i < sector_size+1; i++) {
            BOOST_CHECK(buffer[i] == 0);
        }

        // Check pre and post fences are untouched
        BOOST_CHECK(buffer[0] == 0xff);
        BOOST_CHECK(buffer[sector_size+1] == 0xff);
    }

    BOOST_AUTO_TEST_CASE(Test_NvmeZnsMemoryBackend_read_limit) {
        constexpr uint32_t sector_size = 512;

        NvmeZnsMemoryBackend backend(10,  16, sector_size);

        // Reset the zone so it is zeroed
        backend.reset(0);

        unsigned char buffer[sector_size];

        // Read past last zone
        BOOST_CHECK(backend.read(10, 0, 0, buffer, sector_size) == -1);

        // Read past last sector
        BOOST_CHECK(backend.read(9, 16, 0, buffer, sector_size) == -1);

        // Read past last single byte offset
        BOOST_CHECK(backend.read(9, 15, 1, buffer, sector_size) == -1);

        // Read beyond number of sectors
        BOOST_CHECK(backend.read(0, 17, 0, buffer, sector_size) == -1);

        // Read beyond sector
        BOOST_CHECK(backend.read(0, 0, 513, buffer, sector_size) == -1);
    }

    BOOST_AUTO_TEST_CASE(Test_NvmeZnsMemoryBackend_append_read) {
        constexpr uint32_t sector_size = 512;

        NvmeZnsMemoryBackend backend(10,  16, sector_size);

        // Reset the zone so it is zeroed
        backend.reset(0);

        unsigned char buffer[sector_size];

        uint64_t sector = 0;
        for(uint32_t i = 0; i < 16; i++) {
            BOOST_CHECK(sector == i);
            BOOST_CHECK(backend.append(0, sector, 0, buffer, sector_size) == 0);
            BOOST_CHECK(backend.read(0, 0, 0, buffer, sector_size) == 0);
        }

        // Try to append beyond the size of the zone
        BOOST_CHECK(backend.append(0, sector, 0, buffer, sector_size) == -1);
    }

    BOOST_AUTO_TEST_CASE(Test_NvmeZnsMemoryBackend_write_reset) {
        constexpr uint32_t sector_size = 512;

        NvmeZnsMemoryBackend backend(10,  16, sector_size);

        // Reset the zones so they are zeroed
        backend.reset(0);

        unsigned char buffer[sector_size];

        for(uint32_t i = 0; i < sector_size; i++) {
            buffer[i] = i % UINT8_MAX;
        }

        size_t sector;
        for(uint32_t i = 0; i < 16; i++) {
            backend.append(0, sector, 0, buffer, sector_size);
        }
        BOOST_CHECK(sector == 16);

        // should not affect data in zone 0
        backend.reset(1);

        unsigned char result_buffer[sector_size];

        backend.read(0, 15, 0, result_buffer, sector_size);

        for(uint32_t i = 0; i < sector_size; i++) {
            BOOST_CHECK(result_buffer[i] == buffer[i]);
        }

        backend.append(1, sector, 0, buffer, sector_size);

        // should not affect data in zone 1
        backend.reset(0);

        memset(result_buffer, 0, sector_size);
        backend.read(1, 0, 0, result_buffer, sector_size);

        for(uint32_t i = 0; i < sector_size; i++) {
            BOOST_CHECK(result_buffer[i] == buffer[i]);
        }
    }

BOOST_AUTO_TEST_SUITE_END()
