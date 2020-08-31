#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <functional>
#include "SPSParser.h"

using namespace std;
using namespace ZL_SPS_PARSE;

/* 获取264的帧类型 */
#define H264_TYPE(v) ((uint8_t)(v) & 0x1F)

/* 获取265的帧类型 */
#define H265_TYPE(v) (((uint8_t)(v) >> 1) & 0x3f)

/* h264帧类型 */
typedef enum H264NalType
{
    NAL_SPS_264 = 7,
    NAL_PPS_264 = 8,
    NAL_IDR = 5,
    NAL_SEI = 6,
} H264NalType;

/* h265帧类型 */
typedef enum H265NaleType
{
    NAL_TRAIL_N = 0,
    NAL_TRAIL_R = 1,
    NAL_TSA_N = 2,
    NAL_TSA_R = 3,
    NAL_STSA_N = 4,
    NAL_STSA_R = 5,
    NAL_RADL_N = 6,
    NAL_RADL_R = 7,
    NAL_RASL_N = 8,
    NAL_RASL_R = 9,
    NAL_BLA_W_LP = 16,
    NAL_BLA_W_RADL = 17,
    NAL_BLA_N_LP = 18,
    NAL_IDR_W_RADL = 19,
    NAL_IDR_N_LP = 20,
    NAL_CRA_NUT = 21,
    NAL_RSV_IRAP_VCL22 = 22,
    NAL_RSV_IRAP_VCL23 = 23,

    NAL_VPS = 32,
    NAL_SPS = 33,
    NAL_PPS = 34,
    NAL_AUD = 35,
    NAL_EOS_NUT = 36,
    NAL_EOB_NUT = 37,
    NAL_FD_NUT = 38,
    NAL_SEI_PREFIX = 39,
    NAL_SEI_SUFFIX = 40,
}H265NaleType;

/* 视频信号相关信息结构体 */
typedef struct VideoInfo
{
    int width;   /* 视频宽 */
    int height;  /* 视频高 */
}VideoInfo;

static const char *memfind(const char *buf, int len, const char *subbuf, int sublen) 
{
    for (auto i = 0; i < len - sublen; ++i) 
    {
        if (memcmp(buf + i, subbuf, sublen) == 0) 
        {
            return buf + i;
        }
    }
    
    return NULL;
}

/* 
  获取H264的sps中的宽、高及帧率
  sps: 起始码之后的数据
  sps_len： 去掉起始码之后的长度
*/
static bool getAVCInfo(const char * sps,int sps_len,int &iVideoWidth, int &iVideoHeight, float  &iVideoFps)
{
    T_GetBitContext tGetBitBuf;
    T_SPS tH264SpsInfo;
    memset(&tGetBitBuf,0,sizeof(tGetBitBuf));
    memset(&tH264SpsInfo,0,sizeof(tH264SpsInfo));
    tGetBitBuf.pu8Buf = (uint8_t*)sps + 1;
    tGetBitBuf.iBufSize = sps_len - 1;
    if(0 != h264DecSeqParameterSet((void *) &tGetBitBuf, &tH264SpsInfo))
    {
        return false;
    }
    
    h264GetWidthHeight(&tH264SpsInfo, &iVideoWidth, &iVideoHeight);
    h264GeFramerate(&tH264SpsInfo, &iVideoFps);
    return true;
}

/* 获取H264或265的起始码长度 */
static int prefixSize(const char *ptr, int len)
{
    if (len < 4) 
    {
        return 0;
    }

    if (ptr[0] != 0x00 || ptr[1] != 0x00) 
    {
        //不是0x00 00开头
        return 0;
    }

    if (ptr[2] == 0x00 && ptr[3] == 0x01)
    {
        //是0x00 00 00 01
        return 4;
    }

    if (ptr[2] == 0x01)
    {
        //是0x00 00 01
        return 3;
    }
    
    return 0;
}

//////////////////////////////////////////////////////////////////////////
/// \brief      分割264数据sps、pps数据的回调函数
/// \details    分割264数据sps、pps数据的回调函数
/// \param[in]  ptr    视频帧数据
/// \param[in]  len    视频帧数据长度
/// \param[in]  prefix    起始码长度
//////////////////////////////////////////////////////////////////////////
void H264SplidCB(const char *ptr, int len, int prefix)
{
    int type = H264_TYPE(ptr[prefix]);
    //cout << "264 type " << type << endl;
    switch(type)
    {
        case NAL_SPS_264:
        {
            //const char * sps,int sps_len,int &iVideoWidth, int &iVideoHeight, float  &iVideoFps
            int iVideoWidth,iVideoHeight= 0;
            float  iVideoFps = 0.0;
            getAVCInfo(ptr + prefix, len - prefix, iVideoWidth, iVideoHeight, iVideoFps);
            cout << "############## H264 (width ,height) = (" << iVideoWidth << ", " << iVideoHeight << " ) fps =  " << iVideoFps << endl;
        }break;

        case NAL_PPS_264:
        {

        }break;

        case NAL_IDR:
        {

        }

        default:
        {
            /* 其它类型的帧 */
        }
    }
}

