cmake_minimum_required(VERSION 2.8)
project(RTMP_CLIENT)
set(CMAKE_C_COMPILER g++)
add_definitions(-g -fpermissive -std=c++0x)
add_definitions(-DWITH_OPENSSL -DWITH_DOM -D__STDC_CONSTANT_MACROS)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#ffmpeg
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/ffmpeg/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/ffmpeg/lib)
set(FFMPEG_LIBRARIES avutil avcodec avformat swscale)

aux_source_directory(./ SRC_MAIN)
#aux_source_directory(./AudioRead READ_AUDIO)

add_executable(rtmp_client ${SRC_MAIN} )

target_link_libraries(rtmp_client ${FFMPEG_LIBRARIES} ssl crypto m pthread)




