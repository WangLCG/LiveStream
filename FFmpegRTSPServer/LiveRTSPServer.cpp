//
//  LiveRTSPServer.cpp
//  FFmpegRTSPServer
//
//  Created by Mina Saad on 9/22/15.
//  Copyright (c) 2015 Mina Saad. All rights reserved.
//

#include "LiveRTSPServer.h"

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
               char const* streamName, char const* inputFileName);

namespace MESAI
{
    LiveRTSPServer::LiveRTSPServer(  int port, int httpPort )
        : portNumber(port), httpTunnelingPort(httpPort)
    {
        quit = 0;
    }

    LiveRTSPServer::LiveRTSPServer(RecvMulticastDataModule* MulticastModule, 
            int port, int httpPort )
            : m_MulticastModule(MulticastModule), portNumber(port), httpTunnelingPort(httpPort)
    {
        quit = 0;
    }
    
    LiveRTSPServer::~LiveRTSPServer()
    {

    }

    void LiveRTSPServer::run()
    {
        TaskScheduler    *scheduler;
        UsageEnvironment *env ;
        char RTSP_Address[1024];
        RTSP_Address[0]=0x00;

        scheduler = BasicTaskScheduler::createNew();
        env = BasicUsageEnvironment::createNew(*scheduler);
        
        UserAuthenticationDatabase* authDB = NULL;
        
        // if (m_Enable_Pass){
        // 	authDB = new UserAuthenticationDatabase;
        // 	authDB->addUserRecord(UserN, PassW);
        // }
        
        OutPacketBuffer::maxSize = 2*1024*1024;
        /* 第四个参数 0 ，表示一直保持连接，否则默认持续 65s 都没有rtsp的通信，就关掉rtsp socket */
        RTSPServer* rtspServer = RTSPServer::createNew(*env, portNumber, authDB, 0);
        
        if (rtspServer == NULL)
        {
            *env <<"LIVE555: Failed to create RTSP server: "<<  env->getResultMsg() << "\n";
        }
        else
        {
            if(httpTunnelingPort)
            {
                rtspServer->setUpTunnelingOverHTTP(httpTunnelingPort);
            }
            
            char const* descriptionString = "MESAI Streaming Session";

            //printf("RTSP_Address = %s \n");
            memset(RTSP_Address, 0, 1024);
            snprintf(RTSP_Address, 1024, "%s", "ch0");

            bool ReusedFirstSource = True;
            /* 此处可以添加多个ServerMediaSession实例，每个实例代表一个独立的rtsp通道 */
            ServerMediaSession* sms = ServerMediaSession::createNew(*env, RTSP_Address, RTSP_Address, descriptionString);
            sms->addSubsession(MESAI::LiveServerMediaSubsession::createNew(*env, NULL,m_MulticastModule, ReusedFirstSource));
            
            //sms->addSubsession( G711AudioStreamServerMediaSubsession::createNew(*env, false) );
            sms->addSubsession( AdtsAACAudioStreamServerMediaSubsession::createNew(*env, ReusedFirstSource, m_MulticastModule) );
            rtspServer->addServerMediaSession(sms);
            
            announceStream(rtspServer, sms, "ch0", "2.avi");
            
            //signal(SIGNIT,sighandler);
            env->taskScheduler().doEventLoop(&quit); // does not return
            
            Medium::close(rtspServer);
        }
        
        env->reclaim();
        delete scheduler;
	}
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName)
{
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  env << "\n\"" << streamName << "\" stream, from the file \""
      << inputFileName << "\"\n";
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;
}
