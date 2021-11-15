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

#define FUSE_USE_VERSION 36

extern "C" {
    #include <fuse3/fuse.h>
    #include <fcntl.h>
    #include <string.h>
    #include <errno.h>
    #include <stdlib.h>
}

#include <set>

const char* TEST_PATH = "/test";
const size_t TEST_SIZE = 4096;

std::set<pid_t> csd_enabled;

struct fuse_conn_info *connection = nullptr;
struct fuse_config *config = nullptr;

int x_getattr(const char *path, struct stat *stats, struct fuse_file_info *fi) {
    memset(stats, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stats->st_mode = S_IFDIR | 0755;
        stats->st_nlink = 2;
    }
    else if(strcmp(path, TEST_PATH) == 0) {
        stats->st_mode = S_IFREG | 0444;
        stats->st_nlink = 1;
        stats->st_size = TEST_SIZE;
    }
    else
        return -ENOENT;

    return 0;
}

void* x_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    connection = conn;
    config = cfg;

    return nullptr;
}

int x_open(const char *path, struct fuse_file_info *fi) {
    // Only test file exists
    if (strcmp(path, TEST_PATH) != 0)
        return -ENOENT;

    // Only read access possible
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    return 0;
}

int x_read(const char *path, char *buffer, size_t size, off_t offset,
           struct fuse_file_info *fi)
{
    // Only test file exists
    if(strcmp(path, TEST_PATH) != 0)
        return -ENOENT;

    if (offset < TEST_SIZE) {
        if (offset + size > TEST_SIZE)
            size = TEST_SIZE - offset;

        // Non CSD path
        for(size_t i = 0; i < offset; i++) {
            rand();
        }

        char value = 0;
        for(size_t i = 0; i < size; i++) {
            value = rand();
            memcpy(buffer + i, &value, 1);
        }
    } else
        size = 0;

    return size;
}

int x_release(const char *path, struct fuse_file_info *fi) {
    if(strcmp(path, "/") != 0)
        return 0;

    if(strcmp(path, TEST_PATH) != 0)
        return 0;

    return -ENOENT;
}

int x_setxattr(const char *path, const char *name, const char *value,
               size_t size, int flags)
{

}

int x_getxattr(const char *path, const char *name, char *value, size_t size) {

}

int x_listxattr(const char *path,  char *list, size_t size) {

}

int x_removexattr(const char *path, const char *list) {

}

int x_readdir(const char *path, void *buffer, fuse_fill_dir_t fill_dir,
              off_t offset, struct fuse_file_info *fi,
              enum fuse_readdir_flags flags)
{
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    fill_dir(buffer, ".", NULL, 0, (enum fuse_fill_dir_flags) 0);
    fill_dir(buffer, "..", NULL, 0, (enum fuse_fill_dir_flags) 0);
    fill_dir(buffer, "test", NULL, 0, (enum fuse_fill_dir_flags) 0);

    return 0;
}

static const struct fuse_operations fuse_oper = {
    .getattr        = x_getattr,
    .open           = x_open,
    .read           = x_read,
    .release        = x_release,
    .setxattr       = x_setxattr,
    .getxattr       = x_getxattr,
    .listxattr      = x_listxattr,
    .removexattr    = x_removexattr,
    .readdir        = x_readdir,
    .init           = x_init,

};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &fuse_oper, NULL);
}