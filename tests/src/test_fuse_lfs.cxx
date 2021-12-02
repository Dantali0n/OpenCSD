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

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "tests.hpp"

#include "fuse_lfs.hpp"

BOOST_AUTO_TEST_SUITE(Test_FuseLfs)

    using qemucsd::fuse_lfs::path_node_t;
    using qemucsd::fuse_lfs::FuseLFS;

    class TestFuseLFS : public FuseLFS {
    public:
        using FuseLFS::nvme_info;

        using FuseLFS::path_inode_map;
        using FuseLFS::path_to_inode;

        using FuseLFS::lba_to_position;
        using FuseLFS::position_to_lba;

        using FuseLFS::inode_lba_map;
        using FuseLFS::inode_to_lba;
    };

    /**
     * If the entry does not exist in the map the inode should return 0
     */
    BOOST_AUTO_TEST_CASE(Test_FuseLFS_path_to_inode_not_found) {
        // Create file test as direct parent from root
        path_node_t test = std::make_pair(1, "test");
        // Insert file test with inode 2
        TestFuseLFS::path_inode_map.insert(std::make_pair(test, 2));

        fuse_ino_t inode;
        TestFuseLFS::path_to_inode(1, "missing", inode);

        BOOST_CHECK(inode == 0);
    }

    /**
     * Find a file that is a direct parent of the root
     */
    BOOST_AUTO_TEST_CASE(Test_FuseLFS_path_to_inode_single) {
        // Create file test as direct parent from root
        path_node_t test = std::make_pair(1, "test");
        // Insert file test with inode 2
        TestFuseLFS::path_inode_map.insert(std::make_pair(test, 2));

        fuse_ino_t inode;
        TestFuseLFS::path_to_inode(1, "test", inode);

        BOOST_CHECK(inode == 2);
    }

    /**
     * Create a chain of inodes with each other as parent and the same name.
     * Ensure that having the same name does not influence results.
     */
    BOOST_AUTO_TEST_CASE(Test_FuseLFS_path_to_inode_chain) {

        // Create a change of files with each other as parent and the same name
        for(uint64_t i = 1; i < 6; i++) {
            // Create file test with parent i
            path_node_t test = std::make_pair(i, "test");
            // Insert file test with inode i+1
            TestFuseLFS::path_inode_map.insert(std::make_pair(test, i + 1));
        }

        fuse_ino_t inode;
        TestFuseLFS::path_to_inode(5, "test", inode);

        BOOST_CHECK(inode == 6);
    }

    /**
     * Create two chains of inodes and find the leaf inode on both paths
     */
    BOOST_AUTO_TEST_CASE(Test_FuseLFS_path_to_inode_divergence) {

        path_node_t test1 = std::make_pair(1, "test1");
        path_node_t test2 = std::make_pair(1, "test2");
        TestFuseLFS::path_inode_map.insert(std::make_pair(test1, 2));
        TestFuseLFS::path_inode_map.insert(std::make_pair(test2, 3));

        test1 = std::make_pair(2, "test1");
        test2 = std::make_pair(3, "test2");
        TestFuseLFS::path_inode_map.insert(std::make_pair(test1, 4));
        TestFuseLFS::path_inode_map.insert(std::make_pair(test2, 5));

        fuse_ino_t inode;
        TestFuseLFS::path_to_inode(3, "test2", inode);
        BOOST_CHECK(inode == 5);

        TestFuseLFS::path_to_inode(2, "test1", inode);
        BOOST_CHECK(inode == 4);
    }

    /**
     * Iterate over an absolute path from the root node
     */
    BOOST_AUTO_TEST_CASE(Test_FuseLFS_path_to_inode_delim) {

        // Create the root node entry that is needed to delimit paths that start
        // with /.
        path_node_t root = std::make_pair(0, "");
        TestFuseLFS::path_inode_map.insert(std::make_pair(root, 1));

        // Create a change of files with each other as parent and the same name
        for(uint64_t i = 1; i < 6; i++) {
            // Create file test with parent i
            path_node_t test = std::make_pair(i, "test");
            // Insert file test with inode i+1
            TestFuseLFS::path_inode_map.insert(std::make_pair(test, i + 1));
        }

        fuse_ino_t inode;
        TestFuseLFS::path_to_inode(0, "/test/test/test/test/test", inode);

        BOOST_CHECK(inode == 6);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_lba_to_position) {
        struct qemucsd::fuse_lfs::data_position max_pos =
            {4, 8, 0, 0};

        TestFuseLFS::nvme_info.num_zones = max_pos.zone;
        TestFuseLFS::nvme_info.zone_size = max_pos.sector;

        struct qemucsd::fuse_lfs::data_position result;
        for(uint64_t i = 0; i < max_pos.zone; i++) {
            for(uint64_t j = 0; j < max_pos.sector; j++) {
                TestFuseLFS::lba_to_position(i * max_pos.sector + j, result);
                BOOST_CHECK(result.zone == i);
                BOOST_CHECK(result.sector == j);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_position_to_lba) {
        struct qemucsd::fuse_lfs::data_position max_pos =
                {4, 8, 0, 0};

        TestFuseLFS::nvme_info.num_zones = max_pos.zone;
        TestFuseLFS::nvme_info.zone_size = max_pos.sector;

        uint64_t result = 0;
        struct qemucsd::fuse_lfs::data_position pos =
            {0, 0, 0, 0};
        for(uint64_t i = 0; i < max_pos.zone; i++) {
            pos.zone = i;
            for (uint64_t j = 0; j < max_pos.sector; j++) {
                uint64_t t_res = result;
                pos.sector = j;
                TestFuseLFS::position_to_lba(pos, result);

                if(i != 0 && j != 0)
                    BOOST_CHECK_MESSAGE(result == t_res + 1,
                    "res " << result << " t_res " << t_res + 1);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_inode_to_lba) {
        uint64_t lba;

        TestFuseLFS::inode_lba_map.insert(std::make_pair(1, 1));
        TestFuseLFS::inode_to_lba(1, lba);

        BOOST_CHECK(lba == 1);

        TestFuseLFS::inode_lba_map.insert_or_assign(1, 2);
        TestFuseLFS::inode_to_lba(1, lba);

        BOOST_CHECK(lba == 2);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_super_block) {

    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_dirty_block) {

    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_checkpoint_block) {

    }

BOOST_AUTO_TEST_SUITE_END()
