/*
 * Car eye 车辆管理平台: www.car-eye.cn
 * Car eye 开源网址: https://github.com/Car-eye-team
 * MP4Muxer.cpp
 *
 * Author: Wgj
 * Date: 2019-03-04 22:37
 * Copyright 2019
 *
 * MP4混流器类
 */
 
#include <iostream>
#include "CEDebug.h"
#include "Ffmpeg_stream_pusher.h"

// av_read_frame读取AAC缓冲区大小 1K字节
#define AUDIO_BUFFER_SIZE       2048
// AAC队列项个数
#define AUDIO_QUEUE_SIZE        100
// AAC每个队列项缓冲区大小
#define AUDIO_ITEM_SIZE         2048
// AAC队列标识
#define AUDIO_QUEUE_FLAG        0x10

// av_read_frame读取H264缓冲区大小 1K字节
#define VIDEO_BUFFER_SIZE       (1*1024*1024)
// H264队列项个数
#define VIDEO_QUEUE_SIZE        100
// H264每个队列项缓冲区大小
#define VIDEO_ITEM_SIZE         (1*1024*1024)
// H264队列标识
#define VIDEO_QUEUE_FLAG        0x20

// 保存音频数据用于调试
//#define SAVE_AUDIO_FILE

// 是否已经初始化过FFMPEG库
static bool mIsInitFFMPEG = false;

// aac静音数据, 用于填充MP4文件头
const uint8_t AAC_MUTE_DATA[361] = {
    0xFF,0xF1,0x6C,0x40,0x2D,0x3F,0xFC,0x00,0xE0,0x34,0x20,0xAD,0xF2,0x3F,0xB5,0xDD,
    0x73,0xAC,0xBD,0xCA,0xD7,0x7D,0x4A,0x13,0x2D,0x2E,0xA2,0x62,0x02,0x70,0x3C,0x1C,
    0xC5,0x63,0x55,0x69,0x94,0xB5,0x8D,0x70,0xD7,0x24,0x6A,0x9E,0x2E,0x86,0x24,0xEA,
    0x4F,0xD4,0xF8,0x10,0x53,0xA5,0x4A,0xB2,0x9A,0xF0,0xA1,0x4F,0x2F,0x66,0xF9,0xD3,
    0x8C,0xA6,0x97,0xD5,0x84,0xAC,0x09,0x25,0x98,0x0B,0x1D,0x77,0x04,0xB8,0x55,0x49,
    0x85,0x27,0x06,0x23,0x58,0xCB,0x22,0xC3,0x20,0x3A,0x12,0x09,0x48,0x24,0x86,0x76,
    0x95,0xE3,0x45,0x61,0x43,0x06,0x6B,0x4A,0x61,0x14,0x24,0xA9,0x16,0xE0,0x97,0x34,
    0xB6,0x58,0xA4,0x38,0x34,0x90,0x19,0x5D,0x00,0x19,0x4A,0xC2,0x80,0x4B,0xDC,0xB7,
    0x00,0x18,0x12,0x3D,0xD9,0x93,0xEE,0x74,0x13,0x95,0xAD,0x0B,0x59,0x51,0x0E,0x99,
    0xDF,0x49,0x98,0xDE,0xA9,0x48,0x4B,0xA5,0xFB,0xE8,0x79,0xC9,0xE2,0xD9,0x60,0xA5,
    0xBE,0x74,0xA6,0x6B,0x72,0x0E,0xE3,0x7B,0x28,0xB3,0x0E,0x52,0xCC,0xF6,0x3D,0x39,
    0xB7,0x7E,0xBB,0xF0,0xC8,0xCE,0x5C,0x72,0xB2,0x89,0x60,0x33,0x7B,0xC5,0xDA,0x49,
    0x1A,0xDA,0x33,0xBA,0x97,0x9E,0xA8,0x1B,0x6D,0x5A,0x77,0xB6,0xF1,0x69,0x5A,0xD1,
    0xBD,0x84,0xD5,0x4E,0x58,0xA8,0x5E,0x8A,0xA0,0xC2,0xC9,0x22,0xD9,0xA5,0x53,0x11,
    0x18,0xC8,0x3A,0x39,0xCF,0x3F,0x57,0xB6,0x45,0x19,0x1E,0x8A,0x71,0xA4,0x46,0x27,
    0x9E,0xE9,0xA4,0x86,0xDD,0x14,0xD9,0x4D,0xE3,0x71,0xE3,0x26,0xDA,0xAA,0x17,0xB4,
    0xAC,0xE1,0x09,0xC1,0x0D,0x75,0xBA,0x53,0x0A,0x37,0x8B,0xAC,0x37,0x39,0x41,0x27,
    0x6A,0xF0,0xE9,0xB4,0xC2,0xAC,0xB0,0x39,0x73,0x17,0x64,0x95,0xF4,0xDC,0x33,0xBB,
    0x84,0x94,0x3E,0xF8,0x65,0x71,0x60,0x7B,0xD4,0x5F,0x27,0x79,0x95,0x6A,0xBA,0x76,
    0xA6,0xA5,0x9A,0xEC,0xAE,0x55,0x3A,0x27,0x48,0x23,0xCF,0x5C,0x4D,0xBC,0x0B,0x35,
    0x5C,0xA7,0x17,0xCF,0x34,0x57,0xC9,0x58,0xC5,0x20,0x09,0xEE,0xA5,0xF2,0x9C,0x6C,
    0x39,0x1A,0x77,0x92,0x9B,0xFF,0xC6,0xAE,0xF8,0x36,0xBA,0xA8,0xAA,0x6B,0x1E,0x8C,
    0xC5,0x97,0x39,0x6A,0xB8,0xA2,0x55,0xA8,0xF8
};