//std::bind(&CRecvMsgHdl::HandleMsg, m_recvMsgHdl, _1, _2)
//////////////////////////////////////////////////////////////////////////
/// \brief      分割sps、pps数据
/// \details    有些视频帧的I帧和sps、pps合在一起，故需要分割开
/// \param[in]  ptr    视频帧数据
/// \param[in]  len    视频帧数据长度
/// \param[in]  cb    分割后的数据输出回调函数
//////////////////////////////////////////////////////////////////////////
void splitH264(const char *ptr, int len, int prefix, const std::function<void(const char *, int, int)> &cb)
{
    auto start = ptr + prefix;
    auto end = ptr + len;
    int next_prefix;
    while (true)
    {
        auto next_start = memfind(start, end - start, "\x00\x00\x01", 3);
        if (next_start)
        {
            //找到下一帧
            if (*(next_start - 1) == 0x00)
            {
                //这个是00 00 00 01开头
                next_start -= 1;
                next_prefix = 4;
            } 
            else 
            {
                //这个是00 00 01开头
                next_prefix = 3;
            }

            //记得加上本帧prefix长度
            cb(start - prefix, next_start - start + prefix, prefix);
            //搜索下一帧末尾的起始位置
            start = next_start + next_prefix;
            //记录下一帧的prefix长度
            prefix = next_prefix;
            continue;
        }

        //未找到下一帧,这是最后一帧
        cb(start - prefix, end - start + prefix, prefix);
        break;
    }
}

static bool GetH264WidthAndHeight(char* data, int bytes, VideoInfo& info)
{
    if(!data || bytes <=0 )
    {
        return false;
    }
    
    int prefix_size = prefixSize(data, bytes);
    int type = H264_TYPE(data[prefix_size]);
    if(type == NAL_SPS_264 || type == NAL_SEI )
    {
        auto start = data + prefix_size;
        auto end   = data + bytes;
        int next_prefix = 0;
        char* pps_start = NULL;
        while (true) 
        {
            pps_start = memfind(start, end - start, "\x00\x00\x01", 3);
            if (pps_start) 
            {
                //找到下一帧,pps
                if (*(pps_start - 1) == 0x00) 
                {
                    //这个是00 00 00 01开头
                    pps_start -= 1;
                    next_prefix = 4;
                }
                else
                {
                    //这个是00 00 01开头
                    next_prefix = 3;
                }
            }
            
            //未找到下一帧,这是最后一帧
            break;
        }

        int sps_len = 0;
        if(pps_start)
        {
            sps_len = pps_start - (char*)data ;
            sps_len -= prefix_size;
        }
        else 
        {
            sps_len = bytes - prefix_size;
        }

        int iVideoWidth  = 0;
        int iVideoHeight = 0;
        float iVideoFps  = 0;
        bool ret = getAVCInfo(data+prefix_size, sps_len, iVideoWidth, 
            iVideoHeight, iVideoFps);
        
        if(ret)
        {
            cout << "@@@@@@@@@@ width " << iVideoWidth <<
                "height " << iVideoHeight << " fps " << iVideoFps << endl;
        }
        
        return ret;
    }
    else 
    {
        return false;
    }
}


