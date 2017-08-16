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
