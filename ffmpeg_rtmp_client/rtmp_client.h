////////////////////////////////////////////////////////////////////////
/// @file       rtmp_client.h
/// @brief      基于ffmpeg的rtmp客户端声明 
/// @details    基于ffmpeg的rtmp客户端声明
/// @author     
/// @version    0.1
/// @date       2020/01/14
/// @copyright  (c) 1993-2020
////////////////////////////////////////////////////////////////////////
#ifndef FF_RTMP_CLIENT_H_
#define FF_RTMP_CLIENT_H_ 

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <queue>
#include <pthread.h>
#include <mutex>

using namespace std;

/* aac 的adts头部信息结构体 */
typedef struct ADTSContext
{
      int write_adts;
      int objecttype;
      int sample_rate_index;
      int channel_conf;
}ADTSContext; 

typedef struct FrameInfo
{
    char* data;
    unsigned int data_size;   /* 数据大小 */
    int64_t time_stamp;   /* 时间戳 */

}FrameInfo;

class ff_rtmp_client
{
public:
    //////////////////////////////////////////////////////////////////////////
    /// \brief           构造函数
    /// \details         构造函数
    //// \param[in]      url        rtmp地址 
    //////////////////////////////////////////////////////////////////////////
    ff_rtmp_client(const char* url = NULL);

    ~ff_rtmp_client();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           连接rtmp地址并进行必要的音视频数据参数探测
    /// \details         连接rtmp地址并进行必要的音视频数据参数探测
    //// \param[in]      url        rtmp地址 
    /// \return  成功 -- 0，失败 -- 其他数值
    //////////////////////////////////////////////////////////////////////////
    int rtmp_pull_open(const char* url = NULL);

    static void* rtmp_main_loop(void* ptr);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           循环接收rtmp数据处理函数
    /// \details         循环接收rtmp数据处理函数
    //////////////////////////////////////////////////////////////////////////
    void process_func();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           创建循环接收rtmp数据处理线程
    /// \details         创建循环接收rtmp数据处理线程
    /// \return  成功 -- true，失败 -- false
    //////////////////////////////////////////////////////////////////////////
    bool start_process();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           停止循环接收rtmp数据处理线程
    /// \details         停止循环接收rtmp数据处理线程
    //////////////////////////////////////////////////////////////////////////
    void stop_process();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           获取一帧视频数据
    /// \details         获取一帧视频数据
    //// \param[in/out]      video      视频数据
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void get_video_frame(FrameInfo& video);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           获取一帧音频频数据
    /// \details         获取一帧音频数据
    //// \param[in/out]      audio      音频数据
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void get_audio_frame(FrameInfo& audio);

private:
    //////////////////////////////////////////////////////////////////////////
    /// \brief           音频数据加入队列
    /// \details         音频数据加入队列
    //// \param[in]      audio     音频数据
    //////////////////////////////////////////////////////////////////////////
    void push_aduio(FrameInfo& audio);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           视频数据加入队列
    /// \details         视频数据加入队列
    //// \param[in]      video     视频数据
    //////////////////////////////////////////////////////////////////////////
    void push_video(FrameInfo& video);
    
    //////////////////////////////////////////////////////////////////////////
    /// \brief           音频或视频添加头部信息流程
    /// \details         音频或视频添加头部信息流程
    //// \param[in]      pkt              数据包
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void bsf_process( AVPacket& pkt);

    pthread_t m_processing_thread;

    bool m_IsProcess;
    char m_RtmpUrl[256];      /* rtmp url地址 */
    int m_VideoStreamIndex ;  /* 视频流索引 */
    int m_AudioStreamIndex;  /* 音频流索引 */

    mutex  m_AudioMutex;
    mutex  m_VideoMutex;

    queue<FrameInfo> m_AudioQueue;
    queue<FrameInfo> m_VideoQueue;
    
    AVFormatContext *m_ImfCtx;  /* 输入流上下文 */
    AVCodecContext *m_AudioCodeCtx;  /* 音频解码参数上下文 */

    AVBitStreamFilter *m_VideoAbsFilter;  /* 视频过滤器 */
    AVBSFContext *m_VideoAbsCtx;         /* 视频过滤器上下文 */

    //AVCodecContext *m_VideoCodecCtx; 
};

#endif