/*
* Comments: 创建一个指定大小的队列
* Param aCount: 队列项的个数
* Param aItemSize: 队列每项的字节数
* @Return 创建的队列, 失败返回NULL
*/
static CESimpleQueue* CreateQueue(int aCount, int aItemSize)
{
    CESimpleQueue* queue = (CESimpleQueue *)malloc(sizeof(CESimpleQueue));
    if (queue == NULL)
    {
        return NULL;
    }

    queue->Items = (CEQueueItem *)malloc(sizeof(CEQueueItem) * aCount);
    if (queue->Items == NULL)
    {
        // 释放申请的内存
        free(queue);
        return NULL;
    }
    for (int i = 0; i < aCount; i++)
    {
        queue->Items[i].Bytes = (uint8_t *)malloc(aItemSize);
        queue->Items[i].TotalSize = aItemSize;
        queue->Items[i].EnqueueSize = 0;
        queue->Items[i].DequeueSize = 0;
    }
    queue->Count = aCount;
    queue->DequeueIndex = 0;
    queue->EnqueueIndex = 0;

    return queue;
}

/*
* Comments: 销毁队列
* Param aQueue: 要销毁的队列
* @Return void
*/
static void DestroyQueue(CESimpleQueue* aQueue)
{
    if (aQueue == NULL)
    {
        return;
    }

    // 释放队列所有申请的内存
    for (int i = 0; i < aQueue->Count; i++)
    {
        free(aQueue->Items[i].Bytes);
    }

    free(aQueue->Items);
    free(aQueue);
}

/*
* Comments: 队列数据入队操作
* Param aQueue: 队列实体, 函数内部不进行空检测
* Param aBytes: 要入队的数据
* Param aSize: 要入队的数据字节数
* @Return 入队成功与否
*/
static bool Enqueue(CESimpleQueue* aQueue, uint8_t* aBytes, int aSize)
{
    if (aBytes == NULL || aSize < 1)
    {
        return false;
    }

    int tail = aQueue->EnqueueIndex;
    int size = FFMIN(aSize, aQueue->Items[tail].TotalSize);
    while (size > 0)
    {
        memcpy(aQueue->Items[tail].Bytes, aBytes, size);
        aQueue->Items[tail].EnqueueSize = size;
        aQueue->Items[tail].DequeueSize = 0;
        aQueue->EnqueueIndex = (aQueue->EnqueueIndex + 1) % aQueue->Count;
        tail = aQueue->EnqueueIndex;
        // 数据过多, 继续入队
        aSize -= size;
        aBytes += size;
        size = FFMIN(aSize, aQueue->Items[tail].TotalSize);
    }

    return true;
}

static void PrintRation(AVRational& ration, char* desp)
{
    DEBUG_D("================== [%s] num[%d] den[%d]  ================== ", desp, ration.num, ration.den);
}

/*
* Comments: 读取媒体内存流数据FFMPEG回调方法 出队操作
* Param aHandle: 队列句柄
* Param aBytes: [输出] 要读取的数据
* Param aSize: 要读取的字节数
* @Return 读取到的实际字节个数
*/
static int ReadBytesCallback(void *aHandle, uint8_t *aBytes, int aSize)
{
    CESimpleQueue *pQueue = (CESimpleQueue *)aHandle;

    if (pQueue->DequeueIndex == pQueue->EnqueueIndex)
    {
        //      DEBUG_W("Queue[%02X] head index equal tail index: %d.", pQueue->Flag, pQueue->DequeueIndex);
        return 0;
    }

    int head = pQueue->DequeueIndex;
//  DEBUG_V("Queue[%02X]%d deqSize:%d, enqSize:%d, aSize=%d.", pQueue->Flag, head,
//      pQueue->Items[head].DequeueSize, pQueue->Items[head].EnqueueSize, aSize);
    int size = FFMIN(aSize, pQueue->Items[head].EnqueueSize);

    // 复制数据出队
    memcpy(aBytes, pQueue->Items[head].Bytes + pQueue->Items[head].DequeueSize, size);

    pQueue->Items[head].DequeueSize += size;
    pQueue->Items[head].EnqueueSize -= size;

    if (pQueue->Items[head].EnqueueSize <= 0)
    {
        pQueue->Items[head].DequeueSize = 0;
        pQueue->DequeueIndex = (pQueue->DequeueIndex + 1) % pQueue->Count;
    }

    return size;
}

