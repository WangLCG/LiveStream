/**
 * Simplest Librtmp Receive

 * 本程序用于接收RTMP流媒体并在本地保存成FLV格式的文件，264文件及AAC-LC 48KHZ 16bit采样位数文件。
 */
#include <stdio.h>
#include "rtmp_sys.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>
#include <cmath>

using namespace std;


/* *注意，这里的packetLen参数为raw aac Packet Len + 7; 7 bytes adts header */
static void addADTStoPacket(char* packet, int packetLen, int frequence) 
{
      int profile = 2;  //AAC LC，MediaCodecInfo.CodecProfileLevel.AACObjectLC;
      int freqIdx = -1;  //48K, 见后面注释avpriv_mpeg4audio_sample_rates中32000对应的数组下标，来自ffmpeg源码
      int chanCfg = 2;  //见后面注释channel_configuration，Stero双声道立体声

     int avpriv_mpeg4audio_sample_rates[] = {
          96000, 88200, 64000, 48000, 44100, 32000,
                  24000, 22050, 16000, 12000, 11025, 8000, 7350
      };

      for(int i = 0; i < 13 ; ++i)
      {
          if(frequence == avpriv_mpeg4audio_sample_rates[i])
          {
              freqIdx = i;
              break;
          }
      }
     
     if(freqIdx == -1)
     {
         printf("Invaild frequence [%d] \n", frequence);
         return ;
     }
      /*int avpriv_mpeg4audio_sample_rates[] = {
          96000, 88200, 64000, 48000, 44100, 32000,
                  24000, 22050, 16000, 12000, 11025, 8000, 7350
      };
      channel_configuration: 表示声道数chanCfg
      0: Defined in AOT Specifc Config
      1: 1 channel: front-center
      2: 2 channels: front-left, front-right
      3: 3 channels: front-center, front-left, front-right
      4: 4 channels: front-center, front-left, front-right, back-center
      5: 5 channels: front-center, front-left, front-right, back-left, back-right
      6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
      7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
      8-15: Reserved
      */

      // fill in ADTS data
      packet[0] = 0xFF;
      packet[1] = 0xF1;
      packet[2] = (char)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
      packet[3] = (char)(((chanCfg&3)<<6) + (packetLen>>11));
      packet[4] = (char)((packetLen&0x7FF) >> 3);
      packet[5] = (char)(((packetLen&7)<<5) + 0x1F);
      packet[6] = 0xFC;
}

int InitSockets()
{
#ifdef WIN32
    WORD version;
    WSADATA wsaData;
    version = MAKEWORD(1, 1);
    return (WSAStartup(version, &wsaData) == 0);
#endif
}

void CleanupSockets()
{
#ifdef WIN32
    WSACleanup();
#endif
}

