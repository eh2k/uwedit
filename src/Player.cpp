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

#include <wx/wx.h>
#include "Player.h"

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

static HWAVEOUT s_hWaveOut = NULL; /* device handle */
HANDLE s_event = NULL;

static void VR(MMRESULT result)
{
    if(result != MMSYSERR_NOERROR)
    {
        WCHAR msg[256];
        waveOutGetErrorText(result, msg, 256);
        //wxMessageBox( wxString::Format("waveOutPrepareHeader failed -> %d %s", result, wxString(msg)));
    }
}

class PlayThread: public wxThread
{
    WAVEHDR m_hdr[2];
    static const unsigned int bufferSize = 3512;
    short buffer[2][bufferSize];
    int m_currentBuffer;

    const std::vector<short>& m_wave;
    const unsigned int& m_loopstart;
    const unsigned int& m_loopend;
    unsigned int m_playpos;

public:
    PlayThread(const std::vector<short>& wave, const unsigned int& loopstart, const unsigned int& loopend)
        : wxThread(wxTHREAD_JOINABLE),m_currentBuffer(0),m_wave(wave),m_loopstart(loopstart),m_loopend(loopend), m_playpos(0)
    {
        memset(&m_hdr[0], sizeof(WAVEHDR),0);
        memset(&m_hdr[1], sizeof(WAVEHDR),0);

        if(wxTHREAD_NO_ERROR == Create())
        {
            Run();
        }
    }

    virtual ~PlayThread()
    {
        VR(waveOutReset(s_hWaveOut));
        VR(waveOutUnprepareHeader(s_hWaveOut, &m_hdr[0], sizeof(WAVEHDR)));
        VR(waveOutUnprepareHeader(s_hWaveOut, &m_hdr[1], sizeof(WAVEHDR)));
    }

    unsigned int GetPlayPos()
    {
        return m_playpos;
    }
protected:

    void fillBuffer()
    {
        if(m_hdr[m_currentBuffer].dwFlags & WHDR_PREPARED)
            VR(waveOutUnprepareHeader(s_hWaveOut, &m_hdr[m_currentBuffer], sizeof(WAVEHDR)));

        m_hdr[m_currentBuffer].dwBufferLength = bufferSize*sizeof(short)/sizeof(char);
        m_hdr[m_currentBuffer].lpData = (CHAR*)buffer[m_currentBuffer];
        m_hdr[m_currentBuffer].dwLoops = 0L;
        m_hdr[m_currentBuffer].dwFlags = 0L; //WHDR_BEGINLOOP|WHDR_ENDLOOP;

        short* out = buffer[m_currentBuffer];

        for(unsigned int i=0; i<bufferSize; i++ )
        {
            if(m_loopstart < m_loopend)
            {
                if(m_playpos >= m_loopend)
                    m_playpos = m_loopstart;
            }

            if(m_playpos < m_wave.size())
                *out++ = m_wave.at(m_playpos);
            else
            {
                *out++ = 0;
            }

            m_playpos++;
        }

        VR(waveOutPrepareHeader(s_hWaveOut, &m_hdr[m_currentBuffer], sizeof(WAVEHDR)));
    }

    void PlayBuffer()
    {
        fillBuffer();

        VR(waveOutWrite(s_hWaveOut, &m_hdr[m_currentBuffer], sizeof(WAVEHDR)));

        m_currentBuffer++;
        m_currentBuffer %= 2;
    }
    virtual ExitCode Entry()
    {
        while(!TestDestroy() && m_playpos < m_wave.size())
        {
            PlayBuffer();
            WaitForSingleObject(s_event, INFINITE);
        }

        return static_cast<ExitCode>(NULL);
    }
};

PlayThread* s_playThread = NULL;

Player::Player()
{
    WAVEFORMATEX wfx; /* look this up in your documentation */
    wfx.nSamplesPerSec = 44100; /* sample rate */
    wfx.wBitsPerSample = 16; /* sample size */
    wfx.nChannels = 1; /* channels*/
    wfx.cbSize = 0; /* size of _extra_ info */
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) >> 3;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

    s_event = CreateEvent(NULL,FALSE,FALSE, L"NextBuffer");

    VR(waveOutOpen(&s_hWaveOut, -1, &wfx, (DWORD) s_event, 0, CALLBACK_EVENT));
}

Player::~Player()
{
    Stop();
    CloseHandle(s_event);
    VR(waveOutClose(s_hWaveOut));
}

void Player::Play(const std::vector<short>& wave, const unsigned int& loopstart, const unsigned int& loopend)
{
    if(s_hWaveOut)
    {
        Stop();
        s_playThread = new PlayThread(wave, loopstart, loopend);
    }
}
void Player::Stop()
{
    if(s_playThread)
    {
        s_playThread->Delete(NULL, wxTHREAD_WAIT_BLOCK);
        delete s_playThread;
        s_playThread = NULL;
    }

    //if(!(s_hdr.dwFlags & WHDR_DONE))

    //while(!(s_hdr.dwFlags & WHDR_DONE))
    //    Sleep(100);

    //VR(waveOutUnprepareHeader(s_hWaveOut, &s_hdr[0], sizeof(WAVEHDR)));
    //VR(waveOutUnprepareHeader(s_hWaveOut, &s_hdr[1], sizeof(WAVEHDR)));
}

bool Player::IsPlaying()
{
    return s_playThread != NULL;
}

unsigned int Player::GetPlayPos()
{
    if(s_playThread != NULL)
        return s_playThread->GetPlayPos(); //s_playpos;
    else
        return 0;
    //MMTIME time;
    //memset(&time, sizeof(MMTIME), 0);
    //time.wType = TIME_SAMPLES;
    //VR(waveOutGetPosition(s_hWaveOut, &time, sizeof(MMTIME)));
    //return time.u.sample;
}
