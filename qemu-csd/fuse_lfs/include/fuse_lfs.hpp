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

#ifndef QEMU_CSD_FUSE_LFS_HPP
#define QEMU_CSD_FUSE_LFS_HPP

#define FUSE_USE_VERSION	36

extern "C" {
    #include "fuse3/fuse.h"
}

namespace qemucsd::fuse_lfs {

    /**
     * Static wrapper class around FUSE LFS filesystem.
     */
    class FuseLFS {
    protected:
        static struct fuse_conn_info* connection;
        static struct fuse_config* config;

        static const struct fuse_operations operations;
    public:
        FuseLFS() = delete;
        ~FuseLFS() = delete;

        static void get_operations(const struct fuse_operations** operations);

        static void* init(struct fuse_conn_info* conn, struct fuse_config* cfg);
        static int getattr(
            const char* path, struct stat* stat, struct fuse_file_info* fi);
        static int readdir(
            const char* path, void* callback, fuse_fill_dir_t directory_type,
            off_t offset, struct fuse_file_info *, enum fuse_readdir_flags);
        static int open(const char* path, struct fuse_file_info* fi);
        static int create(
            const char* path, mode_t mode, struct fuse_file_info* fi);
        static int read(
            const char* path, char* buffer, size_t size, off_t offset,
            struct fuse_file_info* fi);
        static int write(
            const char* path, const char* buffer, size_t size, off_t offset,
            struct fuse_file_info* fi);
        static int unlink(const char* path);
    };
}

#endif //QEMU_CSD_FUSE_LFS_HPP