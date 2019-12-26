////////////////////////////////////////////////////////////////////////
/// @file       RecvMulticastDataModule.cpp
/// @brief      接收盒子音视频组播数据的定义
/// @details    接收盒子音视频组播数据的定义
/// @author     王超
/// @version    1.0
/// @date       2019/12/20
/// @copyright  (c) 1993-2020 。保留所有权利
////////////////////////////////////////////////////////////////////////

#include "RecvMulticastDataModule.h"

RecvMulticastDataModule::RecvMulticastDataModule()
{
    pthread_mutex_init(&m_AudioLock,NULL);
    pthread_mutex_init(&m_VideoLock,NULL);
    
    m_audioLoopId = -1;
    m_videoLoopId = -1;

    m_StopAudioThread = false;
    m_StopVideoThread = false;

    m_AudioSocketFd = -1;
    m_VideoSocketFd = -1;

    pthread_create(&m_audioLoopId, NULL, RecvMulticastDataModule::audioLoop, (void *)this);
    pthread_create(&m_videoLoopId, NULL, RecvMulticastDataModule::videoLoop, (void *)this);

    m_TmpVideoFrameBuf = NULL;
    m_TmpVideoFrameBuf = new char [MAX_VIDEO_FRAME_SIZE];
    memset(m_TmpVideoFrameBuf, 0, MAX_VIDEO_FRAME_SIZE);
    m_UsedVideoFrameBufSize = 0;
}

RecvMulticastDataModule::~RecvMulticastDataModule()
{
    m_StopAudioThread = true;
    m_StopVideoThread = true;
    
    pthread_mutex_lock(&m_VideoLock);
    while(m_VideoMutilQueue.size() > 0)
    {
        MultiFrameInfo unit = m_VideoMutilQueue.front();
        if(unit.buf!=NULL) delete[] unit.buf;
        m_VideoMutilQueue.pop();
    }
    pthread_mutex_unlock(&m_VideoLock);

    pthread_mutex_lock(&m_AudioLock);
    while(m_AudioMutilQueue.size() > 0)
    {
        MultiFrameInfo unit = m_AudioMutilQueue.front();
        if(unit.buf!=NULL) delete[] unit.buf;
        m_AudioMutilQueue.pop();
    }
    pthread_mutex_unlock(&m_AudioLock);
    
    pthread_mutex_destroy(&m_AudioLock);
    pthread_mutex_destroy(&m_VideoLock);

    if(m_TmpVideoFrameBuf)
    {
        delete[] m_TmpVideoFrameBuf;
        m_TmpVideoFrameBuf = NULL;
    }
    
}

void * RecvMulticastDataModule::audioLoop(void *ptr)
{
    if( !ptr ) 
        return NULL;
    
    pthread_detach(pthread_self());
    RecvMulticastDataModule *p=(RecvMulticastDataModule *)ptr;
    p->audioLoopFunc();
    pthread_exit(NULL);
    
    return NULL;
}

void RecvMulticastDataModule::audioLoopFunc()
{
    InitAudioFd();
    
    sockaddr_in remoteaddr;
    int sockeLen = sizeof(remoteaddr);
    long recvNum = 0;
    timeval timer={0,100};

    while(!m_StopAudioThread)
    {
        timer={0,100};
        select(0,NULL,NULL,NULL,&timer);
        unsigned char *pBuffer = m_RecvAudioBur;

        recvNum = recvfrom(m_AudioSocketFd, m_RecvAudioBur, sizeof(m_RecvAudioBur), 0, (sockaddr*)&remoteaddr, (socklen_t*)&sockeLen);
        if(recvNum <= 0)
        {
            printf("recvNum=%d\n", recvNum);
            continue;
        }
        else
        {
            if (MSG_FLAG == pBuffer[0])
            {
                analyseHead(pBuffer, recvNum);
            }
        }
    }

    close(m_AudioSocketFd);
    m_AudioSocketFd = -1;

}

