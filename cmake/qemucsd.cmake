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

# Functions and global related to engine modules and includes

# ------------------------------------------------ #
# Add a new module to the global list of modules.  #
# Everything included in this list will eventually #
# be compiled against the engine.                  #
# ------------------------------------------------ #
set_property(GLOBAL PROPERTY module_list)
function(add_qemucsd_module)
    get_property(tmp GLOBAL PROPERTY module_list)

    # Prevent leading whitespace for first entry
    if("${tmp}" STREQUAL "")
        set(first TRUE)
        foreach(arg ${ARGV})
            if(first)
                set(tmp ${arg})
            else()
                set(tmp ${tmp} ${arg})
            endif()
        endforeach()
    else()
        foreach(arg ${ARGV})
            set(tmp ${tmp} ${arg})
        endforeach()
    endif()
    set_property(GLOBAL PROPERTY module_list ${tmp})
endfunction(add_qemucsd_module)

# ------------------------------------------------- #
# Add a new include to the global list of includes. #
# All parts of the engine that need access to other #
# parts can use qemucsd_include_directories() to    #
# add them to the current context.                  #
# ------------------------------------------------- #
set_property(GLOBAL PROPERTY include_list)
function(add_qemucsd_include)
    get_property(tmp GLOBAL PROPERTY include_list)
    foreach(arg ${ARGV})
        set(tmp ${tmp} ${arg})
    endforeach()
    set_property(GLOBAL PROPERTY include_list ${tmp})
endfunction(add_qemucsd_include)

# ----------------------------------- #
# Add all current include directories #
# ----------------------------------- #
function(qemucsd_include_directories)
    get_property(local_includes GLOBAL PROPERTY include_list)
    FOREACH(local_include ${local_includes})
        include_directories(${local_include})
    ENDFOREACH()
endfunction(qemucsd_include_directories)

# ---------------------------------------- #
# perform post_processing for added target #
# ---------------------------------------- #
function(qemucsd_target_postprocess)
    foreach(arg ${ARGV})
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_definitions(${arg} PUBLIC QEMUCSD_DEBUG)
            add_backward(${arg})
        endif()
    endforeach()
endfunction(qemucsd_target_postprocess)