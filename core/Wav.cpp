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

#include <FileRead.h>
#include <FileWrite.h>

extern "C"
{

    int LoadWaveFile(const char *filename, short *data, size_t size)
    {
        stk::FileRead f(filename, false, 2, stk::FileRead::STK_SINT16, 441000);

        stk::StkFrames frames(f.fileSize(), f.channels());
        f.read(frames);

        if (data != nullptr && size > 0)
        {
            size = std::min(size * (size_t)f.channels(), (size_t)frames.size());

            int j = 0;
            for (size_t i = 0; i < size; i += f.channels())
                data[j++] = (short)(frames[i] * 0x7FFF);

            return j;
        }

        return (int)frames.size() / f.channels();
    }

    void SaveWaveFile(const char *filename, const short *data, size_t size)
    {
        stk::FileWrite f(filename, 1, stk::FileWrite::FILE_WAV, stk::FileWrite::STK_SINT16);

        stk::StkFrames frames(size, 1);

        for (size_t i = 0; i < frames.size(); i++)
            frames[i] = ((float)data[i]) / 0x7FFF;

        f.write(frames);
    }
}
