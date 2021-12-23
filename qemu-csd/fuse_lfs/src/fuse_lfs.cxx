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

#include "fuse_lfs.hpp"
#include "nvme_zns_memory.hpp"

namespace qemucsd::fuse_lfs {

    // TODO(Dantali0n): Remove me
    static constexpr uint64_t TEST_SIZE = 4096;

    struct fuse_conn_info* FuseLFS::connection = nullptr;

    struct nvme_zns::nvme_zns_info FuseLFS::nvme_info = {0};
    nvme_zns::NvmeZnsBackend* FuseLFS::nvme = nullptr;

    const std::string FuseLFS::FUSE_LFS_NAME_PREFIX = "[FUSE LFS] ";
    const std::string FuseLFS::FUSE_SEQUENTIAL_PARAM = "-s";

    output::Output FuseLFS::output =
        output::Output(FuseLFS::FUSE_LFS_NAME_PREFIX);

    path_inode_map_t FuseLFS::path_inode_map = path_inode_map_t();

    inode_lba_map_t FuseLFS::inode_lba_map = inode_lba_map_t();

    nat_update_set_t FuseLFS::nat_update_set = nat_update_set_t();

    // Current checkpoint position on drive
    struct data_position FuseLFS::cblock_pos = {
        0, 0, 0, 0
    };

    // Current start of random zone on drive
    struct data_position FuseLFS::random_pos = {
        0, 0, 0, 0
    };

    // Current write pointer into the random zone
    struct data_position FuseLFS::random_ptr = {
        0, 0, 0, 0
    };

    const struct fuse_lowlevel_ops FuseLFS::operations = {
        .init       = FuseLFS::init,
        .destroy    = FuseLFS::destroy,
        .lookup     = FuseLFS::lookup,
        .getattr    = FuseLFS::getattr,
        .unlink     = FuseLFS::unlink,
        .open	    = FuseLFS::open,
        .read       = FuseLFS::read,
        .write      = FuseLFS::write,
        .readdir    = FuseLFS::readdir,
        .create     = FuseLFS::create,
    };

    /**
     * Initialize fuse using the low level API. Will block until unmounted in
     * foreground mode.
     * @return 0 upon success, -1 on failure.
     */
    int FuseLFS::initialize(
        int argc, char* argv[], nvme_zns::NvmeZnsBackend* nvme)
    {
        struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
        struct fuse_session *session;
        struct fuse_cmdline_opts opts = {0};
        struct fuse_loop_config loop_config = {0};

        struct checkpoint_block cblock = {0};
        path_node_t root = std::make_pair(0, "");

        int ret = -1;

        // Check sequential performance mode
        bool safe = false;
        for(int i = 0; i < argc; i++) {
            if(FUSE_SEQUENTIAL_PARAM.compare(argv[i]) == 0) {
                safe = true;
                break;
            }
        }
        if(safe == false) {
            output(std::cerr, "requires -s sequential mode\n");
            return FLFS_RET_ERR;
        }

        if (fuse_parse_cmdline(&args, &opts) != 0)
            return 1;
        if (opts.show_help) {
            output(std::cout, "usage: ", argv[0],
                   " [options] <mountpoint>\n\n");
            fuse_cmdline_help();
            fuse_lowlevel_help();
            ret = 0;
            goto err_out1;
        } else if (opts.show_version) {
            output(std::cout, "FUSE library version ", fuse_pkgversion(),
                   "\n");
            fuse_lowlevel_version();
            ret = 0;
            goto err_out1;
        }

        if(opts.mountpoint == nullptr) {
            output(std::cout, "usage: ", argv[0], " [options] <mountpoint>\n");
            output(std::cout, "       ", argv[0], " --help\n");
            ret = 1;
            goto err_out1;
        }

        /** No good reason to pass data via fuse init in low level api */
        FuseLFS::nvme = nvme;

        /** Fill nvme_info struct */
        nvme->get_nvme_zns_info(&nvme_info);

        /** Verify that sector are of the minimally required size */
        if(SECTOR_SIZE > nvme_info.sector_size) {
            output(std::cerr, "Sector size (", nvme_info.sector_size,
                   ") is to small, minimal is ", SECTOR_SIZE,
                   " bytes, aborting\n");
            ret = 1;
            goto err_out1;
        }

        /** Verify that sector size is clean multiple of data sector */
        if(nvme_info.sector_size % SECTOR_SIZE != 0) {
            output(std::cerr, "Sector size (", nvme_info.sector_size,
                   ") is not clean multiple of ", SECTOR_SIZE, ", aborting");
            ret = 1;
            goto err_out1;
        }

        // TODO(Dantali0n): Only create filesystem when a certain command line
        //                  argument is supplied. See fuse hello_ll for example.
        output(std::cout, "Creating filesystem..");
        if(mkfs() != FLFS_RET_NONE) {
            ret = 1;
            goto err_out1;
        }

        output(std::cout, "Checking super block..");
        if(verify_superblock() != FLFS_RET_NONE) {
            output(std::cerr, "Failed to verify super block, are you ",
                   "sure the partition does not contain another filesystem?");
            ret = 1;
            goto err_out1;
        }

        // TODO(Dantali0n): Filesystem cleanup / recovery from dirty state
        output(std::cout, "Checking dirty block..");
        if(verify_dirtyblock() != FLFS_RET_NONE) {
            output(std::cerr, "Filesystem dirty, no recovery methods yet",
                   " unable to continue :(");
            ret = 1;
            goto err_out1;
        }

        output(std::cout, "Writing dirty block..");
        if(write_dirtyblock() != FLFS_RET_NONE) {
            output(std::cerr, "Unable to write dirty block to drive, "
                   "check that drive is writeable");
            ret = 1;
            goto err_out1;
        }

        /** Set the random_pos to the correct position */
        get_checkpointblock(cblock);
        lba_to_position(cblock.randz_lba, random_pos);

        /** Determine random_ptr now that random_pos is known */
        determine_random_ptr();

        output(std::cout, "Creating root inode..");
        path_inode_map.insert(std::make_pair(root, 1));

        session = fuse_session_new(
            &args, &operations, sizeof(operations), nullptr);
        if (session == nullptr)
            goto err_out1;

        if (fuse_set_signal_handlers(session) != FLFS_RET_NONE)
            goto err_out2;

        if (fuse_session_mount(session, opts.mountpoint) !=
                FLFS_RET_NONE)
            goto err_out3;

        fuse_daemonize(opts.foreground);

        /* Block until ctrl+c or fusermount -u */
        if (opts.singlethread)
            ret = fuse_session_loop(session);
        else {
            loop_config.clone_fd = opts.clone_fd;
            loop_config.max_idle_threads = opts.max_idle_threads;
            ret = fuse_session_loop_mt(session, &loop_config);
        }

        fuse_session_unmount(session);
        err_out3:
        fuse_remove_signal_handlers(session);
        err_out2:
        fuse_session_destroy(session);
        err_out1:
        free(opts.mountpoint);
        fuse_opt_free_args(&args);

        return ret ? 1 : 0;
    }

