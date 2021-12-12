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
        using FuseLFS::append_random_block;
        using FuseLFS::rewrite_random_blocks;
    };

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_mkfs) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_super_block) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::verify_superblock() == -1);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);
        BOOST_CHECK(TestFuseLFS::verify_superblock() == 0);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_dirty_block) {
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

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_checkpoint_block) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        BOOST_CHECK(TestFuseLFS::get_checkpointblock(cblock) == 0);

        BOOST_CHECK(cblock.randz_lba == TestFuseLFS::nvme_info.zone_size *
                    qemucsd::fuse_lfs::RANDZ_POS.zone);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_checkpoint_block_fill) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        uint64_t randz_base = TestFuseLFS::nvme_info.zone_size *
                qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t checkpoint_blocks_x4 = TestFuseLFS::nvme_info.zone_size * 8;
        for(uint64_t i = 0; i < checkpoint_blocks_x4; i++) {
            BOOST_CHECK(TestFuseLFS::update_checkpointblock(randz_base + i) == 0);
            BOOST_CHECK(TestFuseLFS::get_checkpointblock(cblock) == 0);
            BOOST_CHECK_MESSAGE(cblock.randz_lba == randz_base + i,
                "Incorrect randz_lba at checkpoint block update " << i);
        }
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_checkpoint_block_power_atomicity) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        uint64_t randz_base = TestFuseLFS::nvme_info.zone_size *
                              qemucsd::fuse_lfs::RANDZ_POS.zone;

        // mkfs takes first block so zone_size -1
        for(uint64_t i = 0; i < TestFuseLFS::nvme_info.zone_size - 1; i++) {
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

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_append_random_block) {
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

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_append_random_blocks_full) {
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

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_append_random_blocks_illegal_pos) {
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

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_determine_random_ptr_base) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_POS;

        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(TestFuseLFS::random_ptr == TestFuseLFS::random_pos);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_half) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_POS;

        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(TestFuseLFS::random_ptr == TestFuseLFS::random_pos);

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

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_full) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_POS;

        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(TestFuseLFS::random_ptr == TestFuseLFS::random_pos);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint32_t RANDOM_ZONE_ZONES = qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone - qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t RANDOM_ZONE_SECTORS = RANDOM_ZONE_ZONES * TestFuseLFS::nvme_info.zone_capacity;
        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS - 1; i++) {
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
        }
        BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) ==
                    qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);

        BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() == 0);

        auto temp_ptr = TestFuseLFS::random_ptr;
        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(temp_ptr == TestFuseLFS::random_ptr);
    }

    BOOST_AUTO_TEST_CASE(Test_FuseLFS_rewrite_random_zone_half) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(1024, 256, 512);

        TestFuseLFS::nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&TestFuseLFS::nvme_info);

        BOOST_CHECK(TestFuseLFS::mkfs() == 0);

        TestFuseLFS::random_pos = qemucsd::fuse_lfs::RANDZ_POS;

        BOOST_CHECK(TestFuseLFS::determine_random_ptr() == 0);
        BOOST_CHECK(TestFuseLFS::random_ptr == TestFuseLFS::random_pos);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint32_t RANDOM_ZONE_ZONES = qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone - qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t RANDOM_ZONE_SECTORS = RANDOM_ZONE_ZONES * TestFuseLFS::nvme_info.zone_capacity;
        uint64_t RANDOM_ZONE_SECTORS_HALF = RANDOM_ZONE_SECTORS / 2;
        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS_HALF; i++) {
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
        }

        // Rewriting random blocks if the random zone had not been full
        // should not change the random_ptr as It's still valid.
        auto temp_ptr = TestFuseLFS::random_ptr;
        BOOST_CHECK(TestFuseLFS::rewrite_random_blocks() == 0);
        BOOST_CHECK(temp_ptr == TestFuseLFS::random_ptr);
    }

BOOST_AUTO_TEST_SUITE_END()