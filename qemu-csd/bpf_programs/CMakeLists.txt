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

project(${PRJ_PRX}_bpf_programs)

add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR})
qemucsd_include_directories()

# Set custom compiler flags to generate BPF bytecode
set(BPF_C_FLAGS -g -O2 -target bpf -I ${PRJ_ROOT}/include)
# Change compiler to BPF compiler, Clang
use_bpf_compiler_c()
# Custom commands need to be used when using alternative compiler,
# add_executable / add_library CAN NOT BE USED!
# Generate the BPF bytecode
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bpf_zone_int_filter.o
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c ${CMAKE_CURRENT_SOURCE_DIR}/bpf_zone_int_filter.c -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_zone_int_filter.o
#    COMMAND clang-3.8 -g -O2 -emit-llvm -I ${PRJ_ROOT}/include -c ${CMAKE_CURRENT_SOURCE_DIR}/bpf_zone_int_filter.c -o - | llc-10 -march=bpf -mcpu=probe -filetype=obj -o ${CMAKE_CURRENT_BINARY_DIR}/bpf_zone_int_filter.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bpf_zone_int_filter.c generate_qemucsd_bpf_helpers generate_vmlinux generate_bpftool
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#add_custom_target(generate_bpf_bootstrap_program
#    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpf_bootstrap_program.o generate_vmlinux generate_bpftool
#)

# Change back to the regular compiler
use_default_compiler_c()

# Generate the BPF program header using bpftool, depend on the bytecode file
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/bpf_zone_int_filter.h
    COMMAND ${PRJ_ROOT}/bin/bpftool gen skeleton ${CMAKE_CURRENT_BINARY_DIR}/bpf_zone_int_filter.o > ${CMAKE_CURRENT_SOURCE_DIR}/bpf_zone_int_filter.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpf_zone_int_filter.o
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Generate a target that depends on the output file of the custom command, used
# for setting up dependency with runtime program.
add_custom_target(generate_bpf_zone_int_filter_header
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bpf_zone_int_filter.h
)