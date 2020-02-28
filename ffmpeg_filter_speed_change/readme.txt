环境&使用：
    centos6.9
    需要将ffmpeg的库及头文件放入到3rdpart中，可直接从LiveStream/stream_pusher/ffmpeg拷贝一份
    cd build; cmake ../; make; ./filter_demo

功能：
    使用avfilter将现有的视频文件的播放速度变为2倍速播放，暂时只支持视频;