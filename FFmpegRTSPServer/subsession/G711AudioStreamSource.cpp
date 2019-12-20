/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2011 Live Networks, Inc.  All rights reserved.
// A WAV audio file source
// Implementation

#include "G711AudioStreamSource.hh"
//#include "audio_encode.cpp"
//#include "g711_audio.cpp"

int audioOpen(void)
{
    return 0;
}

int audioClose(void)
{
    return 0;
}

int audioGetOneFrame(unsigned char *buf, unsigned int *size)
{
    return 0;
}

#if 0
static FILE *fp;
static int audioGetOneFrame(unsigned char *buf, unsigned int *size){
    int ret;

    ret = fread(buf, 1, 320, fp);
    if(ret < 0) {
        printf("ERR : %s : %d\n", __FILE__, __LINE__);
        return -1;
    }

    *size = 320;

    return 0;
}
#endif

G711AudioStreamSource*
G711AudioStreamSource::createNew(UsageEnvironment& env) {
    do {
        G711AudioStreamSource* newSource = new G711AudioStreamSource(env);
        if (newSource != NULL && newSource->bitsPerSample() == 0) {
            // The WAV file header was apparently invalid.
            Medium::close(newSource);
            break;
        }
        return newSource;
    } while (0);

    return NULL;
}

G711AudioStreamSource::G711AudioStreamSource(UsageEnvironment& env)
    : FramedSource(env), 
    fNumChannels(0), fSamplingFrequency(0),
    fBitsPerSample(0),
    fLimitNumBytesToStream(False),
    fNumBytesToStream(0),
    fLastPlayTime(0),
    fPlayTimePerSample(0){

    fNumChannels = 1;
    fSamplingFrequency = 8000;
    fBitsPerSample = 16;

    fPlayTimePerSample = 1e6/(double)fSamplingFrequency;
    // Although PCM is a sample-based format, we group samples into
    // 'frames' for efficient delivery to clients.  Set up our preferred
    // frame size to be close to 20 ms, if possible, but always no greater
    // than 1400 bytes (to ensure that it will fit in a single RTP packet)
    unsigned maxSamplesPerFrame = (1400*8)/(fNumChannels*fBitsPerSample);
    unsigned desiredSamplesPerFrame = (unsigned)(0.04*fSamplingFrequency);
    unsigned samplesPerFrame = desiredSamplesPerFrame < maxSamplesPerFrame ? desiredSamplesPerFrame : maxSamplesPerFrame;
    fPreferredFrameSize = (samplesPerFrame*fNumChannels*fBitsPerSample)/8;

    audioOpen();
#if 0
    fp = NULL;
    fp = fopen("test.g711", "r");
    if(fp == NULL) {
        printf("ERR : %s : %d\n", __FILE__, __LINE__);
    }
#endif
}

G711AudioStreamSource::~G711AudioStreamSource() {
    audioClose();
#if 0
    if(fp != NULL)
        fclose(fp);
#endif
}

// Note: We should change the following to use asynchronous file reading, #####
// as we now do with ByteStreamFileSource. #####
void G711AudioStreamSource::doGetNextFrame() {
    // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
    if (fLimitNumBytesToStream && fNumBytesToStream < fMaxSize) {
        fMaxSize = fNumBytesToStream;
    }
    if (fPreferredFrameSize < fMaxSize) {
        fMaxSize = fPreferredFrameSize;
    }
    unsigned bytesPerSample = (fNumChannels*fBitsPerSample)/8;
    if (bytesPerSample == 0) bytesPerSample = 1; // because we can't read less than a byte at a time
    //unsigned bytesToRead = fMaxSize - fMaxSize%bytesPerSample;

    //fFrameSize : 1000
    audioGetOneFrame(fTo, &fFrameSize);
    memset(fTo, 2, 320);
    fFrameSize = 320;
    //envir() << "get audio frame success \n";
    
    // Set the 'presentation time' and 'duration' of this frame:
    if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
        // This is the first frame, so use the current time:
        gettimeofday(&fPresentationTime, NULL);
    } else {
        // Increment by the play time of the previous data:
        unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
        fPresentationTime.tv_sec += uSeconds/1000000;
        fPresentationTime.tv_usec = uSeconds%1000000;
    }

    // Remember the play time of this data:
    fDurationInMicroseconds = fLastPlayTime
        = (unsigned)((fPlayTimePerSample*fFrameSize)/bytesPerSample);

    // Switch to another task, and inform the reader that he has data:
#if defined(__WIN32__) || defined(_WIN32)
    // HACK: One of our applications that uses this source uses an
    // implementation of scheduleDelayedTask() that performs very badly
    // (chewing up lots of CPU time, apparently polling) on Windows.
    // Until this is fixed, we just call our "afterGetting()" function
    // directly.  This avoids infinite recursion, as long as our sink
    // is discontinuous, which is the case for the RTP sink that
    // this application uses. #####
    afterGetting(this);
#else
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
            (TaskFunc*)FramedSource::afterGetting, this);
#endif
}


