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

#include <string>
#include <wininet.h>

std::string CheckForUpdate(const char* httpUrl)
{
    std::string ret;

    if ( HINTERNET hINet = InternetOpen("InetURL/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 ) )
    {
        std::string url = httpUrl;

        if ( HINTERNET hFile = InternetOpenUrl( hINet, url.c_str(),
                                                NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD, 0 ) )
        {
            char buffer[1024];
            DWORD dwRead;
            while ( InternetReadFile( hFile, buffer, 1023, &dwRead ) )
            {
                if ( dwRead == 0 )
                    break;

                buffer[dwRead] = 0;

                ret += buffer;
            }

            InternetCloseHandle( hFile );
        }
        InternetCloseHandle( hINet );
    }

    return ret;
}
