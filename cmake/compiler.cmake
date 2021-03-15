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

# ---------------------------------------------------------------------------- #
# Allow to change compiler and associated flags on a per target basis          #                                    #
# Original source:                                                             #
# https://stackoverflow.com/questions/27168094/cmake-how-to-change-compiler-for-individual-target
# ---------------------------------------------------------------------------- #

# Requires Clang compiler, version requirement tested below macros
find_program(CLANG_COMPILER clang REQUIRED)

# Test version of Clang compiler
execute_process(
    COMMAND ${CLANG_COMPILER} --version
    OUTPUT_VARIABLE CLANG_COMPILER_VERSION_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Remove garbo characters around actual version
string(REGEX REPLACE "([a-z]* [a-z]* )|(-.*)" "" CLANG_COMPILER_VERSION ${CLANG_COMPILER_VERSION_RAW})

# Test version against minimal version required for BPF bytecode
# TODO(Dantali0n): Turn into try_compile
if(CLANG_COMPILER_VERSION VERSION_LESS "10.0")
    message(FATAL_ERROR "${PRJ_PRX}: Insufficient Clang version: ${CLANG_COMPILER_VERSION}")
else()
    message("${PRJ_PRX}: Found suitable Clang version for BPF: ${CLANG_COMPILER_VERSION}")
endif()

# Set the current compiler
set(CURRENT_COMPILER "DEFAULT")

macro(use_default_compiler_c)
    if (${CURRENT_COMPILER} STREQUAL "BPF")
        # Save current native flags
        set(BPF_C_FLAGS ${CMAKE_C_FLAGS} CACHE STRING "flags for the BPF compiler." FORCE)

        # Change compiler
        set(CMAKE_C_COMPILER ${DEFAULT_C_COMPILER})
        set(CMAKE_C_FLAGS ${DEFAULT_C_FLAGS})
        set(CURRENT_COMPILER "DEFAULT" CACHE STRING "Which compiler we are using." FORCE)
    endif()
endmacro()

macro(use_bpf_compiler_c)
    if (${CURRENT_COMPILER} STREQUAL "DEFAULT")
        # Save current host flags and compiler
        set(DEFAULT_C_COMPILER ${CMAKE_C_COMPILER})
        set(DEFAULT_C_FLAGS ${CMAKE_C_FLAGS} CACHE STRING "flags for the default compiler." FORCE)

        # Change compiler
        set(CMAKE_C_COMPILER ${CLANG_COMPILER})
        set(CMAKE_C_FLAGS ${BPF_C_FLAGS})
        set(CURRENT_COMPILER "BPF" CACHE STRING "Which compiler we are using." FORCE)
    endif()
endmacro()