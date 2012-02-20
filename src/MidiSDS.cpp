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

/*
* Some Functions base on Code from CMIDISDSSoundTranslator.cpp (http://rezound.sourceforge.net/)
*/

#include <stdio.h>
#include <string>
#include <vector>

using namespace std;

typedef unsigned int sample_pos_t ;
typedef short sample_t;

// for MIDI bytes (3 7bit bytes to native int)
static int bytesToInt(unsigned char bh,unsigned char bm,unsigned char bl)
{
    return (((int)(bh&0x7f))<<7<<7) + (((int)(bm&0x7f))<<7) + ((int)(bl&0x7f));
}

int readMessage(FILE* fd, const char startHeader[], int headerLen, unsigned char* buffer, int size, bool optional = false)
{
    int i = 0;
    char c = 0;
    do
    {
        c = fgetc(fd);
        if(c == startHeader[i] || startHeader[i] == -1 || i >= headerLen)
        {
            buffer[i++] = c;
        }
        else
        {
            if(optional)
            {
                fseek(fd, -i, SEEK_CUR);
                return 0; //i = 0;
            }
            else
                i = 0;
        }
    }
    while (c != EOF && i < size);

    return i; //fread(buffer, 1, size, fd);
}

bool LoadMidiSDS(const char* filename, std::vector<short>& data, unsigned int& loopStart, unsigned int& loopEnd, char* sampleName, int& samplePos)
{
    data.clear();

    FILE* fd = fopen(filename, "rb");

    if(fd == NULL)
        return false;

    unsigned char buffer[256];
    for(int i = 0; i < 256; i++)
        buffer[i] = 0;

    // wait for "Dump Header" pattern: F0 7E cc 01 sl sh ee pl pm ph gl gm gh hl hm hh il im ih jj F7
    static const char dumpHeaderPattern[]= {0xf0,0x7e,-1,0x01,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0xf7};
    static const char dumpHeaderPatternLen=sizeof(dumpHeaderPattern)/sizeof(*dumpHeaderPattern);

    int l = readMessage(fd, dumpHeaderPattern, dumpHeaderPatternLen, buffer, dumpHeaderPatternLen);

    // expecting: F0 7E cc 01 sl sh ee pl pm ph gl gm gh hl hm hh il im ih jj F7 (just a double-check)
    if(!(buffer[0]==0xf0 && buffer[1]==0x7e && buffer[3]==0x01 && buffer[20]==0xf7))
    {
        return false; //" -- invalid data in expected Dump Header");
    }

    samplePos = buffer[4];

    // const int sysExChannel = buffer[2];
    // const int waveformId = bytesToInt(0,buffer[5],buffer[4]);
    const int bitRate = buffer[6];
    const unsigned sampleRate = 1000000000/bytesToInt(buffer[9],buffer[8],buffer[7]);

    // may need to take into account the bitRate when it's not 16 ???
    const sample_pos_t length = bytesToInt(buffer[12],buffer[11],buffer[10]);

    // offset in words
    // may need to take into account the bitRate when it's not 16 ???
    loopStart=bytesToInt(buffer[15],buffer[14],buffer[13]);
    loopEnd=bytesToInt(buffer[18],buffer[17],buffer[16])+1;
    const char loopType = buffer[19];
    if(loopType == 0x7F)
        loopStart = loopEnd = length;

    if(sampleRate<4000 || sampleRate>96000)
        return false;

    if(bitRate!=16)
        return false;

    data.resize(length);

    static const char dumpSampleNamePattern[]= {0xf0,0x0,0x20,0x3c,0x02,0x00,0x73,-1,-1,-1,-1,-1,0xf7};
    static const char dumpSampleNamePatternLen=sizeof(dumpSampleNamePattern)/sizeof(*dumpSampleNamePattern);

    if( readMessage(fd, dumpSampleNamePattern, dumpSampleNamePatternLen, buffer, dumpSampleNamePatternLen, true) == dumpSampleNamePatternLen)
    {
        sampleName[0] = buffer[8];
        sampleName[1] = buffer[9];
        sampleName[2] = buffer[10];
        sampleName[3] = buffer[11];
    }

    sample_pos_t lengthRead=0;
    int expectedPacketNumber=0;
    while(lengthRead<length)
    {
        // read 127 byte "Data Packet": F0 7E cc 02 kk [120 bytes here] ll F7
        static const char dataPacketPattern[]= {0xf0,0x7e,-1,0x02,-1};
        static const char dataPacketPatternLen=sizeof(dataPacketPattern)/sizeof(*dataPacketPattern);

        l = readMessage(fd, dataPacketPattern, dataPacketPatternLen, buffer, dataPacketPatternLen + 122);

        if(l!=127)
        {
            return false;
        }

        // expecting: F0 7E cc 02 kk [120 bytes here] ll F7 (just a double-check)
        if(!(buffer[0]==0xf0 && buffer[1]==0x7e && buffer[3]==0x02 && buffer[126]==0xf7))
        {
            return false; //Warning(string(__func__)+" -- invalid Data Packet received");
        }

        if(buffer[4]!=expectedPacketNumber)
        {
            return false;
        }

        // read the 120 bytes worth of sample data
        // assuming 16 bit data
        //if(typeid(sample_t)==typeid(int16_t))
        {
            unsigned char *ptr=buffer+5;
            for(int t=0; t<120 && lengthRead<length; t+=3)
            {
                // bits are left-justified in 3 byte sections except that bit 7 is unused in each byte
                sample_t s=( (((sample_t)ptr[t+0]))<<9 ) | ( (((sample_t)ptr[t+1]))<<2 ) | (((sample_t)ptr[t+2])>>5);

                data[lengthRead++]= s-0x8000;
            }

            // could perform checksum verification
        }

        // next packet
        expectedPacketNumber= (expectedPacketNumber+1)%128;
    }

    fclose(fd);
    return true;
}

