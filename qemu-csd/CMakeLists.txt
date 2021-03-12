# Commandline argument parsing
add_subdirectory(arguments)

# Register bpf helper headers
add_subdirectory(bpf_helpers)

# Register bpf programs and generate headers
add_subdirectory(bpf_programs)

# SPDK initialization
add_subdirectory(spdk_init)

# Emulated new nvme command set
add_subdirectory(nvm_csd)

qemucsd_include_directories()

set(QEMUCSD_SOURCE
    main.cxx
)

message("${PRJ_PRX}: qemucsd cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(qemucsd ${QEMUCSD_SOURCE})

get_property(local_modules GLOBAL PROPERTY module_list)

target_link_libraries(
    qemucsd
    ${local_modules}
    ${${PRJ_PRX}_LIBRARIES}
)

add_dependencies(qemucsd generate_bpf_zone_int_filter_header)

qemucsd_target_postprocess(qemucsd)