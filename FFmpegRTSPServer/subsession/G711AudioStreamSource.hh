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
// A G711 audio stream source
// C++ header

#ifndef _G711_AUDIO_STREAM_SOURCE_HH
#define _G711_AUDIO_STREAM_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class G711AudioStreamSource: public FramedSource {
public:
  unsigned char bitsPerSample() const { return fBitsPerSample; }
  unsigned char numChannels() const { return fNumChannels; }
  unsigned samplingFrequency() const { return fSamplingFrequency; }

  static G711AudioStreamSource* createNew(UsageEnvironment& env);

protected:
  G711AudioStreamSource(UsageEnvironment& env);
    // called only by createNew()

  virtual ~G711AudioStreamSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  unsigned char fNumChannels;
  unsigned fSamplingFrequency;
  unsigned char fBitsPerSample;
  unsigned fPreferredFrameSize;
  Boolean fLimitNumBytesToStream;
  unsigned fNumBytesToStream;
  unsigned fLastPlayTime;
  double fPlayTimePerSample; // useconds
};

#endif
