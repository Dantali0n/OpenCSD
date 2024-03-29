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

CMAKE_MINIMUM_REQUIRED(VERSION 3.18)
PROJECT(opencsd)

# Project name under shorter variable
set(PRJ_PRX ${CMAKE_PROJECT_NAME})

# Root directory of engine binary
set(PRJ_ROOT ${CMAKE_BINARY_DIR}/${PRJ_PRX})

# Directory containing executable binaries
set(PRJ_BINARIES ${PRJ_ROOT}/bin)

# Directory containing shared and static library files
set(PRJ_LIBRARIES ${PRJ_ROOT}/lib)

# Directory containing external headers
set(PRJ_HEADERS ${PRJ_ROOT}/include)

# Directory containing scripts
set(PRJ_SCRIPTS ${CMAKE_SOURCE_DIR}/scripts)

# Disable build targets
#set(CMAKE_SKIP_INSTALL_RULES TRUE)

# These have to added here even though they are part of dependencies
# as sub-directories can not influence variables of the parent by
# design in CMake (unless using global properties).
# Add the libraries directory for the linker globally
link_directories(BEFORE ${PRJ_LIBRARIES})
# Add the headers directory globally
include_directories(BEFORE ${PRJ_HEADERS})

# Ensure the CMake target directory is not the same as the CMake root directory.
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	MESSAGE(FATAL_ERROR
		"${PRJ_PRX}: Please create a dedicated build directory. "
		"(You may need remove the CmakeCache.txt and CMakeFiles)"
	)
ENDIF()

# Ensure that Windows users are compiling from within CYGWIN
if(MSVC AND NOT CYGWIN)
	message(FATAL_ERROR "${PRJ_PRX}: Windows compilation requires CYGWIN")
endif()

# Add custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(compiler)
include(dependencies)
include(platform)
include(TargetArch)
include(macros)

find_package(KernelHeaders REQUIRED)

# Enable c++17 even when cmake < 3.1
use_cxx17()

# Enable pedantic errors and warnings for c++
# use_cxx_warning_pedantic()

# Disable warnings about unused functions, parameters and variables
disable_unused_cxx_warnings()

# determine current architecture and store it in ${QEMUCSD_ARCHITECTURE}
target_architecture(QEMUCSD_ARCHITECTURE)
message("${PRJ_PRX}: Architecture ${QEMUCSD_ARCHITECTURE}")

determine_address_model(${QEMUCSD_ARCHITECTURE})
if(QEMUCSD_32BIT)
	message("${PRJ_PRX}: Address model 32bit")
elseif(QEMUCSD_64BIT)
	message("${PRJ_PRX}: Address model 64bit")
else()
	message(FATAL_ERROR "${PRJ_PRX}: Address model not set!")
endif()

# Configurable options and defaults
OPTION(ENABLE_TESTS "Enable unit tests" ON)
OPTION(ENABLE_CODECOV "Measure code coverage, overrides build type Release" OFF)
OPTION(ENABLE_DOCUMENTATION "Generate documentation" ON)
OPTION(ENABLE_PLAYGROUND "Enable playground build targets" OFF)
OPTION(ENABLE_LEAK_TESTS "Enable address sanitizer" OFF)

OPTION(IS_DEPLOYED "Indicate if cmake is being run inside qemu" OFF)

# Required packages
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED FATAL_ERROR)

# include_directories(${FairLogger_INCLUDE_DIR})

# -------------------------------------------- #
# This defines global properties and several   #
# functions to interact with these properties: #
# - properties								   #
#   - module_list							   #
# - functions								   #
#   - add_qemucsd_module					   #
#   - add_qemucsd_include					   #
#   - qemucsd_include_directories			   #
#   - qemucsd_target_postprocess			   #
include(qemucsd)							   #
# -------------------------------------------- #

