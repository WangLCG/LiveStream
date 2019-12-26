#include "AdtsAACAudioStreamServerMediaSubsession.h"


AdtsAACAudioStreamServerMediaSubsession* AdtsAACAudioStreamServerMediaSubsession::
  createNew(UsageEnvironment& env,  Boolean reuseFirstSource)
{
    return new AdtsAACAudioStreamServerMediaSubsession(env,reuseFirstSource) ;
}

AdtsAACAudioStreamServerMediaSubsession* AdtsAACAudioStreamServerMediaSubsession::
    createNew(UsageEnvironment& env,    Boolean reuseFirstSource, RecvMulticastDataModule* MulticastModule)
{
    return new AdtsAACAudioStreamServerMediaSubsession(env,reuseFirstSource, MulticastModule) ;
}


AdtsAACAudioStreamServerMediaSubsession::AdtsAACAudioStreamServerMediaSubsession(UsageEnvironment &env, Boolean reuseFirstSource)
    :ADTSAudioFileServerMediaSubsession(env , fileName, reuseFirstSource)
{
    m_MulticastModule = NULL;
}

AdtsAACAudioStreamServerMediaSubsession::AdtsAACAudioStreamServerMediaSubsession(UsageEnvironment &env, 
        Boolean reuseFirstSource, RecvMulticastDataModule* MulticastModule)
    :ADTSAudioFileServerMediaSubsession(env , fileName, reuseFirstSource)
{
    m_MulticastModule = MulticastModule;
}


AdtsAACAudioStreamServerMediaSubsession::~AdtsAACAudioStreamServerMediaSubsession()
{
}


FramedSource* AdtsAACAudioStreamServerMediaSubsession::createNewStreamSource(unsigned clientSessionId,
                        unsigned& estBitrate)
{
    estBitrate = 96; // kbps, estimate

    if(m_MulticastModule)
    {
        return AdtsAACAudioStreamSource::createNew(envir(), m_MulticastModule);
    }
    else
    {
        return AdtsAACAudioStreamSource::createNew(envir());
    }
}

RTPSink* AdtsAACAudioStreamServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
                                  unsigned char rtpPayloadTypeIfDynamic,
                  FramedSource* inputSource)
{
    AdtsAACAudioStreamSource* AdtsAACSource = (AdtsAACAudioStreamSource*)inputSource;
    return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
                      rtpPayloadTypeIfDynamic,
                      AdtsAACSource->samplingFrequency(),
                      "audio", "AAC-hbr", AdtsAACSource->configStr(),
                      AdtsAACSource->numChannels());
}
