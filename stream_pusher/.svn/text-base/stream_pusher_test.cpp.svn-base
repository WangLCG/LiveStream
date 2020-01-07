/*
 * Car eye 车辆管理平台: www.car-eye.cn
 * Car eye 开源网址: https://github.com/Car-eye-team
 * MP4MuxerTest.cpp
 *
 * Author: Wgj
 * Date: 2019-03-04 22:37
 * Copyright 2019
 *
 * MP4混流器测试程序
 */

#include <iostream>
#include "Ffmpeg_stream_pusher.h"
#include "CEDebug.h"
#include <memory>
#include "RecvMulticastDataModule.h"
#include <signal.h>
#include <unistd.h>
#include "Ffmpeg_stream_pusher2.h"

using namespace std;

bool Exit = false;

void ctrl_C_handler(int s)
{
    printf("Caught signal %d\n", s);
    Exit = true;
}


// 包含音频
//#define HAS_AUDIO

const char* TestH264 = "../test.264";
#ifdef HAS_AUDIO
const char* TestAAC = "../test.aac";
#endif

bool threadIsWork = false;

/* 使能ffmpeg填充avpack实例 */
#define  ENABLE_FILL_PACK_INSTANCE   (1)

/*
* Comments: 指定地址数据是否为H264帧起始标识
* Param aBytes: 要检测的内存地址
* @Return 是否为H264起始标识
*/
bool IsH264Flag(uint8_t *aBytes)
{
    if (aBytes[0] == 0x00 && aBytes[1] == 0x00
        && aBytes[2] == 0x00 && aBytes[3] == 0x01)
    {
        uint8_t nal_type = aBytes[4] & 0x1F;
        return (nal_type == 0x07 || nal_type == 0x01);
    }

    return false;
}


int main()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrl_C_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    AVFormatContext* fmt_h264 = NULL;
    AVFormatContext* fmt_aac = NULL;
    int result = 0;
    bool video_finished = false, audio_finished = true;
    FILE *pFile = NULL;
    MP4Muxer *muxer = NULL;
    
    RecvMulticastDataModule* MultiCastModule = NULL;
    FillPKGStreamPusher* PusherHandle        = NULL;
    
    // 判断写音频还是写视频帧
    int64_t   compare_tag = -1;
    bool bwait = true;
    
    char cmd[100]={0};
    sprintf(cmd, "route add -net 225.18.0.0 netmask 255.255.0.0 %s", "eth0");
    system(cmd);

#ifdef HAS_AUDIO
    AVPacket pkt_v;
    audio_finished = false;
#endif

    std::cout << "Start muxer..." << std::endl;

    av_register_all();
    avformat_network_init();
    
    MultiCastModule = new RecvMulticastDataModule();
    MultiCastModule->AddMultGroup("225.18.1.0", 0);  /* video */
    MultiCastModule->AddMultGroup("225.18.1.1", 1);  /* audio */

    //if ((pFile = fopen(TestH264, "rb")) == 0)
    //{
    //    DEBUG_E("Open %s failed. Error[%s]", TestH264, strerror(errno));
    //    goto end;
   // }
