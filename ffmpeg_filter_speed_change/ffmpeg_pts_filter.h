////////////////////////////////////////////////////////////////////////
/// @file       ffmpeg_pts_filter.h
/// @brief      通过avfilter来修改视频的播放速度的声明
/// @details    通过avfilter来修改视频的播放速度的声明
/// @author     
/// @version    1.0
/// @date       2020/01/06
/// @copyright  (c) 2020-2030 。保留所有权利
////////////////////////////////////////////////////////////////////////

#ifndef FFMPEG_PTS_FILTER_H_
#define FFMPEG_PTS_FILTER_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include "libavutil/file.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavformat/avio.h"

#ifdef __cplusplus
};
#endif 

#include <queue>
#include <mutex>
#include <string>
using namespace std;

class ffmpeg_pts_filter
{

public:
    //////////////////////////////////////////////////////////////////////////
    /// \brief           构造函数
    /// \details         构造函数
    //// \param[in]      filter_descr   过滤器字符串描述
    //// \param[in]      filename         待处理文件
    //////////////////////////////////////////////////////////////////////////
    ffmpeg_pts_filter(const char *filter_descr = NULL, const char *filename = NULL);

    ~ffmpeg_pts_filter();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           打开音视频文件
    /// \details         打开音视频文件
    /// \return  成功 -- 0
    //////////////////////////////////////////////////////////////////////////
    int open_input_file();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           初始化过滤器
    /// \details         初始化过滤器
    /// \return  成功 -- 0
    //////////////////////////////////////////////////////////////////////////
    int init_video_filters();
    
    //////////////////////////////////////////////////////////////////////////
    /// \brief           刷新编码器，在编码结束时使用
    /// \details         刷新编码器，在编码结束时使用
    //// \param[in]      fmt_ctx
    //// \param[in]      stream_index         流索引，用于有多个流输出时
    /// \return  成功 -- 0，失败 -- -1
    //////////////////////////////////////////////////////////////////////////
    int flush_encoder(AVFormatContext *fmt_ctx, int stream_index) ;

    //////////////////////////////////////////////////////////////////////////
    /// \brief           初始化视频编码器
    /// \details         初始化视频编码器
    /// \return  成功 -- 0
    //////////////////////////////////////////////////////////////////////////
    int init_video_enc();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           进行视频编码操作
    /// \details         进行视频编码操作
    //// \param[in]      in_frame   待编码原始数据
    /// \return  成功 -- 0
    //////////////////////////////////////////////////////////////////////////
    int run_video_encode(AVFrame *in_frame);

    //////////////////////////////////////////////////////////////////////////
    /// \brief           进行音视频解码操作
    /// \details         进行音视频解码操作
    //// \param[in]      in_frame   待编码原始数据
    /// \return  成功 -- 0
    //////////////////////////////////////////////////////////////////////////
    int run_decode();

    //////////////////////////////////////////////////////////////////////////
    /// \brief           进行音视频播放速度操作
    /// \details         进行音视频播放速度操作
    /// \return  成功 -- 0
    //////////////////////////////////////////////////////////////////////////
    int run();

private:
    string m_filter_descr ;
    string m_filename ;

    AVFormatContext *m_fmt_ctx ;
    AVCodecContext *m_dec_ctx  ;
    AVCodecContext *m_video_enc_ctx;

    AVFilterContext *m_buffersink_ctx;
    AVFilterContext *m_buffersrc_ctx;
    AVFilterGraph *m_filter_graph;
    int m_video_stream_index ;
    int m_audio_stream_index ;

    int64_t m_last_pts;

    AVFormatContext* m_pEncFormatCtx ;  /* 视频编码器上下文 */
    AVStream* m_EncVideoStrem      ;     /* 视频流 */

};

#endif