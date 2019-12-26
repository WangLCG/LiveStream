//
//  FFmpegH264Source.h
//  FFmpegRTSPServer
//
//  Created by Mina Saad on 9/22/15.
//  Copyright (c) 2015 Mina Saad. All rights reserved.
//

#ifndef MESAI_FFMPEGH264_SOURCE_HH
#define MESAI_FFMPEGH264_SOURCE_HH


#include <functional>
#include <FramedSource.hh>
#include <UsageEnvironment.hh>
#include <Groupsock.hh>
#include "RecvMulticastDataModule.h"

namespace MESAI
{
    
  class FFmpegH264Source : public FramedSource {
  public:
    static FFmpegH264Source* createNew(UsageEnvironment& env,     RecvMulticastDataModule* MulticastModule = NULL);
    FFmpegH264Source(UsageEnvironment& env,  RecvMulticastDataModule* MulticastModule = NULL);

    ~FFmpegH264Source();

  private:
    static void deliverFrameStub(void* clientData) {if(clientData)((FFmpegH264Source*) clientData)->deliverFrame();};
    virtual void doGetNextFrame();
    void deliverFrame();
    virtual void doStopGettingFrames();
    void onFrame();
    static void DelayReadFrame(FramedSource* source);
    
  private:
    EventTriggerId m_eventTriggerId;
    RecvMulticastDataModule* m_MulticastModule;
    
  };

}
#endif
