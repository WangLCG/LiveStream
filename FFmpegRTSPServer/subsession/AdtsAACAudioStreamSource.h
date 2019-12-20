#ifndef ADTSSOURCE_H
#define ADTSSOURCE_H

#include "FramedFileSource.hh"
#include <iostream>

/* 直接从AAC文件读取数据测试 */
#define TEST_MY_AAC_READ (1)

#if(TEST_MY_AAC_READ == 1)
#include "readaac.h"
#endif

using namespace std;

class AdtsAACAudioStreamSource : public FramedFileSource
{
public:
    static AdtsAACAudioStreamSource* createNew(UsageEnvironment& env)  ;

    unsigned samplingFrequency() const { return fSamplingFrequency; }
    unsigned numChannels() const { return fNumChannels; }
    char const* configStr() const { return fConfigStr; }

private:
  AdtsAACAudioStreamSource(UsageEnvironment& env , u_int8_t profile,
              u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration);

  virtual ~AdtsAACAudioStreamSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  unsigned fSamplingFrequency;
  unsigned fNumChannels;
  unsigned fuSecsPerFrame;
  char fConfigStr[5];

#if(TEST_MY_AAC_READ == 1)
  ReadAac aac;
#endif

private:
  static void mySleep(void *p);

private:
  int fWatchVariable  ;
};

#endif // ADTSSOURCE_H
