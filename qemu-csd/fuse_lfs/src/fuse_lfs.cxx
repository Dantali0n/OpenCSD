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

namespace qemucsd::fuse_lfs{

    struct fuse_conn_info* FuseLFS::connection = nullptr;

    struct nvme_zns::nvme_zns_info* FuseLFS::nvme_info = nullptr;

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
     * Initialize fuse using the low level API
     * @return
     */
    int FuseLFS::initialize(int argc, char* argv[],
                            struct nvme_zns::nvme_zns_info* nvme_info)
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

        session = fuse_session_new(
            &args, &operations, sizeof(operations), nvme_info);
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

    template<typename T>
    void FuseLFS::output(std::ostream &out, T &&t) {
        out << FUSE_LFS_NAME_PREFIX << t << "\n";
    }

    template<typename Head, typename... Tail>
    void FuseLFS::output(std::ostream &out, Head &&head, Tail&&... tail) {
        out << head;
        output(out, std::forward<Tail>(tail)...);
    }

    void FuseLFS::init(void *userdata, struct fuse_conn_info *conn) {
        connection = conn;

        // Get nvme_zns_info from main call
        nvme_info = (struct nvme_zns::nvme_zns_info*) userdata;

        if(SECTOR_SIZE > nvme_info->sector_size) {
            output(std::cerr, "Sector size (", nvme_info->sector_size,
                   ") is to small, minimal is ", SECTOR_SIZE,
                   " bytes, aborting\n");
            exit(1);
        }

        if(nvme_info->sector_size % SECTOR_SIZE != 0) {
            output(std::cerr, "Sector size (", nvme_info->sector_size,
                   ") is not clean multiple of ", SECTOR_SIZE, ", aborting");
            exit(1);
        }
    }

    void FuseLFS::lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {

    }

    void FuseLFS::getattr(fuse_req_t req, fuse_ino_t ino,
                          struct fuse_file_info *fi)
    {

    }

    void FuseLFS::readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                         struct fuse_file_info *fi)
    {
    }

    void FuseLFS::open(fuse_req_t req, fuse_ino_t ino,
                      struct fuse_file_info *fi)
    {
        return;
    }

    void FuseLFS::create(fuse_req_t req, fuse_ino_t parent, const char *name,
                        mode_t mode, struct fuse_file_info *fi)
    {
        return;
    }

    void FuseLFS::read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                      struct fuse_file_info *fi)
    {
        return;
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