void * RecvMulticastDataModule::videoLoop(void *ptr)
{
    if(!ptr ) 
        return NULL;
    
    pthread_detach(pthread_self());
    RecvMulticastDataModule *p=(RecvMulticastDataModule *)ptr;
    
    p->videoLoopFunc();
    pthread_exit(NULL);
    
    return NULL;
}

void RecvMulticastDataModule::videoLoopFunc()
{
    InitVedioFd();
    sockaddr_in remoteaddr;
    int sockeLen = sizeof(remoteaddr);
    long recvNum = 0;
    timeval timer={0,50};

    int total_length = 0;
    
    while(!m_StopVideoThread)
    {
        timer={0,50};
        select(0,NULL,NULL,NULL,&timer);
        
        try
        {
            unsigned char *pBuffer = m_RecvVideoBur;
            recvNum = recvfrom(m_VideoSocketFd,m_RecvVideoBur,sizeof(m_RecvVideoBur),0,(sockaddr*)&remoteaddr,(socklen_t*)&sockeLen);

            if(recvNum <= 0)
            {
                continue;
            }
            else
            {
                if (MSG_FLAG == pBuffer[0])
                {
                    analyseHead(pBuffer, recvNum);
                }
            }
        }catch (exception& e)
        {
            printf(" Handler RecvMediaData Thread CRITIC ISSUE !!!%s", e.what());
        }
    }
    
    close(m_VideoSocketFd);
    m_VideoSocketFd = -1;
    
}

