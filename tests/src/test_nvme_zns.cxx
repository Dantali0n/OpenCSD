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
#define BOOST_TEST_MODULE TestNvmeZns

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "tests.hpp"

#include "nvme_zns.hpp"
#include "nvme_zns_memory.hpp"

using qemucsd::nvme_zns::NvmeZns;
using qemucsd::nvme_zns::NvmeZnsMemoryBackend;

BOOST_AUTO_TEST_SUITE(Test_NvmeZns)

    BOOST_AUTO_TEST_CASE(Test_NvmeZns_Defaults) {
        uint32_t num_zones = 10;
        uint32_t zone_size = 16;
        uint32_t sector_size = 512;
        NvmeZnsMemoryBackend backend(10,  16, 512);
        NvmeZns<NvmeZnsMemoryBackend> frontend(&backend);
    }

BOOST_AUTO_TEST_SUITE_END()