/*
* Comments: MP4混流器初始化
* Param : 
* @Return void
*/
MP4Muxer::MP4Muxer(RecvMulticastDataModule* MultiCastModule): mMultiCastModule(MultiCastModule)
{
    this->mInputAudioContext = NULL;
    this->mInputVideoContext = NULL;
    this->mOutputFrmtContext = NULL;
    this->mAudioFilter = NULL;
    this->mVideoFilter = NULL;
    this->mAudioBuffer = NULL;
    this->mVideoBuffer = NULL;
    this->mIsStartAudio = false;
    this->mIsStartVideo = false;
    this->mAudioQueue = NULL;
    this->mVideoQueue = NULL;
    this->mAudioIndex = -1;
    this->mVideoIndex = -1;
    this->mAudioOutIndex = -1;
    this->mVideoOutIndex = -1;
    this->mFrameIndex = 0;

    memset(&mVideoCB, 0, sizeof(FFMPEG_READ_PACK_CB));
    memset(&mAudioCB, 0, sizeof(FFMPEG_READ_PACK_CB));

}

MP4Muxer::~MP4Muxer()
{
}

/*
* Comments: 混流器初始化 实例化后首先调用该方法进行内部参数的初始化
* Param aFileName: 要保存的文件名
* @Return 是否成功
*/
bool MP4Muxer::Start(std::string aFileName)
{
    if (this->mOutputFrmtContext)
    {
        // 已经初始化过了.
        return true;
    }

    if (!mIsInitFFMPEG)
    {
        mIsInitFFMPEG = true;
        av_register_all();
    }

    if (aFileName.empty())
    {
        // 文件名为空
        return false;
    }

    DEBUG_V("Start muxer: %s.", aFileName.c_str());

    //av_log_set_level(AV_LOG_TRACE);
    
    // 分别创建AAC跟H264数据队列
    if (this->mAudioQueue == NULL)
    {
        this->mAudioQueue = CreateQueue(AUDIO_QUEUE_SIZE, AUDIO_ITEM_SIZE);
        if (this->mAudioQueue == NULL)
        {
            return false;
        }
        this->mAudioQueue->Flag = AUDIO_QUEUE_FLAG;
    }
    if (this->mVideoQueue == NULL)
    {
        this->mVideoQueue = CreateQueue(VIDEO_QUEUE_SIZE, VIDEO_ITEM_SIZE);
        if (this->mVideoQueue == NULL)
        {
            return false;
        }
        this->mVideoQueue->Flag = VIDEO_QUEUE_FLAG;
    }

    if (!mAudioFilter)
    {
        // 创建AAC滤波器
        mAudioFilter = av_bitstream_filter_init("aac_adtstoasc");
        if (!mAudioFilter)
        {
            // 创建失败
            DEBUG_E("Init aac filter fail.");
            return false;
        }
    }

    if (!mVideoFilter)
    {
        // 创建H264滤波器
        mVideoFilter = av_bitstream_filter_init("h264_mp4toannexb");
        if (!mVideoFilter)
        {
            // 创建失败
            DEBUG_E("Init h264 filter fail.");
            return false;
        }
    }

    this->mFrameIndex = 0;
    this->mFileName = aFileName;

    // 输出流上下文初始化
    //int ret = avformat_alloc_output_context2(&this->mOutputFrmtContext, NULL, "mp4", this->mFileName.c_str());
    int ret = avformat_alloc_output_context2(&this->mOutputFrmtContext, NULL, "rtsp", this->mFileName.c_str());
    if (!this->mOutputFrmtContext)
    {
        DEBUG_E("Alloc output format from file extension failed: %d!", ret);
        return false;
    }

    return true;
}

/*
* Comments: 关闭并释放输出格式上下文
* Param : None
* @Return None
*/
void MP4Muxer::Stop(void)
{
    // lock
    std::lock_guard<std::mutex> lockGuard(this->mLock);

    DEBUG_V("Stop muxer: %s.", this->mFileName.c_str());

    if (this->mOutputFrmtContext)
    {
        if (this->mIsStartVideo)
        {
            // 写入文件尾
            av_write_trailer(this->mOutputFrmtContext);
        }

        // 如果已创建文件则进行关闭
        if (!(this->mOutputFrmtContext->oformat->flags & AVFMT_NOFILE))
        {
            avio_closep(&this->mOutputFrmtContext->pb);
        }

        // 释放上下文
        avformat_free_context(this->mOutputFrmtContext);
        this->mOutputFrmtContext = NULL;
    }

    // 释放滤波器
    if (mAudioFilter)
    {
        av_bitstream_filter_close(mAudioFilter);
        mAudioFilter = NULL;
    }
    if (mVideoFilter)
    {
        av_bitstream_filter_close(mVideoFilter);
        mVideoFilter = NULL;
    }

    // 释放输入流
    if (mInputAudioContext)
    {
        avio_context_free(&mInputAudioContext->pb);
        avformat_close_input(&mInputAudioContext);
        mInputAudioContext = NULL;
        mAudioBuffer = NULL;
    }
    else if (mAudioBuffer)
    {
        av_freep(&mAudioBuffer);
    }
    if (mInputVideoContext)
    {
        avio_context_free(&mInputVideoContext->pb);
        avformat_close_input(&mInputVideoContext);
        mInputVideoContext = NULL;
        mVideoBuffer = NULL;
    }
    else if (mVideoBuffer)
    {
        av_freep(&mVideoBuffer);
    }

    // 释放队列
    if (this->mAudioQueue)
    {
        DestroyQueue(mAudioQueue);
        mAudioQueue = NULL;
    }
    if (this->mVideoQueue)
    {
        DestroyQueue(mVideoQueue);
        mVideoQueue = NULL;
    }

}