    /**
     * Translate the (inefficient) char* path to an inode starting from parent.
     * Absolute paths need to set parent to 0.
     */
    void FuseLFS::path_to_inode(
        fuse_ino_t parent, const char* path, fuse_ino_t &ino)
    {
        std::istringstream spath(path);
        std::string token;
        fuse_ino_t cur_parent = parent;

        // Iterate over the path with increased depth, finding the inode from
        // the parent
        while(std::getline(spath, token, '/')) {
            // If find does not return the end of the map it's a match
            auto it = path_inode_map.find(std::make_pair(cur_parent, token));
            if(it == path_inode_map.end()) {
                // Indicate not found, 0 is invalid inode
                ino = 0;
                return;
            }

            // Update the parent
            cur_parent = it->second;
        }

        // If we traversed the entire path and found an entry at each stage in
        // the map then the final update pointed to the actual inode.
        ino = cur_parent;
    }

    /**
     * Find the Logical Block Address (LBA) of any given inode. Behavior
     * undefined if the inode does not actually exist.
     */
    void FuseLFS::inode_to_lba(fuse_ino_t ino, uint64_t &lba) {
        lba = inode_lba_map.find(ino)->second;
    }

    /**
     * Compute the position of data from a Logical Block Address (LBA).
     * TODO(Dantali0n): Use an LRU cache of configurable size for recent
     *                  positions.
     */
    void FuseLFS::lba_to_position(
        uint64_t lba, struct data_position &position)
    {
        position.zone = lba / nvme_info.zone_size;
        position.sector = lba % nvme_info.zone_size;
    }

    /**
     * Compute the Logical Block Address (LBA) from an position.
     */
    void FuseLFS::position_to_lba(
        struct data_position position, uint64_t &lba)
    {
        nvme->position_to_lba(position.zone, position.sector, lba);
    }

    /**
     * Modify position such that it is advanced by a single sector contextually.
     * Meaning that a position inside the random zone will never advance into
     * the random buffer. Alternatively a position in the checkpoint zone will
     * never advance into the random zone etc...
     */
    void FuseLFS::advance_position(struct data_position &position) {

    }

    /**
     * Find and fill the stbuf information for the given inode.
     * TODO(Dantali0n): Cache these lookups as much as possible.
     */
    int FuseLFS::ino_stat(fuse_ino_t ino, struct stat *stbuf) {

        stbuf->st_ino = ino;
        switch (ino) {
            case 1:
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
                break;

            case 2:
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;
                stbuf->st_size = TEST_SIZE;
                break;

            default:
                return FLFS_RET_ERR;
        }
        return FLFS_RET_NONE;
    }

    /**
     * Create a fuse reply taking into account buffer and offset constraints
     * @return result of fuse_reply_buf, 0 upon success
     */
    int FuseLFS::reply_buf_limited(fuse_req_t req, const char *buf,
                                   size_t bufsize, off_t off, size_t maxsize)
    {
        if (off < bufsize)
            return fuse_reply_buf(req, buf + off,
                                  flfs_min(bufsize - off, maxsize));
        else
            return fuse_reply_buf(req, NULL, 0);
    }

    void FuseLFS::dir_buf_add(fuse_req_t req, struct dir_buf* buf,
                              const char *name, fuse_ino_t ino)
    {
        struct stat stbuf = {0};
        size_t oldsize = buf->size;
        buf->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
        buf->p = (char *) realloc(buf->p, buf->size);
        memset(&stbuf, 0, sizeof(stbuf));
        stbuf.st_ino = ino;
        fuse_add_direntry(req, buf->p + oldsize, buf->size - oldsize, name, &stbuf,
        buf->size);
    }

    /**
     * Create the filesystem and bring it into the initial state
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::mkfs() {
        output(std::cout, "writing super block..");

        if(write_superblock() != FLFS_RET_NONE) {
            output(std::cerr, "Failed to write super block, check",
                   "NvmeZns backend");
            return FLFS_RET_ERR;
        }

        // Write initial checkpoint block
        uint64_t randz_lba = 0;
        position_to_lba(RANDZ_POS, randz_lba);

        uint64_t res_sector;
        struct checkpoint_block cblock = {randz_lba};
        if(nvme->append(CBLOCK_POS.zone, res_sector, CBLOCK_POS.offset,
                        &cblock, sizeof(cblock)) != 0) {
            output(std::cerr, "Failed to write checkpoint block, check",
                   "NvmeZns backend");
            return FLFS_RET_ERR;
        }

        if(res_sector != CBLOCK_POS.sector) {
            output(std::cerr, "Initial checkpoint block written to wrong"
                   "location, check append operations");
            return FLFS_RET_ERR;
        }

        cblock_pos.zone = CBLOCK_POS.zone;
        cblock_pos.sector = CBLOCK_POS.sector;
        cblock_pos.offset = CBLOCK_POS.offset;
        cblock_pos.size = CBLOCK_POS.size;

        return FLFS_RET_NONE;
    }

    /**
     * Read the super block for the filesystem and verify the parameters to
     * prevent overwritten a drive configured for other filesystems.
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::verify_superblock() {
        struct super_block sblock;
        if(nvme->read(SBLOCK_POS.zone, SBLOCK_POS.sector, SBLOCK_POS.offset,
                      &sblock, sizeof(super_block)) != 0)
            return FLFS_RET_ERR;
        if(sblock.magic_cookie != MAGIC_COOKIE)
            return FLFS_RET_ERR;
        if(sblock.zones != nvme_info.num_zones)
            return FLFS_RET_ERR;
        if(sblock.sectors != nvme_info.zone_size)
            return FLFS_RET_ERR;
        if(sblock.sector_size != nvme_info.sector_size)
            return FLFS_RET_ERR;

        return FLFS_RET_NONE;
    }

    /**
     * Write the super block so it can be recognized on subsequent
     * initializations.
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::write_superblock() {
        struct super_block sblock;
        sblock.magic_cookie = MAGIC_COOKIE;
        sblock.zones = nvme_info.num_zones;
        sblock.sectors = nvme_info.zone_size;
        sblock.sector_size = nvme_info.sector_size;

        uint64_t sector;
        if(nvme->append(SBLOCK_POS.zone, sector, SBLOCK_POS.offset, &sblock,
                        sizeof(super_block)) != 0)
            return FLFS_RET_ERR;

        if(sector != SBLOCK_POS.sector)
            return FLFS_RET_ERR;

        return FLFS_RET_NONE;
    }

    /**
     * Checks if the filesystem was left dirty from last time
     * @return 0 when clean, < 0 if dirty
     */
    int FuseLFS::verify_dirtyblock() {
        struct dirty_block dblock = {0};

        // If we can't read the dirty block assume it is unwritten thus clean
        if(nvme->read(DBLOCK_POS.zone, DBLOCK_POS.sector, DBLOCK_POS.offset,
                      &dblock, sizeof(dblock)) != 0)
            return FLFS_RET_NONE;

        // -1 if dirty or 0 otherwise
        return dblock.is_dirty == 1 ? -1 : 0;
    }

