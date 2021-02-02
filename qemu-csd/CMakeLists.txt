# Commandline argument parsing
add_subdirectory(arguments)

# Entry point for engine
add_subdirectory(entry)

qemucsd_include_directories()

set(AIRGLOW_SOURCE
    main.cxx
)

message("${PRJ_PRX}: airglow cxx flags:${CMAKE_CXX_FLAGS}")
add_executable(airglow ${AIRGLOW_SOURCE})

get_property(local_modules GLOBAL PROPERTY module_list)

target_link_libraries(
    airglow
    ${local_modules}
#    ${${PRJ_PRX}_LIBRARIES}
)