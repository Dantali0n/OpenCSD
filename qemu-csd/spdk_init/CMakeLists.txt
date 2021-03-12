project(${PRJ_PRX}_spdk_init)


set(QEMUCSD_SPDK_INIT_LIBRARIES
    #    boost_program_options
)

set(QEMUCSD_SPDK_INIT_SRC
    src/spdk_init.cxx
)

set(QEMUCSD_SPDK_INIT_HEADERS
    include/spdk_init.hpp
)

# Add qemucsd_spdk_init to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

add_library(
    qemucsd_spdk_init STATIC
    ${QEMUCSD_SPDK_INIT_SRC}
    ${QEMUCSD_SPDK_INIT_HEADERS}
)
set_target_properties(${PRJ_LIB} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(
    qemucsd_spdk_init
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
    ${QEMUCSD_SPDK_INIT_LIBRARIES}
)

add_dependencies(qemucsd_spdk_init spdk-build dpdk-build isa-l-build)

# Add qemucsd_spdk_init to the modules
add_qemucsd_module(qemucsd_spdk_init)