bool SaveMidiSDS(int (*write)( unsigned char*, int ), void (*progress)(float ), const std::vector<short>& data, unsigned int loopStart, unsigned int loopStop, const char* sampleName, int samplePos)
{
    unsigned char buffer[256];

    // prompt for some necessary things
    const int sysExChannel = 1; //Mono
    const int sampleRate = 44100;
    const int waveformId = samplePos; //SampleNummber
    const sample_pos_t length = data.size(); // should check size limit ???

    loopStop -= 1;
    const int loopType = loopStart != loopStop && (loopStop - loopStart) < data.size() ? 0: 0x7f;//(00:forwards only; 01:alternating; F7:loop off)

    // build dump header information
    buffer[0]=0xf0;
    buffer[1]=0x7e;
    buffer[2]=sysExChannel;			    // cc
    buffer[3]=0x01;
    buffer[4]=waveformId&0x7f;		    // sl
    buffer[5]=(waveformId>>7)&0x7f;		// sh
    buffer[6]= 16;				        // ee   - 16Bit
    const int samplePeriod=1000000000/sampleRate;
    buffer[7]=samplePeriod&0x7f;		// pl
    buffer[8]=(samplePeriod>>7)&0x7f;	// pm
    buffer[9]=(samplePeriod>>14)&0x7f;	// ph

    buffer[10]=length&0x7f;			// gl
    buffer[11]=(length>>7)&0x7f;		// gm
    buffer[12]=(length>>14)&0x7f;		// gh

    buffer[13]=loopStart&0x7f;		// hl
    buffer[14]=(loopStart>>7)&0x7f;		// hm
    buffer[15]=(loopStart>>14)&0x7f;	// hh

    buffer[16]=loopStop&0x7f;		// il
    buffer[17]=(loopStop>>7)&0x7f;		// im
    buffer[18]=(loopStop>>14)&0x7f;		// ih
    buffer[19]=loopType;			// jj
    buffer[20]=0xf7;

    // write dump header
    int l = write(buffer, 21);
    if(l!=21)
    {
        return false;
    }

    buffer[0]=0xf0;
    buffer[1]=0x0;
    buffer[2]=0x20;
    buffer[3]=0x3c;
    buffer[4]=0x02;
    buffer[5]=0x00;
    buffer[6]=0x73; //MD Set SampleName
    buffer[7]=0;
    buffer[8]=sampleName[0];
    buffer[9]=sampleName[1];
    buffer[10]=sampleName[2];
    buffer[11]=sampleName[3];
    buffer[12]=0xf7;

    write(buffer, 13);

    const int SAMPLES_PER_PACK = 40; // only 40 16bit samples can fit in the packed 120 7bit bytes
    size_t datapos = 0;
    for(int packetSeq = 0; datapos < data.size(); packetSeq++)
    {
        buffer[0]=0xf0;
        buffer[1]=0x7e;
        buffer[2]=sysExChannel;
        buffer[3]=0x02;
        buffer[4]=packetSeq%128;

        unsigned char *b= &buffer[5];
        for(int t = 0; t < SAMPLES_PER_PACK; t++)
        {
            int s = 0;

            if(datapos < data.size())
                s = data[datapos++]+0x8000;
            else
                s = 0;

            (*(b++))=(s>>9)&0x7f;
            (*(b++))=(s>>2)&0x7f;
            (*(b++))=s&0x3;

            if(progress)
                progress(((float)datapos)/data.size());
        }

        // calc checksum
        unsigned char ll=0;
        for(int t=1; t<125; t++)
            ll^=buffer[t];

        ll&=0x7f;

        buffer[125]=ll;
        buffer[126]=0xf7;

        l = write(buffer, 127);
        if(l!=127)
        {
            return false;
        }
    }

    return true;
}

static FILE* s_fd = NULL;

static int s_write(unsigned char* buffer, int size)
{
    return fwrite(buffer, 1, size, s_fd);
}

bool SaveMidiSDS(const char* filename, const std::vector<short>& data, unsigned int loopStart, unsigned int loopEnd, const char* sampleName, int samplePos)
{
    s_fd = fopen(filename, "wb");
    if(s_fd==NULL)
        return false;

    SaveMidiSDS(&s_write, NULL, data, loopStart, loopEnd, sampleName, samplePos);

    fclose(s_fd);
    s_fd = NULL;

    return true;
}
/**/
