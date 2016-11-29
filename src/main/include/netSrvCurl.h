/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#ifndef NETSRVCURL_H
#define NETSRVCURL_H
#include <string>
#include <iostream>
#include <curl/curl.h>
#include "rdk_debug.h"
#include "NetworkMedium.h"
#include "NetworkMgrMain.h"

class CurlObject {
public:
    CurlObject (std::string url,char *data=NULL);
    ~CurlObject ();
    gchar* getCurlData();
    static int curlwritefunc(char *data, size_t size, size_t nmemb, std::string *buffer);
    long gethttpcode();

private:
    CURL *curl_handle;
    std::string curlDataBuffer;
    long m_httpcode;
};

#endif