    /**
     * Write the dirty block to the drive and verify it was appended to the
     * correct location.
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::write_dirtyblock() {
        uint64_t sector;

        struct dirty_block dblock = {0};
        dblock.is_dirty = 1;

        if(nvme->append(DBLOCK_POS.zone, sector, DBLOCK_POS.offset, &dblock,
                     sizeof(dblock)) != 0)
            return FLFS_RET_ERR;

        if(sector != DBLOCK_POS.sector)
            return FLFS_RET_ERR;

        return FLFS_RET_NONE;
    }

    /**
     * Reset the zone containing the dirty block
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::remove_dirtyblock() {
        if(nvme->reset(DBLOCK_POS.zone) != 0)
            return FLFS_RET_ERR;

        return FLFS_RET_NONE;
    }

    /**
     * Write the new start of the random zone to the new checkpoint block
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::update_checkpointblock(uint64_t randz_lba) {
        struct data_position tmp_cblock_pos = cblock_pos;

        // cblock_pos not set yet, somehow update is called before mkfs() or
        // get_checkpointblock()
        if(!cblock_pos.valid())
            return FLFS_RET_ERR;

        // If the current checkpoint block lives on the last sector in the zone
        // Move to the next zone (with rollover back to initial zone).
        if(cblock_pos.sector == nvme_info.zone_capacity - 1) {

            // Tick Tock between initial zone and subsequent zone
            if(CBLOCK_POS.zone == cblock_pos.zone)
                tmp_cblock_pos.zone = cblock_pos.zone + 1;
            else tmp_cblock_pos.zone = CBLOCK_POS.zone;

            tmp_cblock_pos.sector = 0;
        }
        // Otherwise, just advance the sector by 1
        else {
            tmp_cblock_pos.sector += 1;
        }

        uint64_t res_sector;
        struct checkpoint_block cblock = {randz_lba};
        if(nvme->append(tmp_cblock_pos.zone, res_sector, tmp_cblock_pos.offset,
                        &cblock, tmp_cblock_pos.size) != 0)
            return FLFS_RET_ERR;

        if(tmp_cblock_pos.sector != res_sector)
            return FLFS_RET_ERR;

        // If zone changed, reset old zone
        if(tmp_cblock_pos.zone != cblock_pos.zone)
            if(nvme->reset(cblock_pos.zone) != 0)
                return FLFS_RET_ERR;

        cblock_pos = tmp_cblock_pos;

        return FLFS_RET_NONE;
    }

    /**
     * Fetch the known location of the current checkpoint block or call
     * get_checkpointblock_locate.
     */
    int FuseLFS::get_checkpointblock(struct checkpoint_block &cblock) {
        if(!cblock_pos.valid())
            return get_checkpointblock_locate(cblock);

        struct checkpoint_block tmp_cblock = {0};
        if(nvme->read(cblock_pos.zone, cblock_pos.sector, cblock_pos.offset,
                   &tmp_cblock, sizeof(cblock)) != 0)
            return FLFS_RET_ERR;

        cblock = tmp_cblock;

        return FLFS_RET_NONE;
    }

    /**
     * Determine the location of the most up to date checkpoint block by
     * scanning the checkpoint zones and return its data. Must set cblock_pos
     * member variable to speed up future lookups.
     */
    int FuseLFS::get_checkpointblock_locate(struct checkpoint_block &cblock) {
        struct data_position tmp_cblock_pos = {0};
        struct checkpoint_block tmp_cblock = {0};
        uint64_t checkpoint_zone = CBLOCK_POS.zone;
        // Random zone could never start at absolute index limit of drive.
        tmp_cblock.randz_lba = UINT64_MAX;

        // Checkpoints on first zone
        if(nvme->read(CBLOCK_POS.zone, CBLOCK_POS.sector, CBLOCK_POS.offset,
                      &tmp_cblock, sizeof(cblock)) == 0)
            checkpoint_zone = CBLOCK_POS.zone;
        // Checkpoints on second zone
        else if(nvme->read(CBLOCK_POS.zone + 1, CBLOCK_POS.sector,
                           CBLOCK_POS.offset, &tmp_cblock, sizeof(cblock)) == 0)
            checkpoint_zone = CBLOCK_POS.zone + 1;
        else
            return FLFS_RET_ERR;

        // Read until no more checkpoint
        for(uint64_t i = 0; i < nvme_info.zone_capacity; i++) {
            // Read each subsequent block until it fails once
            if(nvme->read(checkpoint_zone, i, CBLOCK_POS.offset,
                          &tmp_cblock, sizeof(cblock)) != 0)
                break;

            tmp_cblock_pos = {checkpoint_zone, i, 0, CBLOCK_POS.size};

            // If we could read the last sector of the first zone we should
            // continue reading the first sector of the second zone.
            if(i == nvme_info.zone_capacity - 1 && checkpoint_zone ==
               CBLOCK_POS.zone)
            {
                if(nvme->read(CBLOCK_POS.zone + 1, CBLOCK_POS.sector,
                           CBLOCK_POS.offset, &tmp_cblock, sizeof(cblock)) == 0)
                {
                    output(std::cerr, "Restored checkpoint block from "
                           "power loss");
                    tmp_cblock_pos = {CBLOCK_POS.zone + 1, CBLOCK_POS.sector,
                                      0, CBLOCK_POS.size};
                }
            }
        }

        // Could not find any checkpoint block on drive
        if(tmp_cblock.randz_lba == UINT64_MAX)
            return FLFS_RET_ERR;

        // Update the located block position
        cblock_pos = tmp_cblock_pos;

        // Update the checkpoint block data
        cblock = tmp_cblock;

        return FLFS_RET_NONE;
    }

