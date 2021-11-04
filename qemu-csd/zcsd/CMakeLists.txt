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

qemucsd_include_directories()

set(ZCSD_SOURCE
    main.cxx
)

message("${PRJ_PRX}: zcsd cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(zcsd ${ZCSD_SOURCE})

get_property(local_modules GLOBAL PROPERTY module_list)

target_link_libraries(
    zcsd
    ${local_modules}
    ${${PRJ_PRX}_LIBRARIES}
)

add_dependencies(zcsd generate_bpf_zone_int_filter_header)

qemucsd_target_postprocess(zcsd)

set(ZCSD_SPDK_NATIVE_SOURCE
    main_spdk.cxx
)

message("${PRJ_PRX}: zcsd-spdk cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(zcsd-spdk ${ZCSD_SPDK_NATIVE_SOURCE})

target_link_libraries(
    zcsd-spdk
    ${local_modules}
    ${${PRJ_PRX}_LIBRARIES}
)

qemucsd_target_postprocess(zcsd-spdk)