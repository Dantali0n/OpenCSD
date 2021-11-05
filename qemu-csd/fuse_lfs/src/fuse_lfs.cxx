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
    struct fuse_config* FuseLFS::config = nullptr;

    const std::string FuseLFS::PATH_ROOT = "/";

    const struct fuse_operations FuseLFS::operations = {
        .getattr    = FuseLFS::getattr,
        .unlink     = FuseLFS::unlink,
        .open	    = FuseLFS::open,
        .read       = FuseLFS::read,
        .write      = FuseLFS::write,
        .readdir    = FuseLFS::readdir,
        .init       = FuseLFS::init,
        .create     = FuseLFS::create,
    };

    void FuseLFS::get_operations(const struct fuse_operations** operations) {
        *operations = &FuseLFS::operations;
    }

    void* FuseLFS::init(struct fuse_conn_info* conn, struct fuse_config* cfg) {
        FuseLFS::connection = conn;
        FuseLFS::config = cfg;

        return nullptr;
    }

    int FuseLFS::getattr(
        const char* path, struct stat* stat, struct fuse_file_info* fi)
    {
        // Root directory
        if (PATH_ROOT.compare(path) == 0) {
            stat->st_mode = S_IFDIR | 0755;
            stat->st_nlink = 2;
            return 0;
        }
        // Find the file
        else if(true == false){

        }
        // File not found
        else {
            return -ENOENT;
        }
    }

    int FuseLFS::readdir(
        const char* path, void* buffer, fuse_fill_dir_t directory_type,
        off_t offset, struct fuse_file_info *, enum fuse_readdir_flags)
    {
        // Only support root directory for now
        if (PATH_ROOT.compare(path) != 0)
            return -ENOENT;

        // I don't think this would still be necessary with fuse > 3
        directory_type(buffer, ".", nullptr, 0, (fuse_fill_dir_flags) 0);
        directory_type(buffer, "..", nullptr, 0, (fuse_fill_dir_flags) 0);

        // TOOD(Dantali0n): Fill directory with all files

        return 0;
    }

    int FuseLFS::open(const char* path, struct fuse_file_info* fi) {
        return 0;
    }

    int FuseLFS::create(
        const char* path, mode_t mode, struct fuse_file_info* fi)
    {
        return 0;
    }

    int FuseLFS::read(
        const char* path, char* buffer, size_t size, off_t offset,
        struct fuse_file_info* fi)
    {
        return 0;
    }

    int FuseLFS::write(
        const char* path, const char* buffer, size_t size, off_t offset,
        struct fuse_file_info* fi)
    {
        return 0;
    }

    int FuseLFS::unlink(const char* path) {
        return 0;
    }

}