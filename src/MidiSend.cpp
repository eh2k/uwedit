/****************************************************************************
* Copyright (C) 2007-2010 by E.Heidt  http://code.google.com/p/uwedit/      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
****************************************************************************/

#include <vector>
#include <string>

#include <RtMidi.h>

bool SaveMidiSDS(int (*write)( unsigned char*, int ), void (*progress)(float), const std::vector<short>& data,
                 unsigned int _loopStart, unsigned int _loopEnd, const char* sampleName, int samplePos);

RtMidiOut* s_device = NULL;

static int s_write(unsigned char* buffer, int size)
{
    std::vector<unsigned char> v;
    v.assign( buffer, buffer + size );

    s_device->sendMessage(&v);
    return size;
}

std::vector<std::string> ListMidiOutDevices()
 {
     RtMidiOut midiOut;

     std::vector<std::string> ret;

     for(unsigned int i = 0; i < midiOut.getPortCount(); i++)
     {
         ret.push_back( midiOut.getPortName(i) );
     }

     return ret;
 }

bool SendMidiSDS(int midiport, std::vector<short>& data, unsigned int loopStart, unsigned int loopEnd, const char* sampleName, int samplePos, void (*progress)(float ))
{
    RtMidiOut midiOut;
    s_device = &midiOut;
    midiOut.openPort(midiport);

    SaveMidiSDS(&s_write, progress, data, loopStart, loopEnd, sampleName, samplePos);

    midiOut.closePort();
    s_device = NULL;

    return true;
}


bool SendDumpRequest(const int waveformId)
{
/*
    static const unsigned char dumpHeaderPattern[]= {0xf0,0x7e,1,0x03,waveformId&0x7f,(waveformId>>7)&0x7f,0xf7};
    static const char dumpHeaderPatternLen=sizeof(dumpHeaderPattern)/sizeof(*dumpHeaderPattern);


    int midiport = 1;

    int result = midiOutOpen(&s_device, midiport, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR)
    {
        return false;
    }

    rawsend(s_device, dumpHeaderPattern, dumpHeaderPatternLen);

    midiOutReset(s_device);

    midiOutClose(s_device);
    s_device = NULL;
*/
    return true;
}
