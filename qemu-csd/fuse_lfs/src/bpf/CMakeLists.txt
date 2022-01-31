qemucsd_include_directories()

# See bpf_programs/CMakeLists.txt for how this works

set(BPF_C_FLAGS -g -O2 -target bpf -I ${PRJ_ROOT}/include)
use_bpf_compiler_c()
add_custom_command(
    OUTPUT ${PRJ_BINARIES}/bpf_flfs_read.o
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${CMAKE_CURRENT_SOURCE_DIR}/flfs_bpf_read.c -o ${PRJ_BINARIES}/bpf_flfs_read.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/flfs_bpf_read.c generate_bpf_helpers generate_bpf_helpers_flfs
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(generate_flfs_bpf_read
    DEPENDS ${PRJ_BINARIES}/bpf_flfs_read.o
)



add_custom_command(
    OUTPUT ${PRJ_BINARIES}/bpf_flfs_read_average.o
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${CMAKE_CURRENT_SOURCE_DIR}/flfs_bpf_read_average.c -o ${PRJ_BINARIES}/bpf_flfs_read_average.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/flfs_bpf_read_average.c generate_bpf_helpers generate_bpf_helpers_flfs
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(generate_flfs_bpf_read_average
    DEPENDS ${PRJ_BINARIES}/bpf_flfs_read_average.o
)



add_custom_command(
    OUTPUT ${PRJ_BINARIES}/bpf_flfs_read_entropy.o
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${CMAKE_CURRENT_SOURCE_DIR}/flfs_bpf_read_entropy.c -o ${PRJ_BINARIES}/bpf_flfs_read_entropy.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/flfs_bpf_read_entropy.c generate_bpf_helpers generate_bpf_helpers_flfs
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(generate_flfs_bpf_read_entropy
    DEPENDS ${PRJ_BINARIES}/bpf_flfs_read_entropy.o
)

use_default_compiler_c()