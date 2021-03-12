project(${PRJ_PRX}_bpf_helpers)

# Add qemucsd_bpf_helpers to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

add_custom_command(
    OUTPUT ${PRJ_HEADERS}/bpf_helpers_prog.h
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/include/bpf_helpers_prog.h
    ${PRJ_HEADERS}/bpf_helpers_prog.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/bpf_helpers_prog.h
)

add_custom_target(generate_qemucsd_bpf_helpers DEPENDS ${PRJ_HEADERS}/bpf_helpers_prog.h)

# target - generate_qemucsd_bpf_helpers
