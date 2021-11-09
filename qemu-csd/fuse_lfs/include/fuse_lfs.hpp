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

#include <map>
#include <iostream>
#include <sstream>
#include <string>

#include "nvme_zns.hpp"

namespace qemucsd::fuse_lfs {

    static const uint32_t SECTOR_SIZE = 512;
    static const uint64_t MAGIC_COOKIE = 0x10ADEDB00BDEC0DE;

    // One time write, read only information. Always stored at zone 0, sector 0.
    struct superblock {
        uint64_t magic_cookie;
        unsigned char padding[504];
    };
    static_assert(sizeof(superblock) == SECTOR_SIZE);

    struct zone_info_table_entry {
        unsigned char padding[512];
    };
    static_assert(sizeof(zone_info_table_entry) == SECTOR_SIZE);

    struct file_info {
        uint64_t size;
        std::string name;
    };

    /**
     * Static wrapper class around FUSE LFS filesystem.
     */
    class FuseLFS {
    protected:
        static struct fuse_conn_info* connection;
        static struct fuse_context* context;
        static struct fuse_config* config;

        static struct nvme_zns::nvme_zns_info* nvme_info;

        // Map filenames and their respective depth to inodes
        static std::map<std::pair<uint32_t, std::string>, int> path_inode_map;

        // Inode to file_info, reconstructed from 'disc' upon initialization
        static std::map<int, struct file_info> inode_map;

        static const std::string PATH_ROOT;

        static const struct fuse_operations operations;
    public:
        FuseLFS() = delete;
        ~FuseLFS() = delete;

        static void get_operations(const struct fuse_operations** operations);

        static void path_to_inode(const char* path, int& fd);

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