void RecvMulticastDataModule::InitAudioFd()
{
    m_AudioSocketFd = socket(AF_INET,SOCK_DGRAM,0);

    if(m_AudioSocketFd == -1)
    {
        cout<<"fail to get audio socket"<<endl;
        return ;
    }

    sockaddr_in addr;

    memset(&addr,0,sizeof(addr));
    int addrlen = sizeof(sockaddr_in);
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
    addr.sin_port=htons(19830);
    unsigned int yes = 1;

    if (setsockopt(m_AudioSocketFd, SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    {
        cout << "Reusing ADDR failed" << endl;
        close(m_AudioSocketFd);
        m_AudioSocketFd = -1;
        return ;
    }
    
    //struct timeval tv_out;
    //tv_out.tv_sec = 3;//等待3秒
    //tv_out.tv_usec = 0;
    //setsockopt(audioFd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));

    if(::bind(m_AudioSocketFd,(struct sockaddr *)&addr,sizeof(addr)) == -1)
    {
        cout<<"bind the audio port fail"<<endl;
        close(m_AudioSocketFd);
        m_AudioSocketFd = -1;
        return;
    }
    
    //isaudioReady = 1;
}

void RecvMulticastDataModule::InitVedioFd()
{
    m_VideoSocketFd=socket(AF_INET,SOCK_DGRAM,0);

    if(m_VideoSocketFd == -1)
    {
        cout<<"fail to get vediosocket"<<endl;
        return ;
    }

    sockaddr_in addr;

    memset(&addr,0,sizeof(addr));
    int addrlen = sizeof(sockaddr_in);
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
    addr.sin_port=htons(19827);
    unsigned int yes=1;

    int recv_size = 2048*1024;
    int optlen = sizeof(recv_size);
    int  err = setsockopt(m_VideoSocketFd,SOL_SOCKET,SO_RCVBUF,&recv_size,optlen);
    
    //struct timeval tv_out;
    //tv_out.tv_sec = 3;//等待3秒
    //tv_out.tv_usec = 0;
    //setsockopt(videoFd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));

    if (setsockopt(m_VideoSocketFd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0)
    {
        cout<<"Reusing ADDR failed"<<endl;
        close(m_VideoSocketFd);
        m_VideoSocketFd = -1;
        return ;
    }
    
    if(::bind(m_VideoSocketFd,(struct sockaddr *)&addr,sizeof(addr)) == -1)
    {
        cout<<"bind the vedioport fail"<<endl;
        close(m_VideoSocketFd);
        m_VideoSocketFd = -1;
        return;
    }
    
    //isvideoReady = 1;
}

int RecvMulticastDataModule::AddMultGroup(const char * mult_addr, int style)
{
    if(!mult_addr)
    {
        return -1;
    }
    
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_multiaddr.s_addr = inet_addr(mult_addr);
    
    cout << "setsockopt IP_ADD_MEMBERSHIP----- " << mult_addr << endl;
    
    // 加入组播
    if(style == STREAM_AUDIO)
    {
        if (setsockopt(m_AudioSocketFd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&mreq.imr_interface.s_addr, sizeof(struct in_addr)) < 0)
        {
            //ERROR_LOG("failed(errno=%u) to enable multi-cast on the interface", errno);
            return -1;
        }
        if (setsockopt(m_AudioSocketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0)
        {
            //DEBUG_LOG("setsockopt IP_ADD_MEMBERSHIP fails(errno=%u)",errno);
            //fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }
    }
    else if (style == STREAM_VIDEO)
    {
        if (setsockopt(m_VideoSocketFd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&mreq.imr_interface.s_addr, sizeof(struct in_addr)) < 0)
        {
            cout << "failed(errno=%u) to enable multi-cast on the interface"<< errno;
            return -1;
        }
        if (setsockopt(m_VideoSocketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0)
        {
            cout << "setsockopt IP_ADD_MEMBERSHIP fails(errno=%u)" << errno;
            //fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }
    }
    
    return 0;
}

int RecvMulticastDataModule::QuitMultGroup(const char * mult_addr, int style)
{
    if(!mult_addr)
        return -1;
    
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_multiaddr.s_addr = inet_addr(mult_addr);
    if(style == STREAM_AUDIO)
    {
        if (setsockopt(m_AudioSocketFd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0)
        {
            //DEBUG_LOG("setsockopt IP_DROP_MEMBERSHIP fails(errno=%u)",errno);
            //fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }
    }
    else if (style == STREAM_VIDEO)
    {
        if (setsockopt(m_VideoSocketFd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0)
        {
            //DEBUG_LOG("setsockopt IP_DROP_MEMBERSHIP fails(errno=%u)",errno);
            //fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }
    }
    
    return 0;
}

int RecvMulticastDataModule::analyseHead(unsigned char* dataBuf, unsigned int dataLen)
{
    if(!dataBuf)
    {
        return -1;
    }
    
    VDataMsgHeader *header    = (VDataMsgHeader *)dataBuf;
    msgNtoh(header);
    
    unsigned short frame_id   = header->fragment & 0x3fff;  /* 帧内分片ID */
    unsigned short frame_flag = header->fragment >> 14;     /* 帧的结束标志 0--结束，1--未结束 */
    
    //char destIp[50]={0};
    //unsigned int ip4 = (header->destination&0xff);
    //unsigned int ip3 = (header->destination&0xff00)>>8;
    //unsigned int ip2 = (header->destination&0xff0000)>>16;
    //unsigned int ip1 = (header->destination&0xff000000)>>24;
    //unsigned int longTime = header->seconds*1000+header->nanoseconds/1000;
    //sprintf(destIp,"%u.%u.%u.%u",ip1,ip2,ip3,ip4); 

    //cout << "RecvDataModule::analyseHead " <<destIp<<" seq =" <<
    //header->sequence_id<<  " dataLen"<<dataLen<<" header data length "<<header->length<<endl;
    
    int nau_type = (int)header->type;

    MultiFrameInfo frame;
    memset(&frame, 0 , sizeof(MultiFrameInfo));
    frame.sequence_id = header->sequence_id;

    //printf("====== nau_type[%d]  data_length[%u] ======\n", nau_type, header->length - VTM_HEADER_LEN);
    if(STREAM_VIDEO == nau_type)
    {
    
#if 0
        FILE* wfd = fopen("rtsp_raw.264","ab+");
        if(wfd)
        {
            fwrite(dataBuf + VTM_HEADER_LEN, 1, header->length - VTM_HEADER_LEN, wfd);
            fclose(wfd);
            wfd = NULL;
        }
#endif
        if(0 == frame_id && 0 == frame_flag)
        {
            frame.buf_size = header->length - VTM_HEADER_LEN;  /* 去掉头部字节 */
            frame.buf      = new char[frame.buf_size];
            memcpy(frame.buf, dataBuf + VTM_HEADER_LEN, frame.buf_size);
            PushVideoFrame(frame);
            
            m_UsedVideoFrameBufSize = 0;
#if 0
            FILE* wfd = fopen("rtsp_test.264","ab+");
            if(wfd)
            {
                fwrite(frame.buf, 1, frame.buf_size, wfd);
                fclose(wfd);
                wfd = NULL;
            }
#endif
        }
        else
        {
            /* 有可能分片 */
            if(frame_flag)
            {
                //printf("----- this[%p], m_UsedVideoFrameBufSize = %u data_length[%u] frame_id[%u]------\n", this,
                //    m_UsedVideoFrameBufSize, header->length - VTM_HEADER_LEN, frame_id);
                
                memcpy(m_TmpVideoFrameBuf + m_UsedVideoFrameBufSize, dataBuf + VTM_HEADER_LEN, header->length - VTM_HEADER_LEN);
                
                MultiFrameInfo TmpFrameInfo;
                TmpFrameInfo.buf_size = header->length - VTM_HEADER_LEN;  /* 去掉头部字节 */
                TmpFrameInfo.buf      = m_TmpVideoFrameBuf + m_UsedVideoFrameBufSize;
                memcpy(TmpFrameInfo.buf, dataBuf + VTM_HEADER_LEN, TmpFrameInfo.buf_size);
                m_TmpVideoFrameMap.insert(pair<int, MultiFrameInfo>(frame_id, TmpFrameInfo));

                m_UsedVideoFrameBufSize += header->length - VTM_HEADER_LEN;
                //printf("1111 m_TmpVideoFrameMap.size[%d] \n", m_TmpVideoFrameMap.size());
            }
            else
            {
                memcpy(m_TmpVideoFrameBuf + m_UsedVideoFrameBufSize, dataBuf + VTM_HEADER_LEN, header->length - VTM_HEADER_LEN);
                
                MultiFrameInfo TmpFrameInfo;
                TmpFrameInfo.buf_size = header->length - VTM_HEADER_LEN;  /* 去掉头部字节 */
                TmpFrameInfo.buf      = m_TmpVideoFrameBuf + m_UsedVideoFrameBufSize;
                memcpy(TmpFrameInfo.buf, dataBuf + VTM_HEADER_LEN, TmpFrameInfo.buf_size);
                m_TmpVideoFrameMap.insert(pair<int, MultiFrameInfo>(frame_id, TmpFrameInfo));

                m_UsedVideoFrameBufSize += header->length - VTM_HEADER_LEN;
                //printf("this[%p], m_UsedVideoFrameBufSize = %u data_length[%u] \n", this,
                //    m_UsedVideoFrameBufSize, header->length - VTM_HEADER_LEN);
                //printf("2222 m_TmpVideoFrameMap.size[%d] \n", m_TmpVideoFrameMap.size());
                
                frame.buf = new char [m_UsedVideoFrameBufSize];
                memset(frame.buf, 0, m_UsedVideoFrameBufSize);
                frame.buf_size = 0;
                
                map<int, MultiFrameInfo>::iterator temp_Iter;
                for (temp_Iter = m_TmpVideoFrameMap.begin(); temp_Iter != m_TmpVideoFrameMap.end(); temp_Iter++)
                {
                    MultiFrameInfo tmpdata = temp_Iter->second;
                    memcpy(frame.buf+frame.buf_size, tmpdata.buf, tmpdata.buf_size);
                    frame.buf_size += tmpdata.buf_size;
                }

                //printf("this[%p] frame.buf_size = %u \n", this, frame.buf_size);
                if(frame.buf_size)
                {
                    PushVideoFrame(frame);
#if 0
                    FILE* wfd = fopen("rtsp_test.264","ab+");
                    if(wfd)
                    {
                        fwrite(frame.buf, 1, frame.buf_size, wfd);
                        fclose(wfd);
                        wfd = NULL;
                    }
#endif
                }

                memset(m_TmpVideoFrameBuf, 0, MAX_VIDEO_FRAME_SIZE);
                m_UsedVideoFrameBufSize = 0;
                m_TmpVideoFrameMap.erase(m_TmpVideoFrameMap.begin(), m_TmpVideoFrameMap.end());
                
            }
            
        }
        
    }
    else if(STREAM_AUDIO == nau_type)
    {
        frame.buf_size = header->length - VTM_HEADER_LEN;  /* 去掉头部字节 */
        frame.buf      = new char[frame.buf_size];
        memcpy(frame.buf, dataBuf + VTM_HEADER_LEN, frame.buf_size);

        //printf("----- this[%p] data_length[%u] frame_id[%u]------\n", this, header->length - VTM_HEADER_LEN, frame_id);
#if 0
        {
            FILE* wfd = fopen("test_raw.aac", "ab+");
            if(wfd)
            {
                fwrite(frame.buf, 1, frame.buf_size, wfd);
                fclose(wfd);
                wfd = NULL;
            }
        }
#endif
        PushAudioFrame(frame);
    }
    
    //if (dataLen != header->length)
    //{
    //    cout << " lenth error" << endl;
    //}
    
    return 0;
}

void RecvMulticastDataModule::msgNtoh(VDataMsgHeader* header)
{
    if(!header)
        return;
    
    header->length      = ntohs(header->length);
    header->sequence_id = ntohs(header->sequence_id);
    header->fragment    = ntohs(header->fragment);
    header->destination = ntohl(header->destination);
}

void RecvMulticastDataModule::PushAudioFrame(MultiFrameInfo& audio)
{
    pthread_mutex_lock(&m_AudioLock);
    m_AudioMutilQueue.push(audio);
    pthread_mutex_unlock(&m_AudioLock);
}

void RecvMulticastDataModule::PushVideoFrame(MultiFrameInfo& video)
{
    pthread_mutex_lock(&m_VideoLock);
    m_VideoMutilQueue.push(video);
    pthread_mutex_unlock(&m_VideoLock);

#if 0
    try
    {
        //m_onVideoFrame();
        //cout << "##### Get next video frame #####" << endl;
    }
    catch(std::bad_function_call& e)
    {
        cout << "error: null m_onVideoFrame function" << endl;
    }
#endif
}

void RecvMulticastDataModule::GetAudioFrame(MultiFrameInfo& audio)
{
    pthread_mutex_lock(&m_AudioLock);
    if(m_AudioMutilQueue.size() > 0)
    {
        audio = m_AudioMutilQueue.front();
        m_AudioMutilQueue.pop();
    }
    else
    {
        memset(&audio, 0, sizeof(MultiFrameInfo));
    }
    pthread_mutex_unlock(&m_AudioLock);
}

void RecvMulticastDataModule::GetVideoFrame(MultiFrameInfo& video)
{
    pthread_mutex_lock(&m_VideoLock);
    if(m_VideoMutilQueue.size() > 0)
    {
        video = m_VideoMutilQueue.front();
        m_VideoMutilQueue.pop();
    }
    else
    {
        memset(&video, 0, sizeof(MultiFrameInfo));
    }
    pthread_mutex_unlock(&m_VideoLock);
}

void RecvMulticastDataModule::setCallbackFunctionFrameIsReady(std::function<void()> func)
{
    m_onVideoFrame = func;
}

