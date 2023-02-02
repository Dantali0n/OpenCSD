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

project(dependencies)

# see cmake/dependencies.cmake for methods

set(DEPENDENCY_BUILD_TYPE ${CMAKE_BUILD_TYPE})

# -------------------------------------------- #
# Generator specific behavior (Thanks CMake..) #
# -------------------------------------------- #

if("${CMAKE_MAKE_PROGRAM}" MATCHES "ninja")
    set(command_nproc "$$(nproc)")
else()
    set(command_nproc "$(nproc)")
endif()

# ------------------------ #
# Dependency build targets #
# ------------------------ #

if(${QEMUCSD_WINDOWS})
    add_custom_target(boost-build DEPENDS boost-directory-fix)
else()
    add_custom_target(boost-build DEPENDS boost-install)
endif()

add_custom_target(activate-build DEPENDS activate-copy download-copy)
add_custom_target(generic-ebpf-build DEPENDS generic-ebpf-copy)
add_custom_target(ubpf-build DEPENDS ubpf-install)
add_custom_target(libfuse-build DEPENDS libfuse-install)
add_custom_target(libbpf-build DEPENDS libbpf-install)
add_custom_target(spdk-build DEPENDS spdk-install)
add_custom_target(dpdk-build DEPENDS dpdk-copy)
add_custom_target(isa-l-build DEPENDS isa-l-install)
add_custom_target(generate_bpftool DEPENDS bpftool_copy)
add_custom_target(qemu-build ALL DEPENDS qemu-image)
add_custom_target(xenium-build ALL DEPENDS xenium-copy)

# ------------------------- #
# Create binary directories #
# ------------------------- #

add_custom_command(OUTPUT generate-bin PRE_BUILD
    COMMAND mkdir -p ${PRJ_BINARIES}
    WORKING_DIRECTORY ${PRJ_ROOT}
)

add_custom_command(OUTPUT generate-include PRE_BUILD
    COMMAND mkdir -p ${PRJ_HEADERS}
    WORKING_DIRECTORY ${PRJ_ROOT}
)

add_custom_command(OUTPUT generate-lib PRE_BUILD
    COMMAND mkdir -p ${PRJ_LIBRARIES}
    WORKING_DIRECTORY ${PRJ_ROOT}
)

# -------------------- #
# Copy activate script #
# -------------------- #

add_custom_command(
    OUTPUT activate-copy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/scripts/activate
    ${PRJ_ROOT}/activate
)

add_custom_command(
    OUTPUT download-copy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/scripts/download-image.sh
    ${PRJ_ROOT}/download-image.sh
)

# ---------------------- #
# Xenium activate script #
# ---------------------- #

file_exists(${PRJ_HEADERS}/xenium/ramalhete_queue.hpp has_xenium)
if(${has_xenium})
    add_custom_target(xenium-copy)
else()
    add_custom_command(
        OUTPUT xenium-copy PRE_BUILD
        COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/xenium/xenium ${PRJ_HEADERS}
        DEPENDS generate-include
    )
endif()

# ----------------------------- #
# Platform dependent attributes #
# ----------------------------- #

