#ifndef ADTSSERVERMEDIASUBSESSION_H
#define ADTSSERVERMEDIASUBSESSION_H

#include "ADTSAudioFileServerMediaSubsession.hh"
#include "MPEG4GenericRTPSink.hh"
#include "AdtsAACAudioStreamSource.h"

class AdtsAACAudioStreamServerMediaSubsession : public ADTSAudioFileServerMediaSubsession
{
public:
  static AdtsAACAudioStreamServerMediaSubsession*
  createNew(UsageEnvironment& env,  Boolean reuseFirstSource);

protected:
    AdtsAACAudioStreamServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource);
    ~AdtsAACAudioStreamServerMediaSubsession();

private:
protected: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
                          unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
                    FramedSource* inputSource);

private:
    char fileName[20];
};

#endif // ADTSSERVERMEDIASUBSESSION_H
