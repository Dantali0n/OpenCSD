# Functions to help with dependency management

# https://www.scivision.dev/cmake-download-verify-file/
function(download_file url filename hash_type hash)
    if(NOT EXISTS "${filename}")
        message("Downloading: ${filename}")
        file(DOWNLOAD ${url} ${filename}
            TIMEOUT 900  # seconds
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