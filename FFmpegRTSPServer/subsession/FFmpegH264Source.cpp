//
//  FFmpegH264Source.cpp
//  FFmpegRTSPServer
//
//  Created by Mina Saad on 9/22/15.
//  Copyright (c) 2015 Mina Saad. All rights reserved.
//

#include "FFmpegH264Source.h"

namespace MESAI
{
    FFmpegH264Source * FFmpegH264Source::createNew(UsageEnvironment& env,
                    RecvMulticastDataModule* MulticastModule) 
    {
        return new FFmpegH264Source(env, MulticastModule);
    }
    
    FFmpegH264Source::FFmpegH264Source(UsageEnvironment& env, 
            RecvMulticastDataModule* MulticastModule): FramedSource(env), m_MulticastModule(MulticastModule)
    {
        m_eventTriggerId = envir().taskScheduler().createEventTrigger(FFmpegH264Source::deliverFrameStub);
        std::function<void()> callback1 = std::bind(&FFmpegH264Source::onFrame,this);
        m_MulticastModule -> setCallbackFunctionFrameIsReady(callback1);
        
    }

    FFmpegH264Source::~FFmpegH264Source()
    {
        cout << " FFmpegH264Source::~FFmpegH264Source() " << endl;
        FramedSource::doStopGettingFrames();
    }

    void FFmpegH264Source::doStopGettingFrames()
    {
        FramedSource::doStopGettingFrames();
    }

    void FFmpegH264Source::onFrame()
    {
        envir().taskScheduler().triggerEvent(m_eventTriggerId, this);
    }

    void FFmpegH264Source::doGetNextFrame()
    {
        deliverFrame();
    }

    void FFmpegH264Source::DelayReadFrame(FramedSource* source)
    {
        source->doGetNextFrame();
    }
        
    void FFmpegH264Source::deliverFrame()
    {
        static uint8_t* newFrameDataStart;
        static unsigned newFrameSize = 0;

        MultiFrameInfo  video;
        memset(&video, 0, sizeof(MultiFrameInfo));
        m_MulticastModule->GetVideoFrame(video);
        if(video.buf && video.buf_size > 0)
        {
            //printf("File[%s]---[%d]: video.buf_size[%u] fMaxSize[%u]\n", __FUNCTION__, __LINE__, 
            //    video.buf_size, fMaxSize);
            
            char* videoBuf = video.buf;
            newFrameSize = video.buf_size;
            unsigned short offset = 0;
            
            /* live555发送H264是不能包括 0x00000001 或 0x000001 起始码，需要去掉 */
            if (newFrameSize >= 4 && videoBuf[0] == 0 && videoBuf[1] == 0 
                && ((videoBuf[2] == 0 && videoBuf[3] == 1) || videoBuf[2] == 1))
            {
                //envir() << "H264or5VideoVideoSource error: MPEG 'start code' seen in the input\n";
                if(videoBuf[2] == 0 && videoBuf[3] == 1)
                {
                    /* 0x00000001 */
                    newFrameSize -= 4;
                    offset = 4;
                }
                else
                {
                    newFrameSize -= 3;
                    offset = 3;
                }
            }
            else
            {
                offset = 0;
            }

            bool IsTruncated = false;
            if (newFrameSize > fMaxSize) 
            {
                fFrameSize = fMaxSize;
                fNumTruncatedBytes = newFrameSize - fMaxSize;
                IsTruncated = true;
            } 
            else 
            {
                fFrameSize = newFrameSize;
            }

            //printf("-------- newFrameSize[%d] fFrameSize[%d]-----\n", newFrameSize,fFrameSize);
            gettimeofday(&fPresentationTime, NULL);
            try
            {
                memcpy(fTo, video.buf + offset, fFrameSize);
            }
            catch (exception& e)
            {
                cout <<" memcpy CRITIC ISSUE !!!"<< e.what() << endl;
            }
            
            if(!IsTruncated)
            {
               delete[] video.buf;
            }
        }
        else
        {
            fFrameSize = 0;
        }
        
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
              (TaskFunc*)FramedSource::afterGetting, this);
        
    }
}