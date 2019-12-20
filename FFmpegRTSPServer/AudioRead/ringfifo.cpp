/*ringbuf .c*/

#include<stdio.h>
#include<ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ringfifo.h"

RingFifo::RingFifo()
:b_Audio(false),
b_audio_collect(false),
b_clean(false)
{
    initMutex() ;
    initSendBuff() ;

}

RingFifo::~RingFifo()
{

    destroyMutex() ;
}


void RingFifo::initSendBuff()
{
    pthread_mutex_lock(&audio_mutex) ;
    memset(audio_buff.buffer , 0 , AUDIO_BUFF_LEN);
    audio_buff.allowRead = 0;
    audio_buff.readIndex = audio_buff.writeIndex = 0  ;
    pthread_mutex_unlock(&audio_mutex) ;
}




void RingFifo::initAudioBuff()
{
    pthread_mutex_lock(&audio_mutex) ;
    memset(audio_buff.buffer , 0 , AUDIO_BUFF_LEN);
    audio_buff.allowRead = 0;
    audio_buff.readIndex = audio_buff.writeIndex = 0  ;
    pthread_mutex_unlock(&audio_mutex) ;
}


void RingFifo::initMutex()
{
    pthread_mutex_init(&audio_mutex , NULL) ;
    pthread_mutex_init(&a_collect_mutex , NULL) ;
}

void RingFifo::destroyMutex()
{
    pthread_mutex_destroy(&audio_mutex) ;
    pthread_mutex_destroy(&a_collect_mutex) ;
}


unsigned int RingFifo::audioWrite(char* data,unsigned int length)
{
    if(length <= 0)
        return 0;

    pthread_mutex_lock(&audio_mutex) ;
    unsigned int writeIndex = audio_buff.writeIndex;
    unsigned int readIndex = audio_buff.readIndex;

    //----------read--------write------
    if(writeIndex >= readIndex)
    {
        if(writeIndex + length > AUDIO_BUFF_LEN)
        {
            if(writeIndex + length -AUDIO_BUFF_LEN <= readIndex)
            {
                memcpy(&audio_buff.buffer[writeIndex] , data, AUDIO_BUFF_LEN - writeIndex);
                memcpy(&audio_buff.buffer[0], data + AUDIO_BUFF_LEN - writeIndex, writeIndex + length - AUDIO_BUFF_LEN);
                audio_buff.writeIndex = writeIndex + length - AUDIO_BUFF_LEN;
            }else
            {
                //cout<<"audio full ...";

            }
        }else
        {
            memcpy(&audio_buff.buffer[writeIndex], data, length);
            audio_buff.writeIndex = writeIndex + length ;
        }
    }
    //writeIndex < readIndex
    //---------write-------read-----------
    else
    {
        if(writeIndex + length > readIndex)
        {
            //cout<<"audio full ...";

            if(writeIndex + length > AUDIO_BUFF_LEN)
            {
                memcpy(&audio_buff.buffer[writeIndex], data, AUDIO_BUFF_LEN - writeIndex);
                memcpy(audio_buff.buffer, data + AUDIO_BUFF_LEN - writeIndex, writeIndex + length - AUDIO_BUFF_LEN);
                audio_buff.writeIndex = writeIndex + length - AUDIO_BUFF_LEN;

            }else
            {

                memcpy(&audio_buff.buffer[writeIndex], data, length);
                audio_buff.readIndex = audio_buff.writeIndex = writeIndex + length ;
            }
            while(audio_buff.readIndex<audio_buff.writeIndex)
            {
                u_int16_t frame_length
                           = ((audio_buff.buffer[audio_buff.readIndex+3]&0x03)<<11) | (audio_buff.buffer[audio_buff.readIndex+4]<<3) | ((audio_buff.buffer[audio_buff.readIndex+5]&0xE0)>>5);
                if((audio_buff.readIndex+7+frame_length)>AUDIO_BUFF_LEN)
                {
                    audio_buff.readIndex = audio_buff.readIndex+7+frame_length-AUDIO_BUFF_LEN;
                }
                else
                    audio_buff.readIndex += (7+frame_length);
            }

        }
        else
        {
            memcpy(&audio_buff.buffer[writeIndex], data, length);
            audio_buff.writeIndex = writeIndex + length ;
        }

    }
    //printf("audio.c: write %d , read %d\n" , writeIndex , readIndex) ;
    audio_buff.allowRead = 1;
    pthread_mutex_unlock(&audio_mutex) ;

     return 0;
}

