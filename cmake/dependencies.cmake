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

# Functions to help with dependency management

# https://www.scivision.dev/cmake-download-verify-file/
function(download_file url filename hash_type hash timeout)
    if(NOT EXISTS "${filename}")
        message("Downloading: ${filename}")
        file(DOWNLOAD ${url} ${filename}
            TIMEOUT ${timeout}  # seconds
            EXPECTED_HASH ${hash_type}=${hash}
            TLS_VERIFY ON)
    endif()
endfunction(download_file)

function(extract_archive file destination working_directory target depends)
    if(NOT EXISTS "${destination}")
        message("Extracting: ${file}")
        if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
            message(WARNING
                "Native ARCHIVE_EXTRACT not supported with this"
                "version of CMake, archive extraction may fail!"
            )
            add_custom_command(
                OUTPUT ${target} PRE_BUILD
                COMMAND tar -xzvf ${file} -C ${destination}
                WORKING_DIRECTORY ${working_directory}
                DEPENDS ${depends}
            )
        else()
            add_custom_target(${target} DEPENDS ${depends})
            file(ARCHIVE_EXTRACT INPUT ${file} DESTINATION ${destination})
        endif()
    else()
        # trigger target if already extracted
        add_custom_target(${target} DEPENDS ${depends})
    endif()
endfunction(extract_archive)

function(directory_exists path output)
    get_filename_component(fullpath "${path}" REALPATH)
    if(EXISTS "${fullpath}")
        set(${output} TRUE PARENT_SCOPE)
    else()
        set(${output} FALSE PARENT_SCOPE)
    endif()
endfunction(directory_exists)

function(file_exists file output)
    if(EXISTS "${file}")
        set(${output} TRUE PARENT_SCOPE)
    else()
        set(${output} FALSE PARENT_SCOPE)
    endif()
endfunction(file_exists)