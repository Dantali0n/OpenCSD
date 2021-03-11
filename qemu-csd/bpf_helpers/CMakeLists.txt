project(${PRJ_PRX}_bpf_helpers)

# Add qemucsd_bpf_helpers to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

#add_library(
#        qemucsd_nvm_csd STATIC
#        ${QEMUCSD_NVM_CSD_SRC}
#        ${QEMUCSD_NVM_CSD_HEADERS}
#)
#set_target_properties(${PRJ_LIB} PROPERTIES LINKER_LANGUAGE CXX)
#target_link_libraries(
#        qemucsd_nvm_csd
#        -Wl,--whole-archive
#        spdk_log.a spdk_env_dpdk.a spdk_nvme.a spdk_util.a spdk_sock.a spdk_json.a
#        spdk_rpc.a spdk_jsonrpc.a
#        -Wl,--no-whole-archive
#        rte_eal.a rte_mempool.a rte_telemetry.a rte_ring.a rte_kvargs.a rte_bus_pci.a
#        rte_pci.a rte_vhost.a rte_power.a
#        isal.a uuid numa dl
#        #    spdk spdk_log spdk_env_dpdk
#        #    rte_eal.a rte_mempool.a rte_telemetry.a rte_ring.a rte_kvargs.a rte_bus_pci.a
#        #    rte_pci.a rte_vhost.a rte_power.a
#        #    isal.a uuid numa dl
#        ${QEMUCSD_NVM_CSD_LIBRARIES}
#)
#
#add_dependencies(qemucsd_nvm_csd spdk-build dpdk-build isa-l-build)
#
## Add qemucsd_nvm_csd to the modules
#add_qemucsd_module(qemucsd_nvm_csd)
