cmake_minimum_required(VERSION 3.13)
project(erd_depth
        VERSION 1.1.0
        DESCRIPTION "ERD depth and ToF-E list"
        LANGUAGES C
        )

set(CMAKE_REQUIRED_LIBRARIES "m")

set(CMAKE_C_STANDARD 99)

find_package(Jibal 0.3.11 REQUIRED)
if(Jibal_FOUND)
    message(STATUS "JIBAL ${Jibal_VERSION} headers found at ${Jibal_INCLUDE_DIR}")
endif()
find_package(OpenMP)

configure_file(erd_depth_config.h.in erd_depth_config.h @ONLY)

if(APPLE) #Hack to fix some issues with rpath on macOS
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif("${isSystemDir}" STREQUAL "-1")
endif()



add_executable(erd_depth erd_depth.c)
add_executable(tofe_list
        tofe_list.c tofe_list.h
        tof_in.c tof_in.h
        message.c message.h
        "$<$<BOOL:${WIN32}>:win_compat.c>"
)

target_include_directories(erd_depth PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>) #Because of erd_depth_config.h

target_link_libraries(erd_depth 
    PRIVATE jibal
    PRIVATE "$<$<BOOL:${UNIX}>:m>"
    )

target_link_libraries(tofe_list
        PRIVATE jibal
        PRIVATE "$<$<BOOL:${UNIX}>:m>"
)

if(OpenMP_C_FOUND)
    target_link_libraries(erd_depth PUBLIC OpenMP::OpenMP_C)
endif()

INSTALL(TARGETS erd_depth tofe_list
        RUNTIME DESTINATION bin)
