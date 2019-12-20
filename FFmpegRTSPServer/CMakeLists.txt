cmake_minimum_required(VERSION 2.8)
project(RTSP_SERVER)
set(CMAKE_C_COMPILER g++)
add_definitions(-g -fpermissive -std=c++0x)
add_definitions(-DWITH_OPENSSL -DWITH_DOM -D__STDC_CONSTANT_MACROS)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/live/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/live/include/groupsock/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/live/include/BasicUsageEnvironment/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/live/include/liveMedia/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/live/include/UsageEnvironment/include)

#ffmpeg
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/ffmpeg/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/subsession)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/AudioRead)


link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/live/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/ffmpeg/lib)
set(FFMPEG_LIBRARIES avutil avcodec avformat swscale)

aux_source_directory(./ SRC_MAIN)
aux_source_directory(./subsession SUB_SESSION)
aux_source_directory(./AudioRead READ_AUDIO)

#set(ONVIF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/onvif/")
#set(ONVIF_SOURCE "${ONVIF_DIR}/duration.c ${ONVIF_DIR}/soapC.c ${ONVIF_DIR}/soapServer.c, ${ONVIF_DIR}/stdsoap2.c")

add_executable(rtsp_server ${SRC_MAIN} ${SUB_SESSION} ${READ_AUDIO})

target_link_libraries(rtsp_server ${FFMPEG_LIBRARIES} 
                    liveMedia BasicUsageEnvironment groupsock UsageEnvironment 
                    ssl crypto m pthread)