if(${QEMUCSD_WINDOWS})
    set(boost_source_url https://nextcloud.dantalion.nl/index.php/s/34HknQ4FT6SspMp/download)
    set(boost_source_file boost.zip)
    set(boost_source_hash f22143b5528e081123c3c5ed437e92f648fe69748e95fa6e2bd41484e2986cc3)
    set(boost_source_bootstrap bootstrap.bat)
    set(boost_source_build b2.exe)
    set(boost_source_build_param link=static threading=multi)
else()
    set(boost_source_url https://nextcloud.dantalion.nl/index.php/s/zC9wqP62SMkaJij/download)
    set(boost_source_file boost.tar.bz2)
    set(boost_source_hash 8681f175d4bdb26c52222665793eef08490d7758529330f98d3b29dd0735bccc)
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
    600
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
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/boost/boost_1_78_0
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
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/boost/boost_1_78_0
    DEPENDS boost-bootstrap activate-build # Boost requires the activate script to work properly
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
            COMMAND mv boost-1_78/boost boost
            COMMAND rm -r boost-1_78
            WORKING_DIRECTORY ${PRJ_HEADERS}
            DEPENDS boost-rename-fix
        )
    else()
        add_custom_target(boost-directory-fix DEPENDS boost-rename-fix)
    endif()
endif()

# ------------ #
# generic-ebpf #
# ------------ #

# Check if libebpf is already build, REQUIRES RE-RUNNING CMAKE COMMAND!
# Limitations: this only tests the presence of a single file
file_exists(${PRJ_ROOT}/lib/libebpf.so has_genericebpf)
if(${has_genericebpf})
    add_custom_target(generic-ebpf-copy)
endif()

add_custom_command(OUTPUT generic-ebpf-compile PRE_BUILD
    COMMAND make clean
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generic-ebpf
)

add_custom_command(OUTPUT generic-ebpf-copy PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/generic-ebpf/libebpf.so ${PRJ_LIBRARIES}
    COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/generic-ebpf/sys/sys ${PRJ_HEADERS}
    DEPENDS generic-ebpf-compile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generic-ebpf
)

# ------- #
# libfuse #
# ------- #

# Check if libbpf is already build, REQUIRES RE-RUNNING CMAKE COMMAND!
# Limitations: this only tests the presence of a single file
file_exists(${PRJ_ROOT}/lib/libfuse3.so has_libfuse)
if(${has_libfuse})
    add_custom_target(libfuse-install)
endif()

add_custom_command(OUTPUT libfuse-install PRE_BUILD
    COMMAND ninja install
    DEPENDS libfuse-compile activate-build # fuse requires the activate script to work properly
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libfuse/build
)

add_custom_command(OUTPUT libfuse-compile PRE_BUILD
    COMMAND ninja
    DEPENDS libfuse-configure
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libfuse/build
)

add_custom_command(OUTPUT libfuse-configure PRE_BUILD
    COMMAND meson setup --wipe || exit 0
    COMMAND meson --prefix ${PRJ_ROOT} --reconfigure .. -Duseroot=false -Dudevrulesdir=${PRJ_ROOT}/etc/udev/rules.d || exit 0
    COMMAND meson --prefix ${PRJ_ROOT} .. -Duseroot=false -Dudevrulesdir=${PRJ_ROOT}/etc/udev/rules.d
    DEPENDS libfuse-directory
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libfuse/build
)

add_custom_command(OUTPUT libfuse-directory PRE_BUILD
    COMMAND mkdir build -p
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libfuse
)

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

file_exists(${PRJ_ROOT}/bin/bpftool has_bpftool)
if(${has_bpftool})
    add_custom_target(bpftool_copy)
endif()

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/bpf/bpftool/bpftool
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/bpf/bpftool
)

