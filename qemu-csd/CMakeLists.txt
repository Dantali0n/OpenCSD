# Commandline argument parsing
add_subdirectory(arguments)

# Register bpf helper headers
add_subdirectory(bpf_helpers)

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

qemucsd_target_postprocess(qemucsd)