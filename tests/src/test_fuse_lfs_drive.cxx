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
#include "nvme_zns_memory.hpp"

/**
 * BACKGROUND INFO:
 *  these tests perform read, append and reset operations on a
 *  NvmeZnsMemoryBackend.
*/

BOOST_AUTO_TEST_SUITE(Test_FuseLfsDrive)

    using qemucsd::fuse_lfs::FuseLFS;

    class TestFuseLFS : public FuseLFS {
    public:
        using FuseLFS::nvme_info;

        using FuseLFS::nvme;

        using FuseLFS::inode_lba_map;

        using FuseLFS::cblock_pos;

        using FuseLFS::random_pos;
        using FuseLFS::random_ptr;

        using FuseLFS::lba_to_position;

        using FuseLFS::mkfs;

        using FuseLFS::verify_superblock;

        using FuseLFS::verify_dirtyblock;
        using FuseLFS::write_dirtyblock;
        using FuseLFS::remove_dirtyblock;

        using FuseLFS::update_checkpointblock;
        using FuseLFS::get_checkpointblock;
        using FuseLFS::get_checkpointblock_locate;

        using FuseLFS::determine_random_ptr;
        using FuseLFS::read_random_zone;
        using FuseLFS::append_random_block;
        using FuseLFS::rewrite_random_blocks;
    };

    struct qemucsd::fuse_lfs::data_position NULL_POS = {0};

    struct TestFuseLFSFixture {
        TestFuseLFSFixture() {
            TestFuseLFS::nvme = nullptr;

            TestFuseLFS::nvme_info.zone_capacity = 0;
            TestFuseLFS::nvme_info.zone_size = 0;
            TestFuseLFS::nvme_info.sector_size = 0;
            TestFuseLFS::nvme_info.num_zones = 0;
            TestFuseLFS::nvme_info.max_open = 0;

            TestFuseLFS::cblock_pos = NULL_POS;

            TestFuseLFS::random_pos = NULL_POS;
            TestFuseLFS::random_ptr = NULL_POS;

            TestFuseLFS::inode_lba_map.clear();
        }
    };

    /**
     * Early setup for rewrite_random_blocks tests
     */
    void setup_rewrite_random_blocks(
        qemucsd::nvme_zns::NvmeZnsMemoryBackend *nvme_memory)
    {
        TestFuseLFS::nvme = nvme_memory;
        nvme_memory->get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_POS;

        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(TestFuseLFS::random_ptr == TestFuseLFS::random_pos);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_mkfs, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_super_block, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::verify_superblock() == -1);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);
        BOOST_CHECK(TestFuseLFS::verify_superblock() == 0);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_dirty_block, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);
        BOOST_CHECK(TestFuseLFS::verify_dirtyblock() == 0);

        BOOST_CHECK(TestFuseLFS::write_dirtyblock() == 0);
        BOOST_CHECK(TestFuseLFS::verify_dirtyblock() == -1);

        BOOST_CHECK(TestFuseLFS::remove_dirtyblock() == 0);
        BOOST_CHECK(TestFuseLFS::verify_dirtyblock() == 0);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_checkpoint_block, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        BOOST_CHECK(TestFuseLFS::get_checkpointblock(cblock) == 0);

        BOOST_CHECK(cblock.randz_lba == TestFuseLFS::nvme_info.zone_size *
                    qemucsd::fuse_lfs::RANDZ_POS.zone);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_checkpoint_block_fill, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        uint64_t randz_base = TestFuseLFS::nvme_info.zone_size *
                qemucsd::fuse_lfs::RANDZ_POS.zone;

        uint64_t checkpoint_blocks_x4 = TestFuseLFS::nvme_info.zone_capacity * 8;

        uint32_t czones = qemucsd::fuse_lfs::RANDZ_POS.zone -
            qemucsd::fuse_lfs::CBLOCK_POS.zone;
        for(uint64_t i = 0; i < checkpoint_blocks_x4; i ++) {
            BOOST_CHECK(TestFuseLFS::update_checkpointblock(randz_base + i) == 0);
            BOOST_CHECK(TestFuseLFS::get_checkpointblock(cblock) == 0);
            BOOST_CHECK_MESSAGE(cblock.randz_lba == randz_base + i,
                "Incorrect randz_lba at checkpoint block update " << i);
        }
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_checkpoint_block_power_atomicity, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        uint64_t randz_base = TestFuseLFS::nvme_info.zone_size *
                              qemucsd::fuse_lfs::RANDZ_POS.zone;

        // mkfs takes first block so zone_capacity -1
        for(uint64_t i = 0; i < TestFuseLFS::nvme_info.zone_capacity - 1; i++) {
            BOOST_CHECK(TestFuseLFS::update_checkpointblock(randz_base + i) == 0);
            BOOST_CHECK(TestFuseLFS::get_checkpointblock_locate(cblock) == 0);
            BOOST_CHECK_MESSAGE(cblock.randz_lba == randz_base + i,
            "Incorrect randz_lba at checkpoint block update " << i);
        }

        uint64_t res_sector;
        cblock.randz_lba = 1337;
        BOOST_CHECK(TestFuseLFS::nvme->append(TestFuseLFS::cblock_pos.zone + 1,
                    res_sector, TestFuseLFS::cblock_pos.offset,
                    &cblock, TestFuseLFS::cblock_pos.size) == 0);
        BOOST_CHECK(res_sector == 0);

        BOOST_CHECK(TestFuseLFS::get_checkpointblock_locate(cblock) == 0);
        BOOST_CHECK(cblock.randz_lba == 1337);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_append_random_block, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_POS;
        TestFuseLFS::random_ptr = TestFuseLFS::random_pos;

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);

        BOOST_CHECK(TestFuseLFS::random_ptr.sector != 0);
    }


    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_append_random_block_verify_data, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_POS;
        TestFuseLFS::random_ptr = TestFuseLFS::random_pos;

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;
        for(uint32_t i = 0; i < qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM; i++) {
            nt_blk.inode[i] = i + 1;
        }

        BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);

        struct qemucsd::fuse_lfs::nat_block nt_blk_test = {0};
        TestFuseLFS::nvme->read(TestFuseLFS::random_ptr.zone, 0, 0, &nt_blk_test,
                                sizeof(qemucsd::fuse_lfs::nat_block));

        for(uint32_t i = 0; i < qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM; i++) {
            BOOST_CHECK(nt_blk_test.inode[i] == nt_blk_test.inode[i]);
        }
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_append_random_blocks_full, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_POS;
        TestFuseLFS::random_ptr = TestFuseLFS::random_pos;

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint32_t RANDOM_ZONE_ZONES = qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone - qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t RANDOM_ZONE_SECTORS = RANDOM_ZONE_ZONES * TestFuseLFS::nvme_info.zone_capacity;

        // Append all but last sector
        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS - 1; i++) {
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
        }

        // Last append must indicate to caller that random zone is full
        BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) ==
                    qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_append_random_blocks_illegal_pos, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_BUFF_POS;
        TestFuseLFS::random_ptr = qemucsd::fuse_lfs::RANDZ_POS;

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        #ifdef QEMUCSD_DEBUG
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == -1);
        #else
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
        #endif
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_base, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);
    }

    /**
     * Write the last sector of the first zone and check determine_random_ptr
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_zone_end, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        for(uint64_t i = 0; i < TestFuseLFS::nvme_info.zone_capacity-1; i++) {
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
        }

        auto temp_ptr = TestFuseLFS::random_ptr;
        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(temp_ptr == TestFuseLFS::random_ptr);
    }

    /**
     * Write half of the random zone and verify that the random_ptr ends up
     * at the right location.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_half, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint32_t RANDOM_ZONE_ZONES = qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone - qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t RANDOM_ZONE_SECTORS = RANDOM_ZONE_ZONES * TestFuseLFS::nvme_info.zone_capacity;
        uint64_t RANDOM_ZONE_SECTORS_HALF = RANDOM_ZONE_SECTORS / 2;
        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS_HALF; i++) {
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
        }

        auto temp_ptr = TestFuseLFS::random_ptr;
        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(temp_ptr == TestFuseLFS::random_ptr);
    }

    /**
     * Fill the random zone with invalid data. Check that the last append
     * returns FLFS_RET_RANDZ_FULL and invalidates the random_ptr. Next
     * rewrite the random zone and verify that random_pos and random_ptr are
     * equal, this indicates the zone is empty.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_full, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint32_t RANDOM_ZONE_ZONES = qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone - qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t RANDOM_ZONE_SECTORS = RANDOM_ZONE_ZONES * TestFuseLFS::nvme_info.zone_capacity;
        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS - 1; i++) {
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
        }

        // This last append should make the random zone full
        // This should invalidate the random_ptr
        BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) ==
                    qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);
        BOOST_CHECK(TestFuseLFS::random_ptr.valid() == false);

        // Rewrite the random zone
        BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() == 0);
        // random_pos and random_ptr should equal each other indicating the zone
        // is empty.
        BOOST_CHECK(TestFuseLFS::random_pos == TestFuseLFS::random_ptr);

        auto temp_ptr = TestFuseLFS::random_ptr;
        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(temp_ptr == TestFuseLFS::random_ptr);
    }

    /***
     * TODO(Dantali0n): Renable these methods when partial zone rewritting is
     *                  supported.
     */

