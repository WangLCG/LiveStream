#include "PCMToAAC.h"

PCM_TO_AAC::PCM_TO_AAC(int samplerate, int chanel, int sample_bit)
{
    m_PCMbitSize = sample_bit;
    m_SampleRate = samplerate;
    m_Channel    = chanel;

    m_hEncoder       = 0;
    m_InputSample    = 0;
    m_MaxOutputBytes = 0;
    
    m_hEncoder = faacEncOpen(m_SampleRate, m_Channel,&m_InputSample, &m_MaxOutputBytes);
    if (!m_hEncoder)
    {
        return;
    }

    printf("#######  m_InputSample [%d] ######## \n", m_InputSample);

    m_PCMBufSize = m_InputSample * m_PCMbitSize / 8;
    m_PtrPCMBuf = new char[m_PCMBufSize];
    m_PtrAACBuf = new char[m_MaxOutputBytes];

    m_pConfiguration = faacEncGetCurrentConfiguration(m_hEncoder);

    m_pConfiguration->inputFormat  = FAAC_INPUT_16BIT;
    m_pConfiguration->outputFormat = 1; /* 0--RAW  1--ADTS header */
    m_pConfiguration->aacObjectType = LOW;
    m_pConfiguration->allowMidside = 0;

    faacEncSetConfiguration(m_hEncoder, m_pConfiguration);

}

PCM_TO_AAC::~PCM_TO_AAC()
{
    if (m_hEncoder)
    {
        faacEncClose(m_hEncoder);
        m_hEncoder = 0;
    }

    delete[] m_PtrPCMBuf;
    delete[] m_PtrAACBuf;
}

int PCM_TO_AAC::ConvertProcess(char* pcmbuf, int PcmSize, unsigned char* aacBuf)
{
    unsigned int inputSample = PcmSize / (m_PCMbitSize / 8);

    return faacEncEncode(m_hEncoder, (int*)pcmbuf, inputSample, aacBuf, m_MaxOutputBytes);
}
