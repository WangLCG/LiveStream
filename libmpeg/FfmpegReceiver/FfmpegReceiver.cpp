////////////////////////////////////////////////////////////////////////
/// @file       FfmpegReceiver.CPP
/// @brief      基于ffmpeg的rtmp客户端定义 
/// @details    基于ffmpeg的rtmp客户端定义
/// @author     
/// @version    0.1
/// @date       2020/01/14
/// @copyright  (c) 1993-2020
////////////////////////////////////////////////////////////////////////

#include "FfmpegReceiver.h"

//////////////////////////////////////////////////////////////////////////
/// \brief           从ffmpeg的音频的extradata中解析出adts所需信息
/// \details         从ffmpeg的音频的extradata中解析出adts所需信息
//// \param[in/out]      adts        adts所需信息 
//// \param[in]      pbuf        extradata
//// \param[in]      bufsize        extradata 大小 
//// \return  0 -- success
//////////////////////////////////////////////////////////////////////////
static int aac_decode_extradata(ADTSContext *adts, unsigned char *pbuf, int bufsize)
{
      int aot, aotext, samfreindex;
      int i, channelconfig;
      unsigned char *p = pbuf;
      if (!adts || !pbuf || bufsize<2)
      {
            return -1;
      }
      aot = (p[0]>>3)&0x1f;
      if (aot == 31)
      {
            aotext = (p[0]<<3 | (p[1]>>5)) & 0x3f;
            aot = 32 + aotext;
            samfreindex = (p[1]>>1) & 0x0f;
            if (samfreindex == 0x0f)
            {
                  channelconfig = ((p[4]<<3) | (p[5]>>5)) & 0x0f;
            }
            else
            {
                  channelconfig = ((p[1]<<3)|(p[2]>>5)) & 0x0f;
            }
      }
      else
      {
            samfreindex = ((p[0]<<1)|p[1]>>7) & 0x0f;
            if (samfreindex == 0x0f)
            {
                  channelconfig = (p[4]>>3) & 0x0f;
            }
            else
            {
                  channelconfig = (p[1]>>3) & 0x0f;
            }
      }

#ifdef AOT_PROFILE_CTRL
      if (aot < 2) aot = 2;
#endif
      adts->objecttype = aot-1;
      adts->sample_rate_index = samfreindex;
      adts->channel_conf = channelconfig;
      adts->write_adts = 1;
      return 0;
}

/* 7字节的adts头部 */
#define ADTS_HEADER_SIZE   (7)
//////////////////////////////////////////////////////////////////////////
/// \brief           从acfg结构体中生成所需要的adts头部
/// \details         从acfg结构体中生成所需要的adts头部
//// \param[in]      acfg        adts头部所需信息 
//// \param[in/out]  buf        生成的adts头部信息
//// \param[in]      size        音频avpacket的size(pkt.size) 
//// \return  0 -- success
//////////////////////////////////////////////////////////////////////////
static int aac_set_adts_head(ADTSContext *acfg, unsigned char *buf, int size)
{
      unsigned char byte = 0;
      if (size < ADTS_HEADER_SIZE)
      {
            return -1;
      }

      buf[0] = 0xff;
      buf[1] = 0xf1;
      byte = 0;
      byte |= (acfg->objecttype & 0x03) << 6;
      byte |= (acfg->sample_rate_index & 0x0f) << 2;
      byte |= (acfg->channel_conf & 0x07) >> 2;
      buf[2] = byte;
      byte = 0;
      byte |= (acfg->channel_conf & 0x07) << 6;
      byte |= (ADTS_HEADER_SIZE + size) >> 11;
      buf[3] = byte;
      byte = 0;
      byte |= (ADTS_HEADER_SIZE + size) >> 3;
      buf[4] = byte;
      byte = 0;
      byte |= ((ADTS_HEADER_SIZE + size) & 0x7) << 5;
      byte |= (0x7ff >> 6) & 0x1f;
      buf[5] = byte;
      byte = 0;
      byte |= (0x7ff & 0x3f) << 2;
      buf[6] = byte;

      return 0;
}

ff_rtmp_client::ff_rtmp_client(const char* url)
{
    memset(m_RtmpUrl, 0,256);
    if(url)
    {
        snprintf(m_RtmpUrl, 256, "%s", url);
    }

    m_VideoStreamIndex  = -1;
    m_AudioStreamIndex  = -1;
    m_ImfCtx = NULL;
    m_processing_thread = -1;

    m_AudioCodeCtx = NULL;

    m_VideoAbsFilter = NULL;
    m_VideoAbsCtx    = NULL;

    av_register_all();
    avformat_network_init();
}

