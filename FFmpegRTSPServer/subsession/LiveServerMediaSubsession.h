//
//  LiveServerMediaSubsession.h
//  FFmpegRTSPServer
//
//  Created by Mina Saad on 9/22/15.
//  Copyright (c) 2015 Mina Saad. All rights reserved.
//

#ifndef MESAI_Live_SERVER_MEDIA_SUBSESSION_HH
#define MESAI_Live_SERVER_MEDIA_SUBSESSION_HH

#include <OnDemandServerMediaSubsession.hh>
#include <StreamReplicator.hh>
#include <H264VideoRTPSink.hh>
#include <H264VideoStreamFramer.hh>
#include <H264VideoStreamDiscreteFramer.hh>
#include <UsageEnvironment.hh>
#include <Groupsock.hh>
#include "RecvMulticastDataModule.h"
#include "FFmpegH264Source.h"

namespace MESAI 
{

  class LiveServerMediaSubsession: public OnDemandServerMediaSubsession
  {
    public:
      static LiveServerMediaSubsession* createNew(UsageEnvironment& env, StreamReplicator* replicator,
                RecvMulticastDataModule* MulticastModule=NULL, Boolean reuseFirstSource=False);

    protected:
      LiveServerMediaSubsession(UsageEnvironment& env, StreamReplicator* replicator,
            RecvMulticastDataModule* MulticastModule=NULL,  Boolean reuseFirstSource=False)
          : OnDemandServerMediaSubsession(env, reuseFirstSource), m_replicator(replicator),m_MulticastModule(MulticastModule) {};
      
      virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
      virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,  unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);    

      StreamReplicator * m_replicator;
      RecvMulticastDataModule* m_MulticastModule;
  };

}
#endif