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