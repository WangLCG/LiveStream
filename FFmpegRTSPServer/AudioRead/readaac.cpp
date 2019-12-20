#include "readaac.h"



ReadAac::ReadAac()
    :aacFile(NULL)
    ,threadStatus(false)
    ,b_retPthreadStatus(false)
{
    pthread_mutex_init(&status_mutex , NULL) ;

}


ReadAac::~ReadAac()
{
    if(b_retPthreadStatus)
    {
        void *tmp;
        if(pthread_join(aacRId,&tmp)!=0)
        {
            cout <<"ReadAac::~ReadAac pthread_join Error\n";
        }
    }

    if( aacFile != NULL )
        fclose(aacFile);

    //cout<<"ReadAac::~ReadAac()" ;
}

void ReadAac::openAac()
{
    char *fileName = "ohyeah.aac" ;
     aacFile = fopen(fileName, "rb");
     if( aacFile == NULL )
     {
        //cout<<"aacFile = fopen(fileName, \"rb\") ERROR!" ;
        return ;
     }

     fseeko(aacFile, (off_t)(SEEK_SET), 0);
}

void ReadAac::stopRead()
{
    setStatus(false);
}


void ReadAac::startRead()
{
    int ret = pthread_create(&aacRId , NULL , readLoop , this) ;
    b_retPthreadStatus = (ret > 0)? true : false ;
    setStatus(true);
}

void *ReadAac::readLoop(void *arg)
{
    ReadAac *pThis = (ReadAac *)arg;
    pThis->doRead();
}

void ReadAac::doRead()
{
    //int iNum = 0 ;

    while(getStatus())
    {
        #define BUF_LEN 400
        char readBuf[BUF_LEN]={'\0'} ;
        int numBytesRead = fread(readBuf, 1, BUF_LEN, aacFile);

        if(numBytesRead <= 0)
        {
            fseeko(aacFile, (off_t)(SEEK_SET), 0);
            //cout<<iNum ;
            //break ;
        }
        else
        {
            mediaBuf.audioWrite(readBuf ,numBytesRead ) ;
            //iNum += numBytesRead ;
            usleep(46*1000) ;

        }
    }

}
