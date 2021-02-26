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
if(CLANG_COMPILER_VERSION VERSION_LESS "3.9")
    message(FATAL_ERROR "Insufficient Clang version: ${CLANG_COMPILER_VERSION}")
else()
    message("Found suitable Clang version for BPF: ${CLANG_COMPILER_VERSION}")
endif()

# Set the current compiler with CACHE FORCE, allows exporting
set(CURRENT_COMPILER "DEFAULT")

macro(use_default_compiler_c)
    if (${CURRENT_COMPILER} STREQUAL "BPF")
        # Save current native flags
        set(BPF_C_FLAGS ${CMAKE_C_FLAGS})

        # Change compiler
        set(CMAKE_C_COMPILER ${DEFAULT_C_COMPILER})
        set(CMAKE_C_FLAGS ${DEFAULT_C_FLAGS})
        set(CURRENT_COMPILER "DEFAULT")
    endif()
endmacro()


macro(use_bpf_compiler_c)
    if (${CURRENT_COMPILER} STREQUAL "DEFAULT")
        # Save current host flags and compiler
        set(DEFAULT_C_COMPILER ${CMAKE_C_COMPILER})
        set(DEFAULT_C_FLAGS ${CMAKE_C_FLAGS})

        # Change compiler
        set(CMAKE_C_COMPILER ${CLANG_COMPILER})
        set(CMAKE_C_FLAGS ${BPF_C_FLAGS})
        set(CURRENT_COMPILER "BPF")
    endif()
endmacro()

## Test for minimal version of Clang compiler
## TODO(Dantali0n): Upgrade to try_compile
#set(BPF_C_FLAGS  "-target bpf")
#use_bpf_compiler_c()
#if(CMAKE_C_COMPILER_VERSION VERSION_LESS "3.9")
#    message(FATAL_ERROR "Insufficient Clang version")
#else()
#    message("Found Clang for BPF version: ${CLANG_COMPILER}")
#endif()
#use_default_compiler_c()