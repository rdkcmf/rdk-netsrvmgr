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
#include <string>
#include <iostream>
#include <curl/curl.h>
#include <glib.h>
#include "netSrvCurl.h"
#include "netsrvmgrUtiles.h"

CurlObject::CurlObject(std::string url,char *data)
{
    LOG_ENTRY_EXIT;

    CURLcode res;
    char errbuf[CURL_ERROR_SIZE];
    long http_code; 

    curl_handle = curl_easy_init();
    if(!curl_handle)
    {
        LOG_ERR("curl failed in init");
    }
    errbuf[0] = 0;
    LOG_DBG("curl url is %s", url.c_str());
    res=curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    if(CURLE_OK != res)
        LOG_ERR("curl failed with curl error %d",res);
    if(data != NULL)
    {
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, strlen(data));
    }

    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errbuf);
    errbuf[0] = 0;
    res=curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &CurlObject::curlwritefunc);
    if(CURLE_OK != res)
        LOG_ERR("curl failed with curl error %d ", res);
    res=curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &curlDataBuffer);
    if(CURLE_OK != res)
        LOG_ERR("curl failed with curl error %d", res);
    res=curl_easy_perform(curl_handle);
    if(CURLE_OK != res)
    {
        LOG_ERR("curl failed with curl error %d ",res);
        {
            size_t len = strlen(errbuf);
            fprintf(stderr, "\nlibcurl: (%d) ", res);
            if(len)
                fprintf(stderr, "%s%s", errbuf,
                        ((errbuf[len - 1] != '\n') ? "\n" : ""));
            else
                fprintf(stderr, "%s\n", curl_easy_strerror(res));
        }
    }
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    if(http_code != 200)
        LOG_ERR("curl failed with http error %d",http_code);
    //Set member variable so that we can access the http code
    m_httpcode = http_code;
    curl_easy_cleanup(curl_handle);

}

int CurlObject::curlwritefunc(char *data, size_t size, size_t nmemb, std::string *buffer) {
    int result = 0;
    if (buffer != NULL) {
        buffer->append(data, size * nmemb);
        result = size * nmemb;
    }
    else
    {
        LOG_ERR("curl buffer NULL");

    }
    return result;
}

gchar* CurlObject::getCurlData() {
    LOG_DBG("Authservice url output %s ",curlDataBuffer.c_str());
    return g_strdup(curlDataBuffer.c_str());
}

long CurlObject::gethttpcode()
{
 return m_httpcode;
}

CurlObject::~CurlObject()
{
}
