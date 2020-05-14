cmake_minimum_required(VERSION 3.15)
project(erd_depth
        VERSION 1.0.0
        DESCRIPTION "ERD depth"
        LANGUAGES C
        )

set(CMAKE_REQUIRED_LIBRARIES "m")

set(CMAKE_C_STANDARD 99)

find_package(Jibal REQUIRED)

configure_file(erd_depth_config.h.in erd_depth_config.h @ONLY)



add_executable(erd_depth erd_depth.c)
#add_subdirectory(pcg)

target_include_directories(erd_depth PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>) #Because of erd_depth_config.h

target_link_libraries(erd_depth PRIVATE Jibal PRIVATE m)


INSTALL(TARGETS erd_depth
        RUNTIME DESTINATION bin)
