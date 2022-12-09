#include "connectivity.h"
#include "logging.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <time.h>
#include <curl/curl.h>

int get_connectivity_test_endpoints(std::vector<std::string>& endpoints)
{
    static const int MAX_CONNECTIVITY_TEST_ENDPOINTS = 5;

    LOG_ENTRY_EXIT;

    const char* filename = "/opt/persistent/connectivity_test_endpoints";

    std::ifstream ifs(filename);
    if (ifs.is_open())
    {
        for (std::string endpoint; endpoints.size() < MAX_CONNECTIVITY_TEST_ENDPOINTS && ifs >> endpoint;)
            endpoints.push_back(endpoint);
        ifs.close();
    }

    if (endpoints.size() < 2) //minimum two endpoints should be there
    {
        LOG_WARN("%d endpoints not sufficient. adding default endpoints to %s", endpoints.size(), filename);

        if (access("/lib/systemd/system/xre-receiver.service", F_OK) == 0)
        {
            endpoints.push_back("xre.ccp.xcal.tv:10601");
            endpoints.push_back("google.com");
            endpoints.push_back("espn.com");
        }
        else
        {
            endpoints.push_back("google.com");
            endpoints.push_back("espn.com");
            endpoints.push_back("speedtest.net");
        }
        set_connectivity_test_endpoints(endpoints);
    }

    std::string endpoints_str;
    for (const auto& endpoint : endpoints)
        endpoints_str.append(endpoint).append(" ");
    LOG_INFO("endpoints = %s", endpoints_str.c_str());

    return endpoints.size();
}

bool set_connectivity_test_endpoints(const std::vector<std::string>& endpoints)
{
    LOG_ENTRY_EXIT;

    std::string endpoints_string;
    for (auto endpoint : endpoints)
    {
        endpoints_string.append(endpoint.c_str());
        endpoints_string.push_back(' ');
    }
    LOG_INFO( "%s", endpoints_string.c_str());
    std::replace(endpoints_string.begin(), endpoints_string.end(), ' ', '\n');

    FILE *f = fopen("/opt/persistent/connectivity_test_endpoints", "w");
    if (f == NULL)
    {
        LOG_ERR("fopen error %d (%s)", errno, strerror(errno));
        return false;
    }
    fprintf(f, "%s", endpoints_string.c_str());
    if (0 != fclose(f))
    {
        LOG_ERR("fclose error %d (%s)", errno, strerror(errno));
        return false;
    }

    return true;
}

static long current_time ()
{
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

bool test_connectivity(long timeout_ms)
{
    LOG_ENTRY_EXIT;

    std::vector<std::string> endpoints;
    int endpoints_count = get_connectivity_test_endpoints(endpoints);
    long deadline = current_time() + timeout_ms, time_now = 0, time_earlier = 0;

    CURLM *curl_multi_handle = curl_multi_init();
    if (!curl_multi_handle)
    {
        LOG_ERR("curl_multi_init returned NULL");
        return -1;
    }

    CURLMcode mc;
    std::vector<CURL*> curl_easy_handles;
    for (const auto& endpoint : endpoints)
    {
        CURL *curl_easy_handle = curl_easy_init();
        if (!curl_easy_handle)
        {
            LOG_ERR("endpoint = <%s> curl_easy_init returned NULL", endpoint.c_str());
            continue;
        }
        curl_easy_setopt(curl_easy_handle, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl_easy_handle, CURLOPT_PRIVATE, endpoint.c_str());
        curl_easy_setopt(curl_easy_handle, CURLOPT_CONNECT_ONLY, 1L);
        curl_easy_setopt(curl_easy_handle, CURLOPT_TIMEOUT_MS, deadline - current_time());
        if (rdk_dbg_enabled(LOG_NMGR, RDK_LOG_DEBUG) == TRUE || rdk_dbg_enabled(LOG_NMGR, RDK_LOG_TRACE2) == TRUE)
             curl_easy_setopt(curl_easy_handle, CURLOPT_VERBOSE, 1L);
        if (CURLM_OK != (mc = curl_multi_add_handle(curl_multi_handle, curl_easy_handle)))
        {
            LOG_ERR("endpoint = <%s> curl_multi_add_handle returned %d (%s)", endpoint.c_str(), mc, curl_multi_strerror(mc));
            curl_easy_cleanup(curl_easy_handle);
            continue;
        }
        curl_easy_handles.push_back(curl_easy_handle);
    }

    int connectivity = 0, handles, msgs_left;
#if LIBCURL_VERSION_NUM < 0x074200
    int numfds, repeats = 0;
#endif
    char *endpoint;
    while (1)
    {
        if (CURLM_OK != (mc = curl_multi_perform(curl_multi_handle, &handles)))
        {
            LOG_ERR("curl_multi_perform returned %d (%s)", mc, curl_multi_strerror(mc));
            break;
        }
        for (CURLMsg *msg; NULL != (msg = curl_multi_info_read(curl_multi_handle, &msgs_left)); )
        {
            if (msg->msg != CURLMSG_DONE)
                continue;
            if (CURLE_OK == msg->data.result)
                connectivity++;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &endpoint);
            LOG_INFO("endpoint = <%s> error = %d (%s)", endpoint, msg->data.result, curl_easy_strerror(msg->data.result));
        }
        time_earlier = time_now;
        time_now = current_time();
        if (handles == 0 || time_now >= deadline)
            break;
#if LIBCURL_VERSION_NUM < 0x074200
        if (CURLM_OK != (mc = curl_multi_wait(curl_multi_handle, NULL, 0, deadline - time_now, &numfds)))
        {
            LOG_ERR("curl_multi_wait returned %d (%s)", mc, curl_multi_strerror(mc));
            break;
        }
        if (numfds == 0)
        {
            repeats++;
            if (repeats > 1)
                usleep(10*1000); /* sleep 10 ms */
        }
        else
            repeats = 0;
#else
        if (CURLM_OK != (mc = curl_multi_poll(curl_multi_handle, NULL, 0, deadline - time_now, NULL)))
        {
            LOG_ERR("curl_multi_poll returned %d (%s)", mc, curl_multi_strerror(mc));
            break;
        }
#endif
    }
    LOG_INFO("connectivity = %d, endpoints count = %d, handles = %d, deadline = %ld, time_now = %ld, time_earlier = %ld",
        connectivity, endpoints_count, handles, deadline, time_now, time_earlier);

    for (const auto& curl_easy_handle : curl_easy_handles)
    {
        curl_easy_getinfo(curl_easy_handle, CURLINFO_PRIVATE, &endpoint);
        LOG_DBG("endpoint = <%s> terminating attempt", endpoint);
        curl_multi_remove_handle(curl_multi_handle, curl_easy_handle);
        curl_easy_cleanup(curl_easy_handle);
    }
    curl_multi_cleanup(curl_multi_handle);

    if (connectivity >= endpoints_count - 1) // checking ~90% endpoints connectivity
        return true;
    return false;
}
