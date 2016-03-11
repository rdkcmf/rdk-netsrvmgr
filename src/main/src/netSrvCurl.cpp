#include <string>
#include <iostream>
#include <curl/curl.h>
#include <glib.h>
#include "netSrvCurl.h"

CurlObject::CurlObject(std::string url)
{
    CURLcode res;
    char errbuf[CURL_ERROR_SIZE];
    long http_code;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    curl_handle = curl_easy_init();
    if(!curl_handle)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "%s(): curl failed in init  \n",__FUNCTION__);
    }
    errbuf[0] = 0;
    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR , "%s(): curl url is %s  \n",__FUNCTION__,url.c_str());
    res=curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    if(CURLE_OK != res)
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "[%s:%d] : curl failed with curl error %d  \n",__FUNCTION__,__LINE__,res);
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errbuf);
    errbuf[0] = 0;
    res=curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &CurlObject::curlwritefunc);
    if(CURLE_OK != res)
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "[%s:%d] : curl failed with curl error %d  \n",__FUNCTION__,__LINE__,res);
    res=curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &curlDataBuffer);
    if(CURLE_OK != res)
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "[%s:%d] : curl failed with curl error %d  \n",__FUNCTION__,__LINE__,res);
    res=curl_easy_perform(curl_handle);
    if(CURLE_OK != res)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "[%s:%d] : curl failed with curl error %d  \n",__FUNCTION__,__LINE__,res);
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
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "%s(): curl failed with http error %d  \n",__FUNCTION__,http_code);

    curl_easy_cleanup(curl_handle);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );

}

int CurlObject::curlwritefunc(char *data, size_t size, size_t nmemb, std::string *buffer) {
    int result = 0;
    if (buffer != NULL) {
        buffer->append(data, size * nmemb);
        result = size * nmemb;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "%s(): curl buffer NULL  \n",__FUNCTION__);

    }
    return result;
}

gchar* CurlObject::getCurlData() {
    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR , "%s(): Authservice url output %s  \n",__FUNCTION__,curlDataBuffer.c_str());
    return g_strdup(curlDataBuffer.c_str());
}

CurlObject::~CurlObject()
{
}
