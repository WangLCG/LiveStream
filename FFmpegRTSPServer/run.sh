#!/bin/bash
SHELL_PATH=$(cd `dirname $0`; pwd)
echo $SHELL_PATH
export LD_LIBRARY_PATH=$SHELL_PATH/3rdpart/ffmpeg/lib

$SHELL_PATH/build/rtsp_server $SHELL_PATH/1.avi 554 84