//    BOOST_AUTO_TEST_CASE(Test_FuseLFS_rewrite_random_zone_two) {
//        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
//        setup_rewrite_random_blocks(&nvme_memory);
//
//        // These nat blocks are empty so upon rewriting the occupy 0 space
//        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
//        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;
//        uint64_t RANDOM_ZONE_SECTORS_TWO = TestFuseLFS::nvme_info.zone_capacity * 2;
//        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS_TWO; i++) {
//            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
//        }
//
//        // Random_ptr should not change if zone is multiple of two and random
//        // zone is not full
//        auto temp_ptr = TestFuseLFS::random_ptr;
//
//        // Freeing up space should move the random_pos.
//        auto temp_pos = TestFuseLFS::random_pos;
//
//        BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() == 0);
//        BOOST_CHECK(temp_ptr == TestFuseLFS::random_ptr);
//        BOOST_CHECK(temp_pos != TestFuseLFS::random_pos);
//
//        // random_pos should never finalize at RANDZ_BUFF_POS
//        BOOST_CHECK(TestFuseLFS::random_pos.zone < qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone);
//    }
//
//    BOOST_AUTO_TEST_CASE(Test_FuseLFS_rewrite_random_zone_three) {
//        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
//        setup_rewrite_random_blocks(&nvme_memory);
//
//        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
//        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;
//
//        uint64_t RANDOM_ZONE_SECTORS_THREE = TestFuseLFS::nvme_info.zone_capacity * 3;
//        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS_THREE; i++) {
//            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
//        }
//
//        auto temp_ptr = TestFuseLFS::random_ptr;
//        auto temp_pos = TestFuseLFS::random_pos;
//        BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() == 0);
//        BOOST_CHECK(TestFuseLFS::random_ptr == TestFuseLFS::random_pos);
//
//        temp_ptr = TestFuseLFS::random_ptr;
//        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
//        BOOST_CHECK(temp_ptr == TestFuseLFS::random_ptr);
//    }

    /**
     * Assert that rewrite_random_blocks does nothing if random_pos and
     * random_ptr are equal (the random zone is empty).
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_none, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);

        auto temp_ptr = TestFuseLFS::random_ptr;
        auto temp_pos = TestFuseLFS::random_pos;
        BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() == 0);

        BOOST_CHECK(TestFuseLFS::random_ptr == temp_ptr);
        BOOST_CHECK(TestFuseLFS::random_pos == temp_pos);
    }

    /**
     * Create nat_blocks to fill the entire random zone with valid data.
     * This should raise the FLFS_RET_RANDZ_INSUFFICIENT error upon rewrite.
     * determine_random_ptr should indicate that the random zone is full.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_rewrite_random_zone_fill_data, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
            qemucsd::fuse_lfs::RANDZ_POS.zone) *
            TestFuseLFS::nvme_info.zone_capacity *
            qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

        // Leave last inode for last append call has different return status
        uint64_t i = 1;
        while(i != max_inodes) {
            TestFuseLFS::inode_lba_map.insert(std::make_pair(i, i));
            uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

            if(index_mod == 0) {
                nt_blk.inode[index_mod] = i;
                BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
            }
            else
                nt_blk.inode[index_mod] = i;

            i++;
        }

        TestFuseLFS::inode_lba_map.insert(std::make_pair(max_inodes + 1, max_inodes + 1));
        nt_blk.inode[qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM-1] - max_inodes + 1;
        BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);

        BOOST_CHECK(TestFuseLFS::random_ptr.valid() == false);

        // This will free 0 space as the entire random zone is linearly filled
        // so it contains no invalid data.
        BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() ==
            qemucsd::fuse_lfs::FLFS_RET_RANDZ_INSUFFICIENT);
        // The random_ptr should still be invalid after this
        BOOST_CHECK(TestFuseLFS::random_ptr.valid() == false);

        // Determining the random pointer should indicate the random zone is
        // full
        BOOST_CHECK(TestFuseLFS::determine_random_ptr() ==
            qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);
    }

    /**
     * Fill half of the random zone with valid data and perform a rewrite
     * freeing no space.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_rewrite_random_zone_fill_data_half, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
                               qemucsd::fuse_lfs::RANDZ_POS.zone) *
                              TestFuseLFS::nvme_info.zone_capacity *
                              qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;
        uint64_t half_inodes = max_inodes / 2;

        uint64_t i = 1;
        while(i != half_inodes + 1) {
            TestFuseLFS::inode_lba_map.insert(std::make_pair(i, i));
            uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

            if(index_mod == 0) {
                nt_blk.inode[index_mod] = i;
                BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
            }
            else
                nt_blk.inode[index_mod] = i;

            i++;
        }
        BOOST_CHECK(TestFuseLFS::random_ptr.valid() == true);

        // This will free 0 space as half the random zone is linearly filled
        // so it contains no invalid data.
        BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() == 0);

        // The random_ptr should still be valid after this
        BOOST_CHECK(TestFuseLFS::random_ptr.valid() == true);
        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
    }

    /**
     * Fill and rewrite the random zone 5 times. only occupy about 2 blocks of
     * inodes.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_rewrite_random_zone_fill_data_rounds, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
                               qemucsd::fuse_lfs::RANDZ_POS.zone) *
                              TestFuseLFS::nvme_info.zone_capacity *
                              qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

        // Fill exactly two nat blocks with unique data
        uint32_t overflow = (qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM * 2);

        uint32_t i, ino;
        for(uint32_t rounds = 0; rounds < 5; rounds++) {
            // Never insert inode 0 as this interpreted as 'no further data'
            ino = 1;
            for(i = 1; i < max_inodes; i++){
                TestFuseLFS::inode_lba_map.insert(std::make_pair(ino, ino));
                uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

                if(index_mod == 0) {
                    nt_blk.inode[index_mod] = ino;
                    BOOST_CHECK_MESSAGE(TestFuseLFS::append_random_block(nt_blk) == 0,
                        "Append node: " << i << " ino: " << ino << " round: " << rounds);
                }
                else
                    nt_blk.inode[index_mod] = ino;

                // Increment and overflow the inode, being sure to never insert 0
                ino = (i % overflow) + 1;
            }

            // After the first round the maximum inodes we can append is
            // decreased by the number inodes that remain valid after the
            // rewrite
            if(rounds == 0) max_inodes -= (overflow);

            TestFuseLFS::inode_lba_map.insert(std::make_pair(ino, ino));
            nt_blk.inode[qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM-1] = ino;
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) ==
                qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);

            BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() == 0);
        }
    }

    /**
     * Fill half of the random zone with valid data and perform a
     * read_random_zone. Verify that both inode_lba maps have the same data.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_rewrite_random_zone_fill_reconstruct, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);
        setup_rewrite_random_blocks(&nvme_memory);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
                               qemucsd::fuse_lfs::RANDZ_POS.zone) *
                              TestFuseLFS::nvme_info.zone_capacity *
                              qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;
        uint64_t half_inodes = max_inodes / 2;

        uint64_t i = 1;
        while(i != half_inodes + 1) {
            TestFuseLFS::inode_lba_map.insert_or_assign(i, i);
            uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

            if(index_mod == 0) {
                nt_blk.inode[index_mod] = i;
                nt_blk.lba[index_mod] = i;
                BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
            }
            else {
                nt_blk.inode[index_mod] = i;
                nt_blk.lba[index_mod] = i;
            }

            i++;
        }
        BOOST_CHECK(TestFuseLFS::random_ptr.valid() == true);

        auto test_inode_map = qemucsd::fuse_lfs::inode_lba_map_t();

        BOOST_CHECK(TestFuseLFS::read_random_zone(&test_inode_map) ==
            qemucsd::fuse_lfs::FLFS_RET_NONE);

        for(auto &entry : TestFuseLFS::inode_lba_map) {
            auto result = test_inode_map.find(entry.first);
            BOOST_CHECK_MESSAGE(
                result != test_inode_map.end(), "Could not find " <<
                entry.first);
            BOOST_CHECK(result->second == entry.second);
        }
    }

BOOST_AUTO_TEST_SUITE_END()