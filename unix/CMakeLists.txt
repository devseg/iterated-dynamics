# Copyright 2018 Richard Thomson

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    add_library(os_hc STATIC unix.cpp)
    target_compile_definitions(os_hc PUBLIC ${OS_DEFINITIONS})
    target_include_directories(os_hc PRIVATE ../headers)

    add_library(os STATIC
        d_x11.cpp
        x11_frame.cpp
        x11_text.cpp
        x11_plot.cpp
        general.cpp
        os_unix.cpp
        unixscr.cpp
        video.cpp)
    target_link_libraries(os PUBLIC os_hc X11 m)
    target_compile_definitions(os PUBLIC ${OS_DEFINITIONS})
    target_include_directories(os PRIVATE ../headers)
    set_src_dir(unix.cpp)
    add_dependencies(os native_help)
endif()
