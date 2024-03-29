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
#define BOOST_TEST_MODULE TestFuseLfs

#include <boost/test/unit_test.hpp>

#include "tests.hpp"

#include "flfs.hpp"


BOOST_AUTO_TEST_SUITE(Test_FuseLfs)

    using qemucsd::fuse_lfs::FuseLFS;

    class DummyBackend : public qemucsd::nvme_zns::NvmeZnsBackend {
    public:

        explicit DummyBackend(qemucsd::nvme_zns::nvme_zns_info* info) :
            qemucsd::nvme_zns::NvmeZnsBackend(info)
        {

        }

        void get_nvme_zns_info(struct qemucsd::nvme_zns::nvme_zns_info* info) {

        }

        int read(uint64_t zone, uint64_t sector, uint64_t offset, void *buffer,
                 uint64_t size)
        {
            return 0;
        }

        int append(uint64_t zone, uint64_t &sector, uint64_t offset,
                   void *buffer, uint64_t size)
        {
            return 0;
        }

        int reset(uint64_t zone) {
            return 0;
        }
    };

    static qemucsd::arguments::options opts = {};

    class TestFuseLFS : public FuseLFS {
    public:
        TestFuseLFS() : FuseLFS(&opts, nullptr) {
        }

        using FuseLFS::nvme;
        using FuseLFS::nvme_info;

        using FuseLFS::path_inode_map;

        using FuseLFS::lba_to_position;
        using FuseLFS::position_to_lba;

        using FuseLFS::inode_lba_map;
    };

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_lba_to_position) {
        struct qemucsd::fuse_lfs::data_position max_pos =
            {4, 8, 0, 0};

        TestFuseLFS testfuse;

        testfuse.nvme_info.num_zones = max_pos.zone;
        testfuse.nvme_info.zone_size = max_pos.sector;

        struct qemucsd::fuse_lfs::data_position result;
        for(uint64_t i = 0; i < max_pos.zone; i++) {
            for(uint64_t j = 0; j < max_pos.sector; j++) {
                testfuse.lba_to_position(i * max_pos.sector + j, result);
                BOOST_CHECK(result.zone == i);
                BOOST_CHECK(result.sector == j);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_position_to_lba) {
        struct qemucsd::fuse_lfs::data_position max_pos =
            {4, 8, 0, 0};

        TestFuseLFS testfuse;

        testfuse.nvme_info.num_zones = max_pos.zone;
        testfuse.nvme_info.zone_size = max_pos.sector;
        auto backend = DummyBackend(&testfuse.nvme_info);
        testfuse.nvme = &backend;

        uint64_t result = 0;
        struct qemucsd::fuse_lfs::data_position pos =
            {0, 0, 0, 0};
        for(uint64_t i = 0; i < max_pos.zone; i++) {
            pos.zone = i;
            for (uint64_t j = 0; j < max_pos.sector; j++) {
                uint64_t t_res = result;
                pos.sector = j;
                testfuse.position_to_lba(pos, result);

                if(i != 0 && j != 0)
                    BOOST_CHECK_MESSAGE(result == t_res + 1,
                    "res " << result << " t_res " << t_res + 1);
            }
        }
    }

BOOST_AUTO_TEST_SUITE_END()
