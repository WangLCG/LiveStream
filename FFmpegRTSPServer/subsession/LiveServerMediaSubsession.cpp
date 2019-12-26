//
//  LiveServerMediaSubsession.cpp
//  FFmpegRTSPServer
//
//  Created by Mina Saad on 9/22/15.
//  Copyright (c) 2015 Mina Saad. All rights reserved.
//

#include "LiveServerMediaSubsession.h"

namespace MESAI
{
    LiveServerMediaSubsession * LiveServerMediaSubsession::createNew(UsageEnvironment& env, 
        StreamReplicator* replicator, RecvMulticastDataModule* MulticastModule, Boolean reuseFirstSource)
    { 
        return new LiveServerMediaSubsession(env, replicator, MulticastModule, reuseFirstSource);
    }

    FramedSource* LiveServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
    {
        estBitrate = 500;
        //FramedSource* source = m_replicator->createStreamReplica();
        //return H264VideoStreamDiscreteFramer::createNew(envir(), source);
        FFmpegH264Source* liveSource =  FFmpegH264Source::createNew(envir(), m_MulticastModule);
        if(!liveSource)
        {
            return NULL;
        }
        return H264VideoStreamDiscreteFramer::createNew(envir(), liveSource,false);
    }

    RTPSink* LiveServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,  
                unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
    {
        return H264VideoRTPSink::createNew(envir(), rtpGroupsock,rtpPayloadTypeIfDynamic);
    }

}
