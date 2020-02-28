/*=============================================================================  
#     FileName: filter_video.c  
#         Desc: an example of ffmpeg fileter 
#       Author: licaibiao  
#   LastChange: 2017-03-16   
=============================================================================*/
#include <unistd.h>  
  
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

using namespace std;

//#define SAVE_FILE  
  
//const char *filter_descr = "scale=iw*2:ih*2";
const char *filter_descr = "setpts=0.5*PTS";

static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *dec_ctx  = NULL;
static AVCodecContext *video_enc_ctx = NULL;

AVFilterContext *buffersink_ctx = NULL;
AVFilterContext *buffersrc_ctx = NULL;
AVFilterGraph *filter_graph = NULL;
static int video_stream_index = -1;
static int audio_stream_index = -1;

static int64_t last_pts = AV_NOPTS_VALUE;

static AVFormatContext* pEncFormatCtx = NULL;  /* 视频编码器上下文 */
static AVStream* EncVideoStrem        = NULL;     /* 视频流 */

static int open_input_file(const char *filename)
{
    int ret;
    AVCodec *dec = NULL;
    AVCodec *video_enc = NULL;
  
    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }
  
    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }
  
    /* select the video stream  判断流是否正常 */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }

    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    
    video_stream_index = ret;
    dec_ctx = fmt_ctx->streams[video_stream_index]->codec;
    av_opt_set_int(dec_ctx, "refcounted_frames", 1, 0);  /* refcounted_frames 帧引用计数 */
  
    /* init the video decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    printf("audio_stream_index = %d \n", audio_stream_index);
    
    /* 初始化视频编码器 */
    //AVDictionary *opts;
    //av_dict_set(&opts, "b", "5M", 0);
#if 0
    video_enc = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (video_enc)
    {
        video_enc_ctx = avcodec_alloc_context3(video_enc);
        if(video_enc_ctx)
        {
            #if 1
            AVCodecParameters video_par;
            memset(&video_par, 0, sizeof(AVCodecParameters));
            ret = avcodec_parameters_from_context(&video_par, dec_ctx);
            if ( ret < 0)
            {
                printf("avcodec_parameters_from_context faile error[%d]\n", ret);
                return ret;
            }
            ret = avcodec_parameters_to_context(video_enc_ctx, &video_par);
            if ( ret < 0)
            {
                printf("avcodec_parameters_to_context faile error[%d]\n", ret);
                return ret;
            }

            //video_enc_ctx->width    = dec_ctx->width*2;
            //video_enc_ctx->height   = dec_ctx->height*2;
            //avcodec_copy_context(video_enc_ctx, fmt_ctx->streams[video_stream_index]->codec);
            #else
            video_enc_ctx->bit_rate = 3*1024*1024;
            video_enc_ctx->width    = dec_ctx->width;
            video_enc_ctx->height   = dec_ctx->height;
            /* 帧率 */
            video_enc_ctx->time_base.num = 1;
            video_enc_ctx->time_base.den = 25;
            video_enc_ctx->gop_size = 25;
            video_enc_ctx->max_b_frames = 0;
            video_enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
            
            #endif
            if ( (ret = avcodec_open2(video_enc_ctx, video_enc, NULL)) < 0)
            {
                printf("avcodec_open2 faile error[%d]\n", ret);
                return ret;
            }
        }
        else
        {
            printf("avcodec_alloc_context3 for h264 encodec faile \n");
            return -1;
        }
    }
#endif
    
    return 0;
}

static int init_filters(const char *filters_descr)
{
    char args[512];
    int ret = 0;
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");     /* 输入buffer filter */
    AVFilter *buffersink = avfilter_get_by_name("buffersink"); /* 输出buffer filter */
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;   /* 时间基数 */
  
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };

    filter_graph = avfilter_graph_alloc();              /* 创建graph  */
    if (!outputs || !inputs || !filter_graph)
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
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
            time_base.num, time_base.den,
            dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    /* 创建并向FilterGraph中添加一个Filter */
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                            args, NULL, filter_graph);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }
  
     /* Set a binary option to an integer list. */
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
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
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;
  
    /* 
     * The buffer sink input must be connected to the output pad of 
     * the last filter described by filters_descr; since the last 
     * filter output label is not specified, it is set to "out" by 
     * default. 
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;
  
    /* Add a graph described by a string to a graph */
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)
        goto end;

    /* Check validity and configure all the links and formats in the graph */
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
  
    return ret;
}

