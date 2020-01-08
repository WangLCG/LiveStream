#include "Ffmpeg_stream_pusher2.h"

FillPKGStreamPusher::FillPKGStreamPusher(int16_t framerate, const char* stream_path, 
                int8_t streamType, int16_t width, int16_t height, RecvMulticastDataModule* MultiCastModule): m_frameRate(framerate)
{
    memset(m_StreamPath, 0, 256);
    snprintf(m_StreamPath, 256, "%s", stream_path);
    
    m_StreamType     = streamType;
    m_videoindex_out = 0;
    m_audioindex_out = 0;

    m_videoWidth       = width;
    m_videoHeight      = height;
    m_MultiCastModule  = MultiCastModule;

    m_cur_pts_v = 0;
    m_cur_pts_a = 0;

    m_i_begin_timeStamp = 0;
    m_first             = true;
    
#if USE_AACBSF
    //m_aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
    m_filter = av_bsf_get_by_name("aac_adtstoasc");
    av_bsf_alloc(m_filter, &m_bsf_ctx);
#endif

}

FillPKGStreamPusher::~FillPKGStreamPusher()
{
    if(NULL != m_o_fmt_ctx)
    {
        av_write_trailer(m_o_fmt_ctx);
        avio_close(m_o_fmt_ctx->pb);
        avformat_free_context(m_o_fmt_ctx);
        m_o_fmt_ctx = NULL;
    }

#if USE_AACBSF
    //av_bitstream_filter_close(m_aacbsfc);
    av_bsf_free(&m_bsf_ctx);
#endif

}

bool FillPKGStreamPusher::CreateOutput(AVFormatContext *&pOutContext, unsigned char* buf,int spsSize)
{
    int ret; // 成功返回0，失败返回1
    AVOutputFormat *fmt;
    AVCodec *video_codec;
    enum AVCodecID codec_id;
    AVStream *i_video_stream = NULL;
    AVStream *i_audio_stream = NULL;

    //CreateDirectoryEx(filePath);
    //cout << "CubeStorageClient::CreateMp4() after" <<filePath<<endl;
    // CREATE MP4 FILE
    bool IsRtsp = strstr(m_StreamPath,"rtsp") ? 1:0;
    bool IsRtmp = strstr(m_StreamPath,"rtmp") ? 1:0;
    
    if(IsRtsp)
    {
        avformat_alloc_output_context2(&pOutContext, NULL, "rtsp", m_StreamPath); //RTSP
    }
    else if(IsRtmp)
    {
        avformat_alloc_output_context2(&pOutContext, NULL, "flv", m_StreamPath);  //RTMP
    }
    
    if (!pOutContext)
    {
        //av_opt_set(&m_pOc, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
        if(IsRtsp)
        {
            avformat_alloc_output_context2(&pOutContext, NULL, "rtsp", m_StreamPath); //RTSP
        }
        else if(IsRtmp)
        {
            avformat_alloc_output_context2(&pOutContext, NULL, "flv", m_StreamPath);  //RTMP
        }
    }
    if (!pOutContext)
    {
        return -1;
    }

    //video stream
    fmt = pOutContext->oformat;

    if(m_StreamType == CUS_H264_VIDEO_STREAM)
    {
        codec_id = AV_CODEC_ID_H264;
    }
    else if(m_StreamType == CUS_H265_VIDEO_STREAM)
    {
        codec_id = AV_CODEC_ID_HEVC;
    }
    else
    {
        codec_id = fmt->video_codec;
    }

    if (fmt->video_codec != AV_CODEC_ID_NONE)
    {
        //fmt->video_codec is default h264, fmt->audio_codec is default aac.
         cout << "codec_id === " <<codec_id << endl;
        i_video_stream = add_stream(pOutContext, &video_codec, codec_id, buf, spsSize); // add video

        if (i_video_stream == NULL)
        {
            return -1;
        }
        m_videoindex_out = i_video_stream->index;
        //ERROR_LOG("videoindex_out------index:%d", s_videoindex_out);
    }

    /* aac 添加adts头部信息, 否则ffmpeg会报错的 */
    unsigned char dsi1[2]={0};
    //unsigned int sampling_frequency_index = (unsigned int) get_sr_index((unsigned int)output_stream->codec->sample_rate);
    
    unsigned int sampling_frequency_index = 3;   /* 48khz  */
    unsigned int object_type = 2; /* AAC LC object type */
    dsi1[0] = (object_type<<3) | (sampling_frequency_index>>1);
    dsi1[1] = ((sampling_frequency_index&1)<<7) | (2<<3);

    //audio stream
    i_audio_stream = add_stream(pOutContext, &video_codec, AV_CODEC_ID_AAC, dsi1, 2); // add audio

    if (i_audio_stream == NULL)
    {
        return -1;
    }
    m_audioindex_out = i_audio_stream->index;
    //ERROR_LOG("audioindex_out------index:%d", s_audioindex_out);
    //av_dump_format(m_pOc, 0, outFileName, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&pOutContext->pb, m_StreamPath, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            cout << "could not open :" <<m_StreamPath << endl;
            return -1;
        }
    }
    
    AVDictionary *dic = NULL;
    ret = av_dict_set(&dic, "bufsize", "10240", 0);
    if (ret < 0)
    {
        cout << "av_dict_set  bufsize fail ";
        //return -1;
    }

    ret = av_dict_set(&dic, "stimeout", "2000000", 0);
    if (ret < 0)
    {
        cout << "av_dict_set  stimeout fail ";
        //return -1;
    }

    ret = av_dict_set(&dic, "max_delay", "500000", 0);
    if (ret < 0)
    {
        cout << "av_dict_set  max_delay fail ";
        //return -1;
    }

    if(IsRtsp)
    {
        /* 设置为TCP方式传输 */
        ret = av_dict_set(&dic, "rtsp_transport", "tcp", 0);
        if (ret < 0)
        {
            cout << "av_dict_set  rtsp_transport fail ";
            //return -1;
        }
    }
    

    av_dict_set(&dic, "tune", "zerolatency", 0);
    
    av_dump_format(pOutContext, 0, m_StreamPath, 1);
    //av_dict_set(&dict, "movflags", "empty_moov", 0);
    //av_dict_set(&dict, "movflags", "+faststart", 0);

    /* Write the stream header, if any */
    ret = avformat_write_header(pOutContext, &dic);
    if (ret < 0)
    {
        cout <<"Error occurred when opening output file" << endl;
        return -1;
    }
    av_dict_free(&dic);
    
    return 0;
}

