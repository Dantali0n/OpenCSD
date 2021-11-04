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

#ifdef QEMUCSD_DEBUG
    #include <backward.hpp>
    using namespace backward;
#endif

/**
* Stack trace printer when encountering seg faults.
*/
struct sigaction glob_sigaction;
void segfault_handler(int signal, siginfo_t *si, void *arg) {
    #ifdef QEMUCSD_DEBUG
        StackTrace st; st.load_here(32);
        Printer p; p.print(st);
    #endif
    exit(1);
}

#include "arguments.hpp"
#include "fuse_lfs.hpp"

/**
 * Entrypoint for fuse LFS filesystem
 */
int main(int argc, char* argv[]) {
    int qemu_argc = 0;
    char** qemu_argv = nullptr;

    int fuse_argc = 0;
    char** fuse_argv = nullptr;

    qemucsd::arguments::t_auto_strip_args stripped_args;

    const struct fuse_operations* ops;
    qemucsd::arguments::options opts;

    // Setup segfault handler to print backward stacktraces
    sigemptyset(&glob_sigaction.sa_mask);
    glob_sigaction.sa_sigaction = segfault_handler;
    glob_sigaction.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &glob_sigaction, nullptr);

    try {
        // Parse commandline arguments
        qemucsd::arguments::auto_strip_args(argc, argv, &stripped_args);

        // First set of arguments is for qemucsd
        if(stripped_args.size() >= 2) {
            qemu_argc = stripped_args.at(1).first;
            qemu_argv = stripped_args.at(1).second.data();
        }

        qemucsd::arguments::parse_args(qemu_argc, qemu_argv, &opts);

        // Second set of arguments is for fuse
        if(stripped_args.size() >= 3) {
            fuse_argc = stripped_args.at(2).first;
            fuse_argv = stripped_args.at(2).second.data();
        }

        // Get fuse operations structure
        qemucsd::fuse_lfs::FuseLFS::get_operations(&ops);
        return fuse_main(fuse_argc, fuse_argv, ops, nullptr);
    }
    catch(...) {
        #ifdef QEMUCSD_DEBUG
            StackTrace st; st.load_here(32);
            Printer p; p.print(st);
        #endif
    }
}