#ifdef SAVE_FILE
FILE * file_fd = NULL;
static void write_frame(const AVFrame *frame)
{  
    static int printf_flag = 0;
    if(!printf_flag)
    {
        printf_flag = 1;
        printf("frame widht=%d,frame height=%d\n",frame->width,frame->height);
          
        if(frame->format==AV_PIX_FMT_YUV420P)
        {
            printf("format is yuv420p\n");
        }
        else
        {
            printf("formet is = %d \n",frame->format);
        }
      
    }
  
    fwrite(frame->data[0],1,frame->width*frame->height,file_fd);
    fwrite(frame->data[1],1,frame->width/2*frame->height/2,file_fd);
    fwrite(frame->data[2],1,frame->width/2*frame->height/2,file_fd);
}
#endif

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) 
{
    int ret;
    int got_frame;
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
            FILE* wfd = fopen("out.h264", "ab+");
            if(wfd)
            {
                fwrite(enc_pkt.data, 1, enc_pkt.size, wfd);
                fclose(wfd);
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

int init_video_enc()
{
    AVOutputFormat* fmt         = NULL;
    AVCodecContext* pCodecCtx   = NULL;
    AVCodecParameters * pCodPar = NULL;
    AVCodec* pCodec             = NULL;
    
    //Input data's width and height
    int in_w = dec_ctx->width;
    int in_h = dec_ctx->height;
    
    int framenum = 50;                                   //Frames to encode
    //const char* out_file = "src01.h264";              //Output Filepath 
    //const char* out_file = "src01.ts";
    //const char* out_file = "src01.hevc";
    const char* out_file = "ds.h264";

    av_register_all();

    //av_log_set_level(AV_LOG_TRACE);
    
    //Method1.
    pEncFormatCtx = avformat_alloc_context();
    //Guess Format
    fmt = av_guess_format(NULL, out_file, NULL);
    pEncFormatCtx->oformat = fmt;

    //Open output URL
    if (avio_open(&pEncFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Failed to open output file! \n");
        return -1;
    }

    EncVideoStrem = avformat_new_stream(pEncFormatCtx, 0);

    if (EncVideoStrem == NULL)
    {
        printf("avformat_new_stream Failed ! \n");
        return -1;
    }
    
    //Param that must set
    pCodecCtx = EncVideoStrem->codec;
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

    pCodPar = EncVideoStrem->codecpar;
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
    av_dump_format(pEncFormatCtx, 0, out_file, 1);

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

int run_video_encode(AVFrame *in_frame)
{
    AVPacket pkt;
    av_init_packet(&pkt);
    
    int ret = avcodec_send_frame(EncVideoStrem->codec, in_frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending a frame for encoding\n");
        return -1;
    }

    //printf("############### 11111 #############\n");
    while (ret == 0)
    {
        //printf("############### 12222221 #############\n");
        ret = avcodec_receive_packet(EncVideoStrem->codec, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret == 0)
        {
            //framecnt++;
            if(pkt.size > 0)
            {
                //printf("############### 1233333331 #############\n");
                //pkt.stream_index = EncVideoStrem->index;
                FILE* wfd = fopen("out.h264", "ab+");
                if(wfd)
                {
                    fwrite(pkt.data, 1, pkt.size, wfd);
                    fclose(wfd);
                }
                //av_write_frame(pEncFormatCtx, &pkt);
            }
            //printf("############### 12444444441 #############\n");
            av_packet_unref(&pkt);
        }
        else
        {
            av_packet_unref(&pkt);
            break;
        }
    }

    
}

int main(int argc, char **argv)
{

    int ret;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    int got_frame;
    int64_t out_pack_pts = 0;
    
    bool first = true;
    
#ifdef SAVE_FILE
    file_fd = fopen("test.yuv","wb+");
#endif

    if (!frame || !filt_frame)
    {
        perror("Could not allocate frame");
        exit(1);
    }

    av_register_all();
    avfilter_register_all();

    const char* file_path = "../cuc_ieschool.flv";
    if ((ret = open_input_file(file_path) < 0))
    {
        printf("error open_input_file %s\n", file_path);
        goto end;
    }
    if ((ret = init_filters(filter_descr)) < 0)
        goto end;

    ret = init_video_enc();
    if(ret != 0)
    {
        printf("init_video_enc faile \n");
        goto end;
    }

    /* read all packets */
    while (1) 
    {
        if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
            break;
  
        if (packet.stream_index == video_stream_index)
        {
            got_frame = 0;
            ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, &packet);
            if (ret < 0) 
            {
                av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
                break;
            }
  
            if (got_frame)
            {
                frame->pts = av_frame_get_best_effort_timestamp(frame);    /* pts: Presentation Time Stamp */
  
                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    break;
                }

                /* pull filtered frames from the filtergraph */
                while (1)
                {
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  
                        break;  
                    if (ret < 0)  
                        goto end;  

                    //filt_frame->pts = out_pack_pts++;
                    //printf("filt_frame.pts[%lld] filt_frame-width[%d]\n", filt_frame->pts, filt_frame->width);
#ifdef SAVE_FILE
                    write_frame(filt_frame);
#endif  

                     /* 
                         设置为0.5速度慢速播放后，解码出的filt_frame->pts值会重复一次（每个相同pts对应的yuv图片都是一样的）
                         所以编码是需要去掉多于的frame
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
        else if(packet.stream_index == audio_stream_index)
        {
            /* process audio */
        }

        av_packet_unref(&packet);
    }

    {
        
        //Flush Encoder
        int ret = flush_encoder(pEncFormatCtx, 0);
        if (ret < 0) 
        {
            printf("Flushing encoder failed\n");
            return -1;
        }
    
        //Write file trailer
        //av_write_trailer(pEncFormatCtx);
    
        //Clean
        if (EncVideoStrem) {
            avcodec_close(EncVideoStrem->codec);
        }
        avio_close(pEncFormatCtx->pb);
        avformat_free_context(pEncFormatCtx);
    }
    
end:
    avfilter_graph_free(&filter_graph);
    avcodec_close(dec_ctx);
    avcodec_close(video_enc_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    if (ret < 0 && ret != AVERROR_EOF) 
    {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }

#ifdef SAVE_FILE
    fclose(file_fd);
#endif

    exit(0);
}
