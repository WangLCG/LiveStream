//
//  LiveRTSPServer.h
//  FFmpegRTSPServer
//
//  Created by Mina Saad on 9/22/15.
//  Copyright (c) 2015 Mina Saad. All rights reserved.
//

#include <UsageEnvironment.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <liveMedia.hh>
#include "LiveServerMediaSubsession.h"
#include "FFmpegH264Source.h"
#include "G711AudioStreamServerMediaSubsession.hh"
#include "AdtsAACAudioStreamServerMediaSubsession.h"

namespace MESAI {

    class LiveRTSPServer
    {
    public:

        LiveRTSPServer(int port, int httpPort );
        LiveRTSPServer(RecvMulticastDataModule* MulticastModule, int port, int httpPort );
        ~LiveRTSPServer();
        void run();

    private:
        int portNumber;
        int httpTunnelingPort;
        RecvMulticastDataModule* m_MulticastModule;
        char quit;

    };
}