if(NOT ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release"))
	set(CMAKE_BUILD_TYPE "Debug")
	message("${PRJ_PRX}: Build type unset, defaulting to Debug")
endif()

# Setup compiler flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	message("${PRJ_PRX}: Disabling optimization & adding extensive "
			"debug symbols")
	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG:FULL /Od")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O0")
	endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	message("${PRJ_PRX}: Enabling extensive optimizations")
	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG:NONE /O2")
	else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -ffast-math -ftree-vectorize -march=native")
	endif()
endif()
message("${PRJ_PRX}: Build type: ${CMAKE_BUILD_TYPE}")

# Include backward
add_subdirectory(dependencies/backward)

# Enable backward on debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(${PRJ_PRX}_DBG_SOURCES
		${BACKWARD_ENABLE}
	)
endif()

# Set of required sources that dynamically gets extended
set(${PRJ_PRX}_SOURCES
	${${PRJ_PRX}_DBG_SOURCES}
)

# Set of required modules that dynamically gets extended
set(${PRJ_PRX}_LIBRARIES
	Threads::Threads
)

# Ensure dependencies is the first subdirectory /
# part of CMake script to define targets.
add_subdirectory(dependencies)

if(ENABLE_LEAK_TESTS)
	if (MSVC)
		message(FATAL_ERROR "${PRJ_PRX}: Cannot perform leak test with Microsoft Visual Studio")
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "(C|c?)lang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fstack-check -finstrument-functions")
		set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fstack-check -fPIC") # -finstrument-functions")
		set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address -fPIC")
	endif()
endif(ENABLE_LEAK_TESTS)

# setup early flags and force build type when code coverage is running
if(ENABLE_CODECOV)
	# Need python interpreter to generate coverage report
	find_package (Python COMPONENTS Interpreter REQUIRED)

	if (MSVC)
		message(FATAL_ERROR "${PRJ_PRX}: Cannot perform code coverage test with Microsoft Visual Studio")
	endif()
	message("${PRJ_PRX}: Running CodeCoverage, forcing ENABLE_TESTS & "
		"CMAKE_BUILD_TYPE=Debug")
	# CLANG -> -fprofile-instr-generate -fcoverage-mapping
	# GCC -> -fprofile-arcs -ftest-coverage -lgcov
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fno-inline -fno-inline-small-functions -fno-default-inline"
	)
	set(ENABLE_TESTS ON) # tests need to be enabled for code coverage
endif()

# Collection of concrete implementations
add_subdirectory(opencsd)

# Toy experiments
if(ENABLE_PLAYGROUND)
	add_subdirectory(playground)
endif()

# Unit tests to validate application
if(ENABLE_TESTS)
	message("${PRJ_PRX}: Enabling tests")
	enable_testing()
	add_subdirectory(tests)
endif()

# wait with importing coverage and setting up targets until
# unit tests have been defined.
if(ENABLE_CODECOV)
	include(CodeCoverage)

	# Need custom target using ctest to combine all test binaries
	include(CTest)
	add_custom_target(ctest COMMAND ${CMAKE_CTEST_COMMAND})

		setup_target_for_coverage_lcov(${PRJ_PRX}_coverage ctest coverage)
endif(ENABLE_CODECOV)

# Project documentation and report
if(ENABLE_DOCUMENTATION)

	add_custom_target(all-documents ALL)
	add_subdirectory(thesis)
	add_subdirectory(thesis-presentation)
	add_subdirectory(ictopen2022)
	add_subdirectory(fosdem2023)
	add_subdirectory(zcsd/compsys2021)
	add_subdirectory(zcsd/documentation)
	add_subdirectory(zcsd/presentation)

	# Generate Doxygen Documentation
	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		# set input and output files
		set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cnf)
		set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cnf)

		# request to configure the file
		configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

		# note the option ALL which allows to build the docs together with the
		# application.
		add_custom_target(doxygen ALL
			COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating API documentation with Doxygen"
			VERBATIM
		)
	else (DOXYGEN_FOUND)
		message(FATAL_ERROR
			"${PRJ_PRX}: Doxygen needs to be installed to generate "
			"the documentation"
		)
	endif (DOXYGEN_FOUND)
endif()
