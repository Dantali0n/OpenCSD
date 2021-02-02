project(dependencies)

# see cmake/dependencies.cmake for methods

set(DEPENDENCY_BUILD_TYPE ${CMAKE_BUILD_TYPE})

# ------------------------ #
# Dependency build targets #
# ------------------------ #

if(${QEMUCSD_WINDOWS})
    add_custom_target(boost-build DEPENDS boost-directory-fix)
else()
    add_custom_target(boost-build DEPENDS boost-install)
endif()

add_custom_target(libbpf-build DEPENDS libbpf-install)

# -------------------- #
# Copy activate script #
# -------------------- #

add_custom_command(
    TARGET boost-build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/scripts/activate.sh
    ${PRJ_ROOT}/activate.sh
)

# ----------------------------- #
# Platform dependent attributes #
# ----------------------------- #

if(${QEMUCSD_WINDOWS})
    set(boost_source_url https://dancloud.dantalion.nl/index.php/s/oc9JTPgPLTxNBre/download)
    set(boost_source_file boost.zip)
    set(boost_source_hash a0e7ce67c52d816708fdeccdd8c9725626ba61254c13c18770498cacd514710a)
    set(boost_source_bootstrap bootstrap.bat)
    set(boost_source_build b2.exe)
    set(boost_source_build_param link=static threading=multi)
else()
    set(boost_source_url https://dl.bintray.com/boostorg/release/1.74.0/source/boost_1_74_0.tar.bz2)
    set(boost_source_file boost.tar.bz2)
    set(boost_source_hash 83bfc1507731a0906e387fc28b7ef5417d591429e51e788417fe9ff025e116b1)
    set(boost_source_bootstrap ./bootstrap.sh)
    set(boost_source_build ./b2)
    set(boost_source_build_param threading=multi)
endif()

# ----- #
# boost #
# ----- #

# Check if boost is already build, REQUIRES RE-RUNNING CMAKE COMMAND!
# Limitations: this only tests the presence of a single file
file_exists(${PRJ_ROOT}/lib/libboost_system.a has_boost)
if(${has_boost})
    add_custom_target(boost-directory-fix)
    add_custom_target(boost-install)
endif()

# Execution order is top to bottom! contrarily, to other dependencies being bottom to top.
download_file(
    ${boost_source_url}
    ${CMAKE_CURRENT_SOURCE_DIR}/${boost_source_file}
    SHA256 ${boost_source_hash}
)

# Extracts an archive in a directory and sets the directory as a file target (Nasty hack)
extract_archive(
    ${CMAKE_CURRENT_SOURCE_DIR}/${boost_source_file} # archive
    ${CMAKE_CURRENT_SOURCE_DIR}/boost # destination
    ${CMAKE_CURRENT_SOURCE_DIR} # working directory
    boost_extract # new target to depend on
    ${CMAKE_CURRENT_SOURCE_DIR}/${boost_source_file} # depends on, can't actually depend on file
)

add_custom_command(OUTPUT boost-bootstrap PRE_BUILD
    COMMAND ${boost_source_bootstrap}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/boost/boost_1_74_0
    DEPENDS boost_extract
)

if(${QEMUCSD_64BIT})
    set(BOOST_ADDRESS 64)
else()
    set(BOOST_ADDRESS 32)
endif()

string(TOLOWER ${DEPENDENCY_BUILD_TYPE} BOOST_BUILD_TYPE)
if("${BOOST_BUILD_TYPE}" STREQUAL "debug")
    set(BOOST_RUN_DEBUG on)
else()
    set(BOOST_RUN_DEBUG off)
endif()
add_custom_command(OUTPUT boost-install PRE_BUILD
    COMMAND ${boost_source_build} address-model=${BOOST_ADDRESS} runtime-debugging=${BOOST_RUN_DEBUG} variant=${BOOST_BUILD_TYPE} ${boost_source_build_param} install --prefix=${PRJ_ROOT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/boost/boost_1_74_0
    DEPENDS boost-bootstrap
)

# Boost uses different directory and file names for windows #genius!
if(${QEMUCSD_WINDOWS})
    add_custom_command(OUTPUT boost-rename-fix PRE_BUILD
        COMMAND bash ${PRJ_SCRIPTS}/boost-windows-rename.sh
        WORKING_DIRECTORY ${PRJ_LIBRARIES}
        DEPENDS boost-install
    )

    directory_exists("${PRJ_HEADERS}/boost" has_directory)
    if(NOT ${has_directory})
        add_custom_command(OUTPUT boost-directory-fix PRE_BUILD
            COMMAND mv boost-1_74/boost boost
            COMMAND rm -r boost-1_74
            WORKING_DIRECTORY ${PRJ_HEADERS}
            DEPENDS boost-rename-fix
        )
    else()
        add_custom_target(boost-directory-fix DEPENDS boost-rename-fix)
    endif()
endif()

# ------ #
# libbpf #
# ------ #

add_custom_command(OUTPUT libbpf-install PRE_BUILD
    COMMAND BUILD_STATIC_ONLY=y PREFIX=${PRJ_ROOT} LIBDIR=${PRJ_ROOT}/lib make install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libbpf/src
)

## ----- #
## glfw3 #
## ----- #
#
#add_custom_command(OUTPUT glfw-shared-install PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build . --target install --config ${DEPENDENCY_BUILD_TYPE}
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glfw
#    DEPENDS glfw-shared-build
#)
#
#add_custom_command(OUTPUT glfw-shared-build PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glfw
#    DEPENDS glfw-shared-cmake
#)
#add_custom_command(OUTPUT glfw-shared-cmake PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=${DEPENDENCY_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${PRJ_ROOT} -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=ON .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glfw
#)
#
## ------ #
## vulkan #
## ------ #
#
#add_custom_command(OUTPUT vulkan-shared-install PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build . --target install --config ${DEPENDENCY_BUILD_TYPE}
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-loader
#    DEPENDS vulkan-shared-build
#)
#
#add_custom_command(OUTPUT vulkan-shared-build PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-loader
#    DEPENDS vulkan-shared-cmake
#)
#
#set(ENV{VULKAN_SDK} ${CMAKE_CURRENT_SOURCE_DIR}/vulkansdk)
#add_custom_command(OUTPUT vulkan-shared-cmake PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=${DEPENDENCY_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${PRJ_ROOT} -DVULKAN_HEADERS_INSTALL_DIR=${PRJ_ROOT}  .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-loader
#    DEPENDS vulkan-shared-headers vulkansdk-extract
#)
#
#download_file(
#    https://dancloud.dantalion.nl/index.php/s/3Zp504wnXbGuWdt/download
#    ${CMAKE_CURRENT_SOURCE_DIR}/vulkansdk.tar.gz
#    SHA256 6d8828fa9c9113ef4083a07994cf0eb13b8d239a5263bd95aa408d2f57585268
#)
#
## Extracts an archive in a directory and sets the directory as a file target (Nasty hack)
#extract_archive(
#    ${CMAKE_CURRENT_SOURCE_DIR}/vulkansdk.tar.gz # archive
#    ${CMAKE_CURRENT_SOURCE_DIR}/vulkansdk # destination
#    ${CMAKE_CURRENT_SOURCE_DIR} # working directory
#    vulkansdk-extract # new target to depend on
#    ${CMAKE_CURRENT_SOURCE_DIR}/vulkansdk.tar.gz # depends on, can't actually depend on file
#)
#
#add_custom_command(OUTPUT vulkan-shared-headers PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${PRJ_ROOT}  ..
#    COMMAND ${CMAKE_COMMAND} --install .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-headers/build
#    DEPENDS vulkan-shared-mkdir
#)
#
#directory_exists("${CMAKE_CURRENT_SOURCE_DIR}/vulkan-headers/build" has_directory)
#if(NOT ${has_directory})
#    add_custom_command(OUTPUT vulkan-shared-mkdir PRE_BUILD
#        COMMAND mkdir build
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-headers
#    )
#else()
#    add_custom_target(vulkan-shared-mkdir)
#endif()
#
## ------ #
## opencl #
## ------ #
#
#add_custom_command(OUTPUT opencl-shared-install PRE_BUILD
#    COMMAND ${CMAKE_COMMAND}  --build . --target install --config ${DEPENDENCY_BUILD_TYPE}
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/opencl-loader/build
#    DEPENDS opencl-shared-build
#)
#
#add_custom_command(OUTPUT opencl-shared-build PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} --build .
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/opencl-loader/build
#    DEPENDS opencl-shared-cmake
#)
#
#add_custom_command(OUTPUT opencl-shared-cmake PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=${DEPENDENCY_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${PRJ_ROOT} ..
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/opencl-loader/build
#    DEPENDS opencl-shared-directory
#)
#
#directory_exists("${CMAKE_CURRENT_SOURCE_DIR}/opencl-loader/build" has_directory)
#if(NOT ${has_directory})
#    add_custom_command(OUTPUT opencl-shared-directory PRE_BUILD
#        COMMAND mkdir build
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/opencl-loader
#        DEPENDS opencl-shared-headers
#    )
#else()
#    add_custom_target(opencl-shared-directory DEPENDS opencl-shared-headers)
#endif()
#
#add_custom_command(OUTPUT opencl-shared-headers PRE_BUILD
#    COMMAND cp -r CL ${PRJ_HEADERS}
#    COMMAND cp -r CL ${CMAKE_CURRENT_SOURCE_DIR}/opencl-loader/inc
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/opencl-headers
#)