    /**
     * Find and set random_ptr starting from random_pos. This is the first
     * unreadable (unwritten) sector from random_pos with overflow from
     * RANDZ_BUFF_POS back to RANDZ_POS (The random zone is interpreted as
     * linearly contiguous from random_pos up until random_pos).
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::determine_random_ptr() {
        struct data_position end_pos = random_pos;
        struct data_position current_pos = random_pos;

        #ifdef QEMUCSD_DEBUG
        // Should always be valid after initialization
        if(!random_pos.valid())
            return FLFS_RET_ERR;
        #endif

        if(current_pos.zone == RANDZ_POS.zone) {
            end_pos.zone = RANDZ_BUFF_POS.zone;
            end_pos.sector = 0;
        }
        else {
            end_pos.zone = current_pos.zone -1;
            end_pos.sector = nvme_info.zone_capacity -1;
        }

        std::array<uint8_t, sizeof(none_block)> data{0};
        while(current_pos != end_pos) {
            if(nvme->read(current_pos.zone, current_pos.sector,
                       current_pos.offset, data.data(),
                       sizeof(none_block)) != 0)
                break;

            current_pos.sector += 1;

            // Reached end of sectors in current zone, advance zone
            if(current_pos.sector == nvme_info.zone_capacity) {
                current_pos.sector = 0;
                current_pos.zone += 1;
            }

            // Reached end of random zone overflow back to beginning but only if
            // this is not the end_pos.
            if(current_pos.zone == RANDZ_BUFF_POS.zone &&
               end_pos.zone != RANDZ_BUFF_POS.zone)
                current_pos.zone = RANDZ_POS.zone;
        }

        // Random zone is full and random_ptr is invalid
        if(current_pos == end_pos) {
            current_pos.size = 0;
            random_ptr = current_pos;
            return FLFS_RET_RANDZ_FULL;
        }

        random_ptr = current_pos;
        return FLFS_RET_NONE;
    }

    /**
     * Compute the distance between the data_positions in the random zone. This
     * zone behaves as contiguous with overflow / wrap around at RANDZ_BUFF_POS
     */
    void FuseLFS::random_zone_distance(
        struct data_position lhs, struct data_position rhs, uint32_t &distance)
    {
        distance = (lhs.zone - RANDZ_BUFF_POS.zone + rhs.zone) %
            RANDZ_BUFF_POS.zone;
    }

    /**
     * Read the entire random zone and reconstruct the inode_lba map. Can only
     * be called after random_pos has been restored from checkpoint blocks.
     * TODO(Dantali0n): Process and reconstruct SIT map.
     */
    int FuseLFS::read_random_zone(inode_lba_map_t *inode_map) {
        auto current_pos = random_pos;
        struct none_block nt_blk = {0};

        // Go through all positions in the random zone
        do {
            // Stop reading after first unreadable sector
            if(nvme->read(current_pos.zone, current_pos.sector,
                          current_pos.offset, &nt_blk, sizeof(nt_blk)) != 0)
                break;

            // Type is nat_block
            if(nt_blk.type == RANDZ_NAT_BLK) {
                auto nat_blk = reinterpret_cast<nat_block *>(&nt_blk);

                for(uint32_t i = 0; i < NAT_BLK_INO_LBA_NUM; i++) {
                    if(nat_blk->inode[i] == 0)
                        break;

                    uint64_t inode = nat_blk->inode[i];
                    uint64_t lba = nat_blk->lba[i];
                    inode_map->insert_or_assign(inode, lba);
                }
            }
            // TODO(Dantali0n): Process SIT blocks.

            current_pos.sector += 1;

            // Reached end of sectors in current zone, advance zone
            if(current_pos.sector == nvme_info.zone_capacity) {
                current_pos.sector = 0;
                current_pos.zone += 1;
            }

            // Reached end of random zone overflow back to beginning.
            if(current_pos.zone == RANDZ_BUFF_POS.zone)
                current_pos.zone = RANDZ_POS.zone;

        } while(current_pos != random_pos);

        return FLFS_RET_NONE;
    }

    /**
     * Fill a nat_block until it is full removing any appendices from nat_set or
     * when nat_set is empty.
     */
    void FuseLFS::fill_nat_block(
        nat_update_set_t *nat_set, struct nat_block &nt_blk)
    {
        uint16_t i = 0;
        for(auto &nt : *nat_set) {
            nt_blk.inode[i] = nt;
            nt_blk.lba[i] = inode_lba_map.at(nt);

            i++;

            // Block has reached limit
            if(i == NAT_BLK_INO_LBA_NUM) {
                break;
            }
        }

        for(uint32_t i = 0 ; i < NAT_BLK_INO_LBA_NUM; i++) {
            if(nt_blk.inode[i] == 0) break;
            nat_set->erase(nt_blk.inode[i]);
        }
    }

