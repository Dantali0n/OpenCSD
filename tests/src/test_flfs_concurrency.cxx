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
#define BOOST_TEST_MODULE TestFuseLfsConcurrency

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <thread>

#include "tests.hpp"

#include "flfs.hpp"
#include "nvme_zns_memory.hpp"

/**
 * BACKGROUND INFO:
 *  these tests assert that datastructures remain consistent with concurrent
 *  operations
*/

BOOST_AUTO_TEST_SUITE(Test_FuseLfsConcurreny)

    using qemucsd::fuse_lfs::FuseLFS;
    static qemucsd::arguments::options opts = {};

    class TestFuseLFS : public FuseLFS {
    public:
        explicit TestFuseLFS(qemucsd::nvme_zns::NvmeZnsBackend* nvme) :
            FuseLFS(&opts, nvme) {

        }

        using FuseLFS::inode_nlookup_map;

        using FuseLFS::inode_nlookup_increment;
        using FuseLFS::inode_nlookup_decrement;
    };

    struct qemucsd::fuse_lfs::data_position NULL_POS = {0};

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_nlookup_insert,
        * boost::unit_test::timeout(5))
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
                1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);

        std::thread thread1(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);
        std::thread thread2(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);
        std::thread thread3(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);
        std::thread thread4(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);

        thread1.join();
        thread2.join();
        thread3.join();
        thread4.join();

        BOOST_CHECK(test_fuse.inode_nlookup_map.at(1) == 4);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_nlookup_insert_multi,
        * boost::unit_test::timeout(5))
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
                1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);

        std::thread thread1(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);
        std::thread thread2(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 2);
        std::thread thread3(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 2);
        std::thread thread4(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);

        thread1.join();
        thread2.join();
        thread3.join();
        thread4.join();

        BOOST_CHECK(test_fuse.inode_nlookup_map.at(1) == 2);
        BOOST_CHECK(test_fuse.inode_nlookup_map.at(2) == 2);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_nlookup_updown,
                         * boost::unit_test::timeout(5))
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
                1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);

        test_fuse.inode_nlookup_map.insert(std::make_pair(1, 2));

        std::thread thread1(
            &TestFuseLFS::inode_nlookup_decrement, &test_fuse, 1, 2);
        std::thread thread2(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);
        std::thread thread3(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);

        thread1.join();
        thread2.join();
        thread3.join();

        BOOST_CHECK(test_fuse.inode_nlookup_map.at(1) == 2);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_nlookup_over_decrease,
                         * boost::unit_test::timeout(5))
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
                1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);

        std::thread thread1(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);
        std::thread thread2(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);
        std::thread thread3(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);
        std::thread thread4(
            &TestFuseLFS::inode_nlookup_increment, &test_fuse, 1);

        thread1.join();
        thread2.join();
        thread3.join();
        thread4.join();

        std::thread thread5(
            &TestFuseLFS::inode_nlookup_decrement, &test_fuse, 1, 5);

        thread5.join();
    }

BOOST_AUTO_TEST_SUITE_END()