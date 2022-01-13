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

#include "flfs.hpp"
#include "nvme_zns_memory.hpp"

namespace qemucsd::fuse_lfs {

    struct fuse_conn_info* FuseLFS::connection = nullptr;

    struct nvme_zns::nvme_zns_info FuseLFS::nvme_info = {0};
    nvme_zns::NvmeZnsBackend* FuseLFS::nvme = nullptr;

    const std::string FuseLFS::FUSE_LFS_NAME_PREFIX = "[FUSE LFS] ";
    const std::string FuseLFS::FUSE_SEQUENTIAL_PARAM = "-s";

    output::Output FuseLFS::output =
        output::Output(FuseLFS::FUSE_LFS_NAME_PREFIX);

    inode_nlookup_map_t FuseLFS::inode_nlookup_map = inode_nlookup_map_t();

    path_inode_map_t FuseLFS::path_inode_map = path_inode_map_t();

    inode_lba_map_t FuseLFS::inode_lba_map = inode_lba_map_t();

    nat_update_set_t FuseLFS::nat_update_set = nat_update_set_t();

    data_blocks_t FuseLFS::data_blocks = data_blocks_t();

    inode_entries_t FuseLFS::inode_entries = inode_entries_t();

    open_inode_map_t FuseLFS::open_inode_map = open_inode_map_t();

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

    // Current start of the log zone on drive
    struct data_position FuseLFS::log_pos = {
        0, 0, 0, 0
    };

    // Current write pointer into the log zone
    struct data_position FuseLFS::log_ptr = {
        0, 0, 0, 0
    };

    // Will be set to at least 2 upon initialization as 0 is invalid and 1 is
    // root inode.
    fuse_ino_t FuseLFS::ino_ptr = 0;

    // In memory session so this always start at 1.
    uint64_t FuseLFS::fh_ptr = 1;