    /**
     * Take whatever random zone block was given and append it to the random
     * zone. The location depends on random_ptr which is advanced after a
     * successful append. random_pos is taken into account to identify if we
     * are out of random zone space.
     *
     * @return FLFS_RET_NONE upon success, < FLFS_RET_ERR upon failure,
     *         FLFS_RET_RANDZ_FULL when random zone full.
     */
    int FuseLFS::append_random_block(struct rand_block_base &block) {
        struct data_position tmp_random_ptr = random_ptr;

        #ifdef QEMUCSD_DEBUG
        // Should always be valid after initialization, set by
        // determine_random_ptr
        if(!random_ptr.valid())
            return FLFS_RET_ERR;

        // random_pos should always be set to RANDZ_POS once RANDZ_BUFF_POS is
        // reached.
        if(random_pos.zone == RANDZ_BUFF_POS.zone)
            return FLFS_RET_ERR;
        #endif

        // Don't flush none blocks to drive
        if(block.type == RANDZ_NON_BLK)
            return FLFS_RET_ERR;

        uint64_t res_sector;
        if(nvme->append(tmp_random_ptr.zone, res_sector, tmp_random_ptr.offset,
                        &block,sizeof(none_block)) != 0)
            return FLFS_RET_ERR;

        // Check that the append was written to the right location
        if(res_sector != tmp_random_ptr.sector)
            return FLFS_RET_ERR;

        // Advance the sector
        tmp_random_ptr.sector += 1;

        // Current zone full, find next zone
        if(res_sector + 1 == nvme_info.zone_capacity) {
            tmp_random_ptr.zone += 1;
            tmp_random_ptr.sector = 0;

            // If reached RAND_BUFF_POS overflow to RANDZ_POS
            if (tmp_random_ptr.zone == RANDZ_BUFF_POS.zone)
                tmp_random_ptr.zone = RANDZ_POS.zone;

            // Out of random zone space!
            if (tmp_random_ptr.zone == random_pos.zone) {
                //if(rewrite_random_blocks() != FLFS_RET_NONE)

                // Invalidate the random_ptr
                random_ptr.size = 0;
                return FLFS_RET_RANDZ_FULL;
            }
        }

        random_ptr = tmp_random_ptr;

        return FLFS_RET_NONE;
    }

    /**
     * Determine the number of nat blocks required to flush the nat_set to
     * the drive. The result is placed in num_blocks.
     */
    void FuseLFS::compute_nat_blocks(
        nat_update_set_t *nat_set, uint64_t &num_blocks)
    {
        uint64_t set_size = nat_set->size();
        num_blocks = 0;
        num_blocks += set_size / NAT_BLK_INO_LBA_NUM;
        num_blocks += set_size % NAT_BLK_INO_LBA_NUM == 0 ? 0 : 1;
    }

    /**
     * Perform a flush to drive of all changes that happened to nat blocks
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure,
     *         FLFS_RET_RANDZ_FULL when random zone full.
     */
    int FuseLFS::update_nat_blocks(
        nat_update_set_t *nat_set = &nat_update_set)
    {
        int ret = 0;

        // Create an instance of each random zone block type
        struct nat_block nt_blk = {0};

        while(nat_set->empty() == false) {
            nt_blk.type = RANDZ_NAT_BLK;
            fill_nat_block(nat_set, nt_blk);
            if(nt_blk.inode[0] == 0) break;

            // Error code of append random block might signal lack of space
            ret = append_random_block(nt_blk);
            if(ret != FLFS_RET_NONE) return ret;

            memset(&nt_blk, 0, sizeof(nat_block));
        }

        return FLFS_RET_NONE;
    }

    /**
     * Buffer the two zones into the random buffer
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFS::buffer_random_blocks(
        const uint64_t zones[2], struct data_position limit)
    {
        // Refuse to copy zones from outside the random zone
        if(zones[0] >= RANDZ_BUFF_POS.zone || zones[0] < RANDZ_POS.zone)
            return FLFS_RET_ERR;
        if(zones[1] >= RANDZ_BUFF_POS.zone || zones[1] < RANDZ_POS.zone)
            return FLFS_RET_ERR;

        uint64_t res_sector;
        uint64_t sector_size = nvme_info.sector_size;
        auto data = (uint8_t*) malloc(sector_size);

        for(uint64_t j = 0; j < 2; j++) {
            uint64_t zone = zones[j];
            for (uint64_t i = 0; i < nvme_info.zone_capacity; i++) {
                if(limit.meets_limit(zone, i))
                    goto buff_rand_blk_none;
                if (nvme->read(zone, i, 0, data, sector_size) != 0)
                    goto buff_rand_blk_err;
                if (nvme->append(RANDZ_BUFF_POS.zone + j, res_sector, 0,
                                 data, sector_size) != 0)
                    goto buff_rand_blk_err;
                if(res_sector != i)
                    goto buff_rand_blk_err;
            }
        }

        buff_rand_blk_none:
        free(data);
        return FLFS_RET_NONE;

        buff_rand_blk_err:
        free(data);
        return FLFS_RET_ERR;
    }

    /**
     * Go through the current contents of the random buffer adding found data
     * to nat_set if it is still present in inodes. Modifies both nat_set and
     * inodes.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFS::process_random_buffer(
        nat_update_set_t *nat_set, inode_lba_map_t *inodes) {
        struct none_block nt_blk = {0};

        // Read all the data in the buffered zones
        for(uint32_t i = 0; i < N_RAND_BUFF_ZONES; i++) {
            for(uint32_t j = 0; j < nvme_info.zone_capacity; j++) {
                if(nvme->read(RANDZ_BUFF_POS.zone + i, j, 0, &nt_blk,
                              sizeof(none_block)) != 0)
                    return FLFS_RET_ERR;

                // Process NAT block
                if(nt_blk.type == RANDZ_NAT_BLK) {
                    auto nat_blk = reinterpret_cast<nat_block *>(&nt_blk);
                    // Iterate over each inode in the block
                    for(uint32_t z = 0; z < NAT_BLK_INO_LBA_NUM; z++) {
                        // If we have yet to rewrite this inode add it to the set
                        // and removes it from our map of pending updates.
                        auto it = inodes->find(nat_blk->inode[z]);

                        #ifdef FLFS_RANDOM_RW_STRICT
                        if(it != inode_lba_copy.end() && it->second == nat_blk->lba[z]) {
                        #else
                        if(it != inodes->end()) {
                        #endif
                            nat_set->insert(nat_blk->inode[z]);
                            // Reuse iterator for erase, prevents additional lookup
                            inodes->erase(it);
                        }
                    }
                }
                // Process SIT block
                // TODO(Dantali0n): SIT blocks
            }
        }

        return FLFS_RET_NONE;
    }

    /**
     * Erase the random buffer from the drive
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::erase_random_buffer() {
        if(nvme->reset(RANDZ_BUFF_POS.zone) != 0)
            return FLFS_RET_ERR;
        if(nvme->reset(RANDZ_BUFF_POS.zone + 1) != 0)
            return FLFS_RET_ERR;
        return FLFS_RET_NONE;
    }

    /**
     * Perform random zone rewrites deciding upon the appropriate strategy based
     * on how full the zone is.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon error and
     *         FLFS_RET_RANDZ_INSUFFICIENT if no random zone data can be freed.
     */
    int FuseLFS::rewrite_random_blocks() {
        // No blocks written nothing to do
        if(random_ptr == random_pos)
            return FLFS_RET_NONE;

        #ifdef QEMUCSD_DEBUG
        // random_pos should always be set to RANDZ_POS once RANDZ_BUFF_POS is
        // reached.
        if(random_pos.zone == RANDZ_BUFF_POS.zone)
            return FLFS_RET_ERR;
        #endif

        // Check if random zone is fully written
        if(!random_ptr.valid())
            return rewrite_random_blocks_full();

        uint32_t distance;
        random_zone_distance(random_pos, random_ptr, distance);
        // TODO(Dantali0n): Support partial random zone rewrites
        if(distance > N_RAND_BUFF_ZONES)
            return FLFS_RET_NONE;
//            return rewrite_random_blocks_partial();

        return FLFS_RET_NONE;
    }