add_custom_command(OUTPUT bpftool_copy PRE_BUILD
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/bpf/bpftool/bpftool ${PRJ_ROOT}/bin
    DEPENDS ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/bpf/bpftool/bpftool generate-bin
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
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
add_custom_target(generate_vmlinux DEPENDS ${PRJ_HEADERS}/vmlinux.h generate_bpftool)

else()

add_custom_command(
    OUTPUT ${PRJ_HEADERS}/vmlinux.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/libbpf-bootstrap/src/vmlinux_508.h ${PRJ_HEADERS}/vmlinux.h
    WORKING_DIRECTORY ${PRJ_HEADERS}
)
add_custom_target(generate_vmlinux DEPENDS ${PRJ_HEADERS}/vmlinux.h generate_bpftool)

endif()

# -------- #
# bpf_load #
# -------- #

set(BPFLOAD_LIBRARIES
    #boost_program_options
)

set(BPFLOAD_SRC
    ${CMAKE_SOURCE_DIR}/dependencies/linux-510/bpf_load.c
    ${CMAKE_SOURCE_DIR}/dependencies/linux/tools/testing/selftests/bpf/trace_helpers.c
)

set(BPFLOAD_HEADERS
    ${CMAKE_SOURCE_DIR}/dependencies/linux-510/bpf_load.h
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
    COMMAND make -j ${command_nproc}
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
#    COMMAND cp -r bin ${PRJ_ROOT}
    COMMAND cp -r lib ${PRJ_ROOT}
    COMMAND cp -r include ${PRJ_ROOT}
    COMMAND cp -r share ${PRJ_ROOT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdk/dpdk/build
    DEPENDS spdk-build
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
    COMMAND make -j ${command_nproc}
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
    DEPENDS spdk-build
)

# ---- #
# ubpf #
# ---- #

# Check if libubpf.a is already build, REQUIRES RE-RUNNING CMAKE COMMAND!
# Limitations: this only tests the presence of a single file
file_exists(${PRJ_ROOT}/lib/libubpf.a has_ubpf)
if(${has_ubpf})
    add_custom_target(ubpf-install)
endif()

add_custom_command(OUTPUT ubpf-compile PRE_BUILD
    COMMAND make -C vm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ubpf
)

add_custom_command(OUTPUT ubpf-install PRE_BUILD
    COMMAND PREFIX=${PRJ_ROOT} make -C vm install
    DEPENDS ubpf-compile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ubpf
)

# --------------------- #
# qemu build and deploy #
# --------------------- #

# Check if qemu is already build, REQUIRES RE-RUNNING CMAKE COMMAND!
# Limitations: this only tests the presence of a single file
# Do not rebuild qemu if being run inside qemu
file_exists(${PRJ_ROOT}/bin/qemu-system-x86_64 has_qemu)
if(${has_qemu} OR IS_DEPLOYED)
    add_custom_target(qemu-image)
endif()

add_custom_command(OUTPUT qemu-image PRE_BUILD
    COMMAND qemu-img create -f raw znsssd.img 34359738368
    WORKING_DIRECTORY ${PRJ_ROOT}
    DEPENDS qemu-install
)

add_custom_command(OUTPUT qemu-install PRE_BUILD
    COMMAND make -j ${command_nproc} install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qemu/build
    DEPENDS qemu-configure activate-build
)

add_custom_command(OUTPUT qemu-configure PRE_BUILD
    COMMAND CFLAGS=-Wno-error=deprecated-declarations ../configure --enable-debug --disable-zstd --target-list=x86_64-softmmu --prefix=${PRJ_ROOT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qemu/build
    DEPENDS qemu-build-dir
)

#add_custom_command(OUTPUT qemu-patch PRE_BUILD
#    COMMAND bash -c 'git apply --check --reverse ../qemu-patches/ebpf-replace-deprecated-bpf_program__set_socket_filter.diff && git apply ../qemu-patches/ebpf-replace-deprecated-bpf_program__set_socket_filter.diff' || exit 0
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qemu
#    DEPENDS qemu-build-dir
#)

add_custom_command(OUTPUT qemu-build-dir PRE_BUILD
    COMMAND mkdir build -p
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qemu
)

# As qemu-configure target since the script is for qemu.
add_custom_command(
    TARGET qemu-build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/scripts/qemu-start-256.sh
    ${PRJ_ROOT}/qemu-start-256.sh
)

add_custom_command(
    TARGET qemu-build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/scripts/qemu-start-256-kvm.sh
    ${PRJ_ROOT}/qemu-start-256-kvm.sh
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
    https://nextcloud.dantalion.nl/index.php/s/xKPgSXcb66ggJ5j/download
    ${PRJ_ROOT}/arch-qemucsd.qcow2
    SHA256 6b35abbf09cb20392b2eaf1770210674deb0d5fd0242788b9ba83ec7fd3deead
    1500
)
endif()
