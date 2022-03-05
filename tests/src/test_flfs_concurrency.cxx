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
#include <future>

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
    protected:
        static void free_data_structure_heap_memory(TestFuseLFS *test_fuse) {
            for(auto &entry : *test_fuse->path_inode_map) {
                delete entry.second;
            }

            for(auto &entry : *test_fuse->data_blocks) {
                delete entry.second;
            }
        }
    public:
        explicit TestFuseLFS(qemucsd::nvme_zns::NvmeZnsBackend* nvme) :
            FuseLFS(&opts, nvme) {

        }

        virtual ~TestFuseLFS() {
            free_data_structure_heap_memory(this);
        }

        using FuseLFS::run_init;

        using FuseLFS::inode_nlookup_map;
        using FuseLFS::inode_nlookup_increment;
        using FuseLFS::inode_nlookup_decrement;

        using FuseLFS::open_inode_vect;
        using FuseLFS::create_file_handle;
        using FuseLFS::find_file_handle_unsafe;
    };

    struct qemucsd::fuse_lfs::data_position NULL_POS = {0};

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_nlookup_insert,
        * boost::unit_test::timeout(5))
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            16, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
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
            16, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
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
            16, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
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
            16, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
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

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_file_handle)
//        * boost::unit_test::timeout(5))
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            16, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        BOOST_CHECK(test_fuse.run_init() == qemucsd::fuse_lfs::FLFS_RET_NONE);

        qemucsd::fuse_lfs::csd_unique_t ctx = std::make_pair(1, 23543);
        struct fuse_file_info fi1;
        struct fuse_file_info fi2;
        struct fuse_file_info fi3;

        std::thread thread1(&TestFuseLFS::create_file_handle, &test_fuse, &ctx,
            &fi1);
        std::thread thread2(&TestFuseLFS::create_file_handle, &test_fuse, &ctx,
            &fi2);
        std::thread thread3(&TestFuseLFS::create_file_handle, &test_fuse, &ctx,
            &fi3);

        thread1.join();
        thread2.join();
        thread3.join();

        BOOST_CHECK(1 == test_fuse.open_inode_vect.at(0).ino);
        BOOST_CHECK(23543 == test_fuse.open_inode_vect.at(0).pid);

        BOOST_CHECK(test_fuse.open_inode_vect.at(0).fh !=
            test_fuse.open_inode_vect.at(1).fh);
        BOOST_CHECK(test_fuse.open_inode_vect.at(1).fh !=
            test_fuse.open_inode_vect.at(2).fh);
        BOOST_CHECK(test_fuse.open_inode_vect.at(2).fh !=
            test_fuse.open_inode_vect.at(0).fh);

        uint64_t fh_st1 = test_fuse.open_inode_vect.at(0).fh;
        uint64_t fh_st2 = test_fuse.open_inode_vect.at(1).fh;
        uint64_t fh_st3 = test_fuse.open_inode_vect.at(2).fh;

        std::thread thread4(&TestFuseLFS::release_file_handle, &test_fuse,
            fh_st1);
        std::thread thread5(&TestFuseLFS::release_file_handle, &test_fuse,
            fh_st2);

        struct qemucsd::fuse_lfs::open_file_entry fh = {};
        BOOST_CHECK(test_fuse.get_file_handle(fh_st3, &fh) ==
            qemucsd::fuse_lfs::FLFS_RET_NONE);

        BOOST_CHECK(test_fuse.get_file_handle(&ctx, &fh) ==
            qemucsd::fuse_lfs::FLFS_RET_NONE);

        thread4.join();
        thread5.join();

        struct qemucsd::fuse_lfs::open_file_entry fh2 = fh;
        fh.read_stream_kernel = 6;
        fh2.read_stream_kernel = 12;

        // std::thread thread6(&TestFuseLFS::update_file_handle, &test_fuse,
        //     fh_st3, &fh);
        // std::thread thread7(&TestFuseLFS::update_file_handle, &test_fuse,
        //     fh_st3, &fh2);

        // thread6.join();
        // thread7.join();

        /** TODO(Dantali0n): Fix race condition that makes this fail */
        struct qemucsd::fuse_lfs::open_file_entry fh3;
        int result = test_fuse.get_file_handle(fh_st3, &fh3);
        BOOST_CHECK(result == qemucsd::fuse_lfs::FLFS_RET_NONE);

        if(result != qemucsd::fuse_lfs::FLFS_RET_NONE) {
            for(auto &entry : test_fuse.open_inode_vect) {
                std::cout << "fh " << entry.fh << " inode " << entry.ino
                    << " pid " << entry.pid;
            }
        }

        BOOST_CHECK(fh.read_stream_kernel == 6 || fh.read_stream_kernel == 12);

        /**
         * https://en.cppreference.com/w/cpp/language/overloaded_address
         * https://en.cppreference.com/w/cpp/thread/packaged_task
         * https://en.cppreference.com/w/cpp/utility/functional/bind
         *
         */
        int (TestFuseLFS::*func1)(uint64_t) = &TestFuseLFS::find_file_handle;
        auto fb_find_uint = std::bind(func1, &test_fuse, fh_st3);
        std::packaged_task<int(uint64_t)> task1(fb_find_uint);

        int (TestFuseLFS::*func2)(qemucsd::fuse_lfs::csd_unique_t*) =
            &TestFuseLFS::find_file_handle;
        auto fb_find_csd = std::bind(func2, &test_fuse, &ctx);
        std::packaged_task<int(qemucsd::fuse_lfs::csd_unique_t*)>
            task2(fb_find_csd);

        // This has to be done before std::move
        auto future1 = task1.get_future();
        auto future2 = task2.get_future();

        std::thread thread8(std::move(task1), fh_st3);
        std::thread thread9(std::move(task2), &ctx);

        thread8.join();
        thread9.join();

        /** TODO(Dantali0n): Fix race condition that makes this fail */
        BOOST_CHECK(future1.get() == 1);
        BOOST_CHECK(future2.get() == 1);
    }

BOOST_AUTO_TEST_SUITE_END()