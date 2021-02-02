project(${PRJ_PRX}_arguments)

#find_package(
#    Boost 1.32 COMPONENTS
#    program_options
#    REQUIRED
#)

set(AIRGLOW_ARGUMENTS_LIBRARIES
    boost_program_options
)

set(AIRGLOW_ARGUMENTS_SRC
    src/arguments.cxx
)

set(AIRGLOW_ARGUMENTS_HEADERS
    include/arguments.hpp
)

# Add airglow_arguments to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

add_library(
    airglow_arguments STATIC
    ${AIRGLOW_ARGUMENTS_SRC}
    ${AIRGLOW_ARGUMENTS_HEADERS}
)
set_target_properties(${PRJ_LIB} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(
    airglow_arguments
    ${AIRGLOW_ARGUMENTS_LIBRARIES}
)

add_dependencies(airglow_arguments boost-build)

# Add airglow_arguments to the modules
add_qemucsd_module(airglow_arguments)
