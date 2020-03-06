#include "ffmpeg_pts_filter.h"

ffmpeg_pts_filter::ffmpeg_pts_filter(const char *filter_descr, const char *filename):
            m_fmt_ctx(NULL),m_dec_ctx(NULL),m_video_enc_ctx(NULL),
            m_buffersink_ctx(NULL),m_buffersrc_ctx(NULL),m_filter_graph(NULL),m_pEncFormatCtx(NULL),m_EncVideoStrem(NULL)
{
    m_video_stream_index = -1;
    m_audio_stream_index = -1;

    if(filter_descr)
    {
        m_filter_descr = filter_descr;
    }

    if(filename)
    {
         m_filename  = filename;
    }

    m_last_pts = AV_NOPTS_VALUE;

}

ffmpeg_pts_filter::~ffmpeg_pts_filter()
{
    avfilter_graph_free(&m_filter_graph);
    avcodec_close(m_dec_ctx);
    avcodec_close(m_video_enc_ctx);
    avformat_close_input(&m_fmt_ctx);
    //av_frame_free(&m_frame);
}

int ffmpeg_pts_filter::open_input_file()
{
    if(m_filename.empty())
    {
        printf("input file name is null \n");
        return -1;
    }

    const char* filename = m_filename.c_str();

    int ret;
    AVCodec *dec = NULL;
    AVCodec *video_enc = NULL;
  
    if ((ret = avformat_open_input(&m_fmt_ctx, filename, NULL, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }
  
    if ((ret = avformat_find_stream_info(m_fmt_ctx, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }
  
    /* select the video stream  判断流是否正常 */
    ret = av_find_best_stream(m_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }

    //m_audio_stream_index = av_find_best_stream(m_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    
    m_video_stream_index = ret;
    m_dec_ctx = m_fmt_ctx->streams[m_video_stream_index]->codec;
    av_opt_set_int(m_dec_ctx, "refcounted_frames", 1, 0);  /* refcounted_frames 帧引用计数 */
  
    /* init the video decoder */
    if ((ret = avcodec_open2(m_dec_ctx, dec, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    //printf("audio_stream_index = %d \n", m_audio_stream_index);

    return 0;
}

int ffmpeg_pts_filter::init_video_filters()
{
    if(m_filter_descr.empty())
    {
        printf("NULL filter description \n");
        return -1;
    }
    const char* filters_descr = m_filter_descr.c_str();

    char args[512];
    int ret = 0;
    AVFilter *buffersrc    = avfilter_get_by_name("buffer");     /* 输入buffer filter */
    AVFilter *buffersink   = avfilter_get_by_name("buffersink"); /* 输出buffer filter */
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base   = m_fmt_ctx->streams[m_video_stream_index]->time_base;   /* 时间基数 */
  
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };

    m_filter_graph = avfilter_graph_alloc();              /* 创建graph  */
    if (!outputs || !inputs || !m_filter_graph)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }  

    #if 0
    /* for audio filter */
    ret = snprintf(asrc_args, sizeof(asrc_args),
                   "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
                   is->audio_filter_src.freq, av_get_sample_fmt_name(is->audio_filter_src.fmt),
                   is->audio_filter_src.channels,
                   1, is->audio_filter_src.freq);
    #endif
    
    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            m_dec_ctx->width, m_dec_ctx->height, m_dec_ctx->pix_fmt,
            time_base.num, time_base.den,
            m_dec_ctx->sample_aspect_ratio.num, m_dec_ctx->sample_aspect_ratio.den);

    /* 创建并向FilterGraph中添加一个Filter */
    ret = avfilter_graph_create_filter(&m_buffersrc_ctx, buffersrc, "in",
                            args, NULL, m_filter_graph);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&m_buffersink_ctx, buffersink, "out",
                                       NULL, NULL, m_filter_graph);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }
  
     /* Set a binary option to an integer list. */
    ret = av_opt_set_int_list(m_buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
    }

#if 0
    /* for audio */
    if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts,  AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;
#endif

    /* 
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */
  
    /* 
     * The buffer source output must be connected to the input pad of 
     * the first filter described by filters_descr; since the first 
     * filter input label is not specified, it is set to "in" by 
     * default. 
     */

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = m_buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;
  
    /* 
     * The buffer sink input must be connected to the output pad of 
     * the last filter described by filters_descr; since the last 
     * filter output label is not specified, it is set to "out" by 
     * default. 
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = m_buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;
  
    /* Add a graph described by a string to a graph */
    if ((ret = avfilter_graph_parse_ptr(m_filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)
        goto end;

    /* Check validity and configure all the links and formats in the graph */
    if ((ret = avfilter_graph_config(m_filter_graph, NULL)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
  
    return ret;
}

int ffmpeg_pts_filter::flush_encoder(AVFormatContext *fmt_ctx, int stream_index) 
{
    if(stream_index == -1)
        return true;

    int ret;

    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
    {
        return 0;
    }

    while (1) 
    {
        ret = avcodec_send_frame(fmt_ctx->streams[stream_index]->codec, NULL);
        if(ret == 0)
        {
            enc_pkt.data = NULL;
            enc_pkt.size = 0;
            av_init_packet(&enc_pkt);
            
            //framecnt++;
            ret = avcodec_receive_packet(fmt_ctx->streams[stream_index]->codec, &enc_pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                return 0;
            }

            //enc_pkt.stream_index = stream_index;
            //ret = av_write_frame(fmt_ctx, &enc_pkt);
            if(stream_index == m_video_stream_index)
            {
                FILE* wfd = fopen("out.h264", "ab+");
                if(wfd)
                {
                    fwrite(enc_pkt.data, 1, enc_pkt.size, wfd);
                    fclose(wfd);
                }
            }
            
            av_packet_unref(&enc_pkt);
        }
        else
        {
            return 0;
        }
        
    }
    
    return ret;
}

int ffmpeg_pts_filter::init_video_enc()
{
    AVOutputFormat* fmt         = NULL;
    AVCodecContext* pCodecCtx   = NULL;
    AVCodecParameters * pCodPar = NULL;
    AVCodec* pCodec             = NULL;
    
    //Input data's width and height
    int in_w = m_dec_ctx->width;
    int in_h = m_dec_ctx->height;
    
    const char* out_file = "ds.h264";

    av_register_all();

    //av_log_set_level(AV_LOG_TRACE);
    
    //Method1.
    m_pEncFormatCtx = avformat_alloc_context();
    //Guess Format
    fmt = av_guess_format(NULL, out_file, NULL);
    m_pEncFormatCtx->oformat = fmt;

    //Open output URL
    if (avio_open(&m_pEncFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Failed to open output file! \n");
        return -1;
    }

    m_EncVideoStrem = avformat_new_stream(m_pEncFormatCtx, 0);

    if (m_EncVideoStrem == NULL)
    {
        printf("avformat_new_stream Failed ! \n");
        return -1;
    }
    
    //Param that must set
    pCodecCtx = m_EncVideoStrem->codec;
    //pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
    pCodecCtx->codec_id = AV_CODEC_ID_H264;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->bit_rate = 4*1024*1024;
    pCodecCtx->gop_size = 25;   //I帧间隔   

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;  //time_base一般是帧率的倒数，但不总是  
    pCodecCtx->framerate.num = 1;  //帧率  
    pCodecCtx->framerate.den = 25;

    ///AVFormatContext* mFormatCtx 
    ///mBitRate   = mFormatCtx->bit_rate;   ///码率存储位置  
    ///mFrameRate = mFormatCtx->streams[stream_id]->avg_frame_rate.num;
    
    //H264
    //pCodecCtx->me_range = 16;
    //pCodecCtx->max_qdiff = 4;
    //pCodecCtx->qcompress = 0.6;
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;

    //Optional Param
    pCodecCtx->max_b_frames = 0;  //不要B帧   

    pCodPar = m_EncVideoStrem->codecpar;
    pCodPar->codec_id = AV_CODEC_ID_H264;
    pCodPar->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodPar->format = AV_PIX_FMT_YUV420P;
    pCodPar->width = in_w;
    pCodPar->height = in_h;
    pCodPar->bit_rate = 4*1024*1024;
    
    // Set Option
    AVDictionary *param = 0;
    
#if 0
    //H.264
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        //av_dict_set(&param, "profile", "main", 0);
    }
    //H.265
    if (pCodecCtx->codec_id == AV_CODEC_ID_H265) {
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zero-latency", 0);
    }
#endif

    //Show some Information
    av_dump_format(m_pEncFormatCtx, 0, out_file, 1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) 
    {
        printf("Can not find encoder! \n");
        return -1;
    }
    
    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) 
    {
        printf("Failed to open encoder! \n");
        return -1;
    }
    
}

int ffmpeg_pts_filter::run_video_encode(AVFrame *in_frame)
{
    AVPacket pkt;
    av_init_packet(&pkt);
    
    int ret = avcodec_send_frame(m_EncVideoStrem->codec, in_frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending a frame for encoding\n");
        return -1;
    }

    while (ret == 0)
    {
        ret = avcodec_receive_packet(m_EncVideoStrem->codec, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret == 0)
        {
            //framecnt++;
            if(pkt.size > 0)
            {
                //pkt.stream_index = m_EncVideoStrem->index;
                FILE* wfd = fopen("out.h264", "ab+");
                if(wfd)
                {
                    fwrite(pkt.data, 1, pkt.size, wfd);
                    fclose(wfd);
                }
                //av_write_frame(pEncFormatCtx, &pkt);
            }
            av_packet_unref(&pkt);
        }
        else
        {
            av_packet_unref(&pkt);
            break;
        }
    }
}

int ffmpeg_pts_filter::run_decode()
{
    int ret = 0;
    AVPacket packet;
    AVFrame *frame      = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    int64_t out_pack_pts = 0;
    bool first = true;

    while (1) 
    {
        if ((ret = av_read_frame(m_fmt_ctx, &packet)) < 0)
            break;
  
        if (packet.stream_index == m_video_stream_index)
        {
            int got_frame = 0;

            ret = avcodec_decode_video2(m_dec_ctx, frame, &got_frame, &packet);
            if (ret < 0) 
            {
                av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
                break;
            }
  
            if (got_frame)
            {
                frame->pts = av_frame_get_best_effort_timestamp(frame);    /* pts: Presentation Time Stamp */
  
                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(m_buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    break;
                }

                /* pull filtered frames from the filtergraph */
                while (1)
                {
                    ret = av_buffersink_get_frame(m_buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  
                        break;
                    if (ret < 0)
                        break;

                    //filt_frame->pts = out_pack_pts++;
                    printf("filt_frame.pts[%lld] filt_frame-width[%d]\n", filt_frame->pts, filt_frame->width);

                     /* 
                         1、设置为setpts=0.5*PTS播放后，解码出的filt_frame->pts值会重复一次（每个相同pts对应的yuv图片都是一样的）
                         所以编码是需要去掉多于的frame；
                         2、若设置为setpts=2*PTS播放时候，解码出来的filt_frame->pts值均为偶数；
                     */

                    if(first)
                    {
                        first = false;
                        run_video_encode(filt_frame);
                        out_pack_pts = filt_frame->pts;
                    }
                    else if(filt_frame->pts != out_pack_pts)
                    {
                        run_video_encode(filt_frame);
                        out_pack_pts = filt_frame->pts;
                    }

                    av_frame_unref(filt_frame);
                }
                /* Unreference all the buffers referenced by frame and reset the frame fields. */
                av_frame_unref(frame);
            }
        }
        else if(packet.stream_index == m_audio_stream_index)
        {
            /* process audio */
        }

        av_packet_unref(&packet);
    }

    flush_encoder(m_fmt_ctx, m_video_stream_index);
    flush_encoder(m_fmt_ctx, m_audio_stream_index);

    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    return 0;
}

int ffmpeg_pts_filter::run()
{
    int ret = 0;
    if( (ret = open_input_file() )  != 0)
    {
        return ret;
    }

     if( (ret = init_video_filters() )  != 0)
    {
        return ret;
    }

     if( (ret = init_video_enc() )  != 0)
    {
        return ret;
    }

    return run_decode();
}
