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

# Platform detection and setup
# cross compilation is not supported !

set(QEMUCSD_WINDOWS FALSE)
set(QEMUCSD_LINUX FALSE)
set(QEMUCSD_APPLE FALSE)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message("${PRJ_PRX}: Operating system: Windows")
    set(QEMUCSD_WINDOWS TRUE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("${PRJ_PRX}: Operating system: Linux")
    set(QEMUCSD_LINUX TRUE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message("${PRJ_PRX}: Operating system: MacOS")
    set(QEMUCSD_APPLE TRUE)
else ()
    message(FATAL "Operating system not supported!")
endif()

set(QEMUCSD_32BIT FALSE)
set(QEMUCSD_64BIT FALSE)

# Determine bits of address model based on TargetArch.cmake target_architecture result
# this function modifies AIRGLOW_32BIT and AIRGLOW_64BIT
function(determine_address_model architecture)
    if("${architecture}" STREQUAL "arm")
        set(QEMUCSD_32BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "armv5")
        set(QEMUCSD_32BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "armv6")
        set(QEMUCSD_32BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "armv7")
        set(QEMUCSD_32BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "armv8")
        set(QEMUCSD_32BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "aarch64")
        set(QEMUCSD_64BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "i386")
        set(QEMUCSD_32BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "x86_64")
        set(QEMUCSD_64BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "ia64")
        set(QEMUCSD_64BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "ppc")
        set(QEMUCSD_32BIT TRUE PARENT_SCOPE)
    elseif("${architecture}" STREQUAL "ppc64")
        set(QEMUCSD_64BIT TRUE PARENT_SCOPE)
    else()
        message("${PRJ_PRX}: ${architecture} does not match any known strings")
    endif()
endfunction(determine_address_model)