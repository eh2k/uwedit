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
#include "Player.h"
#include <RtAudio.h>

const std::vector<short>* s_wave = NULL;
unsigned int s_playpos = 0;
const unsigned int* s_loopstart = 0;
const unsigned int* s_loopend = 0;

static int rtaudio_callback(
    void *outbuf,
    void *inbuf,
    unsigned int nFrames,
    double streamtime,
    RtAudioStreamStatus status,
    void *userdata)
{
    short *out = (short*)outbuf;

    for(unsigned int i=0; i<nFrames; i++ )
    {
        if(*s_loopstart < *s_loopend)
        {
            if(s_playpos >= *s_loopend)
                s_playpos = *s_loopstart;
        }

        if(s_playpos < s_wave->size())
            *out++ = s_wave->at(s_playpos);
        else
        {
            *out++ = 0;
            return 1;
        }

        s_playpos++;
    }
    return 0;
}

static RtAudio *s_audio;

Player::Player()
{
    s_audio = new RtAudio();

    RtAudio::StreamParameters outParam;

    outParam.deviceId = s_audio->getDefaultOutputDevice();
    outParam.nChannels = 1;
    unsigned int bufsize = 4096;
    s_audio->openStream(&outParam, NULL, RTAUDIO_SINT16, 44100, &bufsize, rtaudio_callback, NULL);

}

Player::~Player()
{
    s_audio->closeStream();
    delete s_audio;
}

void Player::Play(const std::vector<short>& wave, const unsigned int& loopstart, const unsigned int& loopend)
{
    s_wave = &wave;
    s_playpos = 0;
    s_loopstart = &loopstart;
    s_loopend = &loopend;

    Stop();

    s_audio->startStream();
    //Pa_Sleep(1000);
}
void Player::Stop()
{
    if(s_audio && s_audio->isStreamRunning())
    {
        s_audio->stopStream();
    }
}

bool Player::IsPlaying()
{
    return s_audio->isStreamRunning();
}

unsigned int Player::GetPlayPos()
{
    return s_playpos;
}
