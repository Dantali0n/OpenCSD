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
    #include <assert.h>
    #include <errno.h>
    #include <fuse3/fuse_lowlevel.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
}

#include <set>

const size_t TEST_SIZE = 4096;

const char* CSD_ENABLE_KEY = "user.process.csd_read";

// I am making these equal length because Hacks~
const char* CSD_ENABLE_VALUE_TRUE = "yes";
const char* CSD_ENABLE_VALUE_FALSE = "no ";

std::set<pid_t> csd_enabled;

struct fuse_conn_info *connection = nullptr;

void x_init(void *userdata, struct fuse_conn_info *conn) {
    connection = conn;
}

int x_stat(fuse_ino_t ino, struct stat *stbuf) {
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

void x_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    struct stat stbuf = {0};
    if (x_stat(ino, &stbuf) == -1)
        fuse_reply_err(req, ENOENT);
    else
        fuse_reply_attr(req, &stbuf, 1.0);
}

void x_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    struct fuse_entry_param e;
    if (parent != 1 || strcmp(name, "test") != 0)
        fuse_reply_err(req, ENOENT);
    else {
        memset(&e, 0, sizeof(e));
        e.ino = 2;
        e.attr_timeout = 1.0;
        e.entry_timeout = 1.0;
        x_stat(e.ino, &e.attr);

        fuse_reply_entry(req, &e);
    }
}

void x_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
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

    // CSD state is reset upon calling open
    const fuse_ctx* context = fuse_req_ctx(req);
    csd_enabled.erase(context->pid);

    // Can only read files in this demo
    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        fuse_reply_err(req, EACCES);
        return;
    }

    // Open the file at inode 2
    fuse_reply_open(req, fi);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
                      off_t off, size_t maxsize)
{
    if (off < bufsize)
        return fuse_reply_buf(req, buf + off,
                              min(bufsize - off, maxsize));
    else
        return fuse_reply_buf(req, NULL, 0);
}

void x_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t offset,
           struct fuse_file_info *fi)
{
    // Only test file exists
    assert(ino == 2);

    const fuse_ctx* context = fuse_req_ctx(req);

    // Non CSD path
    if(csd_enabled.find(context->pid) == csd_enabled.end()) {
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
    // CSD path
    else {
        const char *response = "42";
        reply_buf_limited(req, response, strlen(response), offset, size);
    }
}

void x_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    if(ino > 2)
        fuse_reply_err(req, ENOENT);
    else
        fuse_reply_err(req, 0);
}

void x_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                const char *value, size_t size, int flags)
{
    const fuse_ctx* context = fuse_req_ctx(req);

    // Ignore other extended attributes, pretend read only to indicate failure
    if(strcmp(CSD_ENABLE_KEY, name) != 0) {
        fuse_reply_err(req, EROFS);
        return;
    }

    csd_enabled.insert(context->pid);
    fuse_reply_err(req, 0);
}

void x_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                size_t size) {
    const fuse_ctx* context = fuse_req_ctx(req);
    size_t real = strlen(CSD_ENABLE_VALUE_TRUE);

    // No extended attribute support apart from CSD_ENABLE_KEY
    if(strcmp(CSD_ENABLE_KEY, name) != 0) {
        fuse_reply_err(req, ENODATA);
        return;
    }

    if (size == 0)
        fuse_reply_xattr(req, real);
    else if(size < real)
        fuse_reply_err(req, ERANGE);
    else
        if(csd_enabled.find(context->pid) != csd_enabled.end())
            fuse_reply_buf(req, CSD_ENABLE_VALUE_TRUE, real);
        else
            fuse_reply_buf(req, CSD_ENABLE_VALUE_FALSE, real);
}

void x_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size) {
    fuse_reply_err(req, 0);
}

void x_removexattr(fuse_req_t req, fuse_ino_t ino, const char *name) {
    fuse_reply_err(req, 0);
}

struct dirbuf {
    char *p;
    size_t size;
};

static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name,
                       fuse_ino_t ino)
{
    struct stat stbuf;
    size_t oldsize = b->size;
    b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    b->p = (char *) realloc(b->p, b->size);
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
    fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
                      b->size);
}

void x_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
               struct fuse_file_info *fi)
{
    if(ino != 1) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    struct dirbuf b;

    memset(&b, 0, sizeof(b));
    dirbuf_add(req, &b, ".", 1);
    dirbuf_add(req, &b, "..", 1);
    dirbuf_add(req, &b, "test", 2);
    reply_buf_limited(req, b.p, b.size, off, size);
    free(b.p);
}

static const struct fuse_lowlevel_ops fuse_oper = {
    .init           = x_init,
    .lookup         = x_lookup,
    .getattr        = x_getattr,
    .open           = x_open,
    .read           = x_read,
    .release        = x_release,
    .readdir        = x_readdir,
    .setxattr       = x_setxattr,
    .getxattr       = x_getxattr,
    .listxattr      = x_listxattr,
    .removexattr    = x_removexattr,
};

int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    struct fuse_loop_config config;
    int ret = -1;

    if (fuse_parse_cmdline(&args, &opts) != 0)
        return 1;
    if (opts.show_help) {
        printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
        fuse_cmdline_help();
        fuse_lowlevel_help();
        ret = 0;
        goto err_out1;
    } else if (opts.show_version) {
        printf("FUSE library version %s\n", fuse_pkgversion());
        fuse_lowlevel_version();
        ret = 0;
        goto err_out1;
    }

    if(opts.mountpoint == NULL) {
        printf("usage: %s [options] <mountpoint>\n", argv[0]);
        printf("       %s --help\n", argv[0]);
        ret = 1;
        goto err_out1;
    }

    se = fuse_session_new(&args, &fuse_oper, sizeof(fuse_oper), NULL);
    if (se == NULL)
        goto err_out1;

    if (fuse_set_signal_handlers(se) != 0)
        goto err_out2;

    if (fuse_session_mount(se, opts.mountpoint) != 0)
        goto err_out3;

    fuse_daemonize(opts.foreground);

    /* Block until ctrl+c or fusermount -u */
    if (opts.singlethread)
        ret = fuse_session_loop(se);
    else {
        config.clone_fd = opts.clone_fd;
        config.max_idle_threads = opts.max_idle_threads;
        ret = fuse_session_loop_mt(se, &config);
    }

    fuse_session_unmount(se);
    err_out3:
    fuse_remove_signal_handlers(se);
    err_out2:
    fuse_session_destroy(se);
    err_out1:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);

    return ret ? 1 : 0;
}