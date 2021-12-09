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

project(${PRJ_PRX}_arguments)

#find_package(
#    Boost 1.32 COMPONENTS
#    program_options
#    REQUIRED
#)

set(QEMUCSD_ARGUMENTS_LIBRARIES
    boost_program_options
)

set(QEMUCSD_ARGUMENTS_SRC
    src/arguments.cxx
)

set(QEMUCSD_ARGUMENTS_HEADERS
    include/arguments.hpp
)

# Add qemucsd_arguments to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

add_library(
    qemucsd_arguments STATIC
    ${QEMUCSD_ARGUMENTS_SRC}
    ${QEMUCSD_ARGUMENTS_HEADERS}
)
set_target_properties(${PRJ_LIB} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(
    qemucsd_arguments
    -Wl,--whole-archive
    spdk_log.a spdk_env_dpdk.a spdk_nvme.a spdk_util.a spdk_sock.a spdk_json.a
    spdk_rpc.a spdk_jsonrpc.a
    -Wl,--no-whole-archive
    rte_eal.a rte_mempool.a rte_telemetry.a rte_ring.a rte_kvargs.a rte_bus_pci.a
    rte_pci.a rte_vhost.a rte_power.a
    isal.a uuid numa dl
#    spdk spdk_log spdk_env_dpdk
#    rte_eal.a rte_mempool.a rte_telemetry.a rte_ring.a rte_kvargs.a rte_bus_pci.a
#    rte_pci.a rte_vhost.a rte_power.a
#    isal.a uuid numa dl
    ${QEMUCSD_ARGUMENTS_LIBRARIES}
)

add_dependencies(qemucsd_arguments boost-build spdk-build dpdk-build isa-l-build)

# Add qemucsd_arguments to the modules
add_qemucsd_module(qemucsd_arguments)

qemucsd_target_postprocess(qemucsd_arguments)
