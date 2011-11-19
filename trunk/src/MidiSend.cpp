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

#include <windows.h>
#include <mmsystem.h>

bool SaveMidiSDS(int (*write)( unsigned char*, int ), void (*progress)(float), const std::vector<short>& data,
                 unsigned int _loopStart, unsigned int _loopEnd, const char* sampleName, int samplePos);


int rawsend(HMIDIOUT device, const unsigned char* array, int size)
{
    // Note: this function will work in Windows 95 and Windows NT.
    // This function will not work in Windows 3.x because a
    // different memory model is necessary.

    if (size > 64000 || size < 1)
    {
        return 0;
    }

    MIDIHDR midiheader;   // structure for sending an array of MIDI bytes

    midiheader.lpData = (char *)array;
    midiheader.dwBufferLength = size;
    // midiheader.dwBytesRecorded = size;  // example program doesn't set
    midiheader.dwFlags = 0;                // flags must be set to 0

    int status = midiOutPrepareHeader(device, &midiheader, sizeof(MIDIHDR));

    if (status != MMSYSERR_NOERROR)
    {
        return 0;
    }

    status = midiOutLongMsg(device, &midiheader, sizeof(MIDIHDR));

    if (status != MMSYSERR_NOERROR)
    {
        return 0;
    }

    while (MIDIERR_STILLPLAYING == midiOutUnprepareHeader(device, &midiheader, sizeof(MIDIHDR)))
    {
        Sleep(1);                           // sleep for 1 millisecond
    }

    return size;
}

HMIDIOUT s_device = NULL;

static int s_write(unsigned char* buffer, int size)
{
    return rawsend(s_device, buffer, size);
}

std::vector<std::string> ListMidiOutDevices()
 {
     std::vector<std::string> ret;

     MIDIOUTCAPS caps;
     for(UINT i = 0; i < midiOutGetNumDevs(); i++)
     {
         midiOutGetDevCaps(i, &caps, sizeof(caps));
         ret.push_back( std::string(caps.szPname) );
     }

     return ret;
 }

bool SendMidiSDS(int midiport, std::vector<short>& data, unsigned int loopStart, unsigned int loopEnd, const char* sampleName, int samplePos, void (*progress)(float ))
{
    int result = midiOutOpen(&s_device, midiport, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR)
    {
        return false;
    }

    SaveMidiSDS(&s_write, progress, data, loopStart, loopEnd, sampleName, samplePos);

    midiOutReset(s_device);

    midiOutClose(s_device);
    s_device = NULL;

    return true;
}


bool SendDumpRequest(const int waveformId)
{
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

    return true;
}
