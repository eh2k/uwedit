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

#include <stdio.h>
#include <vector>
#include <RtAudio.h>

static const short *s_wave = NULL;
static int *s_playpos = 0;
static const int *s_loopstart = 0;
static const int *s_loopend = 0;
static unsigned int s_len = 0;
static RtAudio s_audio;

extern "C"
{
    static int rtaudio_callback(
        void *outbuf,
        void *inbuf,
        unsigned int nFrames,
        double streamtime,
        RtAudioStreamStatus status,
        void *userdata)
    {
        short *out = (short *)outbuf;

        for (unsigned int i = 0; i < nFrames; i++)
        {
            if (*s_loopstart < *s_loopend)
            {
                if (*s_playpos >= *s_loopend)
                    *s_playpos = *s_loopstart;
            }

            if (*s_playpos < s_len)
            {
                *out++ = s_wave[*s_playpos]; //Left
                *out++ = s_wave[*s_playpos]; //Right
            }
            else
            {
                *out++ = 0;
                *out++ = 0;
                return 1;
            }

            (*s_playpos)++;
        }
        return 0;
    }

    const char *GetAudioDevice(int i)
    {
        if (i < s_audio.getDeviceCount())
        {
            static std::string tmp;
            tmp = s_audio.getDeviceInfo(i).name;
            return tmp.c_str();
        }
        else
            return nullptr;
    }

    void Player_Stop()
    {
        if (s_audio.isStreamRunning())
        {
            s_audio.stopStream();

            (*s_playpos) = 0;
        }

        if (s_audio.isStreamOpen())
            s_audio.closeStream();
    }

    void Player_Play(int deviceId, const short *wave, const unsigned int len, int *playpos, const int *loopstart, const int *loopend)
    {
        if (s_audio.isStreamOpen() == false)
        {
            RtAudio::StreamParameters outParam;

            if (deviceId < 0)
            {
                outParam.deviceId = s_audio.getDefaultOutputDevice();

                for (unsigned int i = 0; i < s_audio.getDeviceCount(); i++)
                {
                    static std::string tmp;
                    if (s_audio.getDeviceInfo(i).isDefaultOutput || s_audio.getDeviceInfo(i).name == "default")
                    {
                        outParam.deviceId = i;
                        break;
                    }
                }
            }
            else
                outParam.deviceId = deviceId;

            printf("%d: play %d %d %d\n", outParam.deviceId, len, *loopstart, *loopend);

            outParam.nChannels = 2;
            unsigned int bufsize = 256;
            s_audio.openStream(&outParam, NULL, RTAUDIO_SINT16, 44100, &bufsize, rtaudio_callback, NULL);
        }

        s_wave = wave;
        s_len = len;
        s_playpos = playpos;
        s_loopstart = loopstart;
        s_loopend = loopend;

        if (s_audio.isStreamRunning())
            s_audio.stopStream();

        s_audio.startStream();
        //Pa_Sleep(1000);
    }
}