    const struct fuse_lowlevel_ops FuseLFS::operations = {
        .init       = FuseLFS::init,
        .destroy    = FuseLFS::destroy,
        .lookup     = FuseLFS::lookup,
        .forget     = FuseLFS::forget,
        .getattr    = FuseLFS::getattr,
        .setattr    = FuseLFS::setattr,
        .mkdir      = FuseLFS::mkdir,
        .unlink     = FuseLFS::unlink,
        .open	    = FuseLFS::open,
        .read       = FuseLFS::read,
        .write      = FuseLFS::write,
        .release    = FuseLFS::release,
//        .fsync      = FuseLFS::fsync,
        .readdir    = FuseLFS::readdir,
        .statfs     = FuseLFS::statfs,
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

        /** Check compiled sector size matches device sector size */
        if(SECTOR_SIZE != nvme_info.sector_size) {
            output.error("Compiled sector size ", SECTOR_SIZE, " does not ",
                         "match device sector size ", nvme_info.sector_size);
            ret = 1;
            goto err_out1;
        }

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
        if(determine_random_ptr() == FLFS_RET_RANDZ_FULL) {
            output.warning("Filesystem initialized while random zone still ",
                           "full, unclean shutdown attempting recovery..");
            if(rewrite_random_blocks() != FLFS_RET_NONE) {
                ret = 1;
                goto err_out1;
            }

            if(determine_random_ptr() != FLFS_RET_NONE) {
                ret = 1;
                goto err_out1;
            }
        }

        // Root inode
        output(std::cout, "Creating root inode..");
        path_inode_map.insert(std::make_pair(1, new path_map_t()));

        #if QEMUCSD_DEBUG
        output.info("Create debug test file in root directory");
        path_inode_map.find(1)->second->insert(std::make_pair("test", 2));
        inode_lba_map.insert_or_assign(2, 0);
        #endif

        /** Now that random_pos is known reconstruct inode lba map */
        // TODO(Dantali0n): Make this also reconstruct SIT blocks
        read_random_zone(&inode_lba_map);

        /** Find highest free inode number and keep track */
        for(auto &ino : inode_lba_map) {
            if(ino.first > ino_ptr) ino_ptr = ino.first + 1;
        }

        /** Determine the log write pointer */
        determine_log_ptr();

        /** With inodes available build path_inode_map now */
        // TODO(Dantali0n): Build path_inode_map
//        if(build_path_inode_map() != FLFS_RET_NONE) {
//            ret = 1;
//            goto err_out1;
//        }

        session = fuse_session_new(
            &args, &operations, sizeof(operations), nullptr);
        if (session == nullptr) {
            ret = 1;
            goto err_out1;
        }

        if (fuse_set_signal_handlers(session) != FLFS_RET_NONE) {
            ret = 1;
            goto err_out2;
        }

        if (fuse_session_mount(session, opts.mountpoint) !=
                FLFS_RET_NONE) {
            ret = 1;
            goto err_out3;
        }

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
     * Increment nlookup for the given inode by either creating the map
     * entry or incrementing the existing one.
     */
    void FuseLFS::inode_nlookup_increment(fuse_ino_t ino) {
        auto it = inode_nlookup_map.find(ino);
        if(it == inode_nlookup_map.end())
            inode_nlookup_map.insert(std::make_pair(ino, 1));
        else
            it->second += 1;
    }

    void FuseLFS::inode_nlookup_decrement(fuse_ino_t ino, uint64_t count) {
        auto it = inode_nlookup_map.find(ino);

        if(it == inode_nlookup_map.end()) {
            output.error("Requested to decrease nlookup count of inode that ",
                "has already reached count zero!");
            return;
        }

        #ifdef QEMUCSD_DEBUG
        if(count > it->second) {
            output.error("Attempting to decrease nlookup count more than ",
                         "current value!");
            // Can only subtract the current count maximum
            count = it->second;
        }
        #endif

        it->second -= count;

        // TODO(Dantali0n): Implement forget callbacks (unlink, rename, rmdir)
        //                  and fire these once this count reaches zero.
        if(it->second == 0)
            inode_nlookup_map.erase(it);
    }

    /**
     * Perform the necessary cache count management for inodes by tracking
     * the nlookup value by wrapping fuse_reply_entry.
     */
    void FuseLFS::fuse_reply_entry_nlookup(
        fuse_req_t req, struct fuse_entry_param *e)
    {
        inode_nlookup_increment(e->ino);
        //e->attr.st_nlink = inode_nlookup_map.find(e->ino)->second;
        fuse_reply_entry(req, e);
    }

    /**
     * Perform the necessary cache count management for inodes by tracking
     * the nlookup value by wrapping fuse_reply_create.
     */
    void FuseLFS::fuse_reply_create_nlookup(
        fuse_req_t req, struct fuse_entry_param *e,
        const struct fuse_file_info *f)
    {
        inode_nlookup_increment(e->ino);
        //e->attr.st_nlink = inode_nlookup_map.find(e->ino)->second;
        fuse_reply_create(req, e, f);
    }

    /**
     * Compute the position of data from a Logical Block Address (LBA).
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
     * Update an inode_lba_map_t with the provided lba for the vector of inodes
     */
    void FuseLFS::update_inode_lba_map(
        std::vector<fuse_ino_t> *inodes, uint64_t lba,
        inode_lba_map_t *lba_map = &inode_lba_map)
    {
        for(auto &ino : *inodes) {
            #ifdef QEMUCSD_DEBUG
            // Inode 0 is regarded as invalid
            // Inode 1 is hardcoded for root and should not be updated
            if(ino < 2) {
                output.error("Invalid attempt to insert inode ", ino, " into ",
                             "a inode_lba_map_t");
                continue;
            }
            #endif
            lba_map->insert_or_assign(ino, lba);
        }
    }

    /**
     * Modify position such that it is advanced by a single sector contextually.
     * Meaning that a position inside the random zone will never advance into
     * the random buffer. Alternatively a position in the checkpoint zone will
     * never advance into the random zone etc...
     * TODO(Dantali0n): Actually implement advance_position
     */
    void FuseLFS::advance_position(struct data_position &position) {

    }

    /**
     * Output information from the current fuse_file_info struct to stdout
     * using output level debug.
     */
    void FuseLFS::output_fi(const char *name, struct fuse_file_info *fi) {
        output.debug(
            "[", name, "] cache_readdir ", fi->cache_readdir ? 1 : 0,
            " writepage ", fi->writepage ? 1 : 0, " direct_io ",
            fi->direct_io ? 1 : 0, " keep_cache ", fi->keep_cache ? 1 : 0,
            " flush ", fi->flush ? 1 : 0, " nonseekable ",
            fi->nonseekable ? 1 : 0, " flock_release ",
            fi->flock_release ? 1 : 0);
    }

    /**
     * Find and fill the stbuf information for the given inode. Every FUSE call
     * that needs to determine if an inode exists should use this method.
     * TODO(Dantali0n): Cache these lookups as much as possible.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ENONT upon not found
     */
    int FuseLFS::ino_stat(fuse_ino_t ino, struct stat *stbuf) {
        inode_entry_t entry;
        stbuf->st_dev = 0;
        stbuf->st_rdev = 0;
        stbuf->st_blocks = 0;
        stbuf->st_blksize = SECTOR_SIZE;

        stbuf->st_atime = time(nullptr);
        stbuf->st_mtime = time(nullptr);
        stbuf->st_ctime = time(nullptr);

        stbuf->st_ino = ino;

        // 0 is invalid inode
        if(ino == 0) {
            goto ino_stat_enoent;
        }

        // Root inode has hardcoded data
        if(ino == 1) {
            goto ino_stat_dir;
        }

        #ifdef QEMUCSD_DEBUG
        if(ino == 2) {
            stbuf->st_size = 11;
            goto ino_stat_file;
        }
        #endif

        // Read the inode entry from drive or inode_entries synchronization
        // structure.
        if(get_inode_entry_t(ino, &entry) == FLFS_RET_ENOENT)
            return FLFS_RET_ENOENT;

        stbuf->st_size = entry.first.size;
        if(entry.first.type == INO_T_FILE)
            goto ino_stat_file;
        else if(entry.first.type == INO_T_DIR)
            goto ino_stat_dir;
        else {
            output.error("get_inode_entry_t returned entry with type",
                         "INO_T_NONE");
            goto ino_stat_enoent;
        }

        ino_stat_file:
            stbuf->st_mode = S_IFREG | 0777;
            stbuf->st_nlink = 1;
            return FLFS_RET_NONE;
        ino_stat_dir:
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return FLFS_RET_NONE;
        ino_stat_enoent:
            return FLFS_RET_ENOENT;
    }

    /**
     * Create a fuse reply taking into account buffer and offset constraints
     * @return result of fuse_reply_buf, 0 upon success
     */
    int FuseLFS::reply_buf_limited(fuse_req_t req, const char *buf,
                                   size_t bufsize, off_t off, size_t maxsize)
    {
        if (off < bufsize)
            return fuse_reply_buf(req, buf, flfs_min(bufsize, maxsize));
        else
            return fuse_reply_buf(req, nullptr, 0);
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
        fuse_add_direntry(req, buf->p + oldsize, buf->size - oldsize, name,
                          &stbuf, buf->size);
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
     * Add an inode to the nat_set to be processed on next flush
     */
    void FuseLFS::add_nat_update_set_entries(std::vector<fuse_ino_t> *inodes) {
        for(auto &ino : *inodes)
            nat_update_set.insert(ino);
    }

    /**
     * Find and set random_ptr starting from random_pos. This is the first
     * unreadable (unwritten) sector from random_pos with overflow from
     * RANDZ_BUFF_POS back to RANDZ_POS (The random zone is interpreted as
     * linearly contiguous from random_pos up until random_pos).
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure and
     *         FLFS_RET_RANDZ_FULL if the random zone is full
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
    void FuseLFS::read_random_zone(inode_lba_map_t *inode_map) {
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
                        // If we have yet to rewrite this inode add it to the
                        // set and removes it from our map of pending updates.
                        auto it = inodes->find(nat_blk->inode[z]);

                        #ifdef FLFS_RANDOM_RW_STRICT
                        if(it != inode_lba_copy.end() && it->second == nat_blk->lba[z]) {
                        #else
                        if(it != inodes->end()) {
                        #endif
                            nat_set->insert(nat_blk->inode[z]);
                            // Reuse iterator for erase, prevents additional
                            // lookup
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

    /**
     * Increment the log zone pointer and detect when the log zone is full
     * @return FLFS_RET_NONE upon success, FLFS_RET_LOGZ_FULL if log zone full
     */
    int FuseLFS::advance_log_ptr(struct data_position *log_ptr) {
        log_ptr->sector += 1;
        if(log_ptr->sector == nvme_info.zone_capacity) {
            log_ptr->sector = 0;
            log_ptr->zone += 1;
        }

        // Log zone full, invalidate the log pointer
        if(log_ptr->zone == nvme_info.num_zones) {
            log_ptr->size = 0;
            return FLFS_RET_LOGZ_FULL;
        }

        return FLFS_RET_NONE;
    }

    /**
     * Find the location of the current log write pointer and adjust log_ptr
     * accordingly.
     */
    void FuseLFS::determine_log_ptr() {
        struct data_position drive_end =
            {nvme_info.num_zones, 0, 0, SECTOR_SIZE};
        void* buff = malloc(SECTOR_SIZE);
        log_ptr = LOGZ_POS;

        do {
            if(nvme->read(log_ptr.zone, log_ptr.sector, log_ptr.offset, buff,
                          SECTOR_SIZE) != 0)
                break;

            advance_log_ptr(&log_ptr);
        } while(log_ptr != drive_end);

        free(buff);
    }

    /**
     * Read the entire drive its log zone and construct the path_inode_map.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
//    int FuseLFS::build_path_inode_map() {
//
//    }

    /**
     * Fill an inode_block for entries and remove every entry added to block
     * @return FLFS_RET_NONE if block not full, FLFS_RET_INO_BLK_FULL if the
     *         entire block is filled.
     */
    int FuseLFS::fill_inode_block(
        struct inode_block *blck, std::vector<fuse_ino_t> *ino_remove,
        inode_entries_t *entries)
    {
        ino_remove->reserve(INODE_ENTRY_SIZE / INODE_BLOCK_SIZE);

        // Keep track of the occupied size by the current inode_entries
        uint32_t occupied_size = 0;

        // Store inode_entries and names in temporary larger buffer incase of
        // overshoot.
        uint8_t *buffer = (uint8_t*) malloc(INODE_BLOCK_SIZE * 2);

        for(auto &entry : *entries) {
            // If no more space return that the block is full
            if(entry.second.second.size() + 1 + occupied_size +
               INODE_ENTRY_SIZE > INODE_BLOCK_SIZE)
                goto compute_inode_block_full;

            memcpy(buffer + occupied_size,
                   &entry.second.first, INODE_ENTRY_SIZE);
            occupied_size += INODE_ENTRY_SIZE;

            memcpy(buffer + occupied_size,
                   entry.second.second.c_str(), entry.second.second.size() + 1);
            occupied_size += entry.second.second.size() + 1;

            ino_remove->push_back(entry.first);
        }

        // If remaining space insufficient return full
        if(occupied_size > INODE_BLOCK_SIZE - INODE_ENTRY_SIZE)
            goto compute_inode_block_full;

        // Block is not full so return none
        free(buffer);
        return FLFS_RET_NONE;

        compute_inode_block_full:
        memcpy(blck, buffer, INODE_BLOCK_SIZE);
        free(buffer);
        return FLFS_RET_INO_BLK_FULL;
    }

    /**
     * Remove the inode_entries present in ino_remove
     */
    void FuseLFS::erase_inode_entries(
        std::vector<fuse_ino_t> *ino_remove,
        inode_entries_t *entries = &inode_entries)
    {
        for(auto &inode : *ino_remove)
            entries->erase(inode);
    }

    /**
     * Find and populate the inode_entry_t (inode entry + name) for a given
     * inode.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon error or
     *         FLFS_RET_ENOENT if the inode_entry could not be found
     */
    int FuseLFS::get_inode_entry_t(fuse_ino_t ino, inode_entry_t *entry) {
        struct data_position ino_pos = {0};

        // Verify the inode exists, inode_lba_map should always be complete
        auto lookup = inode_lba_map.find(ino);
        if(lookup == inode_lba_map.end())
            return FLFS_RET_ENOENT;

        // Check if inode information still unflushed in inode_entries
        // Always check inode_entries first as unflushed data is most up to date
        auto it = inode_entries.find(ino);
        if(it != inode_entries.end()) {
            *entry = it->second;
            return FLFS_RET_NONE;
        }

        // Read the inode_block from the drive
        auto *ino_blk_ptr = (uint8_t*) malloc(sizeof(inode_block));
        lba_to_position(lookup->second, ino_pos);
        if(nvme->read(ino_pos.zone, ino_pos.sector, ino_pos.offset, ino_blk_ptr,
                      sizeof(inode_block)) != 0) {
            free(ino_blk_ptr);
            return FLFS_RET_ERR;
        }

        uint64_t offset = 0;
        struct inode_entry *ino_entry;
        // Iterate over each inode_entry_t (inode_entry + its name)
        while(offset < INO_BLK_READ_LIM) {
            ino_entry = (struct inode_entry *) (ino_blk_ptr + offset);
            if(ino_entry->inode == ino)
                break;

            offset += INODE_ENTRY_SIZE;
            offset += strlen((const char *)ino_blk_ptr + offset) + 1;
        }

        // Inode was not found while it should have been on this lba
        if(ino_entry->inode != ino) {
            free(ino_blk_ptr);
            return FLFS_RET_ERR;
        }

        // Populate entry to reflect the found inode_entry_t data
        entry->first = *ino_entry;
        entry->second = std::string((const char *)ino_blk_ptr + offset +
            INODE_ENTRY_SIZE);

        free(ino_blk_ptr);

        return FLFS_RET_NONE;
    }

    /**
     * Create a new inode entry and add this to inode_entries. Increment ino_ptr
     * accordingly.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon error,
     *         FLFS_RET_MAX_INO if no more inodes available and
     *         FLFS_RET_LOGZ_FULL if the log zone is full
     *         (LOGZ_FULL only when FLFS_INODE_FLUSH_IMMEDIATE is defined).
     */
    int FuseLFS::create_inode(fuse_ino_t parent, const char *name,
        enum inode_type type, fuse_ino_t &ino)
    {
        #if QEMUCSD_DEBUG
        struct stat stbuf = {0};
        if(ino_stat(parent, &stbuf) == FLFS_RET_ENOENT)
            return FLFS_RET_ERR;

        if(!(stbuf.st_mode & S_IFDIR))
            return FLFS_RET_ERR;
        #endif

        struct inode_entry entry = {0};
        entry.parent = parent;
        entry.type = type;
        entry.inode = ino_ptr;
        entry.data_lba = 0;
        entry.size = 0;

        if(ino_ptr == UINT64_MAX)
            return FLFS_RET_MAX_INO;
        ino_ptr += 1;

        // Use insert_or_assign to prevent having to unpack packed field entry
        inode_entries.insert_or_assign(entry.inode,
            std::make_pair(entry, std::string(name)));

        uint64_t entry_inode = entry.inode;
        inode_lba_map.insert(std::make_pair(entry_inode, 0));

        // Newly created inodes are added to path_inode_map. Normally this is
        // handled by lookup only when a file is looked up and nlookup becomes
        // non zero but since this inode is not flushed to drive yet a call to
        // lookup would fail otherwise as the lba for this inode is still 0.
        path_inode_map.find(parent)->second->insert(
            std::make_pair(name, entry_inode));
        if(type == INO_T_DIR)
            path_inode_map.insert(std::make_pair(entry_inode,
                                                 new path_map_t()));

        // Communicate new inode information to caller
        ino = entry.inode;

        #ifdef FLFS_INODE_FLUSH_IMMEDIATE
        return flush_inodes(true);
        #endif

        return FLFS_RET_NONE;
    }

    /**
     * Update any given inode by adding or overriding the new data to
     * inode_entries
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon error and
     *         FLFS_RET_LOGZ_FULL if the log zone is full
     *         (LOGZ_FULL only when FLFS_INODE_FLUSH_IMMEDIATE is defined).
     */
    int FuseLFS::update_inode_entry_t(inode_entry_t *entry) {

        // This overrides the inode_entry in case the previous one hadn't
        // flushed to drive yet.
        inode_entries.insert_or_assign(
            entry->first.inode, std::make_pair(entry->first, entry->second));

        #ifdef FLFS_INODE_FLUSH_IMMEDIATE
        return flush_inodes(true);
        #endif

        return FLFS_RET_NONE;
    }

    /**
     * Flush data to drive and return start lba of data. All data will be
     * flushed linearly starting from lba.
     * TODO(Dantali0n): Make nvme_zns backend support multi sector linear
     *                  writes.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure and
     *         FLFS_RET_LOGZ_FULL if the log zone is full.
     */
    int FuseLFS::log_append(void *data, size_t size, uint64_t &lba) {
        if(size != SECTOR_SIZE)
            return FLFS_RET_ERR;

        uint64_t res_sector;
        if(nvme->append(log_ptr.zone, res_sector, log_ptr.offset, data, size)
           != 0)
            return FLFS_RET_ERR;

        // Verify append was written to expected location
        if(log_ptr.sector != res_sector)
            return FLFS_RET_ERR;

        // Update caller lba to indicate location
        position_to_lba(log_ptr, lba);

        return advance_log_ptr(&log_ptr);
    }

    /**
     * Determine how many data_blocks are required based on the number of
     * occupied lbas. This number is rounded to the nearest highest value.
     */
    void FuseLFS::compute_data_block_num(uint64_t num_lbas, uint64_t &blocks) {
        blocks = num_lbas / DATA_BLK_LBA_NUM;
        blocks += num_lbas % DATA_BLK_LBA_NUM != 0 ? 1 : 0;
    }

    /**
     * Create or update the data_blocks. The number of data_blocks depends on
     * the size of data_lbas vector while the data_block numbers depends on
     * start_blk.
     */
    void FuseLFS::assign_data_blocks(fuse_ino_t ino, data_map_t *blocks) //, std::vector<data_block> *blocks)
    {
        for(auto &block : *blocks) {
            assign_data_block(ino, block.first, &block.second);
        }
    }

    /**
     * Fill a data_block and insert or update the data_blocks
     */
    void FuseLFS::assign_data_block(fuse_ino_t ino, uint64_t block_num,
        struct data_block *blk) //, struct data_block *blk)
    {
        // See if the inode already exists in data_blocks otherwise create it
        auto lookup = data_blocks.find(ino);
        if(lookup == data_blocks.end())
            data_blocks.insert(std::make_pair(ino, new data_map_t()));

        // Get the data_map from the data_blocks
        auto data_block_map = data_blocks.find(ino)->second;

        // Now insert the new block ready for flush to drive
        data_block_map->insert_or_assign(block_num, *blk);
    }

    /**
     * Get the data block for the given inode
     * @param block_num zero indexed data_block block number
     * @return FLFS_RET_NONE upon success and FLFS_RET_ERR upon failure
     */
    int FuseLFS::get_data_block(inode_entry entry, uint64_t block_num,
                                struct data_block *blk)
    {
        auto lookup = data_blocks.find(entry.inode);

        // Found data blocks in synchronization data structure
        if(lookup != data_blocks.end()) {
            // Lookup if block_num is in data_blocks for the given inode
            auto block_num_lookup = lookup->second->find(block_num);
            if(block_num_lookup != lookup->second->end()) {
                // If so no need to fetch data from drive can return immediately
                *blk = block_num_lookup->second;
                return FLFS_RET_NONE;
            }
        }

        // Data blocks information not in memory must be retrieved from drive
        struct data_position pos = {0};
        lba_to_position(entry.data_lba, pos);

        // It might be faster just to call get_data_block_linked due to
        // the pipeline stalls from branch mis predictions. Then again tight
        // loop optimizations likely don't really apply to event based systems
        // such as filesystems. Suppose we could do a couple of benchmarks with
        // one or the other and see if it influences the results
        if(block_num == 0) return get_data_block_immediate(pos, blk);
        else return get_data_block_linked(pos, block_num, blk);
    }

    /**
     * Get the immediate data_block by reading from the position
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFS::get_data_block_immediate(
        struct data_position pos, struct data_block *blk)
    {
        if(nvme->read(pos.zone, pos.sector, pos.offset, blk,
                      sizeof(data_block)) != 0)
            return FLFS_RET_ERR;

        return FLFS_RET_NONE;
    }

    /**
     * Iterate through the chain of linked data_blocks until the requested one
     * is found. When found fill blk buffer with the data by calling
     * get_data_block_immediate
     * @param block_num is zero indexed
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFS::get_data_block_linked(
        struct data_position pos, uint64_t block_num, struct data_block *blk)
    {
        auto buffer = (struct data_block *) malloc(sizeof(data_block));
        for(uint64_t i = 0; i < block_num; i ++) {
            if(nvme->read(pos.zone, pos.sector, pos.offset, buffer,
                          sizeof(data_block)) != 0)
                return FLFS_RET_ERR;
            // Update the pos with the next block
            lba_to_position(buffer->next_block, pos);
        }
        free(buffer);

        return get_data_block_immediate(pos, blk);
    }

    /**
     * Flush inodes to drive either unconditionally or when an entire block can
     * be filled. Notice; flush_inodes_if_full flushes a single block at most.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon error and
     *         FLFS_RET_LOGZ_FULL if log zone full.
     */
    int FuseLFS::flush_inodes(bool only_if_full = false) {
        if(only_if_full)
            return flush_inodes_if_full();
        else
            return flush_inodes_always();
    }

    /**
     * Flush a single inode block to drive but only of it is filled completely
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon error and
     *         FLFS_RET_LOGZ_FULL if log zone full.
     */
    int FuseLFS::flush_inodes_if_full() {
        struct inode_block blk = {0};
        std::vector<fuse_ino_t> inodes;
        uint64_t res_lba;
        int result;

        if(fill_inode_block(&blk, &inodes, &inode_entries) ==
           FLFS_RET_INO_BLK_FULL)
        {
            result = log_append(&blk, sizeof(inode_block), res_lba);

            if (result == FLFS_RET_NONE) {
                erase_inode_entries(&inodes);
                update_inode_lba_map(&inodes, res_lba);
                add_nat_update_set_entries(&inodes);
            }
            else return result;
        }

        return FLFS_RET_NONE;
    }

    /**
     * Flush one or more inode_blocks to the drive with the first regardless
     * of how full it is.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon error and
     *         FLFS_RET_LOGZ_FULL if log zone full.
     */
    int FuseLFS::flush_inodes_always() {
        struct inode_block blk = {0};
        std::vector<fuse_ino_t> inodes;
        uint64_t res_lba;
        int result = 0;

        fill_inode_block(&blk, &inodes, &inode_entries);
        do {
            result = log_append(&blk, sizeof(inode_block), res_lba);
            if(result == FLFS_RET_NONE) {
                erase_inode_entries(&inodes);
                update_inode_lba_map(&inodes, res_lba);
                add_nat_update_set_entries(&inodes);
            }
            else return result;
        } while(fill_inode_block(&blk, &inodes, &inode_entries) ==
                FLFS_RET_INO_BLK_FULL);

        return FLFS_RET_NONE;
    }

    /**
     * Flush all data_blocks to drive in the correct order
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure
     */
    int FuseLFS::flush_data_blocks() {
        // These inodes will need to be updates
        std::vector<fuse_ino_t> inodes;

        for(auto &block : data_blocks) {
            inodes.push_back(block.first);
        }

        return FLFS_RET_NONE;
    }

    /**
     * Create a uniquely identifying file handle for the inode and calling pid.
     * These are necessary for state management such as CSD operations and in
     * memory snapshots.
     */
    void FuseLFS::create_file_handle(
        fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
    {
        const fuse_ctx *context = fuse_req_ctx(req);

        open_inode_map.insert(
            std::make_pair(fh_ptr, std::make_pair(ino, context->pid)));

        fi->fh = fh_ptr;

        if(fh_ptr == UINT64_MAX)
            output.fatal("Exhausted all possible file handles!");
        fh_ptr += 1;
    }

    /**
     * Called be release to remove the session for a file handle
     */
    void FuseLFS::release_file_handle(struct fuse_file_info *fi) {
        open_inode_map.erase(fi->fh);
    }

    /**
     * Perform log zone garbage collection and compaction.
     */
    int FuseLFS::log_garbage_collect() {
        return FLFS_RET_NONE;
    }

    /**
     * Convert flfs internal return codes to fuse reply codes
     */
    int FuseLFS::flfs_ret_to_fuse_reply(int flfs_ret) {
        switch (flfs_ret) {
            case FLFS_RET_NONE:
                return 0;
            case FLFS_RET_ENOENT:
                return ENOENT;
            case FLFS_RET_EISDIR:
                return EISDIR;
            default:
                return EIO;
        }
    }

    /**
     * Convince the caller that the files are owned by him by modifying the
     * gid & uid in stbuf.
     */
    void FuseLFS::ino_fake_permissions(fuse_req_t req, struct stat *stbuf) {
        const struct fuse_ctx *context = fuse_req_ctx(req);
        stbuf->st_gid = context->gid;
        stbuf->st_uid = context->uid;
    }

    /**
     * Truncate the inode changing its size to what was requested. Can be
     * used to increase and decrease file size.
     * @return FLFS_RET_NONE upon success, FLFS_RET_ERR upon failure,
     *         FLFS_RET_ENOENT if the inode could not be found,
     *         FLFS_RET_EISDIR if the inode is a directory and
     *         FLFS_RET_LOGZ_FULL if the log zone is full but only when
     *         FLFS_INODE_FLUSH_IMMEDIATE is defined.
     */
    int FuseLFS::ftruncate(fuse_ino_t ino, size_t size) {
        inode_entry_t entry;

        int result = get_inode_entry_t(ino, &entry);
        if(result != FLFS_RET_NONE)
            return result;

        if(entry.first.type != INO_T_FILE)
            return FLFS_RET_EISDIR;

        // Update inode size
        entry.first.size = size;

        // Reduce size
        if(entry.first.size > size) {
            // TODO(Dantali0n): Invalidate already occupied SIT blocks
        }
        // Increase size
        else if(entry.first.size < size) {
        }

        result = update_inode_entry_t(&entry);
        if(result != FLFS_RET_NONE)
            return result;

        return FLFS_RET_NONE;
    }

    /**
     *
     * @return
     */

    int FuseLFS::write_sector(size_t size, off_t offset, uint64_t cur_lba,
        const char *data, uint64_t &result_lba)
    {
        if(offset + size > SECTOR_SIZE) return FLFS_RET_ERR;
        auto buffer = (uint8_t*) malloc(SECTOR_SIZE);

        // Fetch current sector data if it exists and sector won't be completely
        // rewritten
        if(cur_lba != 0 && (size != SECTOR_SIZE && offset != 0)) {
            struct data_position cur_data_pos = {0};
            lba_to_position(cur_lba, cur_data_pos);
            if(nvme->read(cur_data_pos.zone, cur_data_pos.sector,
                          cur_data_pos.offset, buffer, SECTOR_SIZE) != 0) {
                free(buffer);
                return FLFS_RET_ERR;
            }
        }

        memcpy(buffer + offset, data, size);
        int result = log_append(buffer, SECTOR_SIZE, result_lba);

        free(buffer);
        return result;
    }

    void FuseLFS::init(void *userdata, struct fuse_conn_info *conn) {
        connection = conn;

        // Disable all these 'reasonable' defaults
        conn->want &= ~(FUSE_CAP_ASYNC_READ);
        conn->want &= ~(FUSE_CAP_POSIX_LOCKS);
        conn->want &= ~(FUSE_CAP_FLOCK_LOCKS);
        conn->want &= ~(FUSE_CAP_ATOMIC_O_TRUNC);
        conn->want &= ~(FUSE_CAP_IOCTL_DIR);
        conn->want &= ~(FUSE_CAP_READDIRPLUS);
        conn->want &= ~(FUSE_CAP_READDIRPLUS_AUTO);
        conn->want &= ~(FUSE_CAP_ASYNC_DIO);
        conn->want &= ~(FUSE_CAP_PARALLEL_DIROPS);
        conn->want &= ~(FUSE_CAP_HANDLE_KILLPRIV);

        conn->want &= ~(FUSE_CAP_SPLICE_READ);
        conn->want &= ~(FUSE_CAP_AUTO_INVAL_DATA);
    }

    void FuseLFS::destroy(void *userdata) {
        output.info("Tearing down filesystem");

        //TODO(Dantali0n): flush all pending datastructures to drive.

        if(remove_dirtyblock() != FLFS_RET_NONE) {
            output.error("Failed to remove dirty block from drive",
                " this will cause issues on subsequent mounts!");
        }
    }

    /**
     * Lookup the given inode for the parent, name pair and use ino_stat
     * to fill out information about the found inode.
     * TODO(Dantali0n): Any inode accessed with lookup should be added to
     *                  path_inode_map. Subsequently, if nlookup reaches 0 it
     *                  should be removed.
     */
    void FuseLFS::lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
        struct fuse_entry_param e = {0};
        e.ino = parent;

        // Check if parent exists
        if(ino_stat(e.ino, &e.attr) == FLFS_RET_ENOENT) {
            fuse_reply_err(req, ENOENT);
            return;
        }

        // TODO(Dantali0n): Do not directly return not found if path_inode_map
        //                  becomes partial / incomplete
        // Search for the name in the path_inode_map
        auto result = path_inode_map.find(parent)->second->find(name);
        if(result == path_inode_map.find(parent)->second->end()) {
            fuse_reply_err(req, ENOENT);
            return;
        }

        // Clear parent fuse_entry_param information
        memset(&e, 0, sizeof(e));

        e.ino = result->second;
        e.attr_timeout = 60.0;
        e.entry_timeout = 60.0;
        if(ino_stat(e.ino, &e.attr) == FLFS_RET_ENOENT) {
            fuse_reply_err(req, ENOENT);
            return;
        }

        #ifdef FLFS_FAKE_PERMS
        ino_fake_permissions(req, &e.attr);
        #endif

        fuse_reply_entry_nlookup(req, &e);
    }

    void FuseLFS::forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup) {
        inode_nlookup_decrement(ino, nlookup);
        fuse_reply_none(req);
    }

    void FuseLFS::getattr(fuse_req_t req, fuse_ino_t ino,
                          struct fuse_file_info *fi)
    {
        struct stat stbuf = {0};
        if (ino_stat(ino, &stbuf) == FLFS_RET_ENOENT)
            fuse_reply_err(req, ENOENT);
        else {
            #ifdef FLFS_FAKE_PERMS
            ino_fake_permissions(req, &stbuf);
            #endif

            fuse_reply_attr(req, &stbuf, 90.0);
        }
    }

    void FuseLFS::setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
        int to_set, struct fuse_file_info *fi) {
        int result;

        // Change permissions (chmod)
        if(to_set & FUSE_SET_ATTR_MODE) {

        }
        // Change owner (chown)
        if(to_set & FUSE_SET_ATTR_UID) {

        }
        // Change group (chgrp)
        if(to_set & FUSE_SET_ATTR_GID) {

        }
        // Change size of file (truncate / ftruncate)
        if(to_set & FUSE_SET_ATTR_SIZE) {
            result = ftruncate(ino, attr->st_size);
            if(result != FLFS_RET_NONE) {
                fuse_reply_err(req, flfs_ret_to_fuse_reply(result));
                return;
            }
        }

        getattr(req, ino, fi);
    }

