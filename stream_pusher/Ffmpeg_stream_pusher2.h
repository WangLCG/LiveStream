////////////////////////////////////////////////////////////////////////
/// @file       ffmpeg_stream_pusher2.h
/// @brief      通过主动填充ffmpeg的AVPacket来实现推rtsp/rtmp流的声明
/// @details    通过主动填充ffmpeg的AVPacket来实现推rtsp/rtmp流的声明
/// @author     王超
/// @version    1.0
/// @date       2020/01/06
/// @copyright  (c) 1993-2020 。保留所有权利
////////////////////////////////////////////////////////////////////////

#ifndef FFMPEG_STREAM_PUSHER_2_H_
#define FFMPEG_STREAM_PUSHER_2_H_

#include <thread>
#include <string>
#include <mutex>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "RecvMulticastDataModule.h"

using namespace std;

/* 48 KHZ */
#define AUDIO_SAMPLE  (48000)
#define FRAME_SAMPLE  (1024)
#define USE_AACBSF    (1)

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif

/* 视频流类型 */
typedef enum
{
    CUS_H264_VIDEO_STREAM = 0X00,
    CUS_H265_VIDEO_STREAM = 0X01
}CUS_VIDEO_STREAM_TYPE;

static AVRational s_refer_time_base={
    s_refer_time_base.num  = 1,
    s_refer_time_base.den = 1000// 1000ms,1s
};
    
static AVRational s_audio_time_base={
    s_audio_time_base.num  = 1,
    s_audio_time_base.den = AUDIO_SAMPLE
};

class FillPKGStreamPusher
{
public:
    FillPKGStreamPusher(int16_t framerate, const char* stream_path, int8_t streamType,
        int16_t width, int16_t height, RecvMulticastDataModule* MultiCastModule);
    
    ~FillPKGStreamPusher();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           填充AVPacket结构体
    /// \details         填充AVPacket结构体
    //// \param[in]      streamIndex   输出流的索引
    //// \param[in]      frame         帧信息
    //// \param[in/out]      pkt   待填充
    /// \return  成功 -- 0，失败 -- -1
    //////////////////////////////////////////////////////////////////////////
    bool FillAvPacket(int streamIndex, MultiFrameInfo& frame,AVPacket& pkt);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           写AVPacket数据到输出
    /// \details         写AVPacket数据到输出
    //// \param[in]      streamIndex   输出流的索引
    //// \param[in]      pkt   数据
    /// \return  成功 -- 0，失败 -- -1
    //////////////////////////////////////////////////////////////////////////
    bool writeFileAvPacket(int streamIndex, AVPacket& pkt);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           创建输出流
    /// \details         创建输出流
    //// \param[in/out]  pOutContext   输出流的上下文
    //// \param[in]      buf           extra_data（sps数据）
    //// \param[in]      spsSize       extra_data的大小
    /// \return  成功 -- 0，失败 -- -1
    //////////////////////////////////////////////////////////////////////////
    bool CreateOutput(AVFormatContext *&pOutContext, unsigned char* buf,int spsSize);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           添加流类型
    /// \details         添加流类型
    //// \param[in/out]  pOutContext   输出流的上下文
    //// \param[in]      codec         编码器上下文
    //// \param[in]      codec_id      编码ID
    //// \param[in]      extra_data    extra_data（sps数据）
    //// \param[in]      size          extra_data的大小
    /// \return  成功 -- 实例，失败 -- NULL
    //////////////////////////////////////////////////////////////////////////
    AVStream* add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, unsigned char* extra_data, int size);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           判断h264/h265视频流的类型是否为I帧
    /// \details         判断h264/h265视频流的类型是否为I帧
    //// \param[in]          dataBuf   视频流数据
    //// \param[in/out]      spssize   sps及pps大小
    /// \return  TRUE -- 是I帧，false -- 不是I帧
    //////////////////////////////////////////////////////////////////////////
    bool judgeIframe(unsigned char* dataBuf, int buf_size, int& spssize);

    int GetVideoOutStreamIndex(){return m_videoindex_out;}
    int GetAudioOutStreamIndex(){return m_audioindex_out;}
    
    unsigned long long int GetCurrentVideoPTS(){return m_cur_pts_v;}
    unsigned long long int GetCurrentAudioPTS(){return m_cur_pts_a;}
    
private:
    int16_t m_videoWidth;
    int16_t m_videoHeight;
    
    char  m_StreamPath[256];
    int16_t m_frameRate;

    unsigned char m_StreamType;  /* @see CUS_VIDEO_STREAM_TYPE */
    
    // 写文件相关
    AVFormatContext *m_o_fmt_ctx ;
    AVStream *m_out_stream ;
    AVPacket m_pkt;

    int64_t  last_pts_v , last_pts_a, tmp_errortime;//打印时间测试用

    int64_t m_i_begin_timeStamp ;//文件起始时间戳
    int64_t m_cur_pts_v, m_cur_pts_a;//当前写到的音视频时间戳

    int m_videoindex_out;
    int m_audioindex_out;
    
    int64_t m_video_duration;

    bool m_first;
    RecvMulticastDataModule* m_MultiCastModule;

    AVBSFContext *m_bsf_ctx;
    const AVBitStreamFilter *m_filter;
};

#endif
