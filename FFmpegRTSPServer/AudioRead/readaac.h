#ifndef READAAC_H
#define READAAC_H

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "ringbufdata.h"
#include <iostream>

using namespace std;

class ReadAac
{
public:
    ReadAac();
    ~ReadAac();

    void openAac();
    void startRead() ;
    void stopRead();
    void setStatus(bool b)
    {
        pthread_mutex_lock(&status_mutex) ;
        threadStatus = b ;
        pthread_mutex_unlock(&status_mutex) ;
    }

    bool getStatus()
    {
       bool b ;
       pthread_mutex_lock(&status_mutex) ;
       b= threadStatus;
       pthread_mutex_unlock(&status_mutex) ;

       return b;
    }

private:
    static void *readLoop(void *arg) ;
    void doRead();
private:
    FILE *aacFile;
    pthread_t aacRId;
    bool threadStatus;
    pthread_mutex_t status_mutex ;

    bool b_retPthreadStatus;

};

#endif // READAAC_H