/*
* Comments: 写入H264视频帧到文件
* Param : None
* @Return void
*/
int MP4Muxer::WriteVideoFrame(void)
{

#if ENABLE_MULTICAST_QUEUE
    if (!this->mIsStartVideo)
    {
        if (OpenVideo() < 0)
        {
            return -1;
        }
        //DEBUG_D("111 Start video \n");
        this->mIsStartVideo = true;
    }
#endif

    int ret = -1;
    int count = 0;

    // 流媒体数据包
    AVPacket pkt;
    AVStream *in_stream = NULL, *out_stream = NULL;
    av_init_packet(&pkt);
    
    //while (1)
    {
        // 获取一个数据包
        ret = av_read_frame(mInputVideoContext, &pkt);
        //DEBUG_D("-------------Read[%d] video frame[%d] ret=-%08X.----------\n", count++, mVideoQueue->DequeueIndex, -ret);
        if (ret < 0)
        {
            // AVERROR_EOF表示流读取完
            if (ret != AVERROR_EOF)
            {
                DEBUG_W("Read video frame ret = -%08X.", -ret);
            }
            return ret;
        }
        
        //DEBUG_V("read index: %d, video index: %d, %d.", pkt.stream_index, mVideoIndex, mVideoOutIndex);
        in_stream  = mInputVideoContext->streams[mVideoIndex];
        out_stream = mOutputFrmtContext->streams[mVideoOutIndex];

        // 如果没有显示时间戳自己加上时间戳并且将显示时间戳赋值给解码时间戳
        if (pkt.pts == AV_NOPTS_VALUE)
        {
            //Write PTS
            AVRational time_base1 = in_stream->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
            //Parameters
            pkt.pts = (double)(mFrameIndex * calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            
            mFrameIndex++;
        }

        // H264视频处理
        //av_bitstream_filter_filter(mVideoFilter, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);

        //Convert PTS/DTS
        DEBUG_D("----------------- pkt.pts [%llu] -----------------", pkt.pts);
#if 1
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
#else
        DEBUG_D("----------------- pkt.pts[%llu], VIDEO cur_time[%llu],  begin[%llu]  -----------------", 
            pkt.pts, mMultiCastModule->GetCurrentVideoPTS(), mMultiCastModule->GetBeginVideoFrameTime());

        if(mMultiCastModule->GetCurrentVideoPTS() == 0)
        {
            av_free_packet(&pkt);
            return -1;
        }
        else
        {
            pkt.pts = av_rescale_q_rnd(mMultiCastModule->GetCurrentVideoPTS() - mMultiCastModule->GetBeginVideoFrameTime(), in_stream->time_base,
                    out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        }
        
        pkt.dts = pkt.pts;
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        
#endif
        pkt.stream_index = mVideoOutIndex;
        pkt.pos = -1;

        //printf("------ video pts[%lld] dts[%lld] ----\n", pkt.pts, pkt.dts);
        //Write
        if (av_interleaved_write_frame(mOutputFrmtContext, &pkt) < 0)
        {
            av_packet_unref(&pkt);
            DEBUG_E("Error muxing packet.");
            return -1;
        }
        av_packet_unref(&pkt);
        //printf("Write %8d frames to output file\n",frame_index);
    }

    return 0;
}

/*
* Comments: 追加一帧H264视频数据到混流器
* Param aBytes: 要追加的字节数据
* Param aSize: 追加的字节数
* @Return 成功与否
*/
bool MP4Muxer::AppendVideo(uint8_t* aBytes, int aSize)
{
    std::lock_guard<std::mutex> lockGuard(this->mLock);

    if (aBytes == NULL || aSize < 1)
    {
        return false;
    }

    if (!Enqueue(this->mVideoQueue, aBytes, aSize))
    {
        return false;
    }

    //DEBUG_D("Start video mIsStartVideo[%d]\n", mIsStartVideo);
    if (!this->mIsStartVideo)
    {
        if (OpenVideo() < 0)
        {
            return false;
        }
        //DEBUG_D("111 Start video \n");
        this->mIsStartVideo = true;
    }
    else
    {
        WriteVideoFrame();
    }

    return true;
}

/*
* Comments: 打开H264视频流
* Param : None
* @Return 0成功, 其他失败
*/
int MP4Muxer::OpenVideo(void)
{
    int result;
    AVIOContext* pb = NULL;
    AVInputFormat* inputFrmt = NULL;

//  if (mInputVideoContext)
//  {
//      // 重新申请
//      avio_context_free(&mInputVideoContext->pb);
//      avformat_close_input(&mInputVideoContext);
//      mInputVideoContext = NULL;
//      mVideoBuffer = NULL;
//  }

    if (mInputVideoContext == NULL)
    {
        if (mVideoBuffer == NULL)
        {
            // 申请视频数据缓冲区
            mVideoBuffer = (uint8_t*)av_malloc(VIDEO_BUFFER_SIZE);
            if (mVideoBuffer == NULL)
            {
                return -1;
            }
        }

        AVDictionary *dic = NULL;
        int ret = av_dict_set(&dic, "bufsize", "1024000", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  bufsize fail ";
            return -1;
        }

        ret = av_dict_set(&dic, "stimeout", "2000000", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  stimeout fail ";
            return -1;
        }

        ret = av_dict_set(&dic, "max_delay", "500000", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  max_delay fail ";
            return -1;
        }

        /* 设置为TCP方式传输 */
        ret = av_dict_set(&dic, "rtsp_transport", "tcp", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  rtsp_transport fail ";
            return -1;
        }
        

        ret = av_dict_set(&dic, "tune", "zerolatency", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  tune fail ";
            return -1;
        }

        // 设置FFMPEG的读取回调函数, 通过回调方式读取内存数据
        
#if (ENABLE_MULTICAST_QUEUE == 0)
        pb = avio_alloc_context(mVideoBuffer, VIDEO_BUFFER_SIZE, 0, (void *)this->mVideoQueue, ReadBytesCallback, NULL, NULL);
#else
        pb = avio_alloc_context(mVideoBuffer, VIDEO_BUFFER_SIZE, 0, (void *)this->mVideoCB.ptr, mVideoCB.read_buffer, NULL, NULL);
#endif 

        mInputVideoContext = avformat_alloc_context();
        if (mInputVideoContext == NULL)
        {
            // 失败则释放上下文
            avio_context_free(&pb);
            mVideoBuffer = NULL;
            DEBUG_E("Allock video format context fail.");
            return -1;
        }
        mInputVideoContext->pb = pb;

        DEBUG_D("============= avformat_alloc_context video context success =============");
    }

    //if (this->mVideoQueue->EnqueueIndex - this->mVideoQueue->DequeueIndex < 10)
    //{
        // 等待音频帧
    //    return -1;
    //}

    if ((result = av_probe_input_buffer(mInputVideoContext->pb, &inputFrmt, "", NULL, 0, 0)) < 0)
    {
        DEBUG_E("Probe video context fail: -%08X.", -result);
        return result;
    }

    if (strcmp(inputFrmt->name, "h264") != 0)
    {
        DEBUG_E("Error input video format name: %s.", inputFrmt->name);
        return -1;
    }

    DEBUG_E("================ input video format name: %s. ================", inputFrmt->name);
    
    // 打开输入流
    if ((result = avformat_open_input(&mInputVideoContext, "", inputFrmt, NULL)) != 0)
    {
        if (!mInputVideoContext)
        {
            mVideoBuffer = NULL;
            if (pb)
            {
                avio_context_free(&pb);
            }
        }
        DEBUG_E("Couldn't open input video stream -%08X.", -result);
        return result;
    }

    // 查找流信息
    if ((result = avformat_find_stream_info(mInputVideoContext, NULL)) < 0)
    {
        DEBUG_E("Failed to retrieve input video stream information -%08X.", -result);
        return result;
    }

    // 遍历输入媒体各个流格式
    for (unsigned int i = 0; i < mInputVideoContext->nb_streams; i++)
    {
        AVStream* input_stream = mInputVideoContext->streams[i];
        if (input_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (input_stream->codec == NULL)
            {
                DEBUG_E("Input video codec is null.");
                return -2;
            }
            if (input_stream->codec->coded_height < 1
                || input_stream->codec->coded_width < 1)
            {
                // 视频尺寸无效
                DEBUG_E("Input video size invalid: %d, %d.",
                    input_stream->codec->coded_width, input_stream->codec->coded_height);
//              input_stream->codec->width = input_stream->codec->coded_width = 352;
//              input_stream->codec->height = input_stream->codec->coded_height = 288;
                return -2;
            }
            mVideoIndex = i;
            AVStream* output_stream = avformat_new_stream(this->mOutputFrmtContext, input_stream->codec->codec);
            if (!output_stream)
            {
                DEBUG_E("Create new video stream format failed!");
                return -2;
            }

            mVideoOutIndex = output_stream->index;
            if ((result = avcodec_parameters_to_context(output_stream->codec, input_stream->codecpar)) < 0)
            {
                DEBUG_E("Failed to copy video context from input to output stream codec context: -%08X!", -result);
                return result;
            }

            result = avcodec_parameters_from_context(output_stream->codecpar, output_stream->codec);
            if (result < 0)
            {
                DEBUG_E ("Failed to copy context from input to output stream codec context\n");
                return result;
            }
        
            output_stream->codec->codec_tag    = 0;
            output_stream->codecpar->codec_tag = 0;
			/* 此处帧率必须和输入源帧率一致，否则音视频不同步 */
            int frame_rate = 20;
            input_stream->avg_frame_rate.num  = frame_rate;
            input_stream->avg_frame_rate.den  = 1;
            input_stream->r_frame_rate.num    = frame_rate;
            input_stream->r_frame_rate.den    = 1;
            
            output_stream->avg_frame_rate = input_stream->avg_frame_rate;
            
            PrintRation(input_stream->avg_frame_rate,"input_stream->avg_frame_rate");
            PrintRation(input_stream->r_frame_rate,  "input_stream->r_frame_rate");
            PrintRation(input_stream->time_base,     "input_stream->time_base");
            
            output_stream->r_frame_rate   = input_stream->r_frame_rate;
            output_stream->time_base      = input_stream->time_base;
            
            if (this->mOutputFrmtContext->oformat->flags & AVFMT_GLOBALHEADER)
            {
                output_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }
            break;
        }
    }

    av_dump_format(mInputVideoContext, 0, inputFrmt->name, 0);
    av_dump_format(mOutputFrmtContext, 0, mFileName.c_str(), 1);
    
    if (!this->mIsStartAudio)
    {
        // 没有等待音频帧则填充默认的音频帧
        //Enqueue(this->mAudioQueue, (uint8_t *)AAC_MUTE_DATA, sizeof(AAC_MUTE_DATA));
        if(OpenAudio(true) == 0)
        {
            this->mIsStartAudio = true;
        }
    }

    // 根据需要打开输出文件
    if (!(this->mOutputFrmtContext->oformat->flags & AVFMT_NOFILE))
    {
        if ((result = avio_open(&this->mOutputFrmtContext->pb, this->mFileName.c_str(), AVIO_FLAG_WRITE)) < 0)
        {
            DEBUG_E("Can't open file: %s: -%08X.", this->mFileName.c_str(), -result);
            return result;
        }
    }

    // 写入文件头
    if ((result = avformat_write_header(this->mOutputFrmtContext, NULL)) < 0)
    {
        DEBUG_E("Error occurred when opening output file: -%08X!", -result);
        return result;
    }

    return 0;
}

/*
* Comments: 写入AAC数据帧到文件
* Param : None
* @Return void
*/
int MP4Muxer::WriteAudioFrame(void)
{
#if ENABLE_MULTICAST_QUEUE
    if (!this->mIsStartAudio)
    {
        if (OpenAudio(false) < 0)
        {
            return -1;
        }
        this->mIsStartAudio = true;
    }

    if (!this->mIsStartVideo)  /* 视频开始后才能有音频 */
    {
        return -1;
    }
#endif

    int result = -1;
    AVStream *in_stream, *out_stream;
    // 流媒体数据包
    AVPacket pkt;

    //while (1)
    {
        AVStream *in_stream = NULL, *out_stream = NULL;
        av_init_packet(&pkt);
        
        // 获取一个数据包
        result = av_read_frame(mInputAudioContext, &pkt);
        //DEBUG_V("Read Audioframe[%d] ret=-%08X.", mAudioQueue->DequeueIndex, -result);
        if (result < 0)
        {
            return result;
        }
        
        in_stream = mInputAudioContext->streams[mAudioIndex];
        out_stream = mOutputFrmtContext->streams[mAudioOutIndex];

        //DEBUG_D("------ audio pts[%lld] dts[%lld] AV_NOPTS_VALUE[%lld]----\n", pkt.pts, pkt.dts, AV_NOPTS_VALUE);
        // 如果没有显示时间戳自己加上时间戳并且将显示时间戳赋值给解码时间戳
        if (pkt.pts == AV_NOPTS_VALUE)
        {
            //Write PTS
            AVRational time_base1 = in_stream->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
            //Parameters
            pkt.pts = (double)(mFrameIndex * calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            mFrameIndex++;
        }

        //DEBUG_D("in_stream->time_base.num[%d] den[%d] ", in_stream->time_base.num, in_stream->time_base.den);
        //DEBUG_D("------ audio pts[%lld] dts[%lld] AV_NOPTS_VALUE[%lld]----\n", pkt.pts, pkt.dts, AV_NOPTS_VALUE);
        
        //av_bitstream_filter_filter(mAudioFilter, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);

        // Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.stream_index = mAudioOutIndex;
        pkt.pos = -1;
        // Write
        if (av_interleaved_write_frame(mOutputFrmtContext, &pkt) < 0)
        {
            av_packet_unref(&pkt);
            printf("Error muxing packet\n");
            return -1;
            //break;
        }
        av_packet_unref(&pkt);
        //printf("Write %8d frames to output file\n",frame_index);
    }

    return 0;
}

/*
* Comments: 追加一帧AAC数据到混流器
* Param aBytes: 要追加的字节数据
* Param aSize: 追加的字节数
* @Return 成功与否
*/
bool MP4Muxer::AppendAudio(uint8_t* aBytes, int aSize)
{
    std::lock_guard<std::mutex> lockGuard(this->mLock);

    if (aBytes == NULL || aSize < 1)
    {
        return false;
    }

    if (!Enqueue(this->mAudioQueue, aBytes, aSize))
    {
        return false;
    }

    if (!this->mIsStartAudio)
    {
        if (OpenAudio(false) < 0)
        {
            return false;
        }
        this->mIsStartAudio = true;
    }
    else if (this->mIsStartVideo)  /* 视频开始后才能有音频 */
    {
        WriteAudioFrame();
    }

    return true;
}

static void make_dsi(unsigned int sampling_frequency_index, unsigned int channel_configuration, unsigned char* dsi)
{
    unsigned int object_type = 2; /* AAC LC object type */
    dsi[0] = (object_type<<3) | (sampling_frequency_index>>1);
    dsi[1] = ((sampling_frequency_index&1)<<7) | (channel_configuration<<3);
}

/*
* Comments: 打开AAC输入上下文
* Param : None
* @Return 0成功, 其他失败
*/
int MP4Muxer::OpenAudio(bool aNew)
{
    int result;
    AVIOContext* pb = NULL;
    AVInputFormat* inputFrmt = NULL;

    if (aNew && mInputAudioContext)
    {
        // 重新申请
        avio_context_free(&mInputAudioContext->pb);
        avformat_close_input(&mInputAudioContext);
        mInputAudioContext = NULL;
        mAudioBuffer = NULL;
    }

    if (mInputAudioContext == NULL)
    {
        if (mAudioBuffer == NULL)
        {
            // 申请音频数据缓冲区
            mAudioBuffer = (uint8_t*)av_malloc(AUDIO_BUFFER_SIZE);
            if (mAudioBuffer == NULL)
            {
                return -1;
            }
        }

        AVDictionary *dic = NULL;
        int ret = av_dict_set(&dic, "bufsize", "10240", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  bufsize fail ";
            return -1;
        }

        ret = av_dict_set(&dic, "stimeout", "2000000", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  stimeout fail ";
            return -1;
        }

        ret = av_dict_set(&dic, "max_delay", "500000", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  max_delay fail ";
            return -1;
        }

        /* 设置为TCP方式传输 */
        ret = av_dict_set(&dic, "rtsp_transport", "tcp", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  rtsp_transport fail ";
            return -1;
        }
        

        ret = av_dict_set(&dic, "tune", "zerolatency", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  tune fail ";
            return -1;
        }

        // 设置FFMPEG的读取回调函数, 通过回调方式读取内存数据
#if (ENABLE_MULTICAST_QUEUE == 0)
        pb = avio_alloc_context(mAudioBuffer, AUDIO_BUFFER_SIZE, 0, (void *)this->mAudioQueue, ReadBytesCallback, NULL, NULL);
#else
        pb = avio_alloc_context(mAudioBuffer, AUDIO_BUFFER_SIZE, 0, (void *)this->mAudioCB.ptr, mAudioCB.read_buffer, NULL, NULL);
#endif
        mInputAudioContext = avformat_alloc_context();
        if (mInputAudioContext == NULL)
        {
            // 失败则释放上下文
            avio_context_free(&pb);
            mAudioBuffer = NULL;
            DEBUG_E("Alloc audio format context fail.");
            return -1;
        }
        mInputAudioContext->pb = pb;

        DEBUG_D("============= avformat_alloc_context audio context success =============");
    }

//  inputFrmt = av_find_input_format("aac");
//  if (inputFrmt == NULL)
//  {
//      DEBUG_E("Find aac input format fail.");
//      return -1;
//  }

    if ((result = av_probe_input_buffer(mInputAudioContext->pb, &inputFrmt, "", NULL, 0, 0)) < 0)
    {
        DEBUG_E("Probe audio context fail: -%08X.", -result);
        return result;
    }

    if (strcmp(inputFrmt->name, "aac") != 0)
    {
        DEBUG_E("Error input audio format name: %s.", inputFrmt->name);
        return -1;
    }

    DEBUG_D("input audio format name: %s.", inputFrmt->name);
    
    // 打开输入流
    if ((result = avformat_open_input(&mInputAudioContext, "", inputFrmt, NULL)) != 0)
    {
        if (!mInputAudioContext)
        {
            mAudioBuffer = NULL;
            if (pb)
            {
                avio_context_free(&pb);
            }
        }
        DEBUG_E("Couldn't open input audio stream -%08X.", -result);
        return result;
    }

    // 查找流信息
    if ((result = avformat_find_stream_info(mInputAudioContext, NULL)) < 0)
    {
        DEBUG_E("Failed to retrieve input audio stream information -%08X.", -result);
        return result;
    }

    // 遍历输入媒体各个流格式
    for (unsigned int i = 0; i < mInputAudioContext->nb_streams; i++)
    {
        AVStream* input_stream = mInputAudioContext->streams[i];
        if (input_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            mAudioIndex = i;
            AVStream* output_stream = avformat_new_stream(this->mOutputFrmtContext, input_stream->codec->codec);
            if (!output_stream)
            {
                DEBUG_E("Create new audio stream format failed!");
                return -2;
            }

            mAudioOutIndex = output_stream->index;
            if ((result = avcodec_parameters_to_context(output_stream->codec, input_stream->codecpar)) < 0)
            {
                DEBUG_E("Failed to copy audio context from input to output stream codec context: -%08X!", -result);
                return result;
            }
            
            result = avcodec_parameters_from_context(output_stream->codecpar, output_stream->codec);
            if (result < 0)
            {
                DEBUG_E ("Failed to copy context from input to output stream codec context\n");
                return result;
            }

             DEBUG_D("============ codec->bits_per_raw_sample[%d], codecpar->bits_per_raw_sample[%d] =========" ,
                        output_stream->codec->bits_per_raw_sample, output_stream->codecpar->bits_per_raw_sample);
            
             unsigned char dsi1[2]={0};
            //unsigned int sampling_frequency_index = (unsigned int) get_sr_index((unsigned int)output_stream->codec->sample_rate);
            unsigned int sampling_frequency_index = 3;   /* 48khz  */
            make_dsi(sampling_frequency_index, (unsigned int)output_stream->codec->channels, dsi1);

            output_stream->codec->extradata = (uint8_t*) av_malloc(2);
            output_stream->codec->extradata_size = 2;
            output_stream->codec->codec_tag = 0;
            memcpy(output_stream->codec->extradata, dsi1, 2);
            output_stream->codec->sample_rate = 48000;
            
            if (this->mOutputFrmtContext->oformat->flags & AVFMT_GLOBALHEADER)
            {
                output_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            output_stream->codecpar->extradata      = (uint8_t*) av_malloc(2);
            output_stream->codecpar->extradata_size = 2;
            output_stream->codecpar->codec_tag      = 0;
            memcpy(output_stream->codecpar->extradata, dsi1, 2);
            output_stream->codecpar->sample_rate = 48000;

            break;
        }
    }

    av_dump_format(mOutputFrmtContext, 0, mFileName.c_str(), 1);
    
    return  mAudioIndex >= 0 ? 0 : -1;
}

int MP4Muxer::ReadVideoFrameCallback(void *opaque, uint8_t *buf, int buf_size)
{
    if(!buf || !opaque)
    {
        return 0;
    }

    MP4Muxer* handler = (MP4Muxer*)opaque;
    RecvMulticastDataModule* MultiCastModule = handler->mMultiCastModule;
    MultiFrameInfo video;
    MultiCastModule->GetVideoFrame(video);
    if(video.buf && video.buf_size>=0)
    {
        int size = FFMIN(buf_size, video.buf_size);

        memcpy(buf, video.buf, size);
        
#if 0
        {
            FILE* vfd = fopen("rtsp.264","ab+");
            if(vfd)
            {
                fwrite(video.buf, 1, video.buf_size, vfd);
                fclose(vfd);
            }
        }
#endif
        if(handler->mIsStartVideo && video.buf_size <= buf_size)
        {
            MultiCastModule->SetCurrentVideoPTS(video.timeStamp);
            //DEBUG_D("---------  video.timeStamp[%llu] --------", video.timeStamp);
#if 0       
            FILE* wfd = fopen("video_time.log", "ab+");
            if(wfd)
            {
                char tmp[256]={0};
                memset(tmp, 0, 256);
                
                sprintf(tmp,"video.timeStamp = [ %llu ] \n", video.timeStamp);
                fwrite(tmp,1,strlen(tmp), wfd);
                fclose(wfd);
            }
#endif
        }
        
        delete[] video.buf;
        video.buf = NULL;
        
        return size;
    }

    return 0;
}

int MP4Muxer::ReadAudioFrameCallback(void *opaque, uint8_t *buf, int buf_size)
{
    if(!buf || !opaque)
    {
        return 0;
    }
    
    MP4Muxer* handler = (MP4Muxer*)opaque;
    RecvMulticastDataModule* MultiCastModule = handler->mMultiCastModule;
    
    MultiFrameInfo audio;
    MultiCastModule->GetAudioFrame(audio);
    if(audio.buf && audio.buf_size>=0)
    {
        int size = FFMIN(buf_size, audio.buf_size);
        memcpy(buf, audio.buf, size);

        if(handler->mIsStartAudio && audio.buf_size <= buf_size)
        {
            MultiCastModule->SetCurrentAudioPTS(audio.timeStamp);
            //DEBUG_D("---------  audio.timeStamp[%llu] --------", audio.timeStamp);
#if 1       
            FILE* wfd = fopen("audio_time.log", "ab+");
            if(wfd)
            {
                char tmp[256]={0};
                memset(tmp, 0, 256);
                
                sprintf(tmp,"audio.timeStamp = [ %llu ] \n", audio.timeStamp);
                fwrite(tmp,1,strlen(tmp), wfd);
                fclose(wfd);
            }
#endif
        }
        
        delete[] audio.buf;
        audio.buf = NULL;
        
        return size;
    }

    return 0;
}

void MP4Muxer::SetVideoCB(FFMPEG_READ_PACK_CB& videoCB)
{
    mVideoCB = videoCB;
}

void MP4Muxer::SetAudioCB(FFMPEG_READ_PACK_CB& audioCB)
{
    mAudioCB = audioCB;
}


