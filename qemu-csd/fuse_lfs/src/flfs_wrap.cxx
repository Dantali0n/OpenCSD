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

#include "flfs_wrap.hpp"

namespace qemucsd::fuse_lfs {

    const struct fuse_lowlevel_ops FuseLFSWrapper::operations = {
        .init        = FuseLFSWrapper::init,
        .destroy     = FuseLFSWrapper::destroy,
        .lookup      = FuseLFSWrapper::lookup,
        .forget      = FuseLFSWrapper::forget,
        .getattr     = FuseLFSWrapper::getattr,
        .setattr     = FuseLFSWrapper::setattr,
        .mkdir       = FuseLFSWrapper::mkdir,
        .unlink      = FuseLFSWrapper::unlink,
        .open        = FuseLFSWrapper::open,
        .read        = FuseLFSWrapper::read,
        .write       = FuseLFSWrapper::write,
        .release     = FuseLFSWrapper::release,
        //        .fsync      = FuseLFSFuse::fsync,
        .readdir     = FuseLFSWrapper::readdir,
        .statfs      = FuseLFSWrapper::statfs,
        .setxattr    = FuseLFSWrapper::setxattr,
        .getxattr    = FuseLFSWrapper::getxattr,
        .listxattr   = FuseLFSWrapper::listxattr,
        .removexattr = FuseLFSWrapper::removexattr,
        .create      = FuseLFSWrapper::create,
    };

    FuseLFS* FuseLFSWrapper::flfs = nullptr;

    int FuseLFSWrapper::initialize(int argc, char* argv[],
                                   nvme_zns::NvmeZnsBackend* nvme)
    {
        try{
            FuseLFS(argc, argv, nvme, &operations);
            return 0;
        }
        catch(...) {
            return -1;
        }
    }

    void FuseLFSWrapper::init(void *userdata, struct fuse_conn_info *conn) {
        flfs = (FuseLFS*) userdata;
        flfs->init(nullptr, conn);
    }

    void FuseLFSWrapper::destroy(void *userdata) {
        flfs->destroy(userdata);
    }

    void FuseLFSWrapper::lookup(fuse_req_t req, fuse_ino_t parent,
                                const char *name)
    {
        flfs->lookup(req, parent, name);
    }

    void FuseLFSWrapper::forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup) {
        flfs->forget(req, ino, nlookup);
    }

    void FuseLFSWrapper::getattr(fuse_req_t req, fuse_ino_t ino,
                                 struct fuse_file_info *fi)
    {
        flfs->getattr(req, ino, fi);
    }

    void FuseLFSWrapper::setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                                 int to_set, struct fuse_file_info *fi)
    {
        flfs->setattr(req, ino, attr, to_set, fi);
    }

    void FuseLFSWrapper::readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                                 off_t off, struct fuse_file_info *fi)
    {
        flfs->readdir(req, ino, size, off, fi);
    }

    void FuseLFSWrapper::open(fuse_req_t req, fuse_ino_t ino,
                              struct fuse_file_info *fi)
    {
        flfs->open(req, ino, fi);
    }

    void FuseLFSWrapper::release(fuse_req_t req, fuse_ino_t ino,
                                 struct fuse_file_info *fi)
    {
        flfs->release(req, ino, fi);
    }

    void FuseLFSWrapper::create(fuse_req_t req, fuse_ino_t parent, const char *name,
                                mode_t mode, struct fuse_file_info *fi)
    {
        flfs->create(req, parent, name, mode, fi);
    }

    void FuseLFSWrapper::mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
                               mode_t mode)
    {
        flfs->mkdir(req, parent, name, mode);
    }

    void FuseLFSWrapper::read(fuse_req_t req, fuse_ino_t ino, size_t size,
                              off_t off, struct fuse_file_info *fi)
    {
        flfs->read(req, ino, size, off, fi);
    }

    void FuseLFSWrapper::write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                               size_t size, off_t off, struct fuse_file_info *fi)
    {
        flfs->write(req, ino, buf, size, off, fi);
    }

    void FuseLFSWrapper::statfs(fuse_req_t req, fuse_ino_t ino) {
        flfs->statfs(req, ino);
    }

    void FuseLFSWrapper::fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
                               struct fuse_file_info *fi)
    {
        flfs->fsync(req, ino, datasync, fi);
    }

    void FuseLFSWrapper::rename(fuse_req_t req, fuse_ino_t parent,
                                const char *name, fuse_ino_t newparent, const char *newname,
                                unsigned int flags)
    {
        flfs->rename(req, parent, name, newparent, newname, flags);
    }

    void FuseLFSWrapper::unlink(fuse_req_t req, fuse_ino_t parent,
                                const char *name)
    {
        flfs->unlink(req, parent, name);
    }

    void FuseLFSWrapper::rmdir(fuse_req_t req, fuse_ino_t parent,
                               const char *name)
    {
        flfs->rmdir(req, parent, name);
    }

    void FuseLFSWrapper::getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                                  size_t size)
    {
        flfs->getxattr(req, ino, name, size);
    }

    void FuseLFSWrapper::setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                                  const char *value, size_t size, int flags)
    {
        flfs->setxattr(req, ino, name, value, size, flags);
    }

    void FuseLFSWrapper::listxattr(fuse_req_t req, fuse_ino_t ino, size_t size) {
        flfs->listxattr(req, ino, size);
    }

    void FuseLFSWrapper::removexattr(fuse_req_t req, fuse_ino_t ino,
                                     const char *name)
    {
        flfs->removexattr(req, ino, name);
    }

}