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


set(QEMUCSD_FUSE_LFS_LIBRARIES
    qemucsd_output
    qemucsd_nvme_zns_backend
    qemucsd_spdk_init
    fuse3
)

set(QEMUCSD_FUSE_LFS_SRC
    src/fuse_lfs.cxx
    src/fuse_lfs_constants.cxx
)

set(QEMUCSD_FUSE_LFS_HEADERS
    include/fuse_lfs.hpp
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

add_dependencies(qemucsd_fuse_lfs libfuse-build)

# Add qemucsd_measurements to the modules
add_qemucsd_module(qemucsd_fuse_lfs)
