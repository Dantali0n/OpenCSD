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
#define BOOST_TEST_MODULE TestFuseLfsDrive

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "tests.hpp"

#include "flfs.hpp"
#include "nvme_zns_memory.hpp"

/**
 * BACKGROUND INFO:
 *  these tests perform read, append and reset operations on a
 *  NvmeZnsMemoryBackend.
*/

BOOST_AUTO_TEST_SUITE(Test_FuseLfsDrive)

    using qemucsd::fuse_lfs::FuseLFS;
    static qemucsd::arguments::options opts = {};

    class TestFuseLFS : public FuseLFS {
    public:
        TestFuseLFS(qemucsd::nvme_zns::NvmeZnsBackend* nvme) :
            FuseLFS(&opts, nvme) {

        }

        using FuseLFS::nvme_info;

        using FuseLFS::nvme;

        using FuseLFS::inode_lba_map;
        using FuseLFS::path_inode_map;

        using FuseLFS::cblock_pos;

        using FuseLFS::random_pos;
        using FuseLFS::random_ptr;

        using FuseLFS::log_ptr;

        using FuseLFS::ino_ptr;

        using FuseLFS::inode_nlookup_map;

        using FuseLFS::inode_entries;

        using FuseLFS::data_blocks;

        using FuseLFS::inode_nlookup_increment;
        using FuseLFS::inode_nlookup_decrement;

        using FuseLFS::lba_to_position;
        using FuseLFS::position_to_lba;
        using FuseLFS::update_inode_lba_map;

        using FuseLFS::ino_stat;

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

        using FuseLFS::determine_log_ptr;

        using FuseLFS::get_inode_entry_t;
        using FuseLFS::create_inode;
    };

    struct qemucsd::fuse_lfs::data_position NULL_POS = {0};

    struct TestFuseLFSFixture {
        TestFuseLFSFixture() {
        }
    };

    void setup_memory_backend(TestFuseLFS *test_fuse)
    {
        test_fuse->nvme->get_nvme_zns_info(&test_fuse->nvme_info);

        BOOST_CHECK(test_fuse->mkfs() == 0);
    }

    /**
     * Early setup for rewrite_random_blocks tests
     */
    void setup_rewrite_random_blocks(TestFuseLFS *test_fuse)
    {
        setup_memory_backend(test_fuse);

        test_fuse->random_pos = qemucsd::fuse_lfs::RANDZ_POS;

        BOOST_CHECK(test_fuse->determine_random_ptr() == 0);
        BOOST_CHECK(test_fuse->random_ptr == test_fuse->random_pos);
    }

    void free_data_structure_heap_memory(TestFuseLFS *test_fuse) {
        for(auto &entry : *test_fuse->path_inode_map) {
            delete entry.second;
        }

        for(auto &entry : *test_fuse->data_blocks) {
            delete entry.second;
        }
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_mkfs, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);

        setup_memory_backend(&test_fuse);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_super_block, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);

        test_fuse.nvme = &nvme_memory;
        nvme_memory.get_nvme_zns_info(&test_fuse.nvme_info);

        BOOST_CHECK(test_fuse.verify_superblock() == -1);

        BOOST_CHECK(test_fuse.mkfs() == 0);
        BOOST_CHECK(test_fuse.verify_superblock() == 0);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_dirty_block, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        BOOST_CHECK(test_fuse.verify_dirtyblock() == 0);

        BOOST_CHECK(test_fuse.write_dirtyblock() == 0);
        BOOST_CHECK(test_fuse.verify_dirtyblock() == -1);

        BOOST_CHECK(test_fuse.remove_dirtyblock() == 0);
        BOOST_CHECK(test_fuse.verify_dirtyblock() == 0);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_checkpoint_block, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        BOOST_CHECK(test_fuse.get_checkpointblock(cblock) == 0);

        BOOST_CHECK(cblock.randz_lba == test_fuse.nvme_info.zone_size *
                    qemucsd::fuse_lfs::RANDZ_POS.zone);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_checkpoint_block_fill,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        uint64_t randz_base = test_fuse.nvme_info.zone_size *
            qemucsd::fuse_lfs::RANDZ_POS.zone;

        uint64_t logz_base = test_fuse.nvme_info.zone_size *
            qemucsd::fuse_lfs::LOGZ_POS.zone;

        uint64_t checkpoint_blocks_x4 = test_fuse.nvme_info.zone_capacity * 8;

        uint32_t czones = qemucsd::fuse_lfs::RANDZ_POS.zone -
            qemucsd::fuse_lfs::CBLOCK_POS.zone;
        for(uint64_t i = 0; i < checkpoint_blocks_x4; i ++) {
            BOOST_CHECK(test_fuse.update_checkpointblock(
                randz_base + i, logz_base) == 0);
            BOOST_CHECK(test_fuse.get_checkpointblock(cblock) == 0);
            BOOST_CHECK_MESSAGE(cblock.randz_lba == randz_base + i,
                "Incorrect randz_lba at checkpoint block update " << i);
        }
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_checkpoint_block_power_atomicity,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        struct qemucsd::fuse_lfs::checkpoint_block cblock;

        uint64_t randz_base = test_fuse.nvme_info.zone_size *
            qemucsd::fuse_lfs::RANDZ_POS.zone;

        uint64_t logz_base = test_fuse.nvme_info.zone_size *
            qemucsd::fuse_lfs::LOGZ_POS.zone;

        // mkfs takes first block so zone_capacity -1
        for(uint64_t i = 0; i < test_fuse.nvme_info.zone_capacity - 1; i++) {
            BOOST_CHECK(test_fuse.update_checkpointblock(
                randz_base + i, logz_base) == 0);
            BOOST_CHECK(test_fuse.get_checkpointblock_locate(cblock) == 0);
            BOOST_CHECK_MESSAGE(cblock.randz_lba == randz_base + i,
            "Incorrect randz_lba at checkpoint block update " << i);
        }

        uint64_t res_sector;
        cblock.randz_lba = 1337;
        BOOST_CHECK(test_fuse.nvme->append(test_fuse.cblock_pos.zone + 1,
                    res_sector, test_fuse.cblock_pos.offset,
                    &cblock, test_fuse.cblock_pos.size) == 0);
        BOOST_CHECK(res_sector == 0);

        BOOST_CHECK(test_fuse.get_checkpointblock_locate(cblock) == 0);
        BOOST_CHECK(cblock.randz_lba == 1337);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_append_random_block,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        test_fuse.random_pos = qemucsd::fuse_lfs::RANDZ_POS;
        test_fuse.random_ptr = test_fuse.random_pos;

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);

        BOOST_CHECK(test_fuse.random_ptr.sector != 0);
    }


    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_append_random_block_verify_data,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        test_fuse.random_pos = qemucsd::fuse_lfs::RANDZ_POS;
        test_fuse.random_ptr = test_fuse.random_pos;

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;
        for(uint32_t i = 0; i < qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM; i++) {
            nt_blk.inode[i] = i + 1;
        }

        BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);

        struct qemucsd::fuse_lfs::nat_block nt_blk_test = {0};
        test_fuse.nvme->read(
            test_fuse.random_ptr.zone, 0, 0, &nt_blk_test,
            sizeof(qemucsd::fuse_lfs::nat_block));

        for(uint32_t i = 0; i < qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM; i++) {
            BOOST_CHECK(nt_blk_test.inode[i] == nt_blk_test.inode[i]);
        }
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_append_random_blocks_full,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        test_fuse.random_pos = qemucsd::fuse_lfs::RANDZ_POS;
        test_fuse.random_ptr = test_fuse.random_pos;

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint32_t RANDOM_ZONE_ZONES = qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone - qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t RANDOM_ZONE_SECTORS = RANDOM_ZONE_ZONES * test_fuse.nvme_info.zone_capacity;

        // Append all but last sector
        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS - 1; i++) {
            BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
        }

        // Last append must indicate to caller that random zone is full
        BOOST_CHECK(test_fuse.append_random_block(nt_blk) ==
                    qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_append_random_blocks_illegal_pos,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
                1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        test_fuse.random_pos = qemucsd::fuse_lfs::RANDZ_BUFF_POS;
        test_fuse.random_ptr = qemucsd::fuse_lfs::RANDZ_POS;

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        #ifdef QEMUCSD_DEBUG
            BOOST_CHECK(test_fuse.append_random_block(nt_blk) == -1);
        #else
            BOOST_CHECK(TestFuseLFS::append_random_block(nt_blk) == 0);
        #endif
    }

    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_base, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);
    }

    /**
     * Write the last sector of the first zone and check determine_random_ptr
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_zone_end, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        for(uint64_t i = 0; i < test_fuse.nvme_info.zone_capacity-1; i++) {
            BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
        }

        auto temp_ptr = test_fuse.random_ptr;
        BOOST_CHECK(test_fuse.determine_random_ptr() == 0);
        BOOST_CHECK(temp_ptr == test_fuse.random_ptr);
    }

    /**
     * Write half of the random zone and verify that the random_ptr ends up
     * at the right location.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_half, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint32_t RANDOM_ZONE_ZONES = qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone - qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t RANDOM_ZONE_SECTORS = RANDOM_ZONE_ZONES * test_fuse.nvme_info.zone_capacity;
        uint64_t RANDOM_ZONE_SECTORS_HALF = RANDOM_ZONE_SECTORS / 2;
        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS_HALF; i++) {
            BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
        }

        auto temp_ptr = test_fuse.random_ptr;
        BOOST_CHECK(test_fuse.determine_random_ptr() == 0);
        BOOST_CHECK(temp_ptr == test_fuse.random_ptr);
    }

    /**
     * Fill the random zone with invalid data. Check that the last append
     * returns FLFS_RET_RANDZ_FULL and invalidates the random_ptr. Next
     * rewrite the random zone and verify that random_pos and random_ptr are
     * equal, this indicates the zone is empty.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_full, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint32_t RANDOM_ZONE_ZONES = qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone - qemucsd::fuse_lfs::RANDZ_POS.zone;
        uint64_t RANDOM_ZONE_SECTORS = RANDOM_ZONE_ZONES * test_fuse.nvme_info.zone_capacity;
        for(uint64_t i = 0; i < RANDOM_ZONE_SECTORS - 1; i++) {
            BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
        }

        // This last append should make the random zone full
        // This should invalidate the random_ptr
        BOOST_CHECK(test_fuse.append_random_block(nt_blk) ==
                    qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);
        BOOST_CHECK(test_fuse.random_ptr.valid() == false);

        // determining random_ptr should indicate zone is full and random_ptr
        // should be invalid.
        BOOST_CHECK(test_fuse.determine_random_ptr() ==
            qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);
        BOOST_CHECK(!test_fuse.random_ptr.valid());

        // Rewrite the random zone
        BOOST_CHECK(test_fuse.rewrite_random_blocks() == 0);
        // random_pos and random_ptr should equal each other indicating the zone
        // is empty.
        BOOST_CHECK(test_fuse.random_pos == test_fuse.random_ptr);

        auto temp_ptr = test_fuse.random_ptr;
        BOOST_CHECK(test_fuse.determine_random_ptr() == 0);
        BOOST_CHECK(temp_ptr == test_fuse.random_ptr);
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
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_determine_random_ptr_restore_none,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        auto temp_ptr = test_fuse.random_ptr;
        auto temp_pos = test_fuse.random_pos;
        BOOST_CHECK(test_fuse.rewrite_random_blocks() == 0);

        BOOST_CHECK(test_fuse.random_ptr == temp_ptr);
        BOOST_CHECK(test_fuse.random_pos == temp_pos);
    }

    /**
     * Create nat_blocks to fill the entire random zone with valid data.
     * This should raise the FLFS_RET_RANDZ_INSUFFICIENT error upon rewrite.
     * determine_random_ptr should indicate that the random zone is full.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_rewrite_random_zone_fill_data,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
            qemucsd::fuse_lfs::RANDZ_POS.zone) *
            test_fuse.nvme_info.zone_capacity *
            qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

        // Leave last inode for last append call has different return status
        uint64_t i = 1;
        while(i != max_inodes) {
            test_fuse.inode_lba_map->insert(std::make_pair(i, i));
            uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

            if(index_mod == 0) {
                nt_blk.inode[index_mod] = i;
                BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
            }
            else
                nt_blk.inode[index_mod] = i;

            i++;
        }

        test_fuse.inode_lba_map->insert(std::make_pair(max_inodes + 1, max_inodes + 1));
        nt_blk.inode[qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM-1] - max_inodes + 1;
        BOOST_CHECK(test_fuse.append_random_block(nt_blk) == qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);

        BOOST_CHECK(test_fuse.random_ptr.valid() == false);

        // This will free 0 space as the entire random zone is linearly filled
        // so it contains no invalid data.
        BOOST_CHECK(test_fuse.rewrite_random_blocks() ==
            qemucsd::fuse_lfs::FLFS_RET_RANDZ_INSUFFICIENT);
        // The random_ptr should still be invalid after this
        BOOST_CHECK(test_fuse.random_ptr.valid() == false);

        // Determining the random pointer should indicate the random zone is
        // full
        BOOST_CHECK(test_fuse.determine_random_ptr() ==
            qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);
    }

    /**
     * Fill half of the random zone with valid data and perform a rewrite
     * freeing no space.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_rewrite_random_zone_fill_data_half,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
           qemucsd::fuse_lfs::RANDZ_POS.zone) *
            test_fuse.nvme_info.zone_capacity *
            qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;
        uint64_t half_inodes = max_inodes / 2;

        uint64_t i = 1;
        while(i != half_inodes + 1) {
            test_fuse.inode_lba_map->insert(std::make_pair(i, i));
            uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

            if(index_mod == 0) {
                nt_blk.inode[index_mod] = i;
                BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
            }
            else
                nt_blk.inode[index_mod] = i;

            i++;
        }
        BOOST_CHECK(test_fuse.random_ptr.valid() == true);

        // This will free 0 space as half the random zone is linearly filled
        // so it contains no invalid data.
        BOOST_CHECK(test_fuse.rewrite_random_blocks() == 0);

        // The random_ptr should still be valid after this
        BOOST_CHECK(test_fuse.random_ptr.valid() == true);
        BOOST_CHECK(test_fuse.determine_random_ptr() == 0);
    }

    /**
     * Fill and rewrite the random zone 5 times. only occupy about 2 blocks of
     * inodes.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_rewrite_random_zone_fill_data_rounds,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
            qemucsd::fuse_lfs::RANDZ_POS.zone) *
            test_fuse.nvme_info.zone_capacity *
            qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

        // Fill exactly two nat blocks with unique data
        uint32_t overflow = (qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM * 2);

        uint32_t i, ino;
        for(uint32_t rounds = 0; rounds < 5; rounds++) {
            // Never insert inode 0 as this interpreted as 'no further data'
            ino = 1;
            for(i = 1; i < max_inodes; i++){
                test_fuse.inode_lba_map->insert(std::make_pair(ino, ino));
                uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

                if(index_mod == 0) {
                    nt_blk.inode[index_mod] = ino;
                    BOOST_CHECK_MESSAGE(test_fuse.append_random_block(nt_blk) == 0,
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

            test_fuse.inode_lba_map->insert(std::make_pair(ino, ino));
            nt_blk.inode[qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM-1] = ino;
            BOOST_CHECK(test_fuse.append_random_block(nt_blk) ==
                qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);

            BOOST_CHECK(test_fuse.rewrite_random_blocks() == 0);
        }
    }

    /**
     * Fill half of the random zone with valid data and perform a
     * read_random_zone. Verify that both inode_lba maps have the same data.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_half_fill_read_random_zone,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
            qemucsd::fuse_lfs::RANDZ_POS.zone) *
            test_fuse.nvme_info.zone_capacity *
            qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;
        uint64_t half_inodes = max_inodes / 2;

        uint64_t i = 1;
        while(i != half_inodes + 1) {
            test_fuse.inode_lba_map->insert_or_assign(i, i);
            uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

            if(index_mod == 0) {
                nt_blk.inode[index_mod] = i;
                nt_blk.lba[index_mod] = i;
                BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
            }
            else {
                nt_blk.inode[index_mod] = i;
                nt_blk.lba[index_mod] = i;
            }

            i++;
        }
        BOOST_CHECK(test_fuse.random_ptr.valid() == true);

        auto test_inode_map = qemucsd::fuse_lfs::inode_lba_map_t();

        test_fuse.read_random_zone(&test_inode_map);

        for(auto &entry : *test_fuse.inode_lba_map) {
            auto result = test_inode_map.find(entry.first);
            BOOST_CHECK_MESSAGE(
                result != test_inode_map.end(), "Could not find " <<
                entry.first);
            BOOST_CHECK(result->second == entry.second);
        }
    }

    /**
     * Overwrite parts of random zone with valid data updating lbas for the same
     * inode and perform a read_random_zone. Verify that both inode_lba maps
     * have the same data.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_multi_fill_read_random_zone,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
            qemucsd::fuse_lfs::RANDZ_POS.zone) *
            test_fuse.nvme_info.zone_capacity *
            qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;
        uint64_t half_inodes = max_inodes / 2;

        uint64_t i = 1;

        // Limit the amount of inodes used but keep incrementing lba.
        uint64_t inode_lim = 512;
        uint64_t inode_limited;
        while(i != half_inodes + 1) {
            inode_limited = i % inode_lim + 1;
            test_fuse.inode_lba_map->insert_or_assign(inode_limited , i);
            uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

            if(index_mod == 0) {
                nt_blk.inode[index_mod] = inode_limited;
                nt_blk.lba[index_mod] = i;
                BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
            }
            else {
                nt_blk.inode[index_mod] = inode_limited;
                nt_blk.lba[index_mod] = i;
            }

            i++;
        }
        BOOST_CHECK(test_fuse.random_ptr.valid() == true);

        auto test_inode_map = qemucsd::fuse_lfs::inode_lba_map_t();

        test_fuse.read_random_zone(&test_inode_map);

        for(auto &entry : *test_fuse.inode_lba_map) {
            auto result = test_inode_map.find(entry.first);
            BOOST_CHECK_MESSAGE(
                result != test_inode_map.end(), "Could not find " <<
                entry.first);
            BOOST_CHECK(result->second == entry.second);
        }
    }

    /**
     * Fill and rewrite the random zone 5 times. only occupy about 512 inodes.
     * Verify contents with read_random_zone.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_read_random_zone_fill_data_rounds,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_rewrite_random_blocks(&test_fuse);

        struct qemucsd::fuse_lfs::nat_block nt_blk = {0};
        nt_blk.type = qemucsd::fuse_lfs::RANDZ_NAT_BLK;

        uint64_t max_inodes = (qemucsd::fuse_lfs::RANDZ_BUFF_POS.zone -
            qemucsd::fuse_lfs::RANDZ_POS.zone) *
            test_fuse.nvme_info.zone_capacity *
            qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

        // Fill exactly two nat blocks with unique data
//        uint32_t overflow = (qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM * 2);

        uint32_t i;
        // Limit the amount of inodes used but keep incrementing lba.
        uint64_t inode_lim = (qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM * 8);;
        uint64_t inode_limited;
        for(uint32_t rounds = 0; rounds < 5; rounds++) {
            for(i = 1; i < max_inodes; i++){
                inode_limited = i % inode_lim + 1;
                test_fuse.inode_lba_map->insert_or_assign(inode_limited , i);
                uint32_t index_mod = i % qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM;

                if(index_mod == 0) {
                    nt_blk.inode[index_mod] = inode_limited;
                    nt_blk.lba[index_mod] = i;
                    BOOST_CHECK(test_fuse.append_random_block(nt_blk) == 0);
                }
                else {
                    nt_blk.inode[index_mod] = inode_limited;
                    nt_blk.lba[index_mod] = i;
                }
            }

            // After the first round the maximum inodes we can append is
            // decreased by the number inodes that remain valid after the
            // rewrite
            if(rounds == 0) max_inodes -= inode_lim;

            test_fuse.inode_lba_map->insert_or_assign(inode_limited , i);
            nt_blk.inode[qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM-1] = inode_limited;
            nt_blk.lba[qemucsd::fuse_lfs::NAT_BLK_INO_LBA_NUM-1] = i;
            BOOST_CHECK(test_fuse.append_random_block(nt_blk) ==
                        qemucsd::fuse_lfs::FLFS_RET_RANDZ_FULL);

            BOOST_CHECK(test_fuse.rewrite_random_blocks() == 0);
        }
    }

    /**
     * Test keeping track of the nlookup count to adhere to memory pressure
     * indications from the kernel.
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_nlookup,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);

        // Test insert and uniqueness
        test_fuse.inode_nlookup_increment(1);
        BOOST_CHECK(test_fuse.inode_nlookup_map->size() == 1);
        test_fuse.inode_nlookup_increment(1);
        BOOST_CHECK(test_fuse.inode_nlookup_map->size() == 1);

        // Test incremented count
        BOOST_CHECK(test_fuse.inode_nlookup_map->find(1)->second == 2);

        // Try decrementing more than is incremented
        test_fuse.inode_nlookup_decrement(1, 3);

        // Decrementing an inode to 0 should remove it
        BOOST_CHECK(test_fuse.inode_nlookup_map->size() == 0);
    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_update_inode_lba_map,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        uint64_t initial_size = test_fuse.inode_lba_map->size();

        // Add two inodes to vector that should never be present in the map
        std::vector<fuse_ino_t> inodes;
        inodes.push_back(0);
        inodes.push_back(1);

        // Update the map with the invalid inodes
        test_fuse.update_inode_lba_map(
            &inodes, 64, test_fuse.inode_lba_map);

        // Verify the map size is the same before and after
        BOOST_CHECK(test_fuse.inode_lba_map->size() == initial_size);

        // Clear the invalid inodes
        inodes.clear();

        // Add two valid inodes
        inodes.push_back(2);
        inodes.push_back(3);

        // Update the map
        test_fuse.update_inode_lba_map(
            &inodes, 64, test_fuse.inode_lba_map);

        // Verify the map size has changed
        BOOST_CHECK(test_fuse.inode_lba_map->size() == initial_size + 2);

        // Verify the lba 64 has been placed in the map
        BOOST_CHECK(test_fuse.inode_lba_map->find(2)->second == 64);
        BOOST_CHECK(test_fuse.inode_lba_map->find(3)->second == 64);

        // Update the map now with 192
        test_fuse.update_inode_lba_map(
            &inodes, 192, test_fuse.inode_lba_map);

        // Verify the lba 192 has been placed in the map
        BOOST_CHECK(test_fuse.inode_lba_map->find(2)->second == 192);
        BOOST_CHECK(test_fuse.inode_lba_map->find(3)->second == 192);
    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_ino_stat, TestFuseLFSFixture) {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        struct stat stbuf = {0};

        // Inode 0 is invalid and should never exist
        BOOST_CHECK(test_fuse.ino_stat(0, &stbuf) ==
            qemucsd::fuse_lfs::FLFS_RET_ENOENT);

        memset(&stbuf, 0, sizeof(struct stat));

        // Root inode should always exist
        BOOST_CHECK(test_fuse.ino_stat(1, &stbuf) ==
            qemucsd::fuse_lfs::FLFS_RET_NONE);
        BOOST_CHECK(stbuf.st_ino == 1);
        BOOST_CHECK(stbuf.st_mode & S_IFDIR);
        BOOST_CHECK(stbuf.st_mode & 0755);
        BOOST_CHECK(stbuf.st_nlink == 2);
        BOOST_CHECK(stbuf.st_size == 0);

        memset(&stbuf, 0, sizeof(struct stat));

        // Hardcoded and invalid inodes should never appear in the map
        BOOST_CHECK(test_fuse.inode_lba_map->find(0) ==
            test_fuse.inode_lba_map->end());
        BOOST_CHECK(test_fuse.inode_lba_map->find(1) ==
            test_fuse.inode_lba_map->end());

        // Create inode 2 entry manually so get_inode_entry can find it
        struct qemucsd::fuse_lfs::inode_entry entry;
        entry.size = 512;
        entry.inode = 2;
        entry.data_lba = 0;
        entry.type = qemucsd::fuse_lfs::INO_T_FILE;
        entry.parent = 1;

        // Check that unkown inodes return FLFS_RET_ENOENT
        BOOST_CHECK(test_fuse.ino_stat(2, &stbuf) ==
                    qemucsd::fuse_lfs::FLFS_RET_ENOENT);

        // Insert inode 2 entry
        test_fuse.inode_lba_map->insert(std::make_pair(2, 0));
        test_fuse.inode_entries->insert(
            std::make_pair(2, std::make_pair(entry, "test")));

        // Newly added file should exist
        BOOST_CHECK(test_fuse.ino_stat(2, &stbuf) ==
                    qemucsd::fuse_lfs::FLFS_RET_NONE);
        BOOST_CHECK(stbuf.st_ino == 2);
        BOOST_CHECK(stbuf.st_mode & S_IFREG);
        BOOST_CHECK(stbuf.st_mode & 0644);
        BOOST_CHECK(stbuf.st_nlink == 1);
        BOOST_CHECK(stbuf.st_size == 512);

        // Update entry to represent directory now
        entry.size = 0;
        entry.inode = 3;
        entry.data_lba = 0;
        entry.type = qemucsd::fuse_lfs::INO_T_DIR;
        entry.parent = 1;

        // Insert inode 3 entry
        test_fuse.inode_lba_map->insert(std::make_pair(3, 0));
        test_fuse.inode_entries->insert(
            std::make_pair(3, std::make_pair(entry, "directory")));

        // Newly added directory should exist
        BOOST_CHECK(test_fuse.ino_stat(3, &stbuf) ==
                    qemucsd::fuse_lfs::FLFS_RET_NONE);
        BOOST_CHECK(stbuf.st_ino == 3);
        BOOST_CHECK(stbuf.st_mode & S_IFDIR);
        BOOST_CHECK(stbuf.st_mode & 0755);
        BOOST_CHECK(stbuf.st_nlink == 2);
        BOOST_CHECK(stbuf.st_size == 0);

        // Update entry to represent none type now
        entry.size = 0;
        entry.inode = 4;
        entry.data_lba = 0;
        entry.type = qemucsd::fuse_lfs::INO_T_NONE;
        entry.parent = 1;
        // Insert inode 4 entry
        test_fuse.inode_lba_map->insert(std::make_pair(4, 0));
        test_fuse.inode_entries->insert(
            std::make_pair(4, std::make_pair(entry, "none")));

        // None type should not exist
        BOOST_CHECK(test_fuse.ino_stat(4, &stbuf) ==
                    qemucsd::fuse_lfs::FLFS_RET_ENOENT);
    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_get_inode_entry,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        setup_memory_backend(&test_fuse);

        test_fuse.determine_log_ptr();

        uint64_t lba;
        test_fuse.position_to_lba(test_fuse.log_ptr, lba);

        // Create 2 inodes at the current log_ptr
        test_fuse.inode_lba_map->insert(std::make_pair(3, lba));
        test_fuse.inode_lba_map->insert(std::make_pair(4, lba));

        // Create entry for an inode
        struct qemucsd::fuse_lfs::inode_entry entry = {0};
        entry.parent = 1;
        entry.type = qemucsd::fuse_lfs::INO_T_FILE;
        entry.data_lba = 0;
        entry.size = 0;

        // We manually write the contents of an inode block and flush it
        // This is to avoid relying on flush_inode method functionality.
        auto *ino_blk_ptr = (uint8_t*) malloc(
            sizeof(qemucsd::fuse_lfs::inode_block));

        // Store offset per entry
        static constexpr uint64_t off = qemucsd::fuse_lfs::INODE_ENTRY_SIZE;
        // Store filename for test files
        static const std::string filename = "testfile";

        entry.inode = 3;
        memcpy(ino_blk_ptr, &entry, off);
        memcpy(ino_blk_ptr + off, filename.c_str(), filename.size() + 1);

        entry.inode = 4;
        memcpy(ino_blk_ptr + off + filename.size() + 1, &entry, off);
        memcpy(ino_blk_ptr + (off * 2) + filename.size() + 1, filename.c_str(),
           filename.size() + 1);

        uint64_t final_offset = (off * 2) + (filename.size() *2 ) + 2;
        memset(ino_blk_ptr + final_offset, 0,
               sizeof(qemucsd::fuse_lfs::inode_block) - final_offset);

        uint64_t result_sector = 0;
        struct qemucsd::fuse_lfs::data_position cpy_log_ptr =
            test_fuse.log_ptr;
        BOOST_CHECK(test_fuse.nvme->append(
            cpy_log_ptr.zone, result_sector, cpy_log_ptr.offset, ino_blk_ptr,
            sizeof(qemucsd::fuse_lfs::inode_block)) == 0);
        BOOST_CHECK(result_sector == cpy_log_ptr.sector);

        // Now get the inode entry for inode 3
        qemucsd::fuse_lfs::inode_entry_t inode_entry;
        BOOST_CHECK(test_fuse.get_inode_entry_t(3, &inode_entry) ==
                    qemucsd::fuse_lfs::FLFS_RET_NONE);
        BOOST_CHECK(inode_entry.first.inode == 3);
        BOOST_CHECK(inode_entry.second == filename);

        // And get the inode entry for inode 4
        BOOST_CHECK(test_fuse.get_inode_entry_t(4, &inode_entry) ==
                    qemucsd::fuse_lfs::FLFS_RET_NONE);
        BOOST_CHECK(inode_entry.first.inode == 4);
        BOOST_CHECK(inode_entry.second == filename);

        // And get the entry that does not exist
        BOOST_CHECK(test_fuse.get_inode_entry_t(5, &inode_entry) ==
                    qemucsd::fuse_lfs::FLFS_RET_ENOENT);

        // Add this invalid entry to the map
        test_fuse.inode_lba_map->insert(std::make_pair(5, lba));

        // Failing to find this entry now should return an error
        BOOST_CHECK(test_fuse.get_inode_entry_t(5, &inode_entry) ==
                    qemucsd::fuse_lfs::FLFS_RET_ERR);

        free(ino_blk_ptr);
        free_data_structure_heap_memory(&test_fuse);
    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_create_inode,
                            TestFuseLFSFixture)
    {
        qemucsd::nvme_zns::NvmeZnsMemoryBackend nvme_memory(
            1024, 256, qemucsd::fuse_lfs::SECTOR_SIZE);
        TestFuseLFS test_fuse(&nvme_memory);
        fuse_ino_t ino;

        test_fuse.ino_ptr = 3;
        test_fuse.path_inode_map->insert(
            std::make_pair(1, new qemucsd::fuse_lfs::path_map_t()));

        // Create a file in the root directory
        BOOST_CHECK(test_fuse.create_inode(1, "testfile1",
            qemucsd::fuse_lfs::INO_T_FILE, ino) == qemucsd::fuse_lfs::FLFS_RET_NONE);

        // Create a directory and add a file to it
        BOOST_CHECK(test_fuse.create_inode(1, "testdir1",
            qemucsd::fuse_lfs::INO_T_DIR, ino) == qemucsd::fuse_lfs::FLFS_RET_NONE);
        BOOST_CHECK(test_fuse.create_inode(ino, "testfile2",
            qemucsd::fuse_lfs::INO_T_FILE, ino) == qemucsd::fuse_lfs::FLFS_RET_NONE);

        // This should fail as the parent inode does not exist
        BOOST_CHECK(test_fuse.create_inode(13373, "testfile1",
            qemucsd::fuse_lfs::INO_T_FILE, ino) == qemucsd::fuse_lfs::FLFS_RET_ERR);

        free_data_structure_heap_memory(&test_fuse);
    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_update_inode,
                            TestFuseLFSFixture)
    {

    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_add_nat_update_set,
                            TestFuseLFSFixture)
    {

    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_log_ptr,
                            TestFuseLFSFixture)
    {

    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_fill_inode_block,
                            TestFuseLFSFixture)
    {

    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_erase_inode_entries,
                            TestFuseLFSFixture)
    {

    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_flush_inodes,
                            TestFuseLFSFixture)
    {

    }

    /**
     *
     */
    BOOST_FIXTURE_TEST_CASE(Test_FuseLFS_file_handle,
                            TestFuseLFSFixture)
    {

    }

BOOST_AUTO_TEST_SUITE_END()