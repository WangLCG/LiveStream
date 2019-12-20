#ifndef __RINGFIFO_H__
#define __RINGFIFO_H__


//#include <pthread.h>

#include <string>
#include <iostream>

using namespace std;

//#include "../glog/logging.h"

#define AUDIO_BUFF_LEN 1024*1024*10

typedef struct audioRingBuf
{
    unsigned int readIndex;
    unsigned int writeIndex;
    unsigned int allowRead;
    char buffer[AUDIO_BUFF_LEN];

}AUDIO_BUFF ;

class RingFifo{

public:
    RingFifo();
    ~RingFifo();



    int getVideoData(char* data,int datalen);
    unsigned int videoWrite(char* data,unsigned int length);

    unsigned int audioWrite(char* data,unsigned int length) ;
    int getAudioData(char* data,int datalen) ;

    int getAudioHead(char* data);
    int copyAudioFixHead(char *data, int len) ;

public:
    bool getVideo()
    {
        //bool b ;
        //pthread_mutex_lock(&v_switch_mutex) ;
        //b = b_Video ;
        //pthread_mutex_unlock(&v_switch_mutex) ;

        return false ;
    }

    bool getAudio()
    {
        //bool b ;
        //pthread_mutex_lock(&a_switch_mutex) ;
        //b = b_Audio ;
        //pthread_mutex_unlock(&a_switch_mutex) ;

        return false ;
    }

private:
    int FindStartCode2 ( char *Buf);
    int FindStartCode3 (char *Buf);
    int nextVideoData(char *data , int rIndex);

private:
    void initVideoBuff();
    void initAudioBuff();

private:
    void initMutex() ;
    void destroyMutex() ;

    void initSendBuff() ;
    void setAudio(bool b)
    {
        //pthread_mutex_lock(&a_switch_mutex) ;
        //b_Audio = b ;
        //pthread_mutex_unlock(&a_switch_mutex) ;
    }

    void setClean(bool b)
    {
        pthread_mutex_lock(&clean_mutex) ;
        b_clean = b ;
        pthread_mutex_unlock(&clean_mutex) ;
    }

    bool getClean()
    {
        bool b ;
        pthread_mutex_lock(&clean_mutex) ;
        b = b_clean ;
        pthread_mutex_unlock(&clean_mutex) ;

        return b ;
    }

private:
    AUDIO_BUFF audio_buff ;


    pthread_mutex_t a_collect_mutex;
    pthread_mutex_t clean_mutex;
    pthread_mutex_t audio_mutex ;

    bool b_Audio;
    bool b_audio_collect;
    bool b_clean  ;


};
#endif