AVStream* FillPKGStreamPusher::add_stream(AVFormatContext *oc, AVCodec **codec, 
            enum AVCodecID codec_id, unsigned char* extra_data, int size)
{
    AVCodecContext *c;
    AVStream *st;
    int ret = -1;
    *codec = avcodec_find_encoder(codec_id);
    if (!*codec)
    {
        cout <<"could not find encoder for "<<avcodec_get_name(codec_id) << endl;
        return NULL;
    }

    st = avformat_new_stream(oc, *codec);
    if (!st)
    {
        cout <<"could not allocate stream \n" << endl;
        return NULL;
    }
    st->id = oc->nb_streams - 1;
    //c = st->codec; //now already no use codec
    //vi = st->index;
    c = avcodec_alloc_context3(*codec);
    uint8_t* p;
    switch ((*codec)->type)
    {
        case AVMEDIA_TYPE_AUDIO:
           {
                c->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
                c->bit_rate = 64000;
                c->sample_rate = AUDIO_SAMPLE;
                c->channels = 2;
                c->time_base.num  = 1;
                c->time_base.den = AUDIO_SAMPLE;

                if(size > 0)
                {
                    p = (uint8_t*)malloc(size);
                    memcpy(p,extra_data,size);
                    c->extradata = p;
                    c->extradata_size = size;
                }
            }
            break;

        case AVMEDIA_TYPE_VIDEO:
            {
                //c->codec_id = AV_CODEC_ID_H265;
                c->codec_id = codec_id;
                c->codec_type = AVMEDIA_TYPE_VIDEO;
                c->bit_rate = 400000;
                c->width    = m_videoWidth;
                c->height   = m_videoHeight;
                c->time_base.num  = 1;
                c->time_base.den = 90000;
                c->gop_size = 30; // keyframe interval
                //c->frame_number = 1;
                c->pix_fmt = AV_PIX_FMT_YUV420P;
                //c->has_b_frames = 0;
                c->codec_tag      = 0;
                
                if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
                {
                    c->max_b_frames = 2;
                }
                if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
                {
                    c->mb_decision = 2;
                }

                p = (uint8_t*)malloc(size);

                memcpy(p,extra_data,size);
                c->extradata = p;
                c->extradata_size = size;       /*set the PPS SPS of H264 */
             }
             break;

        default:
            break;
    }

    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
    {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    ret = avcodec_parameters_from_context(st->codecpar, c);
    free(c->extradata);
    c->extradata = NULL;
    avcodec_free_context(&c);

    if (ret < 0)
    {
        cout << "Failed to copy context input to output stream codec context\n";
    }
    
    return st;
}

bool FillPKGStreamPusher::judgeIframe(unsigned char* dataBuf, int buf_size, int& spssize)
{
    /* 不适用于起始码3个字节的只适用00 00 00 01起始码 */
    bool bret = false;

    unsigned char* tempbuf = dataBuf;
    //判断264 和 265的I帧
    int tmpsize = 4;
    if(m_StreamType == CUS_H265_VIDEO_STREAM)
    {
        if((tempbuf[4]& 0x7E)>>1 != 32 /*&& (tempbuf[4]& 0x7E)>>1 != 26*/)
        {
            return bret;
        }
        else
        {
            while(1)
            {
                if(tempbuf[tmpsize+spssize]==0&&tempbuf[tmpsize+1+spssize]==0&&tempbuf[tmpsize+2+spssize]==0&&tempbuf[tmpsize+3+spssize]==1)
                {
                    spssize += tmpsize;
                    int nal_unit_type1 = (tempbuf[tmpsize+spssize]& 0x7E)>>1;
                    if(nal_unit_type1 == 33)
                    {
                        while(1)
                        {
                            if(tempbuf[tmpsize+spssize]==0&&tempbuf[tmpsize+1+spssize]==0&&tempbuf[tmpsize+2+spssize]==0&&tempbuf[tmpsize+3+spssize]==1)
                            {
                                spssize += tmpsize;
                                int nal_unit_type2 = (tempbuf[tmpsize+spssize]& 0x7E)>>1;
                                if(nal_unit_type2 == 34)
                                {
                                    while(1)
                                    {
                                        if(tempbuf[tmpsize+spssize]==0&&tempbuf[tmpsize+1+spssize]==0&&tempbuf[tmpsize+2+spssize]==0&&tempbuf[tmpsize+3+spssize]==1)
                                        {
                                            break;
                                        }
                                        spssize++;
                                    }
                                }
                                break;
                            }
                            spssize++;

                        }
                    }
                    break;
                }
                spssize++;
            }
        }
    }
    else
    {
        int nal_unit_type = tempbuf[4]& 0x1f;
        if( nal_unit_type == 7)
        {
            /* sps */
            while(1)
            {
                if(tempbuf[tmpsize+spssize]==0&&tempbuf[tmpsize+1+spssize]==0&&tempbuf[tmpsize+2+spssize]==0&&tempbuf[tmpsize+3+spssize]==1)
                {
                    spssize += tmpsize;
                    int nal_unit_type1 = tempbuf[spssize+tmpsize] & 0x1f;
                    //printf("------------- spssize[%d] nal_unit_type1 = %d nal_unit_type = %d \n", spssize, nal_unit_type1, nal_unit_type);
                    if(nal_unit_type1==8)
                    {
                        while(1)
                        {
                            if(tempbuf[tmpsize+spssize]==0&&tempbuf[tmpsize+1+spssize]==0&&tempbuf[tmpsize+2+spssize]==0&&tempbuf[tmpsize+3+spssize]==1)
                            {
                                break;
                            }
                            spssize++;
                        }
                        spssize += tmpsize; /* 加上pps的起始码 */
                    }
                    break;
                }
                spssize++;
            }

        }
        else
        {
            int tLoop = 3;
            while(tLoop && ( spssize < buf_size + tmpsize-4))
            {
                if(tempbuf[tmpsize+spssize]==0&&tempbuf[tmpsize+1+spssize]==0&&tempbuf[tmpsize+2+spssize]==0&&tempbuf[tmpsize+3+spssize]==1){
                    spssize += tmpsize;
                    int nal_unit_type1 = tempbuf[tmpsize+spssize]& 0x1f;
                    if(nal_unit_type1==7)
                    {
                        while(1)
                        {
                            if(tempbuf[tmpsize+spssize]==0&&tempbuf[tmpsize+1+spssize]==0&&tempbuf[tmpsize+2+spssize]==0&&tempbuf[tmpsize+3+spssize]==1)
                            {
                                spssize += tmpsize;
                                int nal_unit_type2 = tempbuf[tmpsize+spssize]& 0x1f;
                                if(nal_unit_type2==8){
                                    while(1){
                                        if(tempbuf[tmpsize+spssize]==0&&tempbuf[tmpsize+1+spssize]==0&&tempbuf[tmpsize+2+spssize]==0&&tempbuf[tmpsize+3+spssize]==1)
                                        {
                                            break;
                                        }
                                        spssize++;
                                    }
                                }
                                break;
                            }
                            spssize++;
                        }
                        break;
                    }
                    tLoop--;
                }
                spssize++;
            }
            if(tLoop==0||(spssize==buf_size+tmpsize-4))
            {
                return bret;
            }
        }
    }
    return true;
}

bool FillPKGStreamPusher::FillAvPacket(int streamIndex, MultiFrameInfo& frame,AVPacket& pkt)
{
    if((NULL == m_o_fmt_ctx) && (streamIndex == m_videoindex_out))
    {
        int spssize = 0;
        if( !judgeIframe(frame.buf, frame.buf_size, spssize))
        {
            printf("Not I frame \n");
            delete[] frame.buf;
            frame.buf = NULL;
            return false;
        }
        else
        {
            printf("spssize [%d] \n", spssize);            
            printf("Begin to create output  \n");
            if(CreateOutput(m_o_fmt_ctx, frame.buf, spssize) < 0)
            {
                printf("create output fail \n");
                delete[] frame.buf;
                frame.buf = NULL;
                return false;
            }
        }
    }

    /* 确保视频首先被写入 */
    if(m_first && streamIndex == m_videoindex_out)
    {
        m_i_begin_timeStamp = frame.timeStamp;
        m_first = false;
    }
    else if(m_first && streamIndex == m_audioindex_out)
    {
        delete[] frame.buf;
        frame.buf = NULL;
        return false;
    }

    printf("m_i_begin_timeStamp [%llu] streamIndex[%d] m_videoindex_out[%d]\n", 
        m_i_begin_timeStamp, streamIndex, m_videoindex_out);
    
    av_init_packet(&pkt);
    m_out_stream = m_o_fmt_ctx->streams[streamIndex];

    pkt.data = frame.buf;
    
    pkt.size = frame.buf_size;
    pkt.pts = av_rescale_q_rnd(frame.timeStamp - m_i_begin_timeStamp , s_refer_time_base, m_out_stream->time_base,
                               (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = pkt.pts;
   
    if(streamIndex == m_videoindex_out)
    {
        if(m_StreamType == CUS_H265_VIDEO_STREAM)
        {
            if( ((pkt.data)[4]& 0x7E) >>1 == 32)
            {
                pkt.flags |= AV_PKT_FLAG_KEY;
            }
        }
        else
        {
            if( ((pkt.data)[4] & 0x1F) == 0x7)
            {
                pkt.flags |= AV_PKT_FLAG_KEY;
            }
        }
        pkt.duration = m_video_duration;
        m_cur_pts_v = frame.timeStamp;
    }
    else
    {
        pkt.duration = FRAME_SAMPLE;
        m_cur_pts_a  = frame.timeStamp;
    }

    if(pkt.pts >= 0)
    {
        return true;
    }

    return false;
}

bool FillPKGStreamPusher::writeFileAvPacket(int streamIndex, AVPacket& pkt)
{
#if USE_AACBSF
    if (streamIndex == m_audioindex_out)
    {
        //audio
        //av_bitstream_filter_filter(client->aacbsfc, m_out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
        av_bsf_send_packet(m_bsf_ctx, &pkt);
        av_bsf_receive_packet(m_bsf_ctx, &pkt);
    }
#endif

    pkt.pos = -1;
    pkt.stream_index = streamIndex;

    //Write
    int ret = av_interleaved_write_frame(m_o_fmt_ctx, &pkt);
    //LOG_DEBUG <<"client->m_videoClientQueue size "<<client->m_videoClientQueue.count()<<" client->m_videoIP= "<<client->m_videoIP;

    if (ret < 0)
    {
        if(streamIndex == m_videoindex_out)
        {
            cout << "cannot write VIDEO.last time:"<<tmp_errortime<<"last pts:"<<last_pts_v<<"curtime:" << pkt.pts<<" begin time:"<<m_i_begin_timeStamp << endl;

        }
        else
        {
            cout << "cannot write AUDIO.last time:"<<tmp_errortime<<"last pts:"<<last_pts_a<<"curtime:" <<pkt.pts<<" begin time:"<<m_i_begin_timeStamp << endl;
        }
        return false;
    }
    else
    {
        if(streamIndex == m_videoindex_out)
        {
            last_pts_v    = pkt.pts;
            tmp_errortime = pkt.pts;
        }
        else
        {
            last_pts_a    = pkt.pts;
            tmp_errortime = pkt.pts;
        }
        
        return true;
    }
}


