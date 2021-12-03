# MIT License
#
# Copyright (c) 2021 Dantali0n
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Order of including submodules is of importance as the can only depend on
# another if that has been included before the current one. This is a known
# limitation either potentially solved using CMake globals or with loosely
# coupled interfaces.

# Generate output to stdout and stderr using log levels
add_subdirectory(output)

# Commandline argument parsing and stripping
add_subdirectory(arguments)

# Performance measurements
add_subdirectory(measurements)

# Register BPF helper headers
add_subdirectory(bpf_helpers)

# Register BPF programs and generate headers
add_subdirectory(bpf_programs)

# SPDK initialization and data wrappers
add_subdirectory(spdk_init)

# Emulated new NVMe command set for Computational Storage Devices (CSx)
add_subdirectory(nvme_csd)

# Abstracted NVMe ZNS backends
add_subdirectory(nvme_zns)

# Userspace LFS filesystem with FUSE
add_subdirectory(fuse_lfs)

# Targets of ZCSD prototype
add_subdirectory(zcsd)

qemucsd_include_directories()

set(FUSE_ENTRY_SOURCE
    fuse_entry.cxx
)

message("${PRJ_PRX}: fuse-entry cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(fuse-entry ${FUSE_ENTRY_SOURCE})

get_property(local_modules GLOBAL PROPERTY module_list)

target_link_libraries(
    fuse-entry
    ${local_modules}
    ${${PRJ_PRX}_LIBRARIES}
)

add_dependencies(fuse-entry libfuse-build)

qemucsd_target_postprocess(fuse-entry)
