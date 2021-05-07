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

#project(playground)

qemucsd_include_directories()

get_property(test DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
message("Includes playground: ${test}")
get_property(test DIRECTORY PROPERTY LINK_DIRECTORIES)
message("Links playground: ${test}")

# Add every playground subdirectory target to this list
add_custom_target(
    playground
    DEPENDS
    play-backward
    play-boost-locale
    play-example
    play-generate-integer-data
    play-generic-ebpf
    play-libbpf-bpf-load
    play-libbpf-bootstrap
    play-filter
    play-spdk
    play-ubpf
    play-spdk-find-fill-filter
)

set(${PRJ_PRX}_PLAYGROUND_LIBRARIES

)

set(${PRJ_PRX}_PLAY_LIBRARIES_PACK
    ${${PRJ_PRX}_LIBRARIES}
    ${${PRJ_PRX}_PLAYGROUND_LIBRARIES}
)

# ------------------- #
# Playground backward #
# ------------------- #

set(PLAYGROUND_BACKWARD_SOURCE
    backward/playground-backward.cxx
)

message("${PRJ_PRX}: playground backward cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-backward ${PLAYGROUND_BACKWARD_SOURCE})


target_link_libraries(
    play-backward
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-backward)

# ----------------------- #
# Playground boost_locale #
# ----------------------- #

set(PLAYGROUND_BOOST_LOCALE_SOURCE
    boost-locale/boost-locale.cxx
)

message("${PRJ_PRX}: playground boost-locale cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-boost-locale ${PLAYGROUND_BOOST_LOCALE_SOURCE})

target_compile_options(play-boost-locale PRIVATE -Wno-deprecated-declarations)
target_link_libraries(
    play-boost-locale
    boost_thread
    boost_locale
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

add_dependencies(play-boost-locale boost-build)

qemucsd_target_postprocess(play-boost-locale)

# ------------------ #
# Playground example #
# ------------------ #

set(PLAYGROUND_EXAMPLE_SOURCE
    example/playground-example.cxx
)

message("${PRJ_PRX}: playground example cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-example ${PLAYGROUND_EXAMPLE_SOURCE})

target_link_libraries(
    play-example
    qemucsd_arguments
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-example)

# ------------------------------------- #
# Playground play-generate-integer-data #
# ------------------------------------- #

set(PLAYGROUND_GENERATE_INTEGER_SOURCE
    generate-integer-data/play-integer-gen.cxx
)

message("${PRJ_PRX}: playground generate-integer-data cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-generate-integer-data ${PLAYGROUND_GENERATE_INTEGER_SOURCE})

target_link_libraries(
    play-generate-integer-data
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-generate-integer-data)

# ----------------------- #
# Playground generic-ebpf #
# ----------------------- #

set(PLAYGROUND_GENERIC_EBPF_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/generic-ebpf
)

# Sources used to generate BPF bytecode
set(PLAYGROUND_GENERIC_EBPF_BYTECODE_SOURCE
    generic-ebpf/hello_world.c
)

# Set custom compiler flags to generate BPF bytecode
set(BPF_C_FLAGS -g -O2 -target bpf -D__TARGET_ARCH_x86 -I ${PRJ_ROOT}/include)
# Change compiler to BPF compiler, Clang
use_bpf_compiler_c()
# Custom commands need to be used when using alternative compiler,
# add_executable / add_library CAN NOT BE USED!
# Generate the BPF bytecode
add_custom_command(
    OUTPUT ${PLAYGROUND_GENERIC_EBPF_DIRECTORY}/bpf_bootstrap_program.o
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${PLAYGROUND_GENERIC_EBPF_BYTECODE_SOURCE} -o ${PLAYGROUND_GENERIC_EBPF_DIRECTORY}/bpf_bootstrap_program.o
    DEPENDS ${PLAYGROUND_GENERIC_EBPF_BYTECODE_SOURCE} generate_vmlinux generate_bpftool
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#add_custom_target(generate_bpf_bootstrap_program
#    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpf_bootstrap_program.o generate_vmlinux generate_bpftool
#)

# Change back to the regular compiler
use_default_compiler_c()

# Generate the BPF program header using bpftool, depend on the bytecode file
add_custom_command(
    OUTPUT ${PLAYGROUND_GENERIC_EBPF_DIRECTORY}/bpf_bootstrap_program.h
    COMMAND ${PRJ_ROOT}/bin/bpftool gen skeleton ${PLAYGROUND_GENERIC_EBPF_DIRECTORY}/bpf_bootstrap_program.o > ${PLAYGROUND_GENERIC_EBPF_DIRECTORY}/bpf_bootstrap_program.h
    DEPENDS ${PLAYGROUND_GENERIC_EBPF_DIRECTORY}/bpf_bootstrap_program.o
    WORKING_DIRECTORY ${PLAYGROUND_GENERIC_EBPF_DIRECTORY}
)

# Generate a target that depends on the output file of the custom command, used
# for setting up dependency with runtime program.
add_custom_target(generate_generic_ebpf_header
    DEPENDS ${PLAYGROUND_GENERIC_EBPF_DIRECTORY}/bpf_bootstrap_program.h
)

set(PLAYGROUND_GENERIC_EBPF_SOURCE
    generic-ebpf/playground-generic-ebpf.cxx
)

set(PLAYGROUND_GENERIC_EBPF_HEADERS
    generic-ebpf/bpf_bootstrap_program.h
)

#include_directories(${CMAKE_SOURCE_DIR}/dependencies/linux/samples/bpf)
#include_directories(${CMAKE_SOURCE_DIR}/dependencies/linux/tools/testing/selftests/bpf)

message("${PRJ_PRX}: playground play-generic-ebpf cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-generic-ebpf ${PLAYGROUND_GENERIC_EBPF_SOURCE} ${PLAYGROUND_GENERIC_EBPF_HEADERS})

add_dependencies(play-generic-ebpf generic-ebpf-build libbpf-build generate_generic_ebpf_header)

#add_custom_command(TARGET play-libbpf-bpf-load PRE_BUILD
#    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${PLAYGROUND_LIBBPF_BPF_LOAD_BYTECODE_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_program.o
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#)

target_link_libraries(
    play-generic-ebpf
    ebpf
    bpf
    elf
    z
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-generic-ebpf)

# --------------------------- #
# Playground libbpf-bootstrap #
# --------------------------- #

set(PLAYGROUND_LIBBPF_BOOTSTRAP_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/libbpf-bootstrap
)

# Sources used to generate BPF bytecode
set(PLAYGROUND_LIBBPF_BOOTSTRAP_BYTECODE_SOURCE
    libbpf-bootstrap/hello_world.c
)

# Set custom compiler flags to generate BPF bytecode
set(BPF_C_FLAGS -g -O2 -target bpf -D__TARGET_ARCH_x86 -I ${PRJ_ROOT}/include)
# Change compiler to BPF compiler, Clang
use_bpf_compiler_c()
# Custom commands need to be used when using alternative compiler,
# add_executable / add_library CAN NOT BE USED!
# Generate the BPF bytecode
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bpf_bootstrap_program.o
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${PLAYGROUND_LIBBPF_BOOTSTRAP_BYTECODE_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_bootstrap_program.o
    DEPENDS ${PLAYGROUND_LIBBPF_BOOTSTRAP_BYTECODE_SOURCE} generate_vmlinux generate_bpftool
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#add_custom_target(generate_bpf_bootstrap_program
#    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpf_bootstrap_program.o generate_vmlinux generate_bpftool
#)

# Change back to the regular compiler
use_default_compiler_c()

# Generate the BPF program header using bpftool, depend on the bytecode file
add_custom_command(
    OUTPUT ${PLAYGROUND_LIBBPF_BOOTSTRAP_DIRECTORY}/bpf_bootstrap_program.h
    COMMAND ${PRJ_ROOT}/bin/bpftool gen skeleton ${CMAKE_CURRENT_BINARY_DIR}/bpf_bootstrap_program.o > ${PLAYGROUND_LIBBPF_BOOTSTRAP_DIRECTORY}/bpf_bootstrap_program.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpf_bootstrap_program.o
    WORKING_DIRECTORY ${PLAYGROUND_LIBBPF_BOOTSTRAP_DIRECTORY}
)

# Generate a target that depends on the output file of the custom command, used
# for setting up dependency with runtime program.
add_custom_target(generate_bpf_bootstrap_header
    DEPENDS ${PLAYGROUND_LIBBPF_BOOTSTRAP_DIRECTORY}/bpf_bootstrap_program.h
)

set(PLAYGROUND_LIBBPF_BOOTSTRAP_SOURCE
    libbpf-bootstrap/playground-libbpf.cxx
)

set(PLAYGROUND_LIBBPF_BOOTSTRAP_HEADERS
    libbpf-bootstrap/bpf_bootstrap_program.h
)

#include_directories(${CMAKE_SOURCE_DIR}/dependencies/linux/samples/bpf)
#include_directories(${CMAKE_SOURCE_DIR}/dependencies/linux/tools/testing/selftests/bpf)

message("${PRJ_PRX}: playground play-libbpf-bootstrap cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-libbpf-bootstrap ${PLAYGROUND_LIBBPF_BOOTSTRAP_SOURCE} ${PLAYGROUND_LIBBPF_BOOTSTRAP_HEADERS})

add_dependencies(play-libbpf-bootstrap libbpf-build generate_bpf_bootstrap_header)

#add_custom_command(TARGET play-libbpf-bpf-load PRE_BUILD
#    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${PLAYGROUND_LIBBPF_BPF_LOAD_BYTECODE_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_program.o
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#)

target_link_libraries(
    play-libbpf-bootstrap
    bpf
    elf
    z
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-libbpf-bootstrap)

# -------------------------- #
# Playground libbpf-bpf-load #
# -------------------------- #

# Sources used to generate BPF bytecoe
set(PLAYGROUND_LIBBPF_BPF_LOAD_BYTECODE_SOURCE
    libbpf-bpf-load/hello_world.c
)

# Set custom compiler flags to generate BPF bytecode
set(BPF_C_FLAGS -O2 -target bpf)
# Change compiler to BPF compiler, Clang
use_bpf_compiler_c()
# Custom commands need to be used when using alternative compiler,
# add_executable / add_library CAN NOT BE USED!
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bpf_program.o
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${PLAYGROUND_LIBBPF_BPF_LOAD_BYTECODE_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_program.o
    DEPENDS ${PLAYGROUND_LIBBPF_BPF_LOAD_BYTECODE_SOURCE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
# Generate a target that depends on the output file of the custom command, used
# for setting up dependency with runtime program.
add_custom_target(generate_bpf_program DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpf_program.o)
# Chabge back to the regular compiler
use_default_compiler_c()

set(PLAYGROUND_LIBBPF_BPF_LOAD_SOURCE
    libbpf-bpf-load/playground-libbpf.cxx
)

include_directories(${CMAKE_SOURCE_DIR}/dependencies/linux/samples/bpf)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/linux/tools/testing/selftests/bpf)

message("${PRJ_PRX}: playground libbpf-bpf-load cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-libbpf-bpf-load ${PLAYGROUND_LIBBPF_BPF_LOAD_SOURCE})

add_dependencies(play-libbpf-bpf-load libbpf-build generate_bpf_program)

#add_custom_command(TARGET play-libbpf-bpf-load PRE_BUILD
#    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${PLAYGROUND_LIBBPF_BPF_LOAD_BYTECODE_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_program.o
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#)

target_link_libraries(
    play-libbpf-bpf-load
    bpfload
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-libbpf-bpf-load)

# ---------------------- #
# Playground play-filter #
# ---------------------- #

set(PLAYGROUND_FILTER_SOURCE
    play-filter/play-filter.cxx
)

message("${PRJ_PRX}: playground filter cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-filter ${PLAYGROUND_FILTER_SOURCE})

target_link_libraries(
    play-filter
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-filter)

# --------------- #
# Playground ubpf #
# --------------- #

set(PLAYGROUND_UBPF_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/ubpf
)

# Sources used to generate BPF bytecode
set(PLAYGROUND_UBPF_BYTECODE_SOURCE
    ubpf/hello_world.c
)

# Set custom compiler flags to generate BPF bytecode
set(BPF_C_FLAGS -g -O2 -target bpf -D__TARGET_ARCH_x86 -I ${PRJ_ROOT}/include)
# Change compiler to BPF compiler, Clang
use_bpf_compiler_c()
# Custom commands need to be used when using alternative compiler,
# add_executable / add_library CAN NOT BE USED!
# Generate the BPF bytecode
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bpf_ubpf_program.o
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${PLAYGROUND_UBPF_BYTECODE_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_ubpf_program.o
    DEPENDS ${PLAYGROUND_UBPF_BYTECODE_SOURCE} generate_vmlinux generate_bpftool
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#add_custom_target(generate_bpf_bootstrap_program
#    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpf_bootstrap_program.o generate_vmlinux generate_bpftool
#)

# Change back to the regular compiler
use_default_compiler_c()

# Generate the BPF program header using bpftool, depend on the bytecode file
add_custom_command(
    OUTPUT ${PLAYGROUND_UBPF_DIRECTORY}/bpf_ubpf_program.h
    COMMAND ${PRJ_ROOT}/bin/bpftool gen skeleton ${CMAKE_CURRENT_BINARY_DIR}/bpf_ubpf_program.o > ${PLAYGROUND_UBPF_DIRECTORY}/bpf_ubpf_program.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpf_ubpf_program.o
    WORKING_DIRECTORY ${PLAYGROUND_UBPF_DIRECTORY}
)

# Generate a target that depends on the output file of the custom command, used
# for setting up dependency with runtime program.
add_custom_target(generate_ubpf_header
    DEPENDS ${PLAYGROUND_UBPF_DIRECTORY}/bpf_ubpf_program.h
)

set(PLAYGROUND_UBPF_SOURCE
    ubpf/playground-ubpf.cxx
)

set(PLAYGROUND_UBPF_HEADERS
    ${PLAYGROUND_UBPF_DIRECTORY}/bpf_ubpf_program.h
)

#include_directories(${CMAKE_SOURCE_DIR}/dependencies/linux/samples/bpf)
#include_directories(${CMAKE_SOURCE_DIR}/dependencies/linux/tools/testing/selftests/bpf)

message("${PRJ_PRX}: playground play-ubpf cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-ubpf ${PLAYGROUND_UBPF_SOURCE} ${PLAYGROUND_UBPF_HEADERS})

add_dependencies(play-ubpf ubpf-build libbpf-build generate_ubpf_header)

#add_custom_command(TARGET play-libbpf-bpf-load PRE_BUILD
#    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${PLAYGROUND_LIBBPF_BPF_LOAD_BYTECODE_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_program.o
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#)

target_link_libraries(
    play-ubpf
    ubpf
    bpf
    elf
    z
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-ubpf)

# --------------- #
# Playground spdk #
# --------------- #

# Target only available on guest
#if(IS_DEPLOYED)

set(PLAYGROUND_SPDK_SOURCE
    spdk/playground-spdk.cxx
)

message("${PRJ_PRX}: playground spdk cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-spdk ${PLAYGROUND_SPDK_SOURCE})

add_dependencies(play-spdk spdk-build dpdk-build isa-l-build)

#target_include_directories(
#    play-spdk
#    INTERFACE
#    $<BUILD_INTERFACE:${PRJ_ROOT}/include/spdk>
#)

target_link_libraries(
    play-spdk
    # spdk_nvme.a spdk_env_dpdk.a spdk_log.a spdk_util.a
    # rte_eal.a rte_mempool.a rte_telemetry.a rte_ring.a rte_kvargs.a rte_bus_pci.a
    # rte_pci.a

#    spdk spdk_env_dpdk
#    rte_eal.a rte_mempool.a rte_telemetry.a rte_ring.a rte_kvargs.a rte_bus_pci.a
#    rte_pci.a rte_vhost.a rte_power.a
#    isal.a uuid numa dl

    -Wl,--whole-archive
    spdk_log.a spdk_env_dpdk.a spdk_nvme.a spdk_util.a spdk_sock.a spdk_json.a
    spdk_rpc.a spdk_jsonrpc.a
    -Wl,--no-whole-archive
    rte_eal.a rte_mempool.a rte_telemetry.a rte_ring.a rte_kvargs.a rte_bus_pci.a
    rte_pci.a rte_vhost.a rte_power.a
    isal.a uuid numa dl

    # spdk_conf.a spdk_env_dpdk.a spdk_bdev_nvme.a spdk_event_bdev.a spdk_bdev.a spdk_bdev_malloc.a
    # spdk_log.a spdk_thread.a libspdk_util.a spdk_conf.a spdk_accel.a spdk_vmd.a spdk_nvme.a
    # spdk_vhost.a
    # rte_eal.a rte_mempool.a rte_ring.a rte_pci.a rte_bus_pci.a rte_telemetry.a rte_kvargs.a
    # uuid numa dl
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-spdk)

#else()
#    # Add dummy target to satisfy playground target.
#    add_custom_target(play-spdk)
#endif()

# ------------------------------------- #
# Playground play-spdk-find-fill-filter #
# ------------------------------------- #

set(PLAYGROUND_SPDK_FIND_FILL_FILTER_SOURCE
    spdk-find-fill-filter/spdk-find-fill-filter.cxx
)

message("${PRJ_PRX}: playground spdk-find-fill-filter cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-spdk-find-fill-filter ${PLAYGROUND_SPDK_FIND_FILL_FILTER_SOURCE})

add_dependencies(play-spdk-find-fill-filter spdk-build dpdk-build isa-l-build)

target_link_libraries(
    play-spdk-find-fill-filter
    -Wl,--whole-archive
    spdk_log.a spdk_env_dpdk.a spdk_nvme.a spdk_util.a spdk_sock.a spdk_json.a
    spdk_rpc.a spdk_jsonrpc.a
    -Wl,--no-whole-archive
    rte_eal.a rte_mempool.a rte_telemetry.a rte_ring.a rte_kvargs.a rte_bus_pci.a
    rte_pci.a rte_vhost.a rte_power.a
    isal.a uuid numa dl
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

qemucsd_target_postprocess(play-spdk-find-fill-filter)

## ------------------ #
## Playground objcopy #
## ------------------ #
#
## Run data.sh to let objcopy turn opencl into data segments
#set(objcopy-data "${CMAKE_CURRENT_SOURCE_DIR}/objcopy/data.o")
#add_custom_command(OUTPUT ${objcopy-data}
#    COMMAND ${PRJ_SCRIPTS}/objcopy-data.sh
#    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/objcopy"
#)
#
#set(PLAYGROUND_OBJCOPY_SOURCE
#    objcopy/objcopy-data.cxx
#    ${objcopy-data}
#)
#
#message("${PRJ_PRX}: playground objcopy cxx flags:${CMAKE_CXX_FLAGS}")
#add_executable(play-objcopy ${PLAYGROUND_OBJCOPY_SOURCE})
#
#target_link_libraries(
#    play-objcopy
#    OpenCL
#    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
#)
#
## Requires opencl to be build
## add_dependencies(play-objcopy)
#
## remove the objcopy source file after compilation so it always gets recompiled
#add_custom_command(TARGET play-objcopy
#    POST_BUILD
#    COMMAND rm ${CMAKE_CURRENT_SOURCE_DIR}/objcopy/data.o
#    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
#)

## ---------------------------- #
## Playground static-dependency #
## ---------------------------- #
#
#add_custom_target(glfw-static DEPENDS glfw-static-build)
#add_custom_command(OUTPUT glfw-static-build PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/static-dependency/glfw
#    DEPENDS glfw-static-cmake
#)
#add_custom_command(OUTPUT glfw-static-cmake PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/static-dependency/glfw
#)
#
#set(PLAYGROUND_STATIC_DEPENDENCY_SOURCE
#    static-dependency/static-dependency.cxx
#)
#
#message("${PRJ_PRX}: playground static-dependency cxx flags:${CMAKE_CXX_FLAGS}")
#add_executable(play-static-dependency ${PLAYGROUND_STATIC_DEPENDENCY_SOURCE})
#add_dependencies(play-static-dependency glfw-static)
#
#target_include_directories(
#    play-static-dependency
#    INTERFACE
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/static-dependency/glfw/include>
#)
#target_link_libraries(
#    play-static-dependency
#    ${CMAKE_CURRENT_SOURCE_DIR}/static-dependency/glfw/src/libglfw3.a
#    dl pthread vulkan X11
#)
#
## ---------------------------- #
## Playground shared-dependency #
## ---------------------------- #
#
#add_custom_target(glfw-shared DEPENDS glfw-shared-soname)
#add_custom_command(OUTPUT glfw-shared-soname PRE_BUILD
#    COMMAND ${PRJ_SCRIPTS}/convert-to-soname.sh
##    COMMAND rm convert-to-soname.sh
#    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#    DEPENDS glfw-shared-file
#)
##add_custom_command(OUTPUT glfw-shared-copy PRE_BUILD
##    COMMAND cp convert-to-soname.sh ${CMAKE_CURRENT_BINARY_DIR}
##    COMMAND chmod u+x ${CMAKE_CURRENT_BINARY_DIR}/convert-to-soname.sh
##    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency
##    DEPENDS glfw-shared-file
##)
#add_custom_command(OUTPUT glfw-shared-file PRE_BUILD
#    COMMAND cp *.so ${CMAKE_CURRENT_BINARY_DIR}
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency/glfw/src
#    DEPENDS glfw-shared-build
#)
#add_custom_command(OUTPUT glfw-shared-build PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency/glfw
#    DEPENDS glfw-shared-cmake
#)
#add_custom_command(OUTPUT glfw-shared-cmake PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=ON .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency/glfw
#)
#
#add_library(import_glfw SHARED IMPORTED)
#set_property(TARGET import_glfw PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libglfw.so)
#add_dependencies(import_glfw glfw-shared)
#
#set(PLAYGROUND_SHARED_DEPENDENCY_SOURCE
#    shared-dependency/shared-dependency.cxx
#)
#
#message("${PRJ_PRX}: playground shared-dependency cxx flags:${CMAKE_CXX_FLAGS}")
#add_executable(play-shared-dependency ${PLAYGROUND_SHARED_DEPENDENCY_SOURCE})
#
#set(${CMAKE_USE_RELATIVE_PATHS} TRUE FORCE)
#set_target_properties(play-shared-dependency PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
#set_target_properties(play-shared-dependency PROPERTIES INSTALL_RPATH "./")
#
#target_link_options(play-shared-dependency BEFORE PRIVATE -L${CMAKE_CURRENT_BINARY_DIR})
#
#target_include_directories(
#    play-shared-dependency
#    INTERFACE
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency/glfw/include>
#)
#target_link_libraries(
#    play-shared-dependency
#    import_glfw dl pthread vulkan
#)