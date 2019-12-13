#include <stdio.h>
#include "include/aacenc_lib.h"
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

using namespace std;

int main()
{
    HANDLE_AACENCODER aacEncHandle;
    
    int out_size = 4096;
    unsigned char aac_buf[4096];
    memset(aac_buf, 0, out_size);
    
    unsigned char pcm_data[4096];
    
    int channel = 2;
    /* 0x01 for AAC module */

    if(aacEncOpen(&aacEncHandle, 0, channel) != AACENC_OK)
    {
        return -1;
    }

    /* 设置为AAC LC模式 */
    if(aacEncoder_SetParam(aacEncHandle, AACENC_AOT, 2) != AACENC_OK )
    {
        return -1;
    }

    /* 设置采样率 */
    if(aacEncoder_SetParam(aacEncHandle, AACENC_SAMPLERATE, 8000) != AACENC_OK )
    {
        return -1;
    }

    /* 左右声道 */
    if(aacEncoder_SetParam(aacEncHandle, AACENC_CHANNELMODE, MODE_2) != AACENC_OK )
    {
        return -1;
    }

    /* ADTS输出 */
    if(aacEncoder_SetParam(aacEncHandle, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK )
    {
        return -1;
    }
    
    if (aacEncoder_SetParam(aacEncHandle, AACENC_BITRATE, 38000) != AACENC_OK)
    {
       return -1; 
    }
    //if(aacEncoder_SetParam(aacEncHandle, AACENC_CHANNELORDER, 0) != AACENC_OK )
    //{
    //    return -1;
    //}
    
    /* 使用NULL来初始化aacEncHandle实例 */
    if(aacEncEncode(aacEncHandle, NULL, NULL, NULL, NULL ) != AACENC_OK )
    {
        return -1;
    }

    AACENC_InfoStruct info = {0};
    if(aacEncInfo(aacEncHandle, &info) != AACENC_OK )
    {
        return -1;
    }
    
    if (aacEncInfo(aacEncHandle, &info) != AACENC_OK)
    {
        fprintf(stderr, "Unable to get the encoder info\n");
        return -1;
    }

    int input_size       = channel * 2 * info.frameLength;
    
    char *input_buf      = (char*) malloc(input_size);
    short*  convert_buf  = (short*) malloc(input_size);
    
    printf("------ input_size[%d] -----\n", input_size);
    
    int in_elem_size = 2; /* 16位采样位数时一个样本2 bytes */
    int in_identifier = IN_AUDIO_DATA;

    int pcm_buf_size = 0;
    void* in_ptr        = convert_buf;
    //inargs.numInSamples = pcm_buf_size / 2 ;  /* 16bit 采样位数的样本数计算方法 */

    int out_identifier = OUT_BITSTREAM_DATA ;
    void* out_ptr      = aac_buf;
    int out_elem_size  = 1;
    
    FILE* rfd = fopen("../Audio/pcm8k.pcm", "rb+");
    if(!rfd)
    {
        return -1;
    }

    FILE* wfd = fopen("../Audio/fdk_aac_out.aac", "wb+");
    if(!wfd)
    {
        return -1;
    }

    int i = 0;
    
    printf("Begin to cnvert .......\n");
    struct timeval adts_start , adts_end;
    gettimeofday(&adts_start , NULL);

    while(!feof(rfd))
    {
        AACENC_BufDesc outBufDesc = {0};
        AACENC_BufDesc inBufDesc  = {0};
        AACENC_InArgs  inargs     = {0};
        AACENC_OutArgs  outargs   = {0};

        memset(input_buf, 0, input_size);
        memset(aac_buf, 0, out_size);
        
	/* 如果输入不是nput_size(4096)byte，则会凑足4096byte才会有输出 */
        int ret = fread(input_buf, 1, 4096, rfd); 

        /* 此处用的是16bit采样位数双通道的PCM数据，故将左右声道的低位及高位合并。详见PCM数据格式可知 */
        for (i = 0; i < ret / 2; i++)
        {
            const char* in = &input_buf[2*i];
            convert_buf[i] = in[0] | (in[1] << 8);
        }
        
        inargs.numInSamples = ret / 2;
        
        inBufDesc.numBufs    = 1;
        inBufDesc.bufs       = &in_ptr;
        inBufDesc.bufSizes   = &ret;
        inBufDesc.bufElSizes = &in_elem_size;
        inBufDesc.bufferIdentifiers = &in_identifier;
        
        outBufDesc.numBufs    = 1;
        outBufDesc.bufs       = &out_ptr;
        outBufDesc.bufSizes   = &out_size;
        outBufDesc.bufElSizes = &out_elem_size;
        outBufDesc.bufferIdentifiers = &out_identifier;

        int aac_ret = aacEncEncode(aacEncHandle, &inBufDesc, &outBufDesc, &inargs, &outargs );
        if(aac_ret != AACENC_OK )
        {
            //printf("######### Convert continue aac_ret[%x]#########\n", aac_ret);
            continue;
        }

        if(outargs.numOutBytes > 0 )
        {
            /* success output */
            fwrite(out_ptr, 1 ,outargs.numOutBytes, wfd);
            //printf("######### Convert success #########\n");
        }
        //printf("######### Converting outargs.numOutBytes[%d] #########\n", outargs.numOutBytes);
    }
    gettimeofday(&adts_end , NULL);
    long adts_cost_ms = (adts_end.tv_sec - adts_start.tv_sec) * 1000 * 1000 + (adts_end.tv_usec - adts_start.tv_usec) ;
    printf("----- cost time [%ld] us ------\n", adts_cost_ms);

    fclose(rfd);
    fclose(wfd);
    
    aacEncClose(&aacEncHandle);
    printf("############## Success ##############\n");
    
    return 0;
}
