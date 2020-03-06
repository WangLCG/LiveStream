cmake_minimum_required(VERSION 2.8)
project(RTSP_SERVER)
set(CMAKE_C_COMPILER g++)
add_definitions(-g -fpermissive -std=c++0x)
add_definitions(-DWITH_OPENSSL -DWITH_DOM -D__STDC_CONSTANT_MACROS -DHAS_AUDIO)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#ffmpeg
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/ffmpeg/include)

#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/FFmpegPusher)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/StreamSource)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/ffmpeg/lib)
set(FFMPEG_LIBRARIES avutil avcodec avformat swscale postproc avfilter)

aux_source_directory(./ SRC_MAIN)
#aux_source_directory(./StreamSource STREAM_SOURCE)

add_executable(filter_demo ${SRC_MAIN}  )
target_link_libraries(filter_demo ${FFMPEG_LIBRARIES} pthread)

#add_executable(filter_demo simplest_ffmpeg_video_filter.cpp  )
#target_link_libraries(filter_demo ${FFMPEG_LIBRARIES} pthread)


