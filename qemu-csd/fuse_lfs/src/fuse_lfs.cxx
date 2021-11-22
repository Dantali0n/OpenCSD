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

namespace qemucsd::fuse_lfs {

    // TODO(Dantali0n): Remove me
    static constexpr uint64_t TEST_SIZE = 4096;

    struct fuse_conn_info* FuseLFS::connection = nullptr;

    struct nvme_zns::nvme_zns_info FuseLFS::nvme_info = {0};
    nvme_zns::NvmeZnsBackend* FuseLFS::nvme = nullptr;

    const std::string FuseLFS::FUSE_LFS_NAME_PREFIX = "[FUSE LFS] ";
    const std::string FuseLFS::FUSE_SEQUENTIAL_PARAM = "-s";

    // Need string manipulations for lookup call
    const std::string FuseLFS::PATH_ROOT = "/";

    typedef std::map<std::pair<uint32_t, std::__cxx11::basic_string<char> >, int> template_hell;
    template_hell FuseLFS::path_inode_map = template_hell();

    const struct fuse_lowlevel_ops FuseLFS::operations = {
        .init       = FuseLFS::init,
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
            return -1;
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

        // TODO(Dantali0n): Only write super block when a certain command line
        //                  argument is supplied.
        if(write_superblock() != 0) {
            output(std::cerr, "Failed to write super block, check",
                   "NvmeZns backend");
            ret = 1;
            goto err_out1;
        }

        if(verify_superblock() != 0) {
            output(std::cerr, "Failed to verify super block, are you "
                   , "sure partition does not contain another filesystem?");
            ret = 1;
            goto err_out1;
        }

        session = fuse_session_new(
            &args, &operations, sizeof(operations), nullptr);
        if (session == nullptr)
            goto err_out1;

        if (fuse_set_signal_handlers(session) != 0)
            goto err_out2;

        if (fuse_session_mount(session, opts.mountpoint) != 0)
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
     * Translate the (inefficient) char* path to an inode.
     */
    void FuseLFS::path_to_inode(const char* path, int& fd) {
        std::istringstream spath(path);
        std::string token;
        int depth = 0;

        // Iterate over the path with increased depth
        while(std::getline(spath, token, '/')) {
            // If find does not return the end of the map its a match
            auto it = path_inode_map.find(std::make_pair(depth, token));
            if(it != path_inode_map.end()) {
                fd = it->second;
                return;
            }
            depth++;
        }
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
                return -1;
        }
        return 0;
    }

    /**
     *
     * @return result of fuse_reply_buf, 0 upon success
     */
    int FuseLFS::reply_buf_limited(fuse_req_t req, const char *buf,
                                   size_t bufsize, off_t off, size_t maxsize)
    {
        if (off < bufsize)
            return fuse_reply_buf(req, buf + off,
                                  fuse_lfs_min(bufsize - off, maxsize));
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

    template<typename T>
    void FuseLFS::output(std::ostream &out, T &&t) {
        out  << t << "\n";
    }

    template<typename Head, typename... Tail>
    void FuseLFS::output(std::ostream &out, Head &&head, Tail&&... tail) {
        out << FUSE_LFS_NAME_PREFIX << head;
        output(out, std::forward<Tail>(tail)...);
    }

    /**
     * Read the super block for the filesystem and verify the parameters to
     * prevent overwritten a drive configured for other filesystems.
     * @return 0 upon success, < 0 upon failure
     */
    int FuseLFS::verify_superblock() {
        struct super_block sblock;
        if(nvme->read(0, 0, 0, &sblock, sizeof(super_block)) != 0)
            return -1;
        if(sblock.magic_cookie != MAGIC_COOKIE)
            return -1;
        if(sblock.zones != nvme_info.num_zones)
            return -1;
        if(sblock.sectors != nvme_info.zone_size)
            return -1;
        if(sblock.sector_size != nvme_info.sector_size)
            return -1;

        return 0;
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
        if(nvme->append(0, sector, 0, &sblock, sizeof(super_block)) != 0)
            return -1;

        return 0;
    }

    void FuseLFS::init(void *userdata, struct fuse_conn_info *conn) {
        connection = conn;
    }

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
        // Only test file exists
        assert(ino == 2);

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