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
#include <FileRead.h>

void LoadWaveFile(const char* filename, std::vector<short>& data)
{
    stk::FileRead f(filename, false, 2, stk::FileRead::STK_SINT16, 441000);

    stk::StkFrames frames(f.fileSize(), f.channels());
    f.read(frames);

    data.clear();

    for(size_t i = 0; i < frames.size(); i+=f.channels())
        data.push_back( (short) (frames[i] * 0x7FFF));
}
