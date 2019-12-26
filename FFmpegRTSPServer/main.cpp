//
//  main.cpp
//  FFmpegRTSPServer
//
//  Created by Mina Saad on 9/22/15.
//  Copyright (c) 2015 Mina Saad. All rights reserved.
//

#include "LiveRTSPServer.h"
#include <signal.h>
#include <unistd.h>

MESAI::LiveRTSPServer * server     = NULL;

int UDPPort        = 554 ;
int HTTPTunnelPort = 80;
pthread_t thread1  = -1;
pthread_t thread2  = -1;

bool Exit = false;

void ctrl_C_handler(int s)
{
    printf("Caught signal %d\n", s);
    Exit = true;
}

void * runServer(void * server)
{
    ((MESAI::LiveRTSPServer * ) server)->run();
    pthread_exit(NULL);
}

int main(int argc, const char * argv[])
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrl_C_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    UDPPort        = 554;
    HTTPTunnelPort = 82;
    
    char cmd[100]={0};
    sprintf(cmd, "route add -net 225.18.0.0 netmask 255.255.0.0 %s", "eth0");
    system(cmd);
    
    RecvMulticastDataModule* MulticastModule = NULL;
    MulticastModule = new RecvMulticastDataModule();
    MulticastModule->AddMultGroup("225.18.1.0", 0);  /* video */
    MulticastModule->AddMultGroup("225.18.1.1", 1);  /* audio */
            
    server = new MESAI::LiveRTSPServer(MulticastModule, UDPPort, HTTPTunnelPort);
    
    pthread_attr_t attr1;
    pthread_attr_init(&attr1);
    pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
    int rc1 = pthread_create(&thread1, &attr1,  runServer, server);
    if (rc1)
    {
        //exception
        return -1;
    }

    
    while(1)
    {
        if(Exit )
            break;
        sleep(1);
    }

    if(MulticastModule)
    {
        delete MulticastModule;
        MulticastModule = NULL;
    }
}

