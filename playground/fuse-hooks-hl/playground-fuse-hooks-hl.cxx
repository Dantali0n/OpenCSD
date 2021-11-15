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

#include <fuse3/fuse.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <cstddef>

static void *init(struct fuse_conn_info *conn,
                        struct fuse_config *cfg)
{
    (void) conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi)
{
    (void) fi;
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, "/test") == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 512;
    }
    else
        res = -ENOENT;

    return res;
}

static int readlink(const char *path, char *link, size_t size) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int f_mknod(const char *path, mode_t mode, dev_t device) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int f_mkdir(const char *path, mode_t) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int unlink(const char *path) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int rmdir(const char *path) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int symlink(const char *path, const char *link) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int f_rename(const char *path, const char *new_path, unsigned int flags) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int link(const char *path, const char *link) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int f_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int f_chown(
    const char *path, uid_t user_id, gid_t group_id, struct fuse_file_info *fi)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int f_truncate(
    const char *path, off_t offset, struct fuse_file_info *fi)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int open(const char *path, struct fuse_file_info *fi)
{
    // Allow all file modes for test as fadvise opens with O_RDWR
    if(strcmp(path, "/test") == 0 && (fi->flags & O_ACCMODE))
        return 0;

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int write(const char *path, const char *data, size_t size, off_t offset,
                 struct fuse_file_info *file_info)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int statfs(const char *path, struct statvfs *stat_fs) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int flush(const char *path, struct fuse_file_info *file_info) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int release(const char *path, struct fuse_file_info *file_info) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int fsync(const char *path, int fd, struct fuse_file_info *file_info) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int setxattr(
    const char *path, const char *data, const char *data2, size_t size,
    int atttribs)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int getxattr(
    const char *path, const char *data, char *data2, size_t size)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int listxattr(
    const char *path, char *data, size_t size)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int f_removexattr(
    const char *path, const char *data)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int opendir(const char *path, struct fuse_file_info *file_info) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi,
                   enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, (enum fuse_fill_dir_flags) 0);
    filler(buf, "..", NULL, 0, (enum fuse_fill_dir_flags) 0);

    return 0;
}

static int releasedir(const char *path, struct fuse_file_info *file_info) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int fsyncdir(const char *path, int fd, struct fuse_file_info *fi) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int init(fuse_conn_info *conn) {
    if(conn == nullptr)
        return 0;

    return -1;
}

static void destroy(void *callback) {
    if(callback == nullptr)
        return;

    return;
}

static int access(const char *path, int fd) {
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int create(
    const char *path, mode_t mode, struct fuse_file_info *file_info)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int ftruncate(
    const char *path, off_t offset, struct fuse_file_info *file_info)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int fgetattr(
    const char *path, struct stat *stats, struct fuse_file_info *file_info)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int lock(
    const char *path, struct fuse_file_info *file_info, int cmd,
    struct flock *lock)
{
    if(strcmp(path, "/") == 0)
        return 0;

    if(strcmp(path, "/test") == 0)
        return 0;

    return -1;
}

static int utimens(
    const char *path, const struct timespec tv[2],
    struct fuse_file_info *fi)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static int bmap(
    const char *path, size_t blocksize, uint64_t *idx)
{
    if(strcmp(path, "/") == 0)
        return 0;

    return -1;
}

static const struct fuse_operations hello_oper = {
    .getattr	 = getattr,
    .readlink    = readlink,
//    .getdir      = getdir,
    .mknod       = f_mknod,
    .mkdir       = f_mkdir,
    .unlink      = unlink,
    .symlink     = symlink,
    .rename      = f_rename,
    .link        = link,
    .chmod       = f_chmod,
    .chown       = f_chown,
    .truncate    = f_truncate,
//    .utime       = DEPRECATED
    .open		 = open,
    .read		 = read,
    .write       = write,
    .statfs      = statfs,
    .flush       = flush,
    .release     = release,
    .fsync       = fsync,
    .setxattr    = setxattr,
    .getxattr    = getxattr,
    .listxattr   = listxattr,
    .removexattr = f_removexattr,
    .opendir     = opendir,
    .readdir	 = readdir,
    .releasedir  = releasedir,
    .fsyncdir    = fsyncdir,
    .init        = init,
    .destroy     = destroy,
    .access      = access,
    .create      = create,
//    .ftruncate   = ftruncate,
//    .fgetattr    = fgetattr,
    .lock        = lock,
    .utimens     = utimens,
    .bmap        = bmap,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &hello_oper, NULL);
}
