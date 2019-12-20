#include "AdtsAACAudioStreamServerMediaSubsession.h"


AdtsAACAudioStreamServerMediaSubsession* AdtsAACAudioStreamServerMediaSubsession::
  createNew(UsageEnvironment& env,  Boolean reuseFirstSource)
{
    return new AdtsAACAudioStreamServerMediaSubsession(env,reuseFirstSource) ;
}


AdtsAACAudioStreamServerMediaSubsession::AdtsAACAudioStreamServerMediaSubsession(UsageEnvironment &env, Boolean reuseFirstSource)
    :ADTSAudioFileServerMediaSubsession(env , fileName, reuseFirstSource)
{

}

AdtsAACAudioStreamServerMediaSubsession::~AdtsAACAudioStreamServerMediaSubsession()
{
}


FramedSource* AdtsAACAudioStreamServerMediaSubsession::createNewStreamSource(unsigned clientSessionId,
                        unsigned& estBitrate)
{
    estBitrate = 96; // kbps, estimate

    return AdtsAACAudioStreamSource::createNew(envir());
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
