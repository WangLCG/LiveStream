cmake_minimum_required(VERSION 2.8)
project(SPS_MODULE)
set(CMAKE_C_COMPILER g++)
add_definitions(-g -fpermissive -std=c++11 )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
aux_source_directory(./ SRC_MAIN)

add_executable(sps_demo ${SRC_MAIN})
target_link_libraries(sps_demo )