    /**
     * rewrite the random zone if it is only partially filled with data.
     * TODO(Dantali0n): Finish this method
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure,
     */
    int FuseLFS::rewrite_random_blocks_partial() {
        uint32_t distance;
        random_zone_distance(random_pos, random_ptr, distance);
        while(distance > N_RAND_BUFF_ZONES) {


            random_zone_distance(random_pos, random_ptr, distance);
        }

        return FLFS_RET_NONE;
    }

    /**
     * Rewrite the entirely full random zone
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure,
     *         FLFS_RET_RANDZ_INSUFFICIENT if no data could be freed.
     */
    int FuseLFS::rewrite_random_blocks_full() {
        uint64_t zones[N_RAND_BUFF_ZONES] = {0};

        // Create a copy of all inodes and remove every encountered one.
        inode_lba_map_t inode_lba_copy = inode_lba_map;

        uint32_t steps = (RANDZ_BUFF_POS.zone - RANDZ_POS.zone) /
            N_RAND_BUFF_ZONES;
        #ifdef QEMUCSD_DEBUG
            if((RANDZ_BUFF_POS.zone - RANDZ_POS.zone) % N_RAND_BUFF_ZONES != 0)
                return FLFS_RET_ERR;
        #endif
        for(uint32_t i = 0; i < steps; i++) {
            /** 1. Copy from random_pos into the random buffer */
            zones[0] = random_pos.zone;
            zones[1] = random_pos.zone + 1;

            if(buffer_random_blocks(zones, RANDZ_BUFF_POS) != FLFS_RET_NONE)
                return FLFS_RET_ERR;

            /** 2. resets the copied zones */
            if(nvme->reset(zones[0]) != 0) return FLFS_RET_ERR;
            if(nvme->reset(zones[1]) != 0) return FLFS_RET_ERR;

            /** 3. advance the random_pos (randz_lba) and put this in a
             * checkpoint block */

            random_pos.zone += 2;
            if(random_pos.zone > RANDZ_BUFF_POS.zone)
                random_pos.zone = RANDZ_POS.zone + 1;
            else if(random_pos.zone == RANDZ_BUFF_POS.zone)
                random_pos.zone = RANDZ_POS.zone;

            uint64_t new_randz_lba;
            position_to_lba(random_pos, new_randz_lba);
            if(update_checkpointblock(new_randz_lba) != FLFS_RET_NONE)
                return FLFS_RET_ERR;

            /** 3.1. If the random_ptr is still or became invalid again
             * point it to the newly writeable region */

            if(!random_ptr.valid()) {
                random_ptr.zone = zones[0];
                random_ptr.sector = 0;

                // Make the random_ptr valid
                random_ptr.size = SECTOR_SIZE;
            }

            uint64_t nat_blocks;
            uint64_t available_blocks = N_RAND_BUFF_ZONES * nvme_info.zone_capacity;
            nat_update_set_t nat_set = nat_update_set_t();

            if(process_random_buffer(&nat_set, &inode_lba_copy) != FLFS_RET_NONE)
                return FLFS_RET_ERR;

            compute_nat_blocks(&nat_set, nat_blocks);
            available_blocks -= nat_blocks;

            uint64_t sit_blocks;
            // TODO(Dantali0n): SIT blocks

            // Worst case, Zones must be entirely rewritten no extra free space
            // claimed. This will cause append_random_block to return
            // FLFS_RET_RANDZ_FULL which is propagated by update_nat_blocks.
            // This in turn changes what return code we should consider as
            // error.
            if(available_blocks == 0) {
                if(update_nat_blocks(&nat_set) < FLFS_RET_NONE)
                    return FLFS_RET_ERR;
            }
            else {
                if(update_nat_blocks(&nat_set) != FLFS_RET_NONE)
                    return FLFS_RET_ERR;
            }

            /** 6. Erase the random buffer */
            if(erase_random_buffer() != FLFS_RET_NONE)
                return FLFS_RET_ERR;
        }

        if(!random_ptr.valid()) {
            output.fatal("Insufficient random zone space! linear rewrite ",
                         "occupied entire zone!!");
            return FLFS_RET_RANDZ_INSUFFICIENT;
        }

        return FLFS_RET_NONE;
    }

//    /**
//     * Call when the random zone is completely full. 1. Copies the first two
//     * zones starting from current randz_lba and migrates these into the random
//     * buffer. 2. Resets the zones that where copied. 3. Advances randz_lba
//     * using a new checkpoint block. 3.1 If random_ptr was invalid (random zone
//     * full) change the random_ptr to the new writeable region.
//     * 4. Identifies most up to date unique content in buffer and adds these to
//     * temporal datastructures. 5. Flush temporal data to drive. 6. Erase the
//     * random buffers 7. Repeats until all unique random zone data is linearly
//     * flushed to the drive (occupies the minimal required amount of space).
//     *
//     * TODO(Dantali0n): Rewrite this entirely, its a huge disgusting heap of
//     *                  trash...
//     * @return 0 upon success, < 0 upon failure
//     */
//    int FuseLFS::rewrite_random_blocks() {
//        // Number of supported zones in rand buff currently hardcoded
//        // Can be computed using LOG_POS.zone = RANDZ_BUFF_POS.zone
//        static constexpr uint8_t n_rand_buf_zones = 2;
//
//        #ifdef QEMUCSD_DEBUG
//        // random_pos should always be set to RANDZ_POS once RANDZ_BUFF_POS is
//        // reached.
//        if(random_pos.zone == RANDZ_BUFF_POS.zone)
//            return FLFS_RET_ERR;
//        #endif
//
//        if(random_ptr == random_pos)
//            return FLFS_RET_NONE;
//
//        bool not_reached_end_pos = true;
////        struct data_position initial_random_pos = random_pos;
//        struct data_position tmp_random_pos = random_pos;
//
//        // Limit of data, buffering random blocks should not continue at this
//        // position.
//        const struct data_position data_lim_pos = random_ptr;
//
//        // Final rand_pos, rewrite is done upon reaching it.
//        struct data_position end_rand_pos = random_ptr;
//
//        // We need to copy any partial zones entirely as the random_pos is
//        // aligned on zone boundaries.
//        if(end_rand_pos.sector != 0) {
//            end_rand_pos.sector = 0;
//            end_rand_pos.zone += 1;
//
//            // Ensure end_rand_pos is valid position
//            end_rand_pos.size = SECTOR_SIZE;
//        }
//        // advancing random_pos happens with zones in multiple of two
//        // align end_rand_pos to match
//        if((RANDZ_POS.zone - end_rand_pos.zone) % 2 != 0)
//            end_rand_pos.zone += 1;
//
//        #ifdef QEMUCSD_DEBUG
//        // It should at most be set equal to RANDZ_BUFF_POS
//        if(end_rand_pos.zone > RANDZ_BUFF_POS.zone)
//            return FLFS_RET_ERR;
//        #endif
//
//        uint64_t zones[n_rand_buf_zones] = {0};
//
//        // Create a copy of all inodes and remove every encountered one.
//        // copy of map will be performant with log(n) lookup time
//        // (rb bin tree).
//        inode_lba_map_t inode_lba_copy = inode_lba_map;
//
//        // Rewrite parts of the random zone until random_pos meets end_rand_pos
//        /** 7. Repeat until all data has been linearly rewritten */
//        while(not_reached_end_pos) {
//
//            /** 1. Copy from random_pos into the random buffer */
//            zones[0] = tmp_random_pos.zone;
//            zones[1] = tmp_random_pos.zone + 1;
//
//            // Wrap around if random_pos is on the border (this can only happen
//            // if RANDZ_BUFF_POS - RANDZ_POS is not a multiple of 2)
//            if(zones[1] == RANDZ_BUFF_POS.zone) {
//                output.warning("number of random zones not multiple of two ",
//                               "configuration not supported!");
//                zones[1] = RANDZ_POS.zone;
//            }
//
//            if(buffer_random_blocks(zones, data_lim_pos) != FLFS_RET_NONE)
//                return FLFS_RET_ERR;
//
//            /** 2. resets the copied zones */
//            if(nvme->reset(zones[0]) != 0) return FLFS_RET_ERR;
//            if(nvme->reset(zones[1]) != 0) return FLFS_RET_ERR;
//
//            // If we reset the zone with the random_ptr invalidate the ptr
//            if(zones[0] == random_ptr.zone || zones[1] == random_ptr.zone)
//                random_ptr.size = 0;
//
//            /** 3. advance the random_pos (randz_lba) and put this in a
//             * checkpoint block */
//
//            tmp_random_pos.zone += 2;
//           // Detect termination
//            if(tmp_random_pos == end_rand_pos)
//                not_reached_end_pos = false;
//
//            if(tmp_random_pos.zone > RANDZ_BUFF_POS.zone)
//                tmp_random_pos.zone = RANDZ_POS.zone + 1;
//            else if(tmp_random_pos.zone == RANDZ_BUFF_POS.zone)
//                tmp_random_pos.zone = RANDZ_POS.zone;
//
//            uint64_t new_randz_lba;
//            position_to_lba(tmp_random_pos, new_randz_lba);
//            if(update_checkpointblock(new_randz_lba) != FLFS_RET_NONE)
//                return FLFS_RET_ERR;
//
//            random_pos = tmp_random_pos;
//
//            /** 3.1. If random zone was full (random_ptr invalid) set random_ptr
//             * to start of newly writeable region */
//
//            if(!random_ptr.valid() && data_lim_pos < random_pos) {
//                random_ptr.zone = zones[0];
//                random_ptr.sector = 0;
//
//                // Make the random_ptr valid
//                random_ptr.size = SECTOR_SIZE;
//            }
//
//            /** 4. Identify unique data in random buffer */
//            struct none_block nt_blk = {0};
//
//            // Subset of inodes to current flush to drive
//            nat_update_set_t nat_set = nat_update_set_t();
//
//            // Total spendable blocks freed from moving data into random buffer
//            uint64_t available_blocks = n_rand_buf_zones * nvme_info.zone_capacity;
//
//            // Read all the data in the buffered zones
//            for(uint32_t i = 0; i < n_rand_buf_zones; i++) {
//                for(uint32_t j = 0; j < nvme_info.zone_capacity; j++) {
//
//                    // If we reach the limit skip the remaining part of the
//                    // buffer
//                    if(data_lim_pos.meets_limit(zones[i], j))
//                        goto skip_remaining_buffer;
//
//                    if(nvme->read(RANDZ_BUFF_POS.zone + i, j, 0, &nt_blk,
//                                  sizeof(none_block)) != 0)
//                        return FLFS_RET_ERR;
//
//                    // Process NAT block
//                    if(nt_blk.type == RANDZ_NAT_BLK) {
//                        auto nat_blk = reinterpret_cast<nat_block *>(&nt_blk);
//                        // Iterate over each inode in the block
//                        for(uint32_t z = 0; z < NAT_BLK_INO_LBA_NUM; z++) {
//                            // If we have yet to rewrite this inode add it to the set
//                            // and removes it from our map of pending updates.
//                            auto it = inode_lba_copy.find(nat_blk->inode[z]);
//
//                            #ifdef FLFS_RANDOM_RW_STRICT
//                            if(it != inode_lba_copy.end() && it->second == nat_blk->lba[z]) {
//                            #else
//                            if(it != inode_lba_copy.end()) {
//                            #endif
//                                nat_set.insert(nat_blk->inode[z]);
//                                // Reuse iterator for erase, prevents additional lookup
//                                inode_lba_copy.erase(it);
//                            }
//                        }
//                    }
//                    // Process SIT block
//                    // TODO(Dantali0n): SIT blocks
//                }
//            }
//
//            /** 5. Flush temporal data to the drive */
//
//            skip_remaining_buffer:
//
//            uint64_t nat_blocks;
//            compute_nat_blocks(&nat_set, nat_blocks);
//            available_blocks -= nat_blocks;
//
//            uint64_t sit_blocks;
//            // TODO(Dantali0n): SIT blocks
//
//            // Worst case, Zones must be entirely rewritten no extra free space
//            // claimed. This will cause append_random_block to return
//            // FLFS_RET_RANDZ_FULL which is propagated by update_nat_blocks.
//            // This in turn changes what return code we should consider as
//            // error.
//            if(available_blocks == 0) {
//                if(update_nat_blocks(&nat_set) < FLFS_RET_NONE)
//                    return FLFS_RET_ERR;
//            }
//            else {
//                if(update_nat_blocks(&nat_set) != FLFS_RET_NONE)
//                    return FLFS_RET_ERR;
//            }
//
//            /** 6. Erase the random buffer */
//
//            // Random buffer can now safely be erased
//            if(erase_random_buffer() != FLFS_RET_NONE)
//                return FLFS_RET_ERR;
//        }
//
//        if(!random_ptr.valid()) {
//            output.fatal("Insufficient random zone space! linear rewrite ",
//                         "occupied entire zone!!");
//            return FLFS_RET_RANDZ_INSUFFICIENT;
//        }
//
//////         In the nearly impossible case the random zone entirely contains
//////         invalid data and the random_ptr.zone % 2 != 0 then and only then will
//////         the random_pos incorrectly overtake the random_ptr. We test for data
//////         at the start of random_pos and if it can't be read reset random_pos
//////         back to random_ptr. Perhaps this can happen if you erase the
//////         entire filesystem contents.
////        if(random_ptr != random_pos) {
////            auto *data = (uint8_t*) malloc(SECTOR_SIZE);
////            if(nvme->read(random_pos.zone, random_pos.sector, random_pos.offset,
////                          data, SECTOR_SIZE) != 0){
////    //            output.warning("random zone does not contain any valid data ",
////    //                           "after rewrite, has the filesystem become empty?!");
////                random_pos = initial_random_pos;
////            }
////            free(data);
////        }
//
//        return FLFS_RET_NONE;
//    }

