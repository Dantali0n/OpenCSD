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

add_dependencies(qemucsd_arguments boost-build)

# Add qemucsd_arguments to the modules
add_qemucsd_module(qemucsd_arguments)
