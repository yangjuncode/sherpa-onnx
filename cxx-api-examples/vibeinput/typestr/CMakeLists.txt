cmake_minimum_required(VERSION 3.10)
project(typestr C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to control whether to build the demo executable
option(build_demo "Build typestr demo program" OFF)

add_library(typestr STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/typestr.c
)

target_include_directories(typestr
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/base
        ${CMAKE_CURRENT_SOURCE_DIR}/key
)

# Demo executable (optional)
if(build_demo)
    add_executable(typestr_demo
        ${CMAKE_CURRENT_SOURCE_DIR}/demo.c
    )
    target_link_libraries(typestr_demo PRIVATE typestr)
endif()

# Platform-specific linking
if(APPLE)
    target_link_libraries(typestr PUBLIC
        "-framework ApplicationServices"
        "-framework IOKit"
        "-framework Carbon"
    )
elseif(WIN32)
    target_link_libraries(typestr PUBLIC user32)
else()
    # Assume X11 on Unix
    find_package(X11 REQUIRED)
    target_link_libraries(typestr PUBLIC X11::X11)
    # Xtst is used by XTest* functions
    find_library(XTST_LIB Xtst)
    if(XTST_LIB)
        target_link_libraries(typestr PUBLIC ${XTST_LIB})
    endif()
endif()

# Optionally export the include dir for consumers
set_target_properties(typestr PROPERTIES PUBLIC_HEADER "typestr.h")
