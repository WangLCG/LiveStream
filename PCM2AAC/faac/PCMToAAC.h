//////////////////////////////////////////////////////////////////////////
/// \file       PCMToAAC.h
/// \brief      PCM转AAC类声明
/// \author     
/// \version    
///             [V1.0] 2019/11/24, draft
/// \copyright  
///             
//////////////////////////////////////////////////////////////////////////

#pragma once
#include <faac.h>
#include <string>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

class PCM_TO_AAC
{
public:
    //////////////////////////////////////////////////////////////////////////
    /// \brief  构造函数
    /// \remark 
    /// \param[in]  samplerate  采样率
    /// \param[in]  chanel      通道号
    /// \param[in]  sample_bit   采样位数
    //////////////////////////////////////////////////////////////////////////
    PCM_TO_AAC(int samplerate, int chanel, int sample_bit);

    ~PCM_TO_AAC();

    //////////////////////////////////////////////////////////////////////////
    /// \brief  PCM到AAC的转化
    /// \remark 
    /// \param[in]  pcmbuf       待转化的PCM数据，该数据大小必须等于InputSample * 采样位数 / 8，否则转化会失败
    /// \param[in]  PcmSize      待转化pcm数据大小,该数据大小必须等于InputSample * 采样位数 / 8
    /// \param[in]  aacBuf       aac数据缓存buf
    /// \return  成功--返回aac数据大小，小于等于0时视为失败
    //////////////////////////////////////////////////////////////////////////
    int ConvertProcess(char* pcmbuf, int PcmSize, unsigned char* aacBuf);

private:
    faacEncHandle   m_hEncoder ;
    unsigned long   m_InputSample ;
    unsigned long   m_MaxOutputBytes;
    int             m_PCMbitSize;
    int             m_SampleRate;
    int             m_Channel;

    int m_PCMBufSize;
    char* m_PtrPCMBuf;
    char* m_PtrAACBuf;

    faacEncConfigurationPtr m_pConfiguration;
};
