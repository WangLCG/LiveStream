////////////////////////////////////////////////////////////////////////
/// @file H264FrameTypeParse.h
/// @brief H264视频帧类型解析 <br>
/// @author 
/// @copyright (c) 2016-2019 
////////////////////////////////////////////////////////////////////////
#ifndef _H264_FRAME_TYPE_PARSER_H_
#define _H264_FRAME_TYPE_PARSER_H_

//nal类型
enum H264_NALU_TYPE
{
    H264_NAL_UNKNOWN        = 0,
    H264_NAL_SLICE          = 1,
    H264_NAL_SLICE_DPA      = 2,
    H264_NAL_SLICE_DPB      = 3,
    H264_NAL_SLICE_DPC      = 4,
    H264_NAL_SLICE_IDR      = 5,    /* ref_idc != 0 */
    H264_NAL_SEI            = 6,    /* ref_idc == 0 */
    H264_NAL_SPS            = 7,
    H264_NAL_PPS            = 8
    /* ref_idc == 0 for 6,9,10,11,12 */
};

enum H264_FRAME_TYPE
{
    H264_FRAME_NALU         = 0,        ///< 参数集类NALU
    H264_FRAME_I            = 1,        ///< I帧
    H264_FRAME_SI           = 2,        ///< I帧
    H264_FRAME_P            = 3,        ///< P帧
    H264_FRAME_SP           = 4,        ///< P帧
    H264_FRAME_B            = 5,        ///< B帧
    H264_FRAME_UNKNOWN
};


//////////////////////////////////////////////////////////////////////////
/// \brief  从264一帧的码流中获取帧类型
/// \param[in]  bitstream       指向码流缓冲的指针
/// \param[in]  bitstreamSize   码流缓冲的长度
/// \param[in]  startCodeSize   NALU起始码长度，3的时候为001, 4的时候为0001，其他无效
/// \return 帧类型代号           \see H264_FRAME_TYPE
///
/// \remark 参数集类NALU会被忽略，输入数据必须具备NALU起始码
//////////////////////////////////////////////////////////////////////////
int H264_GetFrameType(const unsigned char* bitstream,
                      int bitstreamSize,
                      int startCodeSize = 4);


#endif  // !_H264_FRAME_TYPE_PARSER_H_