ff_rtmp_client::~ff_rtmp_client()
{
    m_IsProcess = false;
    if(m_processing_thread != -1)
    {
        pthread_join(m_processing_thread, NULL);
        m_processing_thread = -1;
    }

    if(m_VideoAbsCtx)
    {
        av_bsf_free(&m_VideoAbsCtx);
        m_VideoAbsCtx = NULL;
    }

    if(m_ImfCtx)
    {
        avformat_close_input(&m_ImfCtx);
        m_ImfCtx = NULL;
    }

    FrameInfo unit;
    std::lock_guard<std::mutex> VlockGuard(m_VideoMutex);
    while(m_VideoQueue.size() > 0)
    {
        unit = m_VideoQueue.front();
        if(unit.data!=NULL) delete[] unit.data;
        m_VideoQueue.pop();
    }

    std::lock_guard<std::mutex> AlockGuard(m_AudioMutex);
    while(m_AudioQueue.size() > 0)
    {
        unit = m_AudioQueue.front();
        if(unit.data!=NULL) delete[] unit.data;
        m_AudioQueue.pop();
    }
}

int ff_rtmp_client::rtmp_pull_open(const char* url)
{
    if(url)
    {
        memset(m_RtmpUrl, 0,256);
        snprintf(m_RtmpUrl, 256, "%s", url);
    }

    int ret = -1;
    AVDictionary *dic = NULL;
    ret = av_dict_set(&dic, "bufsize", "102400", 0);
    if (ret < 0)
    {
        cout << "av_dict_set  bufsize fail ";
        return ret;
    }

    ret = av_dict_set(&dic, "stimeout", "2000000", 0);
    if (ret < 0)
    {
        cout << "av_dict_set  stimeout fail ";
        return ret;
    }

    ret = av_dict_set(&dic, "max_delay", "500000", 0);
    if (ret < 0)
    {
        cout << "av_dict_set  max_delay fail ";
        return ret;
    }

    ret = av_dict_set(&dic, "tune", "zerolatency", 0);
    if (ret < 0)
    {
        cout << "av_dict_set  tune fail ";
        return ret;
    }

    if((ret = avformat_open_input(&m_ImfCtx, m_RtmpUrl, 0, &dic)) < 0)
    {
        av_dict_free(&dic);
        return ret;
    }

    av_dict_free(&dic);

    // 查找流信息
    if ((ret = avformat_find_stream_info(m_ImfCtx, NULL)) < 0)
    {
        printf("Failed to retrieve input video stream information -%08X.\n", -ret);
        return ret;
    }

    //printf("m_ImfCtx->nb_streams [%d] \n", m_ImfCtx->nb_streams);
    // 遍历输入媒体各个流格式
    for (unsigned int i = 0; i < m_ImfCtx->nb_streams; i++)
    {
        av_dump_format(m_ImfCtx, i, m_RtmpUrl, 0);
        
        AVStream* input_stream = m_ImfCtx->streams[i];
        printf("input_stream->codec->codec_type = [%d] stream[%d]", input_stream->codec->codec_type, i);
        if (input_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (input_stream->codec == NULL)
            {
                printf("Input video codec is null.\n");
                return -2;
            }
            if (input_stream->codec->coded_height < 1
                || input_stream->codec->coded_width < 1)
            {
                // 视频尺寸无效
                printf("Input video size invalid: %d, %d.\n",
                    input_stream->codec->coded_width, input_stream->codec->coded_height);
                return -2;
            }

            if(input_stream->codec->codec_id == AV_CODEC_ID_H264)
            {
                /* h264_mp4toannexb */
                m_VideoAbsFilter = av_bsf_get_by_name("h264_mp4toannexb");
            }
            else if (input_stream->codec->codec_id == AV_CODEC_ID_H264)
            {
                m_VideoAbsFilter = av_bsf_get_by_name("hevc_mp4toannexb");
            }

            if(!m_VideoAbsFilter)
            {
                cout << "av_bsf_get_by_name for h264_mp4toannexb fail " << endl;
                return -1;
            }

            /* 过滤器分配内存 */
            if( (ret = av_bsf_alloc(m_VideoAbsFilter, &m_VideoAbsCtx)) != 0)
            {
                cout << "av_bsf_alloc for audio fail error_code: " << ret  << endl;
                return ret;
            }

            //3. 添加解码器属性
            avcodec_parameters_copy(m_VideoAbsCtx->par_in, input_stream->codecpar);

            //4. 初始化过滤器上下文
            if( (ret = av_bsf_init(m_VideoAbsCtx)) != 0 )
            {
                return ret;
            }

            m_VideoStreamIndex = i;
        }
        else if (input_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if (input_stream->codec == NULL)
            {
                printf("Input video codec is null.\n");
                return -2;
            }

#if 0
            /* 找到对应的过滤器 */
            m_AudioAbsFilter  = av_bsf_get_by_name("aac_adtstoasc");  /* adts音频 */

            if(!m_AudioAbsFilter)
                return -1;

            /* 过滤器分配内存 */
            if( (ret = av_bsf_alloc(m_AudioAbsFilter, &m_AudioAbsCtx)) != 0)
            {
                cout << "av_bsf_alloc for audio fail error_code: " << ret  << endl;
                return ret;
            }

            //3. 添加解码器属性
            avcodec_parameters_copy(m_AudioAbsCtx->par_in, input_stream->codecpar);

            //4. 初始化过滤器上下文
            if( (ret = av_bsf_init(m_AudioAbsCtx)) != 0 )
            {
                return ret;
            }
#endif

            m_AudioCodeCtx = input_stream->codec;
            m_AudioStreamIndex = i;
        }
    }

    if(m_VideoStreamIndex == -1)
    {
        cout << "Invalid video stream " << endl;
        return -1;
    }
    
    return 0;
}

