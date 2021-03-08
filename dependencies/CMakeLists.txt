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
add_custom_target(spdk-build DEPENDS spdk-install)
add_custom_target(dpdk-build DEPENDS dpdk-copy)
add_custom_target(isa-l-build DEPENDS isa-l-install)

add_custom_target(qemu-build ALL DEPENDS qemu-install)

# -------------------- #
# Copy activate script #
# -------------------- #

# As boost-build target since only this dependency requires it.
add_custom_command(
    TARGET boost-build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/scripts/activate
    ${PRJ_ROOT}/activate
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

# Check if libbpf is already build, REQUIRES RE-RUNNING CMAKE COMMAND!
# Limitations: this only tests the presence of a single file
file_exists(${PRJ_ROOT}/lib/libbpf.a has_libbpf)
if(${has_libbpf})
    add_custom_target(libbpf-install)
endif()

add_custom_command(OUTPUT libbpf-install PRE_BUILD
    COMMAND BUILD_STATIC_ONLY=y PREFIX=${PRJ_ROOT} LIBDIR=${PRJ_ROOT}/lib make install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libbpf/src
)

# ------- #
# bpftool #
# ------- #

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/bpf/bpftool/bpftool
    COMMAND make -C tools/bpf/bpftool/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies/linux
)
add_custom_target(generate_bptfool DEPENDS ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/bpf/bpftool/bpftool)

add_custom_command(TARGET generate_bptfool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/bpf/bpftool/bpftool ${PRJ_ROOT}/bin
)

# --------- #
# vmlinux.h #
# --------- #

if(EXISTS "/sys/kernel/btf/vmlinux")

add_custom_command(
    OUTPUT ${PRJ_HEADERS}/vmlinux.h
    COMMAND ${PRJ_ROOT}/bin/bpftool btf dump file /sys/kernel/btf/vmlinux format c > ${PRJ_HEADERS}/vmlinux.h
    WORKING_DIRECTORY ${PRJ_HEADERS}
)
add_custom_target(generate_vmlinux DEPENDS ${PRJ_HEADERS}/vmlinux.h generate_bptfool)

else()

add_custom_command(
    OUTPUT ${PRJ_HEADERS}/vmlinux.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/libbpf-bootstrap/src/vmlinux_508.h ${PRJ_HEADERS}/vmlinux.h
    WORKING_DIRECTORY ${PRJ_HEADERS}
)
add_custom_target(generate_vmlinux DEPENDS ${PRJ_HEADERS}/vmlinux.h generate_bptfool)

endif()

# -------- #
# bpf_load #
# -------- #

set(BPFLOAD_LIBRARIES
    #boost_program_options
)

set(BPFLOAD_SRC
    ${CMAKE_SOURCE_DIR}/dependencies/linux/samples/bpf/bpf_load.c
    ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/testing/selftests/bpf/trace_helpers.c
)

set(BPFLOAD_HEADERS
    ${CMAKE_SOURCE_DIR}/dependencies/linux/samples/bpf/bpf_load.h
    ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/testing/selftests/bpf/trace_helpers.h
)

add_library(
    bpfload STATIC
    ${BPFLOAD_SRC}
    ${BPFLOAD_HEADERS}
)

target_link_libraries(
    bpfload
    bpf
    elf
    z
)

# Yes,  something inside the kernel tree has an external dependency
add_dependencies(bpfload libbpf-build)

target_include_directories(bpfload PUBLIC ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/lib)
#target_include_directories(bpfload PUBLIC ${CMAKE_SOURCE_DIR}/dependencies/linux/samples/bpf)
target_include_directories(bpfload PUBLIC ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/perf)
target_include_directories(bpfload PUBLIC ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/include)
target_include_directories(bpfload PUBLIC ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/include/uapi)

set_target_properties(bpfload PROPERTIES LINKER_LANGUAGE C)
set_target_properties(bpfload PROPERTIES COMPILER_LANGUAGE C)

# ---- #
# spdk #
# ---- #

# Check if spdk is already build, REQUIRES RE-RUNNING CMAKE COMMAND!
# Limitations: this only tests the presence of a single file
file_exists(${PRJ_ROOT}/lib/libspdk.so has_spdk)
if(${has_spdk})
    add_custom_target(spdk-install)
endif()

add_custom_command(OUTPUT spdk-install PRE_BUILD
    COMMAND make install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk
    DEPENDS spdk-compile
)

