////////////////////////////////////////////////////////////////////////
/// @file       H264Tools.h
/// @brief      判断h264码流工具 
/// @details    判断h264码流工具
/// @author     
/// @version    0.1
/// @date       2019/08/27
/// @copyright  (c) 1993-2019
////////////////////////////////////////////////////////////////////////

#ifndef H264_TOOL1_H
#define H264_TOOL1_H

//frame类型
enum H264_FRAME_TYPE
{
    H264_FRAME_NALU = 0,        ///< 参数集类NALU
    H264_FRAME_I    = 1,        ///< I帧
    H264_FRAME_SI   = 2,        ///< I帧
    H264_FRAME_P    = 3,        ///< P帧
    H264_FRAME_SP   = 4,        ///< P帧
    H264_FRAME_B    = 5,        ///< B帧
    H264_FRAME_UNKNOWN
};

//nal类型
enum H264_NALU_TYPE
{
    H264_NAL_UNKNOWN = 0,
    H264_NAL_SLICE = 1,
    H264_NAL_SLICE_DPA = 2,
    H264_NAL_SLICE_DPB = 3,
    H264_NAL_SLICE_DPC = 4,
    H264_NAL_SLICE_IDR = 5,    /* ref_idc != 0 */
    H264_NAL_SEI = 6,    /* ref_idc == 0 */
    H264_NAL_SPS = 7,
    H264_NAL_PPS = 8
    /* ref_idc == 0 for 6,9,10,11,12 */
};

//nal类型
enum H265_NALU_TYPE
{
    H265_NAL_UNKNOWN    = 0,
    H265_NAL_VPS        = 32,
    H265_NAL_SPS        = 33,
    H265_NAL_PPS        = 34,
    H265_NAL_BLA_N_LP   = 18,
    H265_NAL_BLA_W_LP   = 16,
    H265_NAL_BLA_W_RADL = 17,
    H265_NAL_CRA_NUT    = 21,
    H265_NAL_IDR_N_LP   = 20,
    H265_NAL_IDR_W_RADL = 19
};


typedef struct
{
    unsigned char*  p_start;
    unsigned char*  p;
    unsigned char*  p_end;
    int             i_left;
}
bs_t;

typedef unsigned char uint8_t;
void bs_init(bs_t *s, void *p_data, int i_data);
int bs_read(bs_t *s, int i_count);
int bs_read1(bs_t *s);
int bs_read_ue(bs_t *s);
int H264_GetFrameType(uint8_t* bitstream, int bitstreamSize, int startCodeSize);
bool Hevc_IsIFrame(const char* buf, int buf_size);

#endif

