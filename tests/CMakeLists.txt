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
	testcpp17
	testarguments
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
	src/test-cpp-17.cxx
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

add_exec_post(testcpp17)

# --------------- #
# arguments tests #
# --------------- #

set(TEST_ARGUMENTS_SOURCE
	src/test-arguments.cxx
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
	${${PRJ_PRX}_LIBRARIES_PACK}
)

add_exec_post(testarguments)


add_test(TestArguments testarguments)
add_test(TestCPP17 testcpp17)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS testarguments testcpp17)