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

project(tests)

# Do not include these globally in the include_list as outside of
# testing nothing needs access to test headers!
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

#set(CMAKE_OG_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#set(CMAKE_OG_LINKER_FLAGS "${CMAKE_LINKER_FLAGS}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
#set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")

#find_package(
#    Boost 1.53 COMPONENTS
#    timer
#    unit_test_framework
#    REQUIRED
#)

# Add every playground subdirectory target to this list
add_custom_target(
	tests
	DEPENDS
	testarguments
	testcpp17
	testfuse-lfs
	testfuse-lfs-concurrency
	testfuse-lfs-drive
	testmeasurements
	testnvme-csd
	testnvme-zns-memory
	testspdk-init
)

set(${PRJ_PRX}_TEST_LIBRARIES
	${PRJ_LIB}
	boost_timer
	boost_unit_test_framework
)

set(${PRJ_PRX}_LIBRARIES_PACK
	${${PRJ_PRX}_LIBRARIES}
	${${PRJ_PRX}_TEST_LIBRARIES}
)

# ----------- #
# CPP17 tests #
# ----------- #

set(TEST_CPP17_SOURCE
	src/test_cpp_17.cxx
	src/tests.cxx
)

set(TEST_CPP17_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test cpp17 cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testcpp17 ${TEST_CPP17_SOURCE} ${TEST_CPP17_HEADERS} ${${PRJ_PRX}_SOURCES})
add_dependencies(testcpp17 boost-build)

target_link_libraries(
	testcpp17
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testcpp17)

# --------------- #
# arguments tests #
# --------------- #

set(TEST_ARGUMENTS_SOURCE
	src/test_arguments.cxx
	src/tests.cxx
)

set(TEST_ARGUMENTS_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test arguments cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testarguments ${TEST_ARGUMENTS_SOURCE} ${TEST_ARGUMENTS_HEADERS} ${${PRJ_PRX}_SOURCES})
add_dependencies(testarguments boost-build)

target_link_libraries(
	testarguments
	qemucsd_arguments
	# Have to repeat high level shared libraries as these dynamically cross
	# reference one another.
#	spdk spdk_env_dpdk
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testarguments)

# -------------- #
# fuse_lfs tests #
# -------------- #

set(TEST_FUSE_LFS_SOURCE
	src/test_flfs.cxx
	src/tests.cxx
)

set(TEST_FUSE_LFS_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test fuse_lfs cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testfuse-lfs ${TEST_FUSE_LFS_SOURCE} ${TEST_FUSE_LFS_HEADERS} ${${PRJ_PRX}_SOURCES})
#add_dependencies(testarguments)

target_link_libraries(
	testfuse-lfs
	qemucsd_fuse_lfs
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testfuse-lfs)

# -------------------------- #
# fuse_lfs_concurrency tests #
# -------------------------- #

set(TEST_FUSE_LFS_CONCURRENCY_SOURCE
	src/test_flfs_concurrency.cxx
	src/tests.cxx
)

set(TEST_FUSE_LFS_CONCURRENCY_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test fuse_lfs_concurrency cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testfuse-lfs-concurrency
	${TEST_FUSE_LFS_CONCURRENCY_SOURCE}
	${TEST_FUSE_LFS_CONCURRENCY_HEADERS}
	${${PRJ_PRX}_SOURCES}
)
#add_dependencies(testarguments)

target_link_libraries(
	testfuse-lfs-concurrency
	qemucsd_fuse_lfs
	qemucsd_nvme_zns_memory
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testfuse-lfs-concurrency)

# -------------------- #
# fuse_lfs_drive tests #
# -------------------- #

set(TEST_FUSE_LFS_SOURCE
	src/test_flfs_drive.cxx
	src/tests.cxx
)

set(TEST_FUSE_LFS_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test fuse_lfs_drive cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testfuse-lfs-drive ${TEST_FUSE_LFS_SOURCE} ${TEST_FUSE_LFS_HEADERS} ${${PRJ_PRX}_SOURCES})
#add_dependencies(testarguments)

target_link_libraries(
	testfuse-lfs-drive
	qemucsd_fuse_lfs
	qemucsd_nvme_zns_memory
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testfuse-lfs-drive)

# ------------------ #
# measurements tests #
# ------------------ #

set(TEST_MEASUREMENTS_SOURCE
	src/test_measurements.cxx
	src/tests.cxx
)

set(TEST_MEASUREMENTS_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test measurements cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testmeasurements ${TEST_MEASUREMENTS_SOURCE} ${TEST_MEASUREMENTS_HEADERS} ${${PRJ_PRX}_SOURCES})
#add_dependencies(testarguments)

target_link_libraries(
	testmeasurements
	qemucsd_measurements
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testmeasurements)

# -------------- #
# nvme_csd tests #
# -------------- #

set(TEST_NVME_CSD_SOURCE
	src/test_nvme_csd.cxx
	src/tests.cxx
)

set(TEST_NVME_CSD_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test nvme_csd cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testnvme-csd ${TEST_NVME_CSD_SOURCE} ${TEST_NVME_CSD_HEADERS} ${${PRJ_PRX}_SOURCES})
#add_dependencies(testarguments)

target_link_libraries(
	testnvme-csd
	qemucsd_output
	qemucsd_nvme_csd
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testnvme-csd)

# --------------------- #
# nvme_zns_memory tests #
# --------------------- #

set(TEST_NVME_ZNS_MEMORY_SOURCE
	src/test_nvme_zns_memory.cxx
	src/tests.cxx
)

set(TEST_NVME_ZNS_MEMORY_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test nvme_zns_memory cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testnvme-zns-memory ${TEST_NVME_ZNS_MEMORY_SOURCE} ${TEST_NVME_ZNS_MEMORY_HEADERS} ${${PRJ_PRX}_SOURCES})
#add_dependencies(testarguments)

target_link_libraries(
	testnvme-zns-memory
	qemucsd_nvme_zns_memory
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testnvme-zns-memory)

# --------------- #
# spdk_init tests #
# --------------- #

set(TEST_SPDK_INIT_SOURCE
	src/test_spdk_init.cxx
	src/tests.cxx
)

set(TEST_SPDK_INIT_HEADERS
	include/tests.hpp
)

message("${PRJ_PRX}: test spdk-init cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(testspdk-init ${TEST_SPDK_INIT_SOURCE} ${TEST_SPDK_INIT_HEADERS} ${${PRJ_PRX}_SOURCES})
#add_dependencies(testarguments)

target_link_libraries(
	testspdk-init
	qemucsd_spdk_init
	${${PRJ_PRX}_LIBRARIES_PACK}
)

qemucsd_target_postprocess(testspdk-init)

# ------------- #
# combine tests #
# ------------- #

add_test(TestArguments testarguments)
add_test(TestCPP17 testcpp17)
add_test(TestFuseLfs testfuse-lfs)
add_test(TestFuseLfsConcurrency testfuse-lfs-concurrency)
add_test(TestFuseLfsDrive testfuse-lfs-drive)
add_test(TestMeasurements testmeasurements)
add_test(TestNvmeCsd testnvme-csd)
add_test(TestNvmeZnsBackendMemory testnvme-zns-memory)
add_test(TestSpdkInit testspdk-init)

add_custom_target(check
	COMMAND ${CMAKE_CTEST_COMMAND} --output-junit tests.xml
	DEPENDS testarguments testcpp17 testfuse-lfs testfuse-lfs-concurrency
		testfuse-lfs-drive testmeasurements testnvme-csd testnvme-zns-memory
		testspdk-init
)