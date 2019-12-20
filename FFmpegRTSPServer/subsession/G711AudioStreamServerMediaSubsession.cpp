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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from an G711 audio stream.
// Implementation

#include "G711AudioStreamServerMediaSubsession.hh"
#include "G711AudioStreamSource.hh"
#include "SimpleRTPSink.hh"
#include "MPEG4GenericRTPSink.hh"
G711AudioStreamServerMediaSubsession* G711AudioStreamServerMediaSubsession
::createNew(UsageEnvironment& env, Boolean reuseFirstSource) {
    return new G711AudioStreamServerMediaSubsession(env, reuseFirstSource);
}

G711AudioStreamServerMediaSubsession
::G711AudioStreamServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource)
    : OnDemandServerMediaSubsession(env, reuseFirstSource){
}

G711AudioStreamServerMediaSubsession
::~G711AudioStreamServerMediaSubsession() {
}

FramedSource* G711AudioStreamServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    FramedSource* resultSource = NULL;
    do {
        G711AudioStreamSource* g711Source
            = G711AudioStreamSource::createNew(envir());
        if (g711Source == NULL) 
        {
            envir() << "====== Creat G711 SOURCE faile ========\n" ;
            break;
        }

        // Get attributes of the audio source:

        fBitsPerSample = g711Source->bitsPerSample();
        if (!(fBitsPerSample == 4 || fBitsPerSample == 8 || fBitsPerSample == 16)) {
            envir() << "The input file contains " << fBitsPerSample << " bit-per-sample audio, which we don't handle\n";
            break;
        }
        fSamplingFrequency = g711Source->samplingFrequency();
        fNumChannels = g711Source->numChannels();
        unsigned bitsPerSecond
            = fSamplingFrequency*fBitsPerSample*fNumChannels;

        resultSource = g711Source;

        estBitrate = (bitsPerSecond+500)/1000; // kbps
        return resultSource;
    } while (0);

    // An error occurred:
    Medium::close(resultSource);
    return NULL;
}

RTPSink* G711AudioStreamServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
        unsigned char rtpPayloadTypeIfDynamic,
        FramedSource* /*inputSource*/) {
    const char *mimeType = "PCMA";
    unsigned char payloadFormatCode;
    if (fSamplingFrequency == 8000 && fNumChannels == 1) {
        payloadFormatCode = 8; // a static RTP payload type
    } else {
        payloadFormatCode = rtpPayloadTypeIfDynamic;
    }
    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
            payloadFormatCode, fSamplingFrequency,
            "audio", mimeType, fNumChannels);
}
