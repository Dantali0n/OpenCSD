# enable cxx17 using any version of CMake #
macro(use_cxx17)
    if(CMAKE_VERSION VERSION_LESS "3.1")
        message("use_cxx11: CMake < 3.1 setting cxx flags for c++17 manually")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qstd=c++17")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        endif()
    else()
        set(CMAKE_CXX_STANDARD 17)
    endif()
endmacro(use_cxx17)

# all warnings and errors
macro(use_cxx_warning_pedantic)
    if(MSVC)
        # Force to always compile with W4
        message("use_cxx_warning_pedantic: Found MSVC enabling W4")
        if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
            string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
        endif()
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        # Update if necessary
        message("use_cxx_warning_pedantic: Found GCC enabling pedantic")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Werror")
    endif()
endmacro(use_cxx_warning_pedantic)

# disable unused, call this after enabling more warnings / errors not before!
macro(disable_unused_cxx_warnings)
    if(MSVC)
        # Show warning
        message("disable_unused_cxx_warnings: Found MSVC unable to disable unused warnings")
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        # Update if necessary
        message("disable_unused_cxx_warnings: Found GCC disabling unused warnings")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function"
        )
    endif()
endmacro(disable_unused_cxx_warnings)