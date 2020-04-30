////////////////////////////////////////////////////////////////////////
/// @file mpeg-ps-test.cpp
/// @brief 1、ffmpeg读取音频编码为AAC，视频编码为H264的MP4文件生成PS流文件
/// @brief 2、读取PS流文件生成AAC及H264文件
/// @author 使用的libmpeg为githubmedia_server里面的工具包: https://github.com/ireader/media-server
/// @copyright (c) 2016-2019 
////////////////////////////////////////////////////////////////////////

#include "mpeg-ps.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"
#include <assert.h>
#include <stdio.h>
#include <map>
#include <string.h>
#include <iostream>
#include <FfmpegReceiver.h>
#include "H264FrameTypeParser.h"
#include <unistd.h>

using namespace std;

static void* ps_alloc(void* /*param*/, size_t bytes);
static void ps_free(void* /*param*/, void* /*packet*/);
static void ps_write(void* param, int stream, void* packet, size_t bytes);
static inline const char* ps_type(int type);

/* ps解包 */
static FILE* vfp;
static FILE* afp;

inline const char* ftimestamp(int64_t t, char* buf)
{
    if (PTS_NO_VALUE == t)
    {
        sprintf(buf, "(null)");
    }
    else
    {
        t /= 90;
        sprintf(buf, "%d:%02d:%02d.%03d", (int)(t / 3600000), (int)((t / 60000) % 60), (int)((t / 1000) % 60), (int)(t % 1000));
    }
    return buf;
}

/* 解析ps数据 */
static void onpacket(void* /*param*/, int /*stream*/, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
    static char s_pts[64], s_dts[64];

    if (PSI_STREAM_AAC == avtype)
    {
        static int64_t a_pts = 0, a_dts = 0;
        if (PTS_NO_VALUE == dts)
            dts = pts;
        //assert(0 == a_dts || dts >= a_dts);
        printf("[A] pts: %s(%lld), dts: %s(%lld), diff: %03d/%03d\n", ftimestamp(pts, s_pts), pts, ftimestamp(dts, s_dts), dts, (int)(pts - a_pts) / 90, (int)(dts - a_dts) / 90);
        a_pts = pts;
        a_dts = dts;

        fwrite(data, 1, bytes, afp);
    }
    else if (PSI_STREAM_H264 == avtype)
    {
        static int64_t v_pts = 0, v_dts = 0;
        assert(0 == v_dts || dts >= v_dts);
        printf("[V] pts: %s(%lld), dts: %s(%lld), diff: %03d/%03d\n", ftimestamp(pts, s_pts), pts, ftimestamp(dts, s_dts), dts, (int)(pts - v_pts) / 90, (int)(dts - v_dts) / 90);
        v_pts = pts;
        v_dts = dts;

        fwrite(data, 1, bytes, vfp);
    }
    else
    {
        //assert(0);
    }
}

static uint8_t s_packet[2 * 1024 * 1024];

void mpeg_ps_dec_test(const char* file)
{
    FILE* fp = fopen(file, "rb");
    vfp = fopen("v.h264", "wb");
    afp = fopen("a.aac", "wb");

    size_t n, i= 0;
    ps_demuxer_t* ps = ps_demuxer_create(onpacket, NULL);
    while ((n = fread(s_packet + i, 1, sizeof(s_packet) - i, fp)) > 0)
    {
        size_t r = ps_demuxer_input(ps, s_packet, n + i);
        memmove(s_packet, s_packet + r, n + i - r);
        i = n + i - r;
    }
    ps_demuxer_destroy(ps);

    fclose(fp);
    fclose(vfp);
    fclose(afp);
}

/* ps封装类 */
class ps_muxer_context
{
public:
    ps_muxer_context(char* input_file);
    ~ps_muxer_context();
    
    void ProcessLoop();

    ps_muxer_t* ps;
    char input[256];
    char output[256];
    int audio_id;
    int video_id;
    ff_rtmp_client receiver;
    FILE* fp;

    struct ps_muxer_func_t handler;
};

