cmake_minimum_required(VERSION 2.8)
project(TCP_MODULE)
set(CMAKE_C_COMPILER g++)
add_definitions(-g -fpermissive -std=c++11 )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ffmpeg/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../FfmpegReceiver)

aux_source_directory(./ SRC_MAIN)
aux_source_directory(../source LIBMPEG)
aux_source_directory(../FfmpegReceiver FFMPEGRECEIVER)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ffmpeg/lib)
set(FFMPEG_LIBRARIES avutil avcodec avformat)

add_executable(ps_demo ${SRC_MAIN} ${LIBMPEG} ${FFMPEGRECEIVER})
target_link_libraries(ps_demo ${FFMPEG_LIBRARIES} -pthread -lrt)

#add_executable(ps_dec_demo ${SRC_MAIN} ${LIBMPEG} )
#target_link_libraries(ps_dec_demo -pthread -lrt)