    void FuseLFS::readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t offset, struct fuse_file_info *fi)
    {
        struct stat stbuf = {0};

        // Check if the inode exists
        if (ino_stat(ino, &stbuf) == FLFS_RET_ENOENT)
            fuse_reply_err(req, ENOENT);

        // Check if it is a directory
        if(!(stbuf.st_mode & S_IFDIR))
            fuse_reply_err(req, ENOTDIR);

        struct dir_buf buf = {0};
        dir_buf_add(req, &buf, ".", ino);
        dir_buf_add(req, &buf, "..", ino);

        // TODO(Dantali0n): Fix this once path_inode_map becomes partial
        for(auto &entry : *path_inode_map.find(ino)->second) {
            dir_buf_add(req, &buf, entry.first.c_str(), ino);
        }

        reply_buf_limited(req, buf.p, buf.size, offset, size);
        free(buf.p);
    }

    void FuseLFS::open(fuse_req_t req, fuse_ino_t ino,
                      struct fuse_file_info *fi)
    {
        struct stat stbuf = {0};

        // Check if the inode exists
        if(ino_stat(ino, &stbuf) == FLFS_RET_ENOENT) {
            fuse_reply_err(req, ENONET);
            return;
        }

        // Check if directory
        if (stbuf.st_mode & S_IFDIR) {
            fuse_reply_err(req, EISDIR);
            return;
        }

        // TODO(Dantali0n): Check O_APPEND
        if (stbuf.st_mode & O_APPEND) {
            output.error("Attempt to open file with append not supported!");
            fuse_reply_err(req, EOPNOTSUPP);
            return;
        }

        #ifdef QEMUCSD_DEBUG
        // TODO(Dantali0n): Check O_NONBLOCK
        if (stbuf.st_mode & O_NONBLOCK) {
            output.error("Attempt to open file with O_NONBLOCK not supported!");
            fuse_reply_err(req, EOPNOTSUPP);
            return;
        }

        // TODO(Dantali0n): Check O_NDELAY
        if (stbuf.st_mode & O_NDELAY) {
            output.error("Attempt to open file with O_NDELAY not supported!");
            fuse_reply_err(req, EOPNOTSUPP);
            return;
        }

        // TODO(Dantali0n): Check O_SYNC
        if (stbuf.st_mode & O_SYNC) {
            output.error("Attempt to open file with O_SYNC not supported!");
            fuse_reply_err(req, EOPNOTSUPP);
            return;
        }

        // TODO(Dantali0n): Check O_ASYNC
        if (stbuf.st_mode & O_ASYNC) {
            output.error("Attempt to open file with O_ASYNC not supported!");
            fuse_reply_err(req, EOPNOTSUPP);
            return;
        }
        #endif

        // Can only read files in this demo
        // TODO(Dantali0n): Support opening files in write mode
//        if ((fi->flags & O_ACCMODE) != O_RDONLY) {
//            fuse_reply_err(req, EACCES);
//            return;
//        }

        #ifdef FLFS_DBG_FI
        output_fi("open", fi);
        #endif

        create_file_handle(req, ino, fi);

        fuse_reply_open(req, fi);
    }

    void FuseLFS::release(
        fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
    {
        struct fuse_entry_param e = {0};

        // Check that inode exists and retrieve its basic information
        if(ino_stat(ino, &e.attr) == FLFS_RET_ENOENT)
            fuse_reply_err(req, ENOENT);

        // Release file handle if not directory
        if(e.attr.st_mode & S_IFREG)
            release_file_handle(fi);

        #ifdef FLFS_DBG_FI
        output_fi("release", fi);
        #endif

        fuse_reply_err(req, 0);
    }

    void FuseLFS::create(fuse_req_t req, fuse_ino_t parent, const char *name,
                         mode_t mode, struct fuse_file_info *fi)
    {
        struct fuse_entry_param e = {0};
        e.ino = parent;

        // Maximum length of file / directory name is dominated by sector and
        // inode_entry size.
        if(strlen(name) > MAX_NAME_SIZE) {
            fuse_reply_err(req, ENAMETOOLONG);
        }

        // Verify parent exists
        if(ino_stat(e.ino, &e.attr) == FLFS_RET_ENOENT)
            fuse_reply_err(req, ENOENT);

        // Verify the parent is a directory, this check is already performed by
        // Linux ABI
        #ifdef QEMUCSD_DEBUG
        if(e.attr.st_mode & S_IFREG)
            fuse_reply_err(req, ENOTDIR);
        #endif

        // Check if file exists
        // TODO(Dantali0n): Fix this once path_inode_map becomes partial
        if(path_inode_map.find(parent)->second->find(name) !=
            path_inode_map.find(parent)->second->end())
            fuse_reply_err(req, EEXIST);

        // Clear parent data from e
        memset(&e, 0, sizeof(fuse_entry_param));

        // TODO(Dantali0n): Check create_inode return code and handle these

        #ifdef FLFS_DBG_FI
        if(fi)
            output_fi("create", fi);
        #endif

        // Create directory
        if(mode & S_IFDIR) {
            create_inode(parent, name, INO_T_DIR, e.ino);
            ino_stat(e.ino, &e.attr);
            fuse_reply_entry_nlookup(req, &e);
        }
        // Create file
        else if(mode & S_IFREG) {
            create_inode(parent, name, INO_T_FILE, e.ino);
            ino_stat(e.ino, &e.attr);
            create_file_handle(req, e.ino, fi);
            fuse_reply_create_nlookup(req, &e, fi);
        }
        // Symlinks, block and character devices are not supported
        else {
            // Operation not supported, Never reply ENOSYS this has side effects
            // in FUSE.
            fuse_reply_err(req, EOPNOTSUPP);
        }
    }

    void FuseLFS::mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
        mode_t mode)
    {
        // Do not check parent and name for existence, this is handled by create

        // Unset S_IFREG
        mode &= ~(S_IFREG);
        // Set S_IFDIR
        mode |= S_IFDIR;

        create(req, parent, name, mode, nullptr);
    }

    void FuseLFS::read(fuse_req_t req, fuse_ino_t ino, size_t size,
                       off_t offset, struct fuse_file_info *fi)
    {
        struct fuse_entry_param e = {0};
        const fuse_ctx* context = fuse_req_ctx(req);

        // Check if inode exists
        if(ino_stat(ino, &e.attr) == FLFS_RET_ENOENT) {
            fuse_reply_err(req, ENOENT);
            return;
        }

        #ifdef QEMUCSD_DEBUG
        // Verify inode is regular file
        if(!(e.attr.st_mode & S_IFREG)) {
            fuse_reply_err(req, EISDIR);
            return;
        }

        // Verify file handle (session) exists
        if(open_inode_map.find(fi->fh) == open_inode_map.end()) {
            output.error("File handle ", fi->fh, " not found in open_inode_map!");
            fuse_reply_err(req, EIO);
            return;
        }
        #endif

        #ifdef FLFS_DBG_FI
        output_fi("read", fi);
        #endif

        // Hardcoded data
        if(ino == 2) {
            std::string buffer = "Je moeder!\n";
            reply_buf_limited(req, buffer.c_str(), buffer.size(), offset, size);
            return;
        }

        // Inode is of size 0
        if(e.attr.st_size == 0) {
            reply_buf_limited(req, nullptr, 0, offset, size);
            return;
        }

        // Actual read starts here
        inode_entry_t entry;
        get_inode_entry_t(ino, &entry);

        // Variables corresponding to initial data_block
        uint64_t db_block_num;
        uint64_t db_num_lbas = offset / SECTOR_SIZE;
        compute_data_block_num(db_num_lbas, db_block_num);

        auto *blk = (struct data_block *) malloc(sizeof(data_block));
        if(get_data_block(entry.first, db_block_num, blk) != FLFS_RET_NONE) {
            uint64_t error_lba = entry.first.data_lba;
            output.error(
                "Failed to get data_block at lba ", error_lba,
                " for inode ", ino, " in read");
            free(blk);
            return;
        }

        uint64_t data_limit = flfs_min(size, e.attr.st_size);
        // Initial lba index in the data_block
        uint64_t db_lba_index = db_num_lbas % DATA_BLK_LBA_NUM;

        // Round buffer size to nearest higher multiple of SECTOR_SIZE
        auto buffer = (uint8_t*) malloc(
            data_limit + (SECTOR_SIZE-1) & (-SECTOR_SIZE));

        // Loop through the data until the buffer is filled to the required size
        uint64_t buffer_offset = 0;
        struct data_position data_pos = {0};
        while(buffer_offset < data_limit) {

            // Detect db_lba_index overflow and fetch next data_block
            if(db_lba_index >= DATA_BLK_LBA_NUM) {
                db_lba_index = 0;
                db_block_num += 1;
                if(get_data_block(entry.first, db_block_num, blk) != FLFS_RET_NONE) {
                    uint64_t error_lba = entry.first.data_lba;
                    output.error(
                        "Failed to get data_block at lba ", error_lba,
                        " for inode ", ino, " in read");
                    free(buffer);
                    free(blk);
                    return;
                }
            }

            // Convert the data_block lba at the current index to a position
            lba_to_position(blk->data_lbas[db_lba_index], data_pos);

            // Read the data from this position into the buffer given the
            // currently accumulated offset
            if(nvme->read(data_pos.zone, data_pos.sector, data_pos.offset,
                          buffer + buffer_offset, SECTOR_SIZE) != 0) {
                uint64_t data_lba = blk->data_lbas[db_lba_index];
                output.error(
                    "Failed to retrieve data at at data_block index ",
                    db_lba_index, " with lba ", data_lba, " for inode ", ino);
                fuse_reply_err(req, EIO);
                free(buffer);
                free(blk);
                return;
            }

            buffer_offset += SECTOR_SIZE;
            db_lba_index += 1;
        }

        reply_buf_limited(req, (const char*)buffer, data_limit, offset, size);

        free(buffer);
        free(blk);
    }

    void FuseLFS::write(fuse_req_t req, fuse_ino_t ino, const char *buffer,
        size_t size, off_t off, struct fuse_file_info *fi)
    {
        struct fuse_entry_param e = {0};
        const fuse_ctx* context = fuse_req_ctx(req);

        // Check if inode exists
        if(ino_stat(ino, &e.attr) == FLFS_RET_ENOENT) {
            fuse_reply_err(req, ENOENT);
            return;
        }

        #ifdef QEMUCSD_DEBUG
        // Verify inode is regular file
        if(!(e.attr.st_mode & S_IFREG)) {
            fuse_reply_err(req, EISDIR);
            return;
        }

        // Verify file handle (session) exists
        if(open_inode_map.find(fi->fh) == open_inode_map.end()) {
            output.error("File handle ", fi->fh, " not found in",
                         "open_inode_map!");
            fuse_reply_err(req, EIO);
            return;
        }
        #endif

        inode_entry_t entry;
        if(get_inode_entry_t(ino, &entry) != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        // Number of sectors total write will cover
        uint64_t num_sectors = size / SECTOR_SIZE;
        if((size + off) % SECTOR_SIZE != 0) num_sectors += 1;
        if(off % SECTOR_SIZE + size > SECTOR_SIZE) num_sectors += 1;

        // Compute initial data_block and index
        uint64_t cur_db_blk_num = (off / SECTOR_SIZE) / DATA_BLK_LBA_NUM;
        uint64_t cur_db_lba_index = (off / SECTOR_SIZE) % DATA_BLK_LBA_NUM;

        // Create data_block and fetch existing data_block info if it exists
        struct data_block cur_db_blk = {0};
        if(e.attr.st_size > 0 && get_data_block(entry.first, cur_db_blk_num,
            &cur_db_blk) != FLFS_RET_NONE)
        {
           output.error("Failed to get data_block ", cur_db_blk_num, " for ",
                        "inode", ino);
            fuse_reply_err(req, EIO);
            return;
        }

        uint64_t write_res_lba;
        uint64_t b_off = 0;
        uint64_t s_size = size;
        uint64_t s_off = off % SECTOR_SIZE;
        for(uint64_t i = 0; i < num_sectors; i++) {
            if(write_sector(
                    s_size + s_off > SECTOR_SIZE ? SECTOR_SIZE - s_off : s_size,
                    s_off, cur_db_blk.data_lbas[cur_db_lba_index],
                buffer + b_off, write_res_lba) != FLFS_RET_NONE)
            {
                fuse_reply_err(req, EIO);
                return;
            }

            // Update location of data for current sector
            cur_db_blk.data_lbas[cur_db_lba_index] = write_res_lba;

            // Increment data index in current data_block
            cur_db_lba_index += 1;

            // Handle overflow to next data block
            if(cur_db_lba_index >= DATA_BLK_LBA_NUM) {
                assign_data_block(ino, cur_db_blk_num, &cur_db_blk);

                cur_db_lba_index = 0;
                cur_db_blk_num += 1;

                memset(&cur_db_blk, 0, sizeof(data_block));
            }

            // Get new data_block if file sufficiently sized such that it should
            // exist.
            if(cur_db_lba_index == 0 && e.attr.st_size > DATA_BLK_LBA_NUM *
                SECTOR_SIZE * cur_db_blk_num)
            {
                if(get_data_block(entry.first, cur_db_blk_num, &cur_db_blk)
                    != FLFS_RET_NONE)
                {
                    output.error("Failed to get data_block ", cur_db_blk_num,
                                 " for inode", ino);
                    fuse_reply_err(req, EIO);
                    return;
                }
            }

            // Compute offset into provided data buffer
            b_off += SECTOR_SIZE - s_off;
            // Reduce remaining size to be written
            s_size -= SECTOR_SIZE - s_off;
            // Only first write has sector offset.
            if(i == 0) s_off = 0;
        }

        if(cur_db_lba_index != 0)
            assign_data_block(ino, cur_db_blk_num, &cur_db_blk);
        entry.first.size =
            entry.first.size > off + size ? entry.first.size : off + size;
        update_inode_entry_t(&entry);
        fuse_reply_write(req, size);
    }

    void FuseLFS::statfs(fuse_req_t req, fuse_ino_t ino) {
        struct statvfs statbuf = {0};
        statbuf.f_bsize = nvme_info.sector_size;
        statbuf.f_frsize = SECTOR_SIZE;

        // TODO(Dantali0n): Use rollover from log_pos to compute
        statbuf.f_bfree = ((nvme_info.num_zones - N_LOG_BUFF_ZONES -
            log_ptr.zone) * nvme_info.zone_capacity) - log_ptr.sector;

        // TODO(Dantali0n): Compute based on occupation from SIT blocks
        statbuf.f_bavail = 0;

        statbuf.f_blocks = ((nvme_info.num_zones - N_LOG_BUFF_ZONES -
            LOGZ_POS.zone) * nvme_info.zone_capacity);

        // Number of inodes used, don't ask why its called f_files
        statbuf.f_files = inode_lba_map.size();
        statbuf.f_ffree = (SECTOR_SIZE / INODE_ENTRY_SIZE) * statbuf.f_bfree;

        statbuf.f_namemax = MAX_NAME_SIZE;
        fuse_reply_statfs(req, &statbuf);
    }

    /**
     * Flush data to drive. Flush order is always 1. data_blocks
     * 2. inode_blocks. 3. NAT blocks. data blocks contain raw data LBAs,
     * inode_blocks contain data_blocks LBAs and NAT blocks contain inode LBAs.
     */
    void FuseLFS::fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
                        struct fuse_file_info *fi)
    {
        int result;

        /** 1. Update the data blocks */
        result = flush_data_blocks();
        if(result == FLFS_RET_LOGZ_FULL) {
            if(log_garbage_collect() != FLFS_RET_NONE) {
                fuse_reply_err(req, EIO);
                return;
            }

            if(flush_data_blocks() != FLFS_RET_NONE) {
                fuse_reply_err(req, EIO);
                return;
            }
        }

        /** 2. Update the inode blocks */
        result = flush_inodes();
        if(result == FLFS_RET_LOGZ_FULL) {
            if(log_garbage_collect() != FLFS_RET_NONE) {
                fuse_reply_err(req, EIO);
                return;
            }

            if(flush_inodes() != FLFS_RET_NONE) {
                fuse_reply_err(req, EIO);
                return;
            }
        }
        else if (result != FLFS_RET_NONE) {
            fuse_reply_err(req, EIO);
            return;
        }

        /** 3. Update NAT Blocks */
        result = update_nat_blocks();
        if(result == FLFS_RET_RANDZ_FULL) {
            if(rewrite_random_blocks() != FLFS_RET_NONE) {
                fuse_reply_err(req, EIO);
                return;
            }
            if(update_nat_blocks() != FLFS_RET_NONE) {
                fuse_reply_err(req, EIO);
                return;
            }
        }
    }

    void FuseLFS::unlink(fuse_req_t req, fuse_ino_t parent, const char *name) {
        return;
    }

}