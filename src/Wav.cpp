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
#include <sndfile.h>

void LoadWaveFile(const char* filename, std::vector<short>& data)
{
    SF_INFO	mInfo;
    if(SNDFILE *mFile = sf_open(filename, SFM_READ, &mInfo))
    {
        data.reserve(mInfo.frames);
        data.resize(0);

        const int BUFFER_LEN = (1<<16);

        short* srcbuffer = new short[BUFFER_LEN*mInfo.channels] ;

        int buffpos = 0;

        sf_count_t block = BUFFER_LEN / mInfo.channels;
        do
        {
            block = sf_readf_short(mFile, srcbuffer, block);

            if (block)
            {
                for(int c=0; c<1; c++)//mInfo.channels; c++)
                {
                    for(int j=0; j<block; j++)
                        data.push_back( srcbuffer[mInfo.channels*j+c] );
                    //channels[c]->Append(buffer, (mFormat == int16Sample)?int16Sample:floatSample, block);
                }

                buffpos += block;
            }
        }
        while (block > 0);

        sf_close(mFile);
    }
}
