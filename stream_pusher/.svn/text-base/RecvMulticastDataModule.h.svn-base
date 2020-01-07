////////////////////////////////////////////////////////////////////////
/// @file       RecvMulticastDataModule.h
/// @brief      接收盒子音视频组播数据的声明
/// @details    接收盒子音视频组播数据的声明
/// @author     王超
/// @version    1.0
/// @date       2019/12/20
/// @copyright  (c) 1993-2020 。保留所有权利
////////////////////////////////////////////////////////////////////////

#ifndef RECV_MULTICAST_DATA_MODULE_H_
#define RECV_MULTICAST_DATA_MODULE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <queue>
#include <iostream>
#include <map>
#include <string>
#include <stdlib.h>
#include <list>
#include <functional>
#include "H264Tools.h"

#define PACKET_MAX_SIZE  (1024*9) //9k
#define DIMIS_DATA
#define MSG_FLAG          (0xfc)
#define VTM_HEADER_LEN    (24)
#define AUDIO_FRAME_SIZE  (1024*2)

#define MAX_VIDEO_FRAME_SIZE  (2*1024*1024)

using namespace std;

/* 组播数据头 */
typedef struct VDataMsgHeader 
{
    unsigned char flag;/* Message Flag: 0xfc */
    unsigned char encChanId:5,//=1f,IPcamera NO ENC cube
              type:3;// Message Type (0:Video 1:Audio)
    unsigned short length;/* Total Length */
    unsigned int seconds;
    unsigned int nanoseconds;
    unsigned short sequence_id;/* Sequence ID */
    unsigned short fragment;
    unsigned int srcIPWidthHeight;
    unsigned int destination;         /* Destination ip */
} VDataMsgHeader;

/* 组播数据流类型 */
typedef enum
{
    STREAM_VIDEO = 0X00,
    STREAM_AUDIO = 0X01
}MULLTICAST_STREAM_TYPE;

/* 接收到的组播数据帧信息 */
typedef struct MultiFrameInfo
{
    char* buf;  /* 音频或视频数据 */
    unsigned short sequence_id; /* 帧序列号 */
    unsigned int buf_size;  /* 音频或视频数据 */
    unsigned long long timeStamp; /* 时间戳 */
}MultiFrameInfo;

class RecvMulticastDataModule
{
public:
    RecvMulticastDataModule();
    ~RecvMulticastDataModule();
    
    //////////////////////////////////////////////////////////////////////////
    /// \brief           获取音频队列中的一帧
    /// \details         获取音频队列中的一帧
    //// \param[out]      audio   音频帧
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void GetAudioFrame(MultiFrameInfo& audio);
    
    //////////////////////////////////////////////////////////////////////////
    /// \brief           获取视频队列中的一帧
    /// \details         获取视频队列中的一帧
    //// \param[out]      video   音频帧
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void GetVideoFrame(MultiFrameInfo& video);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           加入组播组
    /// \details         加入组播组
    //// \param[in]      mult_addr        组播地址 
    //// \param[in]       style   数据类型@see MULLTICAST_STREAM_TYPE
    /// \return  成功 -- 0，失败 -- -1
    //////////////////////////////////////////////////////////////////////////
    int AddMultGroup(const char * mult_addr, int style);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           设置调用live555的视频的doGetNextFrame的回调函数
    /// \details         设置调用live555的视频的doGetNextFrame的回调函数
    //// \param[in]      func 回调函数 
    /// \return  成功 -- 0，失败 -- -1
    //////////////////////////////////////////////////////////////////////////
    void setCallbackFunctionFrameIsReady(std::function<void()> func);

    void SetCurrentVideoPTS(unsigned long long int& time);
    void SetCurrentAudioPTS(unsigned long long int& time);

    unsigned long long int GetCurrentVideoPTS(){return m_cur_pts_v;}
    unsigned long long int GetCurrentAudioPTS(){return m_cur_pts_a;}

    size_t GetAudioQueueSize(){ return m_AudioMutilQueue.size();}

    unsigned long long GetBeginVideoFrameTime() {return m_i_begin_timeStamp;}
    
    int m_VideoFrameRate;          /* 视频帧率 */
    
private:
    static void *audioLoop(void *ptr);
    void audioLoopFunc();