#if(ENABLE_FILL_PACK_INSTANCE == 0)
    // 实例化混流器
    muxer = new MP4Muxer(MultiCastModule);
    FFMPEG_READ_PACK_CB videocb = {muxer, MP4Muxer::ReadVideoFrameCallback};
    FFMPEG_READ_PACK_CB audiocb = {muxer, MP4Muxer::ReadAudioFrameCallback};
    
    muxer->SetVideoCB(videocb);
    muxer->SetAudioCB(audiocb);
    if (!muxer->Start("rtsp://192.168.1.12:554/test"))
    {
        delete muxer;
        DEBUG_E("Muxer start fail.");
        goto end;
    }

    //sleep(1);
    
    while (1)
    {
        MultiFrameInfo video;
        MultiFrameInfo audio;
        usleep(2000);

        if(Exit)
        {
            break;
        }
        
        compare_tag = -1;
        bwait       = true;
        
        if(MultiCastModule->GetAudioQueueSize() > 0)
        {
            bwait = false;
        }
        if(!bwait)
        {
            compare_tag = MultiCastModule->GetCurrentVideoPTS() - MultiCastModule->GetCurrentAudioPTS();  //compare fileDate
        }

        printf("======= compare_tag[%lld] =====\n", compare_tag);
 #if (ENABLE_MULTICAST_QUEUE)
        //if (compare_tag - 80 <= 0 && compare_tag + 80 >= 0)
        if (compare_tag <= 0 )
        {
            // 读取一帧视频数据
            muxer->WriteVideoFrame();
            
        }
        else
        {
            // 读取一帧音频数据
            muxer->WriteAudioFrame();
            
        }
#else
        if (compare_tag <= 0)
        {
            //printf("1111Start Get video frame video_finished[%d]\n", video_finished);
            // 读取一帧视频数据
            if (!video_finished)
            {   
                //printf("Start Get video frame\n");
                MultiCastModule->GetVideoFrame(video);
                if(!video.buf || video.buf_size <=0)
                {
                    //DEBUG_D("Can't get video data");
                    continue;
                }
                
                MultiCastModule->SetCurrentVideoPTS(video.timeStamp);
                muxer->AppendVideo(video.buf, video.buf_size);
                delete [] video.buf;
                video.buf = NULL;
            }
        }
        else
        {
            // 读取一帧音频数据
            if (!audio_finished)
            {
                MultiCastModule->GetAudioFrame(audio);
                if(!audio.buf || audio.buf_size <=0)
                {
                    //DEBUG_D("Can't get Audio data");
                    continue;
                }

                MultiCastModule->SetCurrentAudioPTS(audio.timeStamp);
                
                // 需要确保传入的是一帧数据
                muxer->AppendAudio(audio.buf, audio.buf_size);
                delete [] audio.buf;
                audio.buf = NULL;
            }
        }

#endif
    }

    muxer->Stop();
    delete muxer;
#else
    const char* path = "rtsp://192.168.1.12:554/test";
    PusherHandle = new FillPKGStreamPusher(25, path, 0, 1920, 1080, MultiCastModule);

    while (1)
    {
        MultiFrameInfo video;
        MultiFrameInfo audio;
        AVPacket pkt;
        
        int stream_index = 0;
        
        usleep(2000);

        if(Exit)
        {
            break;
        }

        compare_tag = -1;
        bwait       = true;
        bool bwrite = false;
        
        if(MultiCastModule->GetAudioQueueSize() > 0)
        {
            bwait = false;
        }
        if(!bwait)
        {
            compare_tag = PusherHandle->GetCurrentVideoPTS() - PusherHandle->GetCurrentAudioPTS();  //compare fileDate
        }

        
        if (compare_tag <= 0)
        {
            //printf("1111Start Get video frame video_finished[%d]\n", video_finished);
            // 读取一帧视频数据
            //printf("Start Get video frame\n");
            MultiCastModule->GetVideoFrame(video);
            if(!video.buf || video.buf_size <=0)
            {
                //DEBUG_D("Can't get video data");
                continue;
            }

            stream_index = PusherHandle->GetVideoOutStreamIndex();
            bwrite = PusherHandle->FillAvPacket(stream_index, video, pkt);
            
        }
        else
        {
            MultiCastModule->GetAudioFrame(audio);
            if(!audio.buf || audio.buf_size <=0)
            {
                //DEBUG_D("Can't get video data");
                continue;
            }

           stream_index = PusherHandle->GetAudioOutStreamIndex();
           bwrite = PusherHandle->FillAvPacket(stream_index, audio, pkt);
        }

        printf("======= compare_tag[%lld] bwrite[%d] stream_index[%d]=====\n", compare_tag, bwrite, stream_index);
        if(bwrite)
        {
            PusherHandle->writeFileAvPacket(stream_index, pkt);
            if(compare_tag <= 0)
            {
                delete[] video.buf;
                video.buf = NULL;
            }
            else
            {
                delete[] audio.buf;
                audio.buf = NULL;
            }
        }
    }
    
    if(PusherHandle)
    {
        delete PusherHandle;
        PusherHandle = NULL;
    }
#endif

    if(MultiCastModule)
    {
        delete MultiCastModule;
        MultiCastModule = NULL;
    }

end:
    _DEBUG_I("Muxer finished.\n");
    getchar();
}

