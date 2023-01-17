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

project(${PRJ_PRX}_nvme_csd)


set(QEMUCSD_NVME_CSD_LIBRARIES
    qemucsd_nvme_zns_backend
    qemucsd_output
    ubpf bpf elf z
)

set(QEMUCSD_NVME_CSD_SRC
    src/nvme_csd.cxx
)

set(QEMUCSD_NVME_CSD_HEADERS
    include/nvme_csd.hpp
)

# Add qemucsd_nvm_csd to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

add_library(
    qemucsd_nvme_csd STATIC
    ${QEMUCSD_NVME_CSD_SRC}
    ${QEMUCSD_NVME_CSD_HEADERS}
)
set_target_properties(${PRJ_LIB} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(
    qemucsd_nvme_csd
    ${QEMUCSD_NVME_CSD_LIBRARIES}
)

add_dependencies(qemucsd_nvme_csd ubpf-build libbpf-build)

# Add qemucsd_nvm_csd to the modules
add_qemucsd_module(qemucsd_nvme_csd)

qemucsd_target_postprocess(qemucsd_nvme_csd)
