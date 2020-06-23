cmake_minimum_required(VERSION 2.8)
project(ONVIF_CLIENT)
set(CMAKE_C_COMPILER g++ )

message(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-g -fpermissive -std=c++0x)
add_definitions(-DWITH_OPENSSL -DWITH_DOM)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libonvif/)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libonvif)

aux_source_directory(./ SRC_MAIN)

add_executable(onvif_client_demo ${SRC_MAIN} )

target_link_libraries(onvif_client_demo onvif ssl crypto)