ps_muxer_context::ps_muxer_context(char* input_file)
{
    memset(output, 0, 256);
    memset(input, 0, 256);

    snprintf(output, sizeof(output), "%s.ps", input_file);
    snprintf(input, sizeof(input), "%s", input_file);

    handler.alloc = ps_alloc;
    handler.write = ps_write;
    handler.free = ps_free;

    fp = fopen(output, "wb");
    ps = ps_muxer_create(&handler, this);

    audio_id = ps_muxer_add_stream(ps, PSI_STREAM_AAC, NULL, 0);
    video_id = ps_muxer_add_stream(ps, PSI_STREAM_H264, NULL, 0);

    cout << "##### audio_id " << audio_id << " video_id " << video_id << endl;
    receiver.rtmp_pull_open(input_file);

}

ps_muxer_context::~ps_muxer_context()
{
    ps_muxer_destroy(ps);
    fclose(fp);
}

void ps_muxer_context::ProcessLoop()
{
    FrameInfo video;
    FrameInfo audio;

    int cnt = 25000;   /* 处理15000帧 */
    if(receiver.start_process())
    {
        cout << " start_process success " << endl;

        int64_t current_video_pts = 0;
        int64_t current_audio_pts = 0;
        while(1)
        {
            --cnt;
            if(cnt <= 0)
            {
                break;
            }

            int flags = MPEG_FLAG_H264_H265_WITH_AUD;
            int64_t compare_tag = -1;
            bool bwait       = true;

            if(receiver.GetAudioQueueSize() > 0)
            {
                bwait = false;
            }

            /* 粗略的音视频同步策略 */
            if(!bwait)
            {
                compare_tag = current_video_pts - current_audio_pts;  //compare fileDate
            }

            cout << "------------------- compare_tag " << compare_tag << endl;
            if (compare_tag <= 0)
            {
                receiver.get_video_frame(video);
                if(video.data && video.data_size > 0 )
               { 
                   int frame_type = H264_FRAME_NALU;
                    frame_type = H264_GetFrameType(video.data, video.data_size, 4);
                    if (frame_type == H264_FRAME_I || frame_type == H264_FRAME_SI)
                    {
                        flags |= MPEG_FLAG_IDR_FRAME;
                    }
                    else
                    {
                        frame_type = H264_GetFrameType(video.data, video.data_size, 3);
                        if (frame_type == H264_FRAME_I || frame_type == H264_FRAME_SI)
                        {
                            flags |= MPEG_FLAG_IDR_FRAME;
                        }
                    }

                    
                    ps_muxer_input(ps, video_id, flags, video.time_stamp, video.time_stamp, video.data, video.data_size);
                    current_video_pts = video.time_stamp;
                    delete[] video.data;
                    video.data = NULL;
                }
            }
            else
            {
                receiver.get_audio_frame(audio);
                if(audio.data && audio.data_size > 0 )
                {
                    flags = MPEG_FLAG_H264_H265_WITH_AUD;
                    ps_muxer_input(ps, audio_id, flags, audio.time_stamp, audio.time_stamp, audio.data, audio.data_size);

                    current_audio_pts = audio.time_stamp;
                    delete[] audio.data;
                    audio.data = NULL;
                }
            }

            usleep(200);
        }
    }

}

static void* ps_alloc(void* /*param*/, size_t bytes)
{
    static char s_buffer[2 * 1024 * 1024];
    assert(bytes <= sizeof(s_buffer));
    return s_buffer;
}

static void ps_free(void* /*param*/, void* /*packet*/)
{
    return;
}

static void ps_write(void* param, int stream, void* packet, size_t bytes)
{
    printf("stream = %d \n", stream);
    ps_muxer_context* handle = (ps_muxer_context*) param;

    fwrite(packet, bytes, 1, handle->fp);
    fflush(handle->fp);
}

static inline const char* ps_type(int type)
{
    switch (type)
    {
    case PSI_STREAM_MP3: return "MP3";
    case PSI_STREAM_AAC: return "AAC";
    case PSI_STREAM_H264: return "H264";
    case PSI_STREAM_H265: return "H265";
    default: return "*";
    }
}

int main()
{
    char* file = "../cus_ies.mp4";
    ps_muxer_context ps_muxer(file);
    ps_muxer.ProcessLoop();

    //printf("Success\n");

    char output[256]={'\0'};
    char* in_file = "../PsMux.mpeg";
    snprintf(output, sizeof(output), "%s.ps", file);
    //mpeg_ps_dec_test(in_file);

    getchar();
    getchar();
    getchar();

    return 0;
}