void* ff_rtmp_client::rtmp_main_loop(void* ptr)
{
    if(!ptr)
    {
        cout << "Null ptr" << endl;
        return NULL;
    }

    ff_rtmp_client* handler = (ff_rtmp_client*) ptr;
    handler->process_func();

    return NULL;
}

void ff_rtmp_client::process_func()
{
    AVPacket        pkt;
    av_init_packet(&pkt);

    m_IsProcess = true;
    int ret     = -1;

    while(m_IsProcess)
    {
        if( (ret = av_read_frame(m_ImfCtx, &pkt)) < 0)
        {
            // AVERROR_EOF表示流读取完
            if (ret != AVERROR_EOF)
            {
                printf("Read video frame ret = -%08X.\n", -ret);
                continue;
            }
        }

        bsf_process(pkt);

        av_packet_unref(&pkt);
    }
}

bool ff_rtmp_client::start_process()
{
    int ret = pthread_create(&m_processing_thread, NULL,  rtmp_main_loop, this);
    
    return (ret == 0) ? true:false;
}

void ff_rtmp_client::stop_process()
{
    m_IsProcess = false;

    if(m_processing_thread != -1)
    {
        pthread_join(m_processing_thread, NULL);
        m_processing_thread = -1;
    }
}

void ff_rtmp_client::push_aduio(FrameInfo& audio)
{
    std::lock_guard<std::mutex> lockGuard(m_AudioMutex);
    m_AudioQueue.push(audio);
}

size_t ff_rtmp_client::GetAudioQueueSize()
{
    std::lock_guard<std::mutex> lockGuard(m_AudioMutex);
    return m_AudioQueue.size();
}

void ff_rtmp_client::push_video(FrameInfo& video)
{
    std::lock_guard<std::mutex> lockGuard(m_VideoMutex);
    m_VideoQueue.push(video);
}

void ff_rtmp_client::get_video_frame(FrameInfo& video)
{
    std::lock_guard<std::mutex> lockGuard(m_VideoMutex);
    if(m_VideoQueue.size() > 0)
    {
        video = m_VideoQueue.front();
        m_VideoQueue.pop();
    }
    else
    {
        memset(&video, 0, sizeof(FrameInfo));
    }
}

void ff_rtmp_client::get_audio_frame(FrameInfo& audio)
{
    std::lock_guard<std::mutex> lockGuard(m_AudioMutex);
    if(m_AudioQueue.size() > 0)
    {
        audio = m_AudioQueue.front();
        m_AudioQueue.pop();
    }
    else
    {
        memset(&audio, 0, sizeof(FrameInfo));
    }
    
}

void ff_rtmp_client::bsf_process(AVPacket& pkt)
{
    FrameInfo frame;
    int ret = 0;
    AVRational time_base_q = {1, 90000};

    if(pkt.stream_index == m_VideoStreamIndex)
    {
        av_bsf_send_packet(m_VideoAbsCtx, &pkt);
        while ((ret = av_bsf_receive_packet(m_VideoAbsCtx, &pkt)) == 0)
        {
            memset(&frame, 0, sizeof(FrameInfo));
            frame.data_size  = pkt.size;
            
            frame.time_stamp = av_rescale_q_rnd(pkt.pts , m_ImfCtx->streams[m_VideoStreamIndex]->time_base, time_base_q,
                               (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

            cout << " video time stamp : " << frame.time_stamp << endl;

            frame.data       = new char[frame.data_size];
            memcpy(frame.data, pkt.data, frame.data_size);
            push_video(frame);
            av_packet_unref(&pkt);
        }
    }
    else if (pkt.stream_index == m_AudioStreamIndex)
    {
        memset(&frame, 0, sizeof(FrameInfo));
        ADTSContext AdtsCtx;
        memset(&AdtsCtx, 0, sizeof(ADTSContext));

        frame.data_size  = pkt.size + 7;
        frame.time_stamp = av_rescale_q_rnd(pkt.pts , m_ImfCtx->streams[m_AudioStreamIndex]->time_base, time_base_q, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        frame.data = new char[frame.data_size];

        cout << " 11111111111 audio time stamp : " << frame.time_stamp << endl;

        aac_decode_extradata(&AdtsCtx, m_AudioCodeCtx->extradata, m_AudioCodeCtx->extradata_size);
        aac_set_adts_head(&AdtsCtx, frame.data, pkt.size);
       
        memcpy(frame.data + 7, pkt.data, pkt.size);
        #if 0
        {
            FILE* wfd = fopen("../out.aac", "ab+");
            if(wfd)
            {
                fwrite(frame.data, 1, frame.data_size, wfd);
                fclose(wfd);
            }
        }
        #endif
        push_aduio(frame);
    }

}