/* 
  获取H265的宽、高及帧率
  vps ： 起始码之后的数据
  vps_len： 去掉起始码之后长度
  sps: 起始码之后的数据
  sps_len： 去掉起始码之后的长度
*/
bool getHEVCInfo(const char * vps, int vps_len,const char * sps,int sps_len,int &iVideoWidth,
         int &iVideoHeight, float  &iVideoFps)
{
    T_GetBitContext tGetBitBuf;
    T_HEVCSPS tH265SpsInfo;
    T_HEVCVPS tH265VpsInfo;
    if ( vps_len > 2 )
    {
        memset(&tGetBitBuf,0,sizeof(tGetBitBuf));	
        memset(&tH265VpsInfo,0,sizeof(tH265VpsInfo));
        tGetBitBuf.pu8Buf = (uint8_t*)vps+2;
        tGetBitBuf.iBufSize = vps_len-2;
        if(0 != h265DecVideoParameterSet((void *) &tGetBitBuf, &tH265VpsInfo)){
            return false;
        }
    }

    if ( sps_len > 2 )
    {
        memset(&tGetBitBuf,0,sizeof(tGetBitBuf));
        memset(&tH265SpsInfo,0,sizeof(tH265SpsInfo));
        tGetBitBuf.pu8Buf = (uint8_t*)sps+2;
        tGetBitBuf.iBufSize = sps_len-2;
        if(0 != h265DecSeqParameterSet((void *) &tGetBitBuf, &tH265SpsInfo)){
            return false;
        }
    }
    else 
        return false;

    h265GetWidthHeight(&tH265SpsInfo, &iVideoWidth, &iVideoHeight);
    iVideoFps = 0;
    h265GeFramerate(&tH265VpsInfo, &tH265SpsInfo, &iVideoFps);
    //ErrorL << iVideoWidth << " " << iVideoHeight << " " << iVideoFps;

    return true;
}

char* vps = NULL;     /* 不包含起始码 */
int vps_size = 0;    /* 去掉起始码之后长度 */

char* hevc_sps      = NULL;   /* 不包含起始码 */
int hevc_sps_size   = 0;  /* 去掉起始码之后长度 */

//////////////////////////////////////////////////////////////////////////
/// \brief      分割265数据sps、pps数据的回调函数
/// \details    分割265数据sps、pps数据的回调函数
/// \param[in]  ptr    视频帧数据
/// \param[in]  len    视频帧数据长度
/// \param[in]  prefix    起始码长度
//////////////////////////////////////////////////////////////////////////
void H265SplidCB(const char *ptr, int len, int prefix)
{
    int type = H265_TYPE(ptr[prefix]);
    //cout << " 265 type " << type << endl;
    switch(type)
    {
        case NAL_VPS:
        {
            int raw_len = len - prefix;
            vps = new char[raw_len];
            memset(vps, 0 , raw_len);
            memcpy(vps, ptr + prefix, raw_len);
            vps_size = raw_len;

        }break;

        case NAL_SPS:
        {
            int raw_len = len - prefix;
            hevc_sps = new char[raw_len];
            memset(hevc_sps, 0 , raw_len);
            memcpy(hevc_sps, ptr + prefix, raw_len);
            hevc_sps_size = raw_len;
        }break;

        case NAL_PPS:
        {

        }

        default:
        {
            /* 其它类型的帧 */
        }
    }

    if(vps && hevc_sps)
    {
        int iVideoWidth,iVideoHeight= 0;
        float  iVideoFps = 0.0;
        getHEVCInfo(vps, vps_size, hevc_sps, hevc_sps_size, iVideoWidth, iVideoHeight, iVideoFps);
        cout << "############# H265 (width ,height) = (" << iVideoWidth << ", " << iVideoHeight << " ) fps =  " << iVideoFps << endl;

        delete[] vps;
        delete[] hevc_sps;

        vps      = NULL;
        hevc_sps = NULL;
    }
}

void testHevc()
{
    string input_file = "../video/cus.hevc";

    FILE* rfd = fopen(input_file.c_str(), "rb");
    if(rfd)
    {
        char buf[4096];
        int len = 3203;   /* 测试的cus.hev的I帧的长度，demo的测试视频的第一帧必须是I帧 */
        memset(buf, 0, 4096);
        while(len = fread(buf, 1, 3203, rfd))
        {
            int prefix = prefixSize(buf, len);
            cout << " read length " << len << " prefix size " << prefix << endl;
            splitH264(buf, len, prefix, std::bind(H265SplidCB, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) );
            break;
        }

        fclose(rfd);
    }
    else 
    {
        cout << "1111  File no exist  " << input_file << endl;
    }
}


void testH264()
{
    string input_file = "../video/cus.264";

    FILE* rfd = fopen(input_file.c_str(), "rb");
    if(rfd)
    {
        char buf[4096];
        int len = 3203;   /* 测试的cus.264的I帧的长度，demo的测试视频的第一帧必须是I帧 */
        memset(buf, 0, 4096);
        while(len = fread(buf, 1, 3203, rfd))
        {
            int prefix = prefixSize(buf, len);
            cout << " read length " << len << " prefix size " << prefix << endl;
            splitH264(buf, len, prefix, std::bind(H264SplidCB, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) );
            break;
        }

        fclose(rfd);
    }
    else 
    {
        cout << "1111  File no exist  " << input_file << endl;
    }
}

int main()
{
    testH264();

    testHevc();

    return 0;
}