add_custom_command(OUTPUT spdk-compile PRE_BUILD
    COMMAND make -j $(nproc)
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk
    DEPENDS spdk-configure
)

add_custom_command(OUTPUT spdk-configure PRE_BUILD
    COMMAND ./configure --prefix=${PRJ_ROOT} --with-shared
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk
    DEPENDS spdk-init
)

add_custom_command(OUTPUT spdk-init PRE_BUILD
    COMMAND git submodule update --init
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk
)

# ---- #
# dpdk #
# ---- #

file_exists(${PRJ_ROOT}/lib/pkgconfig/libdpdk.pc has_dpdk)
if(${has_dpdk})
    add_custom_target(dpdk-copy)
endif()

#add_custom_command(OUTPUT dpdk-replace PRE_BUILD
#    COMMAND sed -i 's/prefix=/prefix=${PRJ_ROOT}/g' libdpdk.pc
#    COMMAND sed -i 's/prefix=/prefix=${PRJ_ROOT}/g' libdpdk-libs.pc
#    WORKING_DIRECTORY ${PRJ_ROOT}/lib/pkgconfig
#    DEPENDS dpdk-copy
#)

add_custom_command(OUTPUT dpdk-copy PRE_BUILD
    COMMAND cp -r bin ${PRJ_ROOT}
    COMMAND cp -r lib ${PRJ_ROOT}
    COMMAND cp -r include ${PRJ_ROOT}
    COMMAND cp -r share ${PRJ_ROOT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk/dpdk/build
    DEPENDS spdk-compile
)

# ----- #
# isa-l #
# ----- #

file_exists(${PRJ_ROOT}/lib/libisal.a has_isal)
if(${has_isal})
    add_custom_target(isa-l-install)
endif()

add_custom_command(OUTPUT isa-l-install PRE_BUILD
    COMMAND make install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk/isa-l
    DEPENDS isa-l-compile
)

add_custom_command(OUTPUT isa-l-compile PRE_BUILD
    COMMAND make -j $(nproc)
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk/isa-l
    DEPENDS isa-l-configure
)

add_custom_command(OUTPUT isa-l-configure PRE_BUILD
    COMMAND ./configure --prefix=${PRJ_ROOT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk/isa-l
    DEPENDS isa-l-autogen
)

add_custom_command(OUTPUT isa-l-autogen PRE_BUILD
    COMMAND ./autogen.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk/isa-l
    DEPENDS spdk-compile
)

# --------------------- #
# qemu build and deploy #
# --------------------- #

# Check if qemu is already build, REQUIRES RE-RUNNING CMAKE COMMAND!
# Limitations: this only tests the presence of a single file
# Do not rebuild qemu if being run inside qemu
file_exists(${PRJ_ROOT}/bin/qemu-system-x86_64 has_qemu)
if(${has_qemu} OR IS_DEPLOYED)
    add_custom_target(qemu-install)
endif()

add_custom_command(OUTPUT qemu-install PRE_BUILD
    COMMAND make -j $(nproc) install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qemu/build
    DEPENDS qemu-configure
)

add_custom_command(OUTPUT qemu-configure PRE_BUILD
    COMMAND ../configure --enable-debug --disable-zstd --target-list=x86_64-softmmu --prefix=${PRJ_ROOT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qemu/build
    DEPENDS qemu-build-dir
)

add_custom_command(OUTPUT qemu-build-dir PRE_BUILD
    COMMAND mkdir build -p
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qemu
)

# As qemu-configure target since the script is for qemu.
add_custom_command(
    TARGET qemu-build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/scripts/qemu-start.sh
    ${PRJ_ROOT}/qemu-start.sh
)

# As qemu-configure target since the script is for qemu.
add_custom_command(
    TARGET qemu-build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/scripts/qemu-stop.sh
    ${PRJ_ROOT}/qemu-stop.sh
)

# ---------- #
# Arch image #
# ---------- #

# Download qemu image if preparing deployment / on host development
if(NOT IS_DEPLOYED)
download_file(
    https://dancloud.dantalion.nl/index.php/s/YqXSNbKvgjW0iG7/download
    ${PRJ_ROOT}/arch-qemucsd.qcow2
    SHA256 e3fc42ddbbb33b193e59315e949ae176361e0c2af8e1f588aa145b38d32e5a42
)
endif()

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