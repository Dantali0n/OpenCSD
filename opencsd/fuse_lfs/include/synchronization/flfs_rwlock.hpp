/**
* MIT License
*
* Copyright (c) 2022 Dantali0n
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

#ifndef QEMU_CSD_FLFS_RWLOCK_HPP
#define QEMU_CSD_FLFS_RWLOCK_HPP

extern "C" {
    #include <pthread.h>
}

#include "flfs_constants.hpp"

namespace qemucsd::fuse_lfs {

    /**
     * STL equivalent of lock_guard to work with pthread reader writer locks
     */
    template<typename pthread_mutex>
    class lock_guard {
    public:
        explicit lock_guard(pthread_mutex &pm, bool write = false) : _pm(pm) {
            if(write)
                pthread_rwlock_wrlock(&this->_pm);
            else
                pthread_rwlock_rdlock(&this->_pm);
        }

        ~lock_guard() {
            pthread_rwlock_unlock(&this->_pm);
        }

        lock_guard(const lock_guard&) = delete;
        lock_guard& operator=(const lock_guard&) = delete;

    private:
        pthread_mutex &_pm;
    };

    /**
     * Manage rwlock initialization and error reporting to reduce duplicate code
     */
    inline void rwlock_init(pthread_rwlock_t *lck, pthread_rwlockattr_t *attr,
        const char* name)
    {
        // Initialize the lock attributes
        if(pthread_rwlockattr_init(attr) != 0) {
            output.error("Failed to initialize ", name, " lock attributes");
        }

        // Disable support for recursive reads but enable writer preference
        if(pthread_rwlockattr_setkind_np(attr,
             PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP) != 0)
        {
            output.error("Invalid arguments for configuring ", name, "lock");
        }

        // Initialize the reader writer lock
        if(pthread_rwlock_init(lck, attr) != 0) {
            output.error("Failed to initialize ", name, " lock");
        }
    }
    /**
     * Manage rwlock destruction and error reporting to reduce duplicate code
     */
    inline void rwlock_destroy(pthread_rwlock_t *lck,
        pthread_rwlockattr_t *attr, const char* name)
    {
        if(pthread_rwlockattr_destroy(attr) != 0) {
            output.error("Failed to destroy ", name);
        }

        if(pthread_rwlock_destroy(lck)!= 0) {
            output.error("Failed to destroy ", name);
        }
    }
}

#endif // QEMU_CSD_FLFS_RWLOCK_HPP