    static void *videoLoop(void *ptr);
    void videoLoopFunc();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           初始化音频socket
    /// \details         初始化音频socket
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void InitAudioFd();
    
    //////////////////////////////////////////////////////////////////////////
    /// \brief           初始化视频socket
    /// \details         初始化视频socket
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void InitVedioFd();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           离开组播组
    /// \details         离开组播组
    //// \param[in]      mult_addr        组播地址 
    //// \param[in]       style   数据类型@see MULLTICAST_STREAM_TYPE
    /// \return  成功 -- 0，失败 -- -1
    //////////////////////////////////////////////////////////////////////////
    int QuitMultGroup(const char * mult_addr, int style);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           分析获取到的组播数据头
    /// \details         分析获取到的组播数据头，并输出到外部模块
    //// \param[in]      dataBuf   数据
    //// \param[in]      dataLen   数据大小
    /// \return  成功 -- 0，失败 -- -1
    //////////////////////////////////////////////////////////////////////////
    int analyseHead(unsigned char* dataBuf, unsigned int dataLen);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           分析获取到的组播数据的时间戳
    /// \details         分析获取到的组播数据的时间戳
    //// \param[in]          header   数据
    //// \param[in/out]      timeStamp   获取到的时间戳
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void analyseTime(VDataMsgHeader* header, unsigned long long& timeStamp);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           分析获取到的组播视频数据的宽和高
    /// \details         分析获取到的组播视频数据的宽和高
    //// \param[in]          data   [VDataMsgHeader.srcIPWidthHeight]
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void analyseIpResolution(unsigned int data);
        
    //////////////////////////////////////////////////////////////////////////
    /// \brief           转换网络字节序
    /// \details         转换网络字节序
    //// \param[in]      header   待转化数据
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void msgNtoh(VDataMsgHeader* header);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           将接收到的音频数据存入队列
    /// \details         将接收到的音频数据存入队列
    //// \param[in]      audio   音频帧信息
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void PushAudioFrame(MultiFrameInfo& audio);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           将接收到的视频数据存入队列
    /// \details         将接收到的视频数据存入队列
    //// \param[in]      video   视频帧信息
    /// \return  
    //////////////////////////////////////////////////////////////////////////
    void PushVideoFrame(MultiFrameInfo& video);

    pthread_t m_audioLoopId, m_videoLoopId;
    bool m_StopAudioThread, m_StopVideoThread;  /* 线程退出标志，ture--退出 */

    unsigned long long m_i_begin_timeStamp ;/* 第一个I帧的时间戳 */
    
    unsigned long long m_TotalVideoTimeStamp;  /*  */
    unsigned long long m_PreVideoTimeStamp;  /*  */
    unsigned int m_VideoFramNum;
    
    pthread_rwlock_t m_VideoRWLock, m_AudioRWLock;   /* 读写锁，用于保护m_StopVideoThread及m_StopAudioThread */
    
    pthread_mutex_t m_AudioLock;
    pthread_mutex_t m_VideoLock;
    
    queue<MultiFrameInfo> m_AudioMutilQueue;
    queue<MultiFrameInfo> m_VideoMutilQueue;

    int m_AudioSocketFd,m_VideoSocketFd;   /* 音视频的套接字文件描述符 */

    unsigned char m_RecvAudioBur[AUDIO_FRAME_SIZE];
    unsigned char m_RecvVideoBur[PACKET_MAX_SIZE];

    map<int, MultiFrameInfo> m_TmpVideoFrameMap;  /* first--framd_id[分片ID] 被分片的视频帧临时缓存map，借用map的自动排序特性 */
    char* m_TmpVideoFrameBuf;   /* 临时缓存帧buffer */
    unsigned int m_UsedVideoFrameBufSize;   /* 已使用的m_TmpVideoFrameBuf大小 */

    std::function<void()> m_onVideoFrame;  /* 音频帧回调 */

    unsigned long long int m_cur_pts_v;   /* 视频帧的时间戳 */
    unsigned long long int m_cur_pts_a;  /*  音频帧的时间戳 */
    bool m_GotFirstIFrame;
};

#endif
