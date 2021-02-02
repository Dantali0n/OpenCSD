project(${PRJ_PRX}_arguments)

#find_package(
#    Boost 1.32 COMPONENTS
#    program_options
#    REQUIRED
#)

set(QEMUCSD_ARGUMENTS_LIBRARIES
    boost_program_options
)

set(QEMUCSD_ARGUMENTS_SRC
    src/arguments.cxx
)

set(QEMUCSD_ARGUMENTS_HEADERS
    include/arguments.hpp
)

# Add qemucsd_arguments to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

add_library(
    qemucsd_arguments STATIC
    ${QEMUCSD_ARGUMENTS_SRC}
    ${QEMUCSD_ARGUMENTS_HEADERS}
)
set_target_properties(${PRJ_LIB} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(
    qemucsd_arguments
    ${QEMUCSD_ARGUMENTS_LIBRARIES}
)

add_dependencies(qemucsd_arguments boost-build)

# Add qemucsd_arguments to the modules
add_qemucsd_module(qemucsd_arguments)
