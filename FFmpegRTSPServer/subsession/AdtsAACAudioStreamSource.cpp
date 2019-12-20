#include "AdtsAACAudioStreamSource.h"


static unsigned const samplingFrequencyTable[16] = {
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};


AdtsAACAudioStreamSource* AdtsAACAudioStreamSource::createNew(UsageEnvironment& env)
{
    // Get and check the 'profile':
    u_int8_t profile = 2;  //AAC LC // 2 bits
    
    // Get and check the 'sampling_frequency_index':
    u_int8_t sampling_frequency_index = 3; // 4 bits 48K HZ, 
    if (samplingFrequencyTable[sampling_frequency_index] == 0) 
    {
      cout <<"Bad 'sampling_frequency_index' in first frame of ADTS file \n";
      return NULL;
    }

    // Get and check the 'channel_configuration':
    u_int8_t channel_configuration = 2; // 3 bits 双通道


    cout << "profile: "<< profile << "\n";
    cout << "sampling_frequency_index: " << sampling_frequency_index << "\n";
    cout << "channel_configuration: " << channel_configuration << "\n";


    return new AdtsAACAudioStreamSource(env,  profile,
                       sampling_frequency_index, channel_configuration);

    //return new AdtsAACAudioStreamSource(env, 1, 8, 1);
}

AdtsAACAudioStreamSource::AdtsAACAudioStreamSource(UsageEnvironment& env , u_int8_t profile,
            u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration)
    :FramedFileSource(env, NULL)
{
      fSamplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];
      fNumChannels = channelConfiguration == 0 ? 2 : channelConfiguration;
      fuSecsPerFrame
        = (1024/*samples-per-frame*/*1000000) / fSamplingFrequency/*samples-per-second*/;
       
      // Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
      unsigned char audioSpecificConfig[2];
      u_int8_t const audioObjectType = profile + 1;
      audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
      audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
      sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

      cout << "fConfigStr: " << fConfigStr << "\n";
}


AdtsAACAudioStreamSource::~AdtsAACAudioStreamSource()
{
    cout << "AdtsAACAudioStreamSource::~AdtsAACAudioStreamSource() \n" ;
}

void AdtsAACAudioStreamSource::mySleep(void *p)
{
    AdtsAACAudioStreamSource* obj= (AdtsAACAudioStreamSource*)p;
    obj->fWatchVariable = ~0;

    cout << "AdtsAACAudioStreamSource::mySleep \n";
}


void AdtsAACAudioStreamSource::doGetNextFrame()
{

    char headers[7];
    u_int16_t frame_length =0;
    unsigned numBytesToRead ;
    
    numBytesToRead  = 320;

    if (numBytesToRead > fMaxSize) 
    {
               fNumTruncatedBytes = numBytesToRead - fMaxSize;
               numBytesToRead = fMaxSize;
    }

    //int numBytesRead =  mediaBuf.getAudioData((char *)fTo,numBytesToRead) ;
    memset(fTo, 2, numBytesToRead);
    int numBytesRead = numBytesToRead;
    if (numBytesRead < 0)
    {
        numBytesRead = 0;
    }
    fFrameSize = numBytesRead;
    fNumTruncatedBytes += numBytesToRead - numBytesRead;

    //LOG(INFO)<<fFrameSize ;
    // Set the 'presentation time':
    if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
      // This is the first frame, so use the current time:
      gettimeofday(&fPresentationTime, NULL);

    } else {
      // Increment by the play time of the previous frame:
      unsigned uSeconds = fPresentationTime.tv_usec + fuSecsPerFrame;
      fPresentationTime.tv_sec += (uSeconds/1000000);
      fPresentationTime.tv_usec = uSeconds%1000000;

    }

    fDurationInMicroseconds = fuSecsPerFrame;

//usleep(1*1000) ;

    // Switch to another task, and inform the reader that he has data:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                  (TaskFunc*)FramedSource::afterGetting, this);

}
