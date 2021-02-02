project(${PRJ_PRX}_entry)

# Define required libraries, any library from dependencies
# also requires adding: add_dependencies(${TARGET} ${DEPENDENCY}-build)
set(AIRGLOW_ENTRY_LIBRARIES
    vulkan
    glfw
)

set(AIRGLOW_ENTRY_SRC
    src/entry.cxx
)

set(AIRGLOW_ENTRY_HEADERS
    include/entry.hpp
)

# Add airglow_entry to the includes
add_qemucsd_include(${CMAKE_CURRENT_SOURCE_DIR}/include)
qemucsd_include_directories()

get_property(test DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
message("Includes: ${test}")
get_property(test DIRECTORY PROPERTY LINK_DIRECTORIES)
message("Links: ${test}")
message("Boost: ${BOOST_LIBRARIES}")

add_library(
    airglow_entry STATIC
    ${AIRGLOW_ENTRY_SRC}
    ${AIRGLOW_ENTRY_HEADERS}
)
set_target_properties(${PRJ_LIB} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(
    airglow_entry
    ${AIRGLOW_ENTRY_LIBRARIES}
)
# Requires glfw to be build
add_dependencies(airglow_entry glfw-build vulkan-build)

# Add airglow_arguments to the modules
add_qemucsd_module(airglow_entry)
