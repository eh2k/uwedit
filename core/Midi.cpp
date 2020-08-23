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

//#include "_cgo_export.h"
#include <vector>
#include <string>
#include <stdio.h>
#include <Stk.h>
#include <RtMidi.h>

bool SaveMidiSDS(int (*write)(unsigned char *, int), void (*progress)(float), const std::vector<short> &data,
                 unsigned int _loopStart, unsigned int _loopEnd, const char *sampleName, int samplePos);

static RtMidiOut& midiOut()
{
    static RtMidiOut _midiOut;
    return _midiOut;
} 
static RtMidiIn& midiIn()
{
    static RtMidiIn _midiIn;
    return _midiIn;
} 

extern "C"
{
    const char *GetMidiOutDevice(int i)
    {
        if (i < midiOut().getPortCount())
        {
            static std::string tmp;
            tmp = midiOut().getPortName(i);
            return tmp.c_str();
        }
        else
            return nullptr;
    }

    void MidiOut_openPort(int port)
    {
        midiOut().openPort(port);
    }

    void MidiIn_openPort(int port)
    {
        midiIn().openPort(port);
    }

    void MidiOut_closePort()
    {
        midiOut().closePort();
    }

    void MidiIn_closePort()
    {
        midiIn().closePort();
    }

    void MidiOut_sendMessage(const unsigned char *rawBytes, size_t len)
    {
        midiOut().sendMessage(rawBytes, len);
    }

    void MidiIn_setCallback(/* goMidiInCallback */)
    {
        void goMidiInCallback(double timeStamp, const unsigned char *rawBytes, size_t len);

        midiIn().ignoreTypes( false, true, false );
        midiIn().cancelCallback();
        midiIn().setCallback([](double timeStamp, std::vector<unsigned char> *message, void *userData )
        {
            goMidiInCallback(timeStamp, &(*message)[0], message->size());
        });
    }

    void SendMidiSDS(int delay_ms, const short *wave, size_t len, unsigned int loopStart, unsigned int loopEnd, const char *sampleName, int samplePos)
    {
        static int delay = 50;
        delay = delay_ms;

        std::vector<short> data(wave, wave + len);

        void goProgressCB(float p);

        goProgressCB(0);

        SaveMidiSDS([](unsigned char *buffer, int size){
            midiOut().sendMessage(buffer, size);
            stk::Stk::sleep(delay);
            return size;
        }, goProgressCB, data, loopStart, loopEnd, sampleName, samplePos);
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
}