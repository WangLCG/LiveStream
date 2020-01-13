cmake_minimum_required(VERSION 2.8)
project(RTSP_SERVER)
set(CMAKE_C_COMPILER g++)
add_definitions(-g -fpermissive -std=c++0x)
add_definitions(-DWITH_OPENSSL -DWITH_DOM -D__STDC_CONSTANT_MACROS)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/librtmp/include)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/librtmp/lib)

aux_source_directory(./ SRC_MAIN)

add_executable(rtmp_client ${SRC_MAIN} )

target_link_libraries(rtmp_client rtmp ssl crypto m pthread)




