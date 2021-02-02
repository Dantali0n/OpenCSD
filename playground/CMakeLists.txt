#project(playground)

qemucsd_include_directories()

get_property(test DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
message("Includes playground: ${test}")
get_property(test DIRECTORY PROPERTY LINK_DIRECTORIES)
message("Links playground: ${test}")

# Add every playground subdirectory target to this list
add_custom_target(
    playground
    DEPENDS
    play-example
#    play-objcopy
#    play-static-dependency
#    play-shared-dependency
)

set(${PRJ_PRX}_PLAYGROUND_LIBRARIES

)

set(${PRJ_PRX}_PLAY_LIBRARIES_PACK
    ${${PRJ_PRX}_LIBRARIES}
    ${${PRJ_PRX}_PLAYGROUND_LIBRARIES}
)

# ----------------------- #
# Playground boost_locale #
# ----------------------- #

set(PLAYGROUND_BOOST_LOCALE_SOURCE
    boost-locale/boost-locale.cxx
)

message("${PRJ_PRX}: playground boost-locale cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-boost-locale ${PLAYGROUND_BOOST_LOCALE_SOURCE})

target_compile_options(play-boost-locale PRIVATE -Wno-deprecated-declarations)
target_link_libraries(
    play-boost-locale
    boost_thread
    boost_locale
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

add_dependencies(play-boost-locale boost-build)

# ------------------ #
# Playground example #
# ------------------ #

set(PLAYGROUND_EXAMPLE_SOURCE
    example/playground-example.cxx
)

message("${PRJ_PRX}: playground example cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(play-example ${PLAYGROUND_EXAMPLE_SOURCE})

target_link_libraries(
    play-example
    qemucsd_arguments
    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
)

## ------------------ #
## Playground objcopy #
## ------------------ #
#
## Run data.sh to let objcopy turn opencl into data segments
#set(objcopy-data "${CMAKE_CURRENT_SOURCE_DIR}/objcopy/data.o")
#add_custom_command(OUTPUT ${objcopy-data}
#    COMMAND ${PRJ_SCRIPTS}/objcopy-data.sh
#    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/objcopy"
#)
#
#set(PLAYGROUND_OBJCOPY_SOURCE
#    objcopy/objcopy-data.cxx
#    ${objcopy-data}
#)
#
#message("${PRJ_PRX}: playground objcopy cxx flags:${CMAKE_CXX_FLAGS}")
#add_executable(play-objcopy ${PLAYGROUND_OBJCOPY_SOURCE})
#
#target_link_libraries(
#    play-objcopy
#    OpenCL
#    ${${PRJ_PRX}_PLAY_LIBRARIES_PACK}
#)
#
## Requires opencl to be build
## add_dependencies(play-objcopy)
#
## remove the objcopy source file after compilation so it always gets recompiled
#add_custom_command(TARGET play-objcopy
#    POST_BUILD
#    COMMAND rm ${CMAKE_CURRENT_SOURCE_DIR}/objcopy/data.o
#    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
#)

## ---------------------------- #
## Playground static-dependency #
## ---------------------------- #
#
#add_custom_target(glfw-static DEPENDS glfw-static-build)
#add_custom_command(OUTPUT glfw-static-build PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/static-dependency/glfw
#    DEPENDS glfw-static-cmake
#)
#add_custom_command(OUTPUT glfw-static-cmake PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/static-dependency/glfw
#)
#
#set(PLAYGROUND_STATIC_DEPENDENCY_SOURCE
#    static-dependency/static-dependency.cxx
#)
#
#message("${PRJ_PRX}: playground static-dependency cxx flags:${CMAKE_CXX_FLAGS}")
#add_executable(play-static-dependency ${PLAYGROUND_STATIC_DEPENDENCY_SOURCE})
#add_dependencies(play-static-dependency glfw-static)
#
#target_include_directories(
#    play-static-dependency
#    INTERFACE
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/static-dependency/glfw/include>
#)
#target_link_libraries(
#    play-static-dependency
#    ${CMAKE_CURRENT_SOURCE_DIR}/static-dependency/glfw/src/libglfw3.a
#    dl pthread vulkan X11
#)
#
## ---------------------------- #
## Playground shared-dependency #
## ---------------------------- #
#
#add_custom_target(glfw-shared DEPENDS glfw-shared-soname)
#add_custom_command(OUTPUT glfw-shared-soname PRE_BUILD
#    COMMAND ${PRJ_SCRIPTS}/convert-to-soname.sh
##    COMMAND rm convert-to-soname.sh
#    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#    DEPENDS glfw-shared-file
#)
##add_custom_command(OUTPUT glfw-shared-copy PRE_BUILD
##    COMMAND cp convert-to-soname.sh ${CMAKE_CURRENT_BINARY_DIR}
##    COMMAND chmod u+x ${CMAKE_CURRENT_BINARY_DIR}/convert-to-soname.sh
##    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency
##    DEPENDS glfw-shared-file
##)
#add_custom_command(OUTPUT glfw-shared-file PRE_BUILD
#    COMMAND cp *.so ${CMAKE_CURRENT_BINARY_DIR}
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency/glfw/src
#    DEPENDS glfw-shared-build
#)
#add_custom_command(OUTPUT glfw-shared-build PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency/glfw
#    DEPENDS glfw-shared-cmake
#)
#add_custom_command(OUTPUT glfw-shared-cmake PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=ON .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency/glfw
#)
#
#add_library(import_glfw SHARED IMPORTED)
#set_property(TARGET import_glfw PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libglfw.so)
#add_dependencies(import_glfw glfw-shared)
#
#set(PLAYGROUND_SHARED_DEPENDENCY_SOURCE
#    shared-dependency/shared-dependency.cxx
#)
#
#message("${PRJ_PRX}: playground shared-dependency cxx flags:${CMAKE_CXX_FLAGS}")
#add_executable(play-shared-dependency ${PLAYGROUND_SHARED_DEPENDENCY_SOURCE})
#
#set(${CMAKE_USE_RELATIVE_PATHS} TRUE FORCE)
#set_target_properties(play-shared-dependency PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
#set_target_properties(play-shared-dependency PROPERTIES INSTALL_RPATH "./")
#
#target_link_options(play-shared-dependency BEFORE PRIVATE -L${CMAKE_CURRENT_BINARY_DIR})
#
#target_include_directories(
#    play-shared-dependency
#    INTERFACE
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/shared-dependency/glfw/include>
#)
#target_link_libraries(
#    play-shared-dependency
#    import_glfw dl pthread vulkan
#)