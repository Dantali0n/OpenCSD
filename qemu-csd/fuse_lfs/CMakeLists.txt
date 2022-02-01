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

project(${PRJ_PRX}_fuse_lfs)

add_custom_command(
    OUTPUT ${PRJ_HEADERS}/bpf_helpers_flfs.h
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/include/bpf/bpf_helpers_flfs.h
    ${PRJ_HEADERS}/bpf_helpers_flfs.h
    DEPENDS include/bpf/bpf_helpers_flfs.h
)

#add_custom_command(
#    OUTPUT ${PRJ_HEADERS}/fixedptc.h
#    COMMAND ${CMAKE_COMMAND} -E copy
#    ${CMAKE_CURRENT_SOURCE_DIR}/include/bpf/fixedptc.h
#    ${PRJ_HEADERS}/fixedptc.h
#    DEPENDS include/bpf/fixedptc.h
#)

add_custom_target(generate_bpf_helpers_flfs DEPENDS ${PRJ_HEADERS}/bpf_helpers_flfs.h) # ${PRJ_HEADERS}/fixedptc.h)

add_subdirectory(src/bpf)

set(QEMUCSD_FUSE_LFS_LIBRARIES
    qemucsd_output
    qemucsd_nvme_csd
    qemucsd_nvme_zns_backend
    qemucsd_spdk_init
    fuse3
)

set(QEMUCSD_FUSE_LFS_SRC
    src/flfs.cxx
    src/flfs_csd.cxx
    src/flfs_dirtyblock.cxx
    src/concurrent_datastructures/flfs_file_handle.cxx
    src/concurrent_datastructures/flfs_inode_entry.cxx
    src/concurrent_datastructures/flfs_inode_lba.cxx
    src/concurrent_datastructures/flfs_nlookup.cxx
    src/concurrent_datastructures/flfs_snapshot.cxx
    src/flfs_read.cxx
    src/flfs_init.cxx
    src/flfs_superblock.cxx
    src/flfs_wrap.cxx
    src/flfs_write.cxx
)

set(QEMUCSD_FUSE_LFS_HEADERS
    include/flfs.hpp
    include/flfs_csd.hpp
    include/flfs_dirtyblock.hpp
    include/concurrent_datastructures/flfs_file_handle.hpp
    include/concurrent_datastructures/flfs_inode_entry.hpp
    include/concurrent_datastructures/flfs_inode_lba.hpp
    include/concurrent_datastructures/flfs_nlookup.hpp
    include/concurrent_datastructures/flfs_snapshot.hpp
    include/flfs_read.hpp
    include/flfs_init.hpp
    include/flfs_superblock.hpp
    include/flfs_wrap.hpp
    include/flfs_write.hpp
)

# Add qemucsd_measurements to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

add_library(
    qemucsd_fuse_lfs STATIC
    ${QEMUCSD_FUSE_LFS_SRC}
    ${QEMUCSD_FUSE_LFS_HEADERS}
)
set_target_properties(${PRJ_LIB} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(
    qemucsd_fuse_lfs
    ${QEMUCSD_FUSE_LFS_LIBRARIES}
)

add_dependencies(qemucsd_fuse_lfs
    libfuse-build
    generate_bpf_helpers
    generate_bpf_helpers_flfs
    generate_flfs_bpf_read
    generate_flfs_bpf_read_average
    generate_flfs_bpf_read_entropy
)

# Add qemucsd_measurements to the modules
add_qemucsd_module(qemucsd_fuse_lfs)

# Enable backward or other definitions for Debug builds
qemucsd_target_postprocess(qemucsd_fuse_lfs)