int RingFifo::getAudioHead(char* data)
{
     pthread_mutex_lock(&audio_mutex) ;
     if(audio_buff.allowRead == 0)
     {
         pthread_mutex_unlock(&audio_mutex) ;
         //cout<<"getAudioHead!\n" ;
         return 0 ;
     }
     unsigned int writeIndex = audio_buff.writeIndex;
     unsigned int readIndex  = audio_buff.readIndex;

    if (readIndex < writeIndex)
    {
        if (readIndex + 7 <= writeIndex)
        {
            memcpy(data, &audio_buff.buffer[readIndex], 7);

        }
        else
        {
            cout<<"audio buff is empty" ;
            audio_buff.allowRead = 0 ;
             pthread_mutex_unlock(&audio_mutex) ;
            return 0;
        }
    }
    //readIndex >= writeIndex
    //------write----read------
    else
    {

        if(readIndex + 7 < AUDIO_BUFF_LEN)
        {
            memcpy(data, &audio_buff.buffer[readIndex], 7);

        }
        else
        {
            if (readIndex + 7 - AUDIO_BUFF_LEN < writeIndex)
            {
                memcpy(data, &audio_buff.buffer[readIndex], AUDIO_BUFF_LEN - readIndex);
                memcpy(data + AUDIO_BUFF_LEN - readIndex, audio_buff.buffer, readIndex + 7 - AUDIO_BUFF_LEN);

            }else
            {
                cout<<"audio buff is empty" ;
                audio_buff.allowRead = 0 ;
                 pthread_mutex_unlock(&audio_mutex) ;
                return 0 ;
            }
        }
    }
//    if(data[0] != (char)0xff)
//    {
//        //cout<<(char)data[0];

//        pthread_mutex_unlock(&audio_mutex) ;
//        return 0;
//    }
    audio_buff.readIndex += 7;
    if(audio_buff.readIndex>=AUDIO_BUFF_LEN)
    {
        audio_buff.readIndex-=AUDIO_BUFF_LEN;
    }
    u_int16_t frame_length
            = ((data[3]&0x03)<<11) | (data[4]<<3) | ((data[5]&0xE0)>>5);
    pthread_mutex_unlock(&audio_mutex) ;
    return frame_length;
}

int RingFifo::copyAudioFixHead(char *data , int len)
{
    pthread_mutex_lock(&audio_mutex) ;
    if(audio_buff.allowRead == 0)
    {
        pthread_mutex_unlock(&audio_mutex) ;
        printf("getAudioHead!\n") ;
        return 0 ;
    }
    unsigned int writeIndex = audio_buff.writeIndex;
    unsigned int readIndex  = audio_buff.readIndex;

   if (readIndex < writeIndex)
   {
       if (readIndex + len <= writeIndex)
       {
           memcpy(data, &audio_buff.buffer[readIndex], len);

       }
       else
       {
           //cout<<"audio buff is empty" ;
            pthread_mutex_unlock(&audio_mutex) ;
           return 0;
       }
   }
   //readIndex >= writeIndex
   //------write----read------
   else
   {

       if(readIndex + len < AUDIO_BUFF_LEN)
       {
           memcpy(data, &audio_buff.buffer[readIndex], len);

       }
       else
       {
           if (readIndex + len - AUDIO_BUFF_LEN < writeIndex)
           {
               memcpy(data, &audio_buff.buffer[readIndex], AUDIO_BUFF_LEN - readIndex);
               memcpy(data + AUDIO_BUFF_LEN - readIndex, audio_buff.buffer, readIndex + len - AUDIO_BUFF_LEN);

           }else
           {
               cout<<"audio buff is empty" ;
                pthread_mutex_unlock(&audio_mutex) ;
               return 0 ;
           }
       }
   }

    pthread_mutex_unlock(&audio_mutex) ;
   return len ;

   u_int16_t frame_length
           = ((data[3]&0x03)<<11) | (data[4]<<3) | ((data[5]&0xE0)>>5);

   return frame_length;
}


int RingFifo::getAudioData(char* data,int datalen)
{
    pthread_mutex_lock(&audio_mutex) ;
    if(audio_buff.allowRead == 0)
    {
        pthread_mutex_unlock(&audio_mutex) ;
        //cout<<"getAudioData!\n" ;
        return 0 ;
    }
    unsigned int writeIndex = audio_buff.writeIndex;
    unsigned int readIndex  = audio_buff.readIndex;
    int ret = 0;
    //------read------write-------
    if (readIndex < writeIndex)
    {
        if (readIndex + datalen <= writeIndex)
        {
            memcpy(data, &audio_buff.buffer[readIndex], datalen);
            ret = datalen;
            audio_buff.readIndex += ret;
        }
        else
        {
            //cout<<"audio buff is empty";
            audio_buff.allowRead = 0 ;
            ret = 0 ;
#if 0
            memcpy(data, &audio_buff.buffer[readIndex], writeIndex - readIndex);
            ret = writeIndex - readIndex;
            audio_buff.readIndex = writeIndex;
#endif
        }
    }
    //readIndex >= writeIndex
    //------write----read------
    else
    {

        if(readIndex + datalen < AUDIO_BUFF_LEN)
        {
            memcpy(data, &audio_buff.buffer[readIndex], datalen);
            ret = datalen;
            audio_buff.readIndex += ret;
        }
        else
        {
            if (readIndex + datalen - AUDIO_BUFF_LEN < writeIndex)
            {
                memcpy(data, &audio_buff.buffer[readIndex], AUDIO_BUFF_LEN - readIndex);
                memcpy(data + AUDIO_BUFF_LEN - readIndex, audio_buff.buffer, readIndex + datalen - AUDIO_BUFF_LEN);
                ret = datalen;
                audio_buff.readIndex = readIndex + datalen - AUDIO_BUFF_LEN;
            }else
            {
                //cout<<"audio buff is empty";
                audio_buff.allowRead = 0 ;
                ret = 0 ;
#if 0
                memcpy(data, &audio_buff.buffer[readIndex], AUDIO_BUFF_LEN - readIndex);
                memcpy(data + AUDIO_BUFF_LEN - readIndex, audio_buff.buffer, datalen+readIndex-AUDIO_BUFF_LEN);
                ret = datalen;
                audio_buff.readIndex = datalen+readIndex-AUDIO_BUFF_LEN;
#endif
            }
        }
    }
    pthread_mutex_unlock(&audio_mutex) ;

    //printf("%d\n" , ret) ;
    return ret;
}


