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
#define CURL_STATICLIB
#include <curl/curl.h>
#include <curl/easy.h>

size_t write_data(void *ptr, size_t size, size_t nmemb, std::string *out)
{
    for(size_t i = 0; i < size*nmemb; i++)
        *out += *(((char*)ptr)+i);

    return size*nmemb;
}

std::string CheckForUpdate(const char* httpUrl)
{
    std::string ret;

    if (CURL *curl = curl_easy_init())
    {
        curl_easy_setopt(curl, CURLOPT_URL, httpUrl);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    return ret;
}
