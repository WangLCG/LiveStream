#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "rtmp_client.h"
#include <string>

bool Exit = false;

void ctrl_C_handler(int s)
{
    printf("Caught signal %d\n", s);
    Exit = true;
}

void write_file(FILE* fd, char* data, unsigned int size)
{
    if(!fd || !data)
    {
        return;
    }

    fwrite(data, 1, size, fd);
    fflush(fd);
}

int main()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrl_C_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    //const char* url = "rtmp://eduzz.itc-pa.cn:1935/live/012A305B4C0945CFBD2C";
    //string url = "rtmp://172.16.41.251/vod/十九大.mp4";
    string url = "rtmp://eduzz.itc-pa.cn:1935/live/012A305B4C0945CFBD2C";
    ff_rtmp_client rtmp_client(url.c_str());
    if(rtmp_client.rtmp_pull_open() != 0)
    {
        printf("file[%s] rtmp_pull_open faile \n", __FILE__);
        return -1;
    }

    rtmp_client.start_process();

    FrameInfo video;
    FrameInfo audio;

    FILE* vfd = fopen("rtmp.264", "wb+");
    if(!vfd)
    {
        printf("fopen rtmp.264 falie \n");
        return -1;
    }

    FILE* afd = fopen("rtmp.aac", "wb+");
    if(!afd)
    {
        printf("fopen rtmp.aac falie \n");
        return -1;
    }

    while(!Exit)
    {
        rtmp_client.get_video_frame(video);
        if(video.data && video.data_size > 0)
        {
            write_file(vfd, video.data, video.data_size);
            delete[] video.data;
        }

        rtmp_client.get_audio_frame(audio);
        if(audio.data && audio.data_size > 0)
        {
            write_file(afd, audio.data, audio.data_size);
            delete[] audio.data;
        }

        usleep(10000);
    }

    rtmp_client.stop_process();
    fclose(afd);
    fclose(vfd);

    return 0;
}