int main(int argc, char* argv[])
{
    InitSockets();
    
    double duration=-1;
    int nRead;
    //is live stream ?
    bool bLiveStream=true;   /* 如果是直播流(true)则不支持回放和搜索操作 */
    
    
    int bufsize=1024*1024*10;           
    unsigned char *buf=(char*)malloc(bufsize);
    memset(buf,0,bufsize);
    long countbufsize=0;
    
    FILE *fp=fopen("receive.flv","wb+");
    if (!fp){
        RTMP_LogPrintf("Open File Error.\n");
        CleanupSockets();
        return -1;
    }
    
    /* set log level */
    //RTMP_LogLevel loglvl=RTMP_LOGDEBUG;
    //RTMP_LogSetLevel(loglvl);

    RTMP *rtmp=RTMP_Alloc();
    RTMP_Init(rtmp);
    //set connection timeout,default 30s
    rtmp->Link.timeout=10;  
    // HKS's live URL
    if(!RTMP_SetupURL(rtmp,"rtmp://eduzz.itc-pa.cn:1935/live/012A305B4C0945CFBD2C"))
    {
        RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }
    if (bLiveStream){
        rtmp->Link.lFlags|=RTMP_LF_LIVE;
    }
    
    //1hour
    RTMP_SetBufferMS(rtmp, 3600*1000);      
    
    if(!RTMP_Connect(rtmp,NULL)){
        RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }

    if(!RTMP_ConnectStream(rtmp,0)){
        RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }

    bool IsFirst = true;
    bool HadGotSPS_PPS = false;
    char video_start_code[4] = {0x00, 0x00,0x00,0x01};
    char start_code[4] = {0x00, 0x00, 0x00, 0x01};

    char sps_pps[128]={0};
    memset(sps_pps, 0, 128);
    unsigned int sps_pps_size = 0;  /* 包含4字节起始码的sps,pps大小 */

    bool writeVideo = true;
    while(nRead=RTMP_Read(rtmp,(char*)buf,bufsize))
    {
        fwrite(buf,1,nRead,fp);

        writeVideo = true;

        countbufsize+=nRead;
        if(IsFirst)
        {
            /* 第一帧包含有flv头部信息及Meta TAG[0x12] */
            IsFirst = false;
            /* datdatype == 0x05 表示既有音频又有视频， ==0x04 是只有音频， ==0x01只有视频 */
            //RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB datdatype[0X%0x], buf_13[0X%0x]\n",nRead,countbufsize*1.0/1024, rtmp->m_read.dataType, buf[13]);
        }
        else
        {
            //RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB datdatype[0X%0x], buf_0[0X%0x]\n",nRead,countbufsize*1.0/1024, rtmp->m_read.dataType, buf[0]);
            if(buf[0] == 0x8)
            {
                /* audio_size = tag_data_size - 1字节的audio类型信息 - 1字节的AAC pack类型 */
                unsigned int audio_data_length = buf[1]*65536 + buf[2]*256 + buf[3] - 1;
                unsigned int time_stamp        = buf[4]*65536 + buf[5]*256 + buf[6];
                RTMP_LogPrintf("audio_data_length: %d time[%u]\n",audio_data_length, time_stamp);

                unsigned char* audio_data = buf + 11;
                unsigned int fmt = (audio_data[0]&0xF0) >> 4;
                printf("audio_fmt[%u], \n", fmt);  /* 10 == AAC */

                
                #if 1
                    {
                        
                        FILE* afd = fopen("out.aac","ab+");
                        if(afd)
                        {
                            #if 0
                            {
                                FILE* flv_aac_raw_fd = fopen("out_raw.aac","ab+");
                                if(flv_aac_raw_fd)
                                {
                                    fwrite(buf,1,nRead,flv_aac_raw_fd);
                                    fclose(flv_aac_raw_fd);
                                }
                            }
                            #endif

                            /* 1字节的audio类型信息 + 1字节的AAC pack类型  */
                            unsigned char* audio_true_start = audio_data + 2;
                            int frenquence = 48000;
                            int aac_lc_48K_data_size = audio_data_length + 7;

                            memmove(audio_true_start + 7, audio_true_start, audio_data_length);
                            addADTStoPacket((char*) audio_true_start, aac_lc_48K_data_size, frenquence);

                            fwrite(audio_true_start, 1, aac_lc_48K_data_size, afd);
                            fclose(afd);
                        }
                    }
                #endif
            }
            else if (buf[0] == 0x9)
            {
                /* video*/
                unsigned int video_data_length = buf[1]*65536 + buf[2]*256 + buf[3] ;
                unsigned int time_stamp        = buf[4]*65536 + buf[5]*256 + buf[6];
                RTMP_LogPrintf("video_data_length: %d time[%u]\n",video_data_length, time_stamp);

                unsigned char* video_data = buf + 11;
                unsigned int Frame_type = (video_data[0]&0xF0) >> 4;

                if(Frame_type == 1)
                {
                    /* H264的I帧 */
                    printf("================ Frame_type[%d] KEY_frame =============== \n", Frame_type);
                }

                unsigned int CodecId = video_data[0]&0x0F;
                //printf("==== CodecId[%d] ====  \n", CodecId);
                if(CodecId == 7)
                {
                    /* H264 */
                    /* 如果CodecId是AVC(H264)的话 videoTagHeader会多出4四节 */
                    /* AVCPacketType(1 byte) 和CompositionTime(3 byte) */

                    unsigned int AVCPacketType = video_data[1] & 0X0F;
                    //printf("================ CodecId[%d] AVCPacketType[%d] =============  \n", CodecId, AVCPacketType);
                    switch(AVCPacketType)
                    {
                        case 0:
                        {
                            /* sps pps 数据
                            给AVC解码器送数据流之前一定要把sps和pps信息送出，否则的话解码器不能正常解码。而且在解码器stop之后再次start之前如seek、快进快退状态切换等，都需要重新送一遍sps和pps的信息.AVCDecoderConfigurationRecord在FLV文件中一般情况也是出现1次，也就是第一个video tag.
                            */
                           HadGotSPS_PPS = true;
                           /* 添加4字节的起始码 */
                           memcpy(sps_pps, start_code, 4);
                           sps_pps_size += 4;
                          
                           /* sps_start = video_data + 1字节的视频参数 + 4字节的AVC参数 */
                           unsigned char* sps_start = video_data + 5;
                           /* sps_start[0] = 0x01标志码 所以无需拷贝 */
                           unsigned int sps_size = sps_start[6]*256 + sps_start[7];
                           RTMP_LogPrintf("============ sps_size = %d \n", sps_size);
                           /* 跳过8字节rtmp的sps头部 */
                           memcpy(sps_pps+sps_pps_size, sps_start+8, sps_size);
                           sps_pps_size += sps_size;

                           /* psp_start = sps_start + 一字节的sps的0x01 + 7字节RTMP的SPS头 + sps_size字节的sps内容 */
                           unsigned char*  pps_start = sps_start + 8 + sps_size;
                           /* pps_start[0] = 0x01标志码 所以无需拷贝*/
                           unsigned int pps_size = pps_start[1]*256 + pps_start[2];

                           RTMP_LogPrintf("============ pps_size = %u \n", pps_size);

                           /* 添加4字节的起始码 */
                           memcpy(sps_pps+sps_pps_size, start_code, 4);
                           sps_pps_size +=4;

                            /* 跳过3字节rtmp的pps头部 */
                           memcpy(sps_pps+sps_pps_size, pps_start+3, pps_size);
                           sps_pps_size += pps_size;
                           writeVideo = false;
                        }break;

                        case 1:
                        {
                            /* 视频数据帧，不包含sps,pps */
                        }break;

                        case 2:
                        {

                        }break;

                        default:
                        break;
                    }

                    #if 1
                    if(HadGotSPS_PPS && writeVideo)
                    {
                        
                        FILE* vfd = fopen("out.264","ab+");
                        if(vfd)
                        {
                            //RTMP_LogPrintf("-------- Frame_type = %d -------\n", Frame_type);

                            if(Frame_type == 1)
                            {
                                /* I帧的话添加SPS，PPS信息*/
                                fwrite(sps_pps, 1, sps_pps_size, vfd);

                                /* 去掉1字节的视频参数 + 4字节的AVC参数, 再去掉sps+pps的大小 */
                                unsigned char* SEI_start = video_data + 5 + sps_pps_size;
                                unsigned int SEI_size = SEI_start[0] * pow(2,32)+ SEI_start[1]*65536 + SEI_start[2]*256+SEI_start[3];
                                //RTMP_LogPrintf("----------- SEI_size = %d sps_pps_size[%d] -----------\n", 
                                //      SEI_size, sps_pps_size);

                                fwrite(start_code, 1, 4, vfd);
                                fwrite(SEI_start + 4, 1, SEI_size, vfd);

                                unsigned char* data_size_start = SEI_start + 4 + SEI_size;
                                unsigned int data_size = data_size_start[0] * pow(2,32)+ data_size_start[1]*65536 + data_size_start[2]*256+ data_size_start[3];
                                //RTMP_LogPrintf("-----------  data_size[%u]-----------\n", data_size);
                                //for(int i = 0; i< 4; ++i)
                                //{
                                //    RTMP_LogPrintf("data_size_start[%d] = 0x%0x \t", i, data_size_start[i]);
                                //}
                                //RTMP_LogPrintf("\n");

                                fwrite(video_start_code, 1, 4, vfd);
                                unsigned char* true_video_start = SEI_start + SEI_size + 4 + 4;
                                
                                fwrite(true_video_start, 1, data_size, vfd);

                            }
                            else
                            {
                                 /* 4字节起始码添加 */
                                fwrite(video_start_code, 1, 4, vfd);
                                /* 视频数据区域组成： 1字节视频参数| AVC的4字节参数 | 4字节的NALU数据长度 | 0X01 | 数据 */
                                /* 除此之外还需要跳过1字节的标志码0X01 */
                                unsigned char* video_data_start = video_data + 1 + 4;
                                unsigned int data_size = video_data_start[0] * pow(2,32)+ video_data_start[1]*65536 + video_data_start[2]*256 + video_data_start[3];
                                
                                //for(int i = 0; i< 4; ++i)
                                //{
                                //    RTMP_LogPrintf("video_data_start[%d] = 0X%0x \t", i, video_data_start[i]);
                                //}
                                //RTMP_LogPrintf("\n");

                                //RTMP_LogPrintf("LINE[%d]: data_size = %u nRead-(20+4) = %d \n", __LINE__, data_size, nRead-(20+4));

                                /* 跳过4字节NALU长度 */
                                unsigned char* true_video_start =  video_data_start + 4;
                                //fwrite(true_video_start, 1, data_size, vfd);
                                fwrite(true_video_start, 1, data_size, vfd);

                                #if 0
                                {
                                    FILE* pfd = fopen("out.P", "ab+");
                                    if(pfd)
                                    {
                                        fwrite(buf, 1, nRead,pfd);
                                        fclose(pfd);
                                    }
                                }
                                #endif
                            }
                            
                            fclose(vfd);
                        }
                    }
                    #endif
                }

            }
        }
    }

    if(fp)
        fclose(fp);

    if(buf){
        free(buf);
    }

    if(rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        CleanupSockets();
        rtmp=NULL;
    }   
    return 0;
}