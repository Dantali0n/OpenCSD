# Copyright (c) 2012 - 2015, Lars Bilke
# Copyright (c) 2021, Corne Lukken
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
#
# 2012-01-31, Lars Bilke
# - Enable Code Coverage
#
# 2013-09-17, Joakim Söderberg
# - Added support for Clang.
# - Some additional usage instructions.
#
# 2021-02-25, Corne Lukken
# - Changed cobertura to use lcov-to-cobertura instead of gcovr directly.
#
# 2021-12-29, Corne Lukken
# - Removed dead code
#
# USAGE:

# 0. (Mac only) If you use Xcode 5.1 make sure to patch geninfo as described here:
#      http://stackoverflow.com/a/22404544/80480
#
# 1. Copy this file into your cmake modules path.
#
# 2. Add the following line to your CMakeLists.txt:
#      INCLUDE(CodeCoverage)
#
# 3. Set compiler flags to turn off optimization and enable coverage:
#    SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
#	 SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
#
# 3. Use the function SETUP_TARGET_FOR_COVERAGE to create a custom make target
#    which runs your test executable and produces a lcov code coverage report:
#    Example:
#	 SETUP_TARGET_FOR_COVERAGE(
#				my_coverage_target  # Name for custom target.
#				test_driver         # Name of the test driver executable that runs the tests.
#									# NOTE! This should always have a ZERO as exit code
#									# otherwise the coverage generation will not complete.
#				coverage            # Name of output directory.
#				)
#
# 4. Build a Debug build:
#	 cmake -DCMAKE_BUILD_TYPE=Debug ..
#	 make
#	 make my_coverage_target
#
#

# Check prereqs
FIND_PROGRAM(GCOV_PATH gcov )
FIND_PROGRAM(LCOV_PATH lcov )
FIND_PROGRAM(GENHTML_PATH genhtml )
FIND_PROGRAM(GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/tests)

IF(NOT GCOV_PATH)
    MESSAGE(FATAL_ERROR "gcov not found! Aborting...")
ENDIF() # NOT GCOV_PATH

IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    IF("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 3)
        MESSAGE(FATAL_ERROR "Clang version must be 3.0.0 or greater! Aborting...")
    ENDIF()
ELSEIF(NOT CMAKE_COMPILER_IS_GNUCXX)
    MESSAGE(FATAL_ERROR "Compiler is not GNU gcc! Aborting...")
ENDIF() # CHECK VALID COMPILER

IF ( NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "Coverage"))
    MESSAGE( WARNING "Code coverage results with an optimized (non-Debug) build may be misleading" )
ENDIF() # NOT CMAKE_BUILD_TYPE STREQUAL "Debug"

# Param _targetname     The name of new the custom make target
# Param _testrunner     The name of the target which runs the tests
# Param _outputname     cobertura output is generated as _outputname.xml
# Optional fourth parameter is passed as arguments to _testrunner
#   Pass them in list form, e.g.: "-j;2" for -j 2
FUNCTION(SETUP_TARGET_FOR_COVERAGE_LCOV _targetname _testrunner _outputname)

    IF(NOT Python_EXECUTABLE)
        MESSAGE(FATAL_ERROR "Python not found! Aborting...")
    ENDIF() # NOT Python_EXECUTABLE

    IF(NOT GCOVR_PATH)
        MESSAGE(FATAL_ERROR "gcovr not found! Aborting...")
    ENDIF() # NOT GCOVR_PATH

    IF(NOT LCOV_PATH)
        MESSAGE(FATAL_ERROR "lcov not found! Aborting...")
    ENDIF() # NOT LCOV_PATH

    IF(NOT GENHTML_PATH)
        MESSAGE(FATAL_ERROR "genhtml not found! Aborting...")
    ENDIF() # NOT GENHTML_PATH

    SET(coverage_base "${CMAKE_BINARY_DIR}/${_outputname}.base.info")
    SET(coverage_info "${CMAKE_BINARY_DIR}/${_outputname}.test.info")
    SET(coverage_total "${CMAKE_BINARY_DIR}/${_outputname}.total.info")
    SET(coverage_cleaned "${CMAKE_BINARY_DIR}/${_outputname}.total.info")

    ADD_CUSTOM_TARGET(${_targetname}

        # Run tests
        ${_testrunner} ${ARGV3}

        # Running gcovr
        COMMAND ${LCOV_PATH} -i --directory . --capture --output-file ${coverage_base}
        COMMAND ${LCOV_PATH} --directory . --capture --output-file ${coverage_info}
        COMMAND ${LCOV_PATH} -a ${coverage_base} -a ${coverage_info} -o ${coverage_total}
        COMMAND ${LCOV_PATH} --remove ${coverage_total} '${CMAKE_BINARY_DIR}/opencsd/include/*' '${CMAKE_SOURCE_DIR}/dependencies/*' '${CMAKE_SOURCE_DIR}/tests/*' '/usr/*' --output-file ${coverage_cleaned}
        COMMAND ${GENHTML_PATH} -o ${_outputname} ${coverage_cleaned}
        COMMAND ${CMAKE_SOURCE_DIR}/python/lcov-to-cobertura/lcov_cobertura/lcov_cobertura.py ${coverage_cleaned}
        COMMAND ${CMAKE_COMMAND} -E remove ${coverage_info} ${coverage_base}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running gcovr to produce Cobertura code coverage report."
    )

    # Show info where to find the report
    ADD_CUSTOM_COMMAND(TARGET ${_targetname} POST_BUILD
        COMMAND ;
        COMMENT "LCOV code coverage report saved in ${CMAKE_BINARY_DIR}/coverage/index.html"
    )

ENDFUNCTION() # SETUP_TARGET_FOR_COVERAGE_COBERTURA