    void FuseLFS::init(void *userdata, struct fuse_conn_info *conn) {
        connection = conn;
    }

    void FuseLFS::destroy(void *userdata) {
        output(std::cout, "Tearing down filesystem");

        if(remove_dirtyblock() != FLFS_RET_NONE) {
            output(std::cerr, "Failed to remove dirty block from drive"
                   " this will cause issues on subsequent mounts!");
        }
    }

    /**
     * Lookup the given inode for the parent, name pair and use ino_stat
     * to fill out information about the found inode.
     * TODO(Dantali0n): Make lookup use path_to_inode function.
     */
    void FuseLFS::lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
        struct fuse_entry_param e;
        if (parent != 1 || strcmp(name, "test") != 0)
            fuse_reply_err(req, ENOENT);
        else {
            memset(&e, 0, sizeof(e));
            e.ino = 2;
            e.attr_timeout = 1.0;
            e.entry_timeout = 1.0;
            ino_stat(e.ino, &e.attr);

            fuse_reply_entry(req, &e);
        }
    }

    void FuseLFS::getattr(fuse_req_t req, fuse_ino_t ino,
                          struct fuse_file_info *fi)
    {
        struct stat stbuf = {0};
        if (ino_stat(ino, &stbuf) == -1)
            fuse_reply_err(req, ENOENT);
        else
            fuse_reply_attr(req, &stbuf, 1.0);
    }

    void FuseLFS::readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t offset, struct fuse_file_info *fi)
    {
        if(ino != 1) {
            fuse_reply_err(req, ENOTDIR);
            return;
        }

        struct dir_buf buf;

        memset(&buf, 0, sizeof(buf));
        dir_buf_add(req, &buf, ".", 1);
        dir_buf_add(req, &buf, "..", 1);
        dir_buf_add(req, &buf, "test", 2);
        reply_buf_limited(req, buf.p, buf.size, offset, size);
        free(buf.p);
    }

    void FuseLFS::open(fuse_req_t req, fuse_ino_t ino,
                      struct fuse_file_info *fi)
    {
        // Inode 1 is root directory
        if (ino == 1) {
            fuse_reply_err(req, EISDIR);
            return;
        }

        // If it is not inode 2 it does not exist.
        if(ino != 2) {
            fuse_reply_err(req, ENONET);
            return;
        }

        // Can only read files in this demo
        if ((fi->flags & O_ACCMODE) != O_RDONLY) {
            fuse_reply_err(req, EACCES);
            return;
        }

        // Open the file at inode 2
        fuse_reply_open(req, fi);
    }

    void FuseLFS::create(fuse_req_t req, fuse_ino_t parent, const char *name,
                        mode_t mode, struct fuse_file_info *fi)
    {
        return;
    }

    void FuseLFS::read(fuse_req_t req, fuse_ino_t ino, size_t size,
                       off_t offset, struct fuse_file_info *fi)
    {
        const fuse_ctx* context = fuse_req_ctx(req);

        // Non CSD path
        char *buffer = (char *) malloc(TEST_SIZE);

        for (size_t i = 0; i < offset; i++) {
            rand();
        }

        char value = 0;
        for (size_t i = 0; i < size; i++) {
            value = rand();
            memcpy(buffer + i, &value, 1);
        }

        reply_buf_limited(req, buffer, TEST_SIZE, offset, size);
    }

    void FuseLFS::write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                       size_t size, off_t off, struct fuse_file_info *fi)
    {
        return;
    }

    void FuseLFS::unlink(fuse_req_t req, fuse_ino_t parent, const char *name) {
        return;
    }

}