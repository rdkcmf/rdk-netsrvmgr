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

#include <iostream>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <glib.h>
#include <cJSON.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include "routeSrvMgr.h"
#include "NetworkMgrMain.h"
#include "NetworkMedium.h"
#include "netsrvmgrUtiles.h"
#include <fstream>
#ifdef ENABLE_NLMONITOR
#include "netlinkifc.h"
#endif //ENABLE_NLMONITOR



#define ROUTE_PRIORITY 50
#define GW_SETUP_FILE "/lib/rdk/gwSetup.sh"
#define MAX_CJSON_EMPTY_LENGTH 40
#define IP_SIZE 46
#define DHCP_LEASE_FLAG "/tmp/usingdhcp"
#define PREFERRED_GATEWAY_FILE		"/opt/prefered-gateway"

#ifdef YOCTO_BUILD
extern "C" {
#include "secure_wrapper.h"
}
#endif

int messageLength;
GList* gwList = NULL;
GList* gwRouteInfo = NULL;
bool lastRouteSetV4=FALSE;
char routeIf[15];
bool signalUpnpDataReady=true;

RouteNetworkMgr* RouteNetworkMgr::instance = NULL;
bool RouteNetworkMgr::instanceIsReady = false;

static bool gwSelected = false;
static bool xb3Selected = false;
static bool upnpGwyLost = true;

pthread_cond_t condRoute = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexRoute = PTHREAD_MUTEX_INITIALIZER;

RouteNetworkMgr::RouteNetworkMgr() {}
RouteNetworkMgr::~RouteNetworkMgr() { }

RouteNetworkMgr* RouteNetworkMgr::getInstance()
{
    if (instance == NULL)
    {
        instance = new RouteNetworkMgr();
        instanceIsReady = true;
    }
    return instance;
}


int  RouteNetworkMgr::Start()
{
    IARM_Bus_RegisterEventHandler(IARM_BUS_SYSMGR_NAME,IARM_BUS_SYSMGR_EVENT_XUPNP_DATA_UPDATE,_evtHandler);
    IARM_Bus_RegisterCall(IARM_BUS_ROUTE_MGR_API_getCurrentRouteData, getCurrentRouteData);


    //Check to see the preferred Gateway contents.
    std::ifstream cfgFile(PREFERRED_GATEWAY_FILE);
    std::string prefgw;
    if (cfgFile.is_open())
    {
       cfgFile>>prefgw;
       if (prefgw == "XB3")
       {
          xb3Selected = true;
          RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Preferred Gateway is XB \n", MODULE_NAME,__FUNCTION__, __LINE__ );
       }
       cfgFile.close();
    }


    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    getGatewayRouteData();
    return 0;
}



/**
 * @fn void RouteNetworkMgr::_evtHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
 * @brief This function is a common event handler for IARM Manager events. It receives events
 * from Power Manager, SYS Manager, IR Manager etc.
 *
 * @param[in] owner Name of the Manager application in string format.
 * @param[in] eventId Variable of IARM_EventId_t type representing event identifier.
 * @param[in] data Event data as void pointer.
 * @param[in] len Length of the data buffer.
 *
 * @return None.
 */
static void _evtHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if (strcmp(owner, IARM_BUS_SYSMGR_NAME) == 0) {
        switch(eventId) {
        case IARM_BUS_SYSMGR_EVENT_XUPNP_DATA_UPDATE:
        {
            IARM_Bus_SYSMgr_EventData_t *eventData = (IARM_Bus_SYSMgr_EventData_t*)data;
            pthread_mutex_lock(&mutexRoute);
            signalUpnpDataReady=true;
            if(0 == pthread_cond_signal(&condRoute))
            {
                messageLength = eventData->data.xupnpData.deviceInfoLength;
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Signal to fetch the data from upnp  %d \n", MODULE_NAME,__FUNCTION__, __LINE__, eventData->data.xupnpData.deviceInfoLength );
            }
            pthread_mutex_unlock(&mutexRoute);
            break;
        }
        }
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
}

/**
 * @fn bool RouteNetworkMgr::getGatewayResults(char* gatewayResults, unsigned int messageLength)
 * @brief This function used IARM Bus call to get XUPNP device information and returns
 * the result as a string.
 *
 * @param[out] gatewayResults String to store discovery results.
 * @param[in] messageLength Unsigned integer variable representing buffer length.
 *
 * @return None.
 */

bool RouteNetworkMgr::getGatewayResults(char* gatewayResults, unsigned int messageLength)
{
    IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t *param = NULL;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool returnStatus = FALSE;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    ret = IARM_Malloc(IARM_MEMTYPE_PROCESSLOCAL, sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t) + messageLength + 1, (void**)&param);
    if(ret == IARM_RESULT_SUCCESS)
    {
        param->bufLength = messageLength;

        ret = IARM_Bus_Call(_IARM_XUPNP_NAME,IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo,
                            (void *)param, sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t) + messageLength + 1);

        if(ret == IARM_RESULT_SUCCESS)
        {
            memcpy(gatewayResults, ((char *)param + sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t)), param->bufLength);
            gatewayResults[param->bufLength] = '\0';
            returnStatus =  TRUE;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo IARM failed in the fetch  \n", MODULE_NAME,__FUNCTION__, __LINE__);
        }
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] gatewayResults %s \n", MODULE_NAME,__FUNCTION__, __LINE__,gatewayResults);
        IARM_Free(IARM_MEMTYPE_PROCESSLOCAL, param);
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo , IARM_Malloc Failed  \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    return returnStatus;
}

void getGatewayRouteData()
{
    pthread_t getGatewayRouteDataThread;
    pthread_attr_t attr;
    int rc;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&getGatewayRouteDataThread, &attr, &getGatewayRouteDataThrd, NULL);
    if (rc) {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"ERROR; return code from pthread_create() is %d\n", rc);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

/**
 * @fn void RouteNetworkMgr::storeRouteDetails(unsigned int messageLength)
 * @brief This is invoked when there is a new gateway data
 *
 * @param[in] messageLength Length of buffer.
 *
 * @return None.
 */

gboolean RouteNetworkMgr::storeRouteDetails(unsigned int messageLength)
{
    char upnpResults[messageLength+1];
    gboolean retVal=FALSE;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    memset(&upnpResults,0,sizeof(upnpResults));
    if(getGatewayResults(&upnpResults[0], messageLength))
    {
        if(parse_store_gateway_data(&upnpResults[0]))
            retVal=TRUE;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"Failure in getting gateway results. \n");
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retVal;
}

void* getGatewayRouteDataThrd(void* arg)
{
    int ret = 0;
    int msgLength;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    while (true ) {
        pthread_mutex_lock(&mutexRoute);
        while(signalUpnpDataReady == false) {
            pthread_cond_wait(&condRoute, &mutexRoute);
        }
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n[%s:%s:%d] ***** Started fetching gateway data from upnp msg len = %d  ***** \n", MODULE_NAME,__FUNCTION__, __LINE__,messageLength );
        msgLength = messageLength;
        signalUpnpDataReady=false;
        pthread_mutex_unlock(&mutexRoute);
//        RouteNetworkMgr::delGatewayList();
//        if(msgLength > MAX_CJSON_EMPTY_LENGTH)
//        {
        if(RouteNetworkMgr::storeRouteDetails(msgLength))
        {
            if(RouteNetworkMgr::checkExistingRouteValid())
            {
                if(RouteNetworkMgr::setRoute())
                    RouteNetworkMgr::sendCurrentRouteData();
            }
//                sendDefaultGatewayRoute();
        }
        else
        {
            RouteNetworkMgr::delRouteList();
        }
	gwRouteInfo = g_list_first(gwRouteInfo);
        if((g_list_length(gwRouteInfo) == 0) && ( access( DHCP_LEASE_FLAG, F_OK ) == -1 ))
        {
            if (!upnpGwyLost)
            {
                if (!xb3Selected)
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Triggering dhcp lease since no XG gateway  \n", MODULE_NAME,__FUNCTION__, __LINE__);
                    netSrvMgrUtiles::triggerDhcpLease();
                    upnpGwyLost=true;
                }
            }
#ifdef ENABLE_NLMONITOR
            //Clear out /tmp/resolv.dnsmasq.upnp
            std::ofstream ofs;
            ofs.open("/tmp/resolv.dnsmasq.upnp", std::ofstream::out | std::ofstream::trunc);
            ofs.close();

            if (gwSelected)
            {
               list<std::string> ifcList;
               if (getenv("WIFI_INTERFACE") != NULL)
               {
                  ifcList.push_back(getenv("WIFI_INTERFACE"));
               }

               if (getenv("MOCA_INTERFACE") != NULL)
               {
                  ifcList.push_back(getenv("MOCA_INTERFACE"));
               }
               gwSelected = false;
               for (auto const& i : ifcList)
               {
                  std::string ifcStr = i;
                  std::string cmd = "/lib/rdk/enableIpv6Autoconf.sh ";
                  cmd += ifcStr;
                  //Invoke API to cleanup Global IPs assigned.
                  NetLinkIfc::get_instance()->deleteinterfaceip(ifcStr,AF_INET6);
                  NetLinkIfc::get_instance()->deleteinterfaceroutes(ifcStr,AF_INET6);
                  //Call script to enable SLAAC ra support.
#ifdef YOCTO_BUILD
                  v_secure_system(cmd.c_str());
#else
                  system(cmd.c_str());
#endif
               }
               RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] No Gateway Detected, SLAAC Support is enabled\n", MODULE_NAME,__FUNCTION__, __LINE__ );
            }
#endif//ENABLE_NLMONITOR
        }
//        }
//	else
//	{
//            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n[%s:%s:%d] No Gateway in UPNP list  msgLength = %d msglenlocal = %d  ***** \n", MODULE_NAME,__FUNCTION__, __LINE__,messageLength,msgLength );
//	}
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

/**
 * @brief This will check whether the input IP Address is a valid IPv4 or IPv6 address.
 *
 * @param[in] ipAddress IP Address in string format.
 *
 * @return Returns TRUE if IP address is valid else returns FALSE.
 * @ingroup
 */
gboolean checkvalidip( char* ipAddress)
{
    struct in_addr addr4;
    struct in6_addr addr6;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    int validip4 = inet_pton(AF_INET, ipAddress, &addr4);
    int validip6 = inet_pton(AF_INET6, ipAddress, &addr6);
    if (g_strrstr(g_strstrip(ipAddress),"null") || ! *ipAddress )
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] ipaddress are empty %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,ipAddress);
        return TRUE;
    }
    if ((validip4 == 1 ) || (validip6 == 1 ))
    {
        return TRUE;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Not a valid ip address %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,ipAddress);
        return FALSE;
    }
}
/**
 * @brief This function will check whether a Host name is valid by validating all the associated IP addresses.
 *
 * @param[in] hostname Host name represented as a string.
 *
 * @return Returns TRUE if host name is valid else returns FALSE.
 * @ingroup
 */
gboolean checkvalidhostname( char* hostname)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if (g_strrstr(g_strstrip(hostname),"null") || ! *hostname )
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] hostname  values are empty %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,hostname);
        return TRUE;
    }
    gchar **tokens = g_strsplit_set(hostname," ;\n\0", -1);
    guint toklength = g_strv_length(tokens);
    guint loopvar=0;
    for (loopvar=0; loopvar<toklength; loopvar++)
    {
        if (checkvalidip(tokens[loopvar]))
        {
            return TRUE;
        }
    }
    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] no valid ip so rejecting the dns values %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,hostname);
    if(tokens)
        g_strfreev(tokens); /*CID-24000*/
    return FALSE;
}

gboolean RouteNetworkMgr::init_gwydata(GwyDeviceData* gwydata)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    gwydata->serial_num = g_string_new(NULL);
    gwydata->gwyip = g_string_new(NULL);
    gwydata->gwyipv6 = g_string_new(NULL);
    gwydata->ipv6prefix = g_string_new(NULL);
    gwydata->isRouteSet=FALSE;
    gwydata->dnsconfig = g_string_new(NULL);
    gwydata->devicetype = g_string_new(NULL);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}

gboolean RouteNetworkMgr::free_gwydata(GwyDeviceData* gwydata)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(gwydata)
    {
        g_string_free(gwydata->serial_num, TRUE);
        g_string_free(gwydata->gwyip, TRUE);
        g_string_free(gwydata->gwyipv6, TRUE);
        g_string_free(gwydata->dnsconfig, TRUE);
        g_string_free(gwydata->ipv6prefix, TRUE);
        g_string_free(gwydata->devicetype, TRUE);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}
gboolean RouteNetworkMgr::readDevFile(char *deviceFile,char *mocaIface,char *wifiIface)
{
    GError                  *error=NULL;
    gboolean                result = FALSE;
    gchar* devfilebuffer = NULL;
    gchar counter=0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if (deviceFile == NULL)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] device properties file not found \n", MODULE_NAME,__FUNCTION__, __LINE__);
        return result;
    }
    result = g_file_get_contents (deviceFile, &devfilebuffer, NULL, &error);
    if (result == FALSE)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] No contents in device properties \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    else
    {
        /* reset result = FALSE to identify device properties from devicefile contents */
        result = FALSE;
        gchar **tokens = g_strsplit_set(devfilebuffer,",='\n'", -1);
        guint toklength = g_strv_length(tokens);
        guint loopvar=0;
        for (loopvar=0; loopvar<toklength; loopvar++)
        {
            if (g_strrstr(g_strstrip(tokens[loopvar]), "MOCA_INTERFACE"))
            {
                if ((loopvar+1) < toklength )
                {
                    counter++;
                    g_stpcpy(mocaIface, g_strstrip(tokens[loopvar+1]));
                }
                if (counter == 2)
                {
                    break;
                }
            }
            if (g_strrstr(g_strstrip(tokens[loopvar]), "WIFI_INTERFACE"))
            {
                if ((loopvar+1) < toklength )
                {
                    counter++;
                    g_stpcpy(wifiIface, g_strstrip(tokens[loopvar+1]));
                }
                if (counter == 2)
                {
                    break;
                }
            }
        }
        g_strfreev(tokens);
        g_free(devfilebuffer);
    }
    if((!mocaIface) && (!wifiIface))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] MOCA_INTERFACE  and WIFI_INTERFACE  not found in %s \n", MODULE_NAME,__FUNCTION__, __LINE__,deviceFile);
    }
    else
        result = TRUE;
    if(error)
    {
        /* g_clear_error() frees the GError *error memory and reset pointer if set in above operation */
        g_clear_error(&error);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return result;
}
gboolean RouteNetworkMgr::delGatewayList()
{
    guint gwListLength=0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    gwList = g_list_first(gwList);
    gwListLength = g_list_length(gwList);
    if (gwListLength > 0)
    {
        GwyDeviceData *gwdata = NULL;
        while (gwList && (gwListLength > 0))
        {
            gwdata = (GwyDeviceData*)gwList->data;
            gwList = g_list_next(gwList);
            free_gwydata(gwdata);
            g_free(gwdata);
            gwList = g_list_remove(gwList, gwdata);
            gwListLength--;
        }
        g_list_free(gwList);
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] NO gateway data available \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}

gboolean RouteNetworkMgr::delRouteList()
{
    guint gwRouteLength=0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    gwRouteInfo = g_list_first(gwRouteInfo);
    gwRouteLength = g_list_length(gwRouteInfo);
    if (gwRouteLength > 0)
    {
        routeInfo *gwdata = NULL;
        while (gwRouteInfo && (gwRouteLength > 0))
        {
            gwdata = (routeInfo*)gwRouteInfo->data;
            checkRemoveRouteInfo(gwdata->ipStr->str,gwdata->isIPv4);
            gwRouteInfo = g_list_next(gwRouteInfo);
            gwRouteLength--;
        }
        if(gwRouteInfo)
        {
          g_list_free(gwRouteInfo);
          gwRouteInfo=NULL;
        }
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] NO gateway data available \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}

gboolean  RouteNetworkMgr::parse_store_gateway_data(char *array)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    GList *tempGwList;
    guint counter;
    gboolean retVal=TRUE;
    if(array)
    {
        delGatewayList();
        cJSON *rootJson=cJSON_Parse(array);
        if(rootJson)
        {
            cJSON *gwFullData = cJSON_GetObjectItem(rootJson,"xmediagateways");
            if(gwFullData)
            {
                guint gwCount = cJSON_GetArraySize(gwFullData);
		RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] gateway count in json %d \n", __FUNCTION__, __LINE__,gwCount);
                char *isgateway,*dnsConfig,*gatewayIp,*gatewayIpV6,*serialNum,*ipv6Prefix,*devType;

                for (counter = 0; counter < gwCount; counter++)
                {
                    cJSON *gwData = cJSON_GetArrayItem(gwFullData, counter);
                    isgateway = dnsConfig = gatewayIp = gatewayIpV6 = serialNum = ipv6Prefix = devType = NULL;
                    if(gwData) {
                        if (cJSON_GetObjectItem(gwData, "isgateway"))
                            isgateway = cJSON_GetObjectItem(gwData, "isgateway")->valuestring;
                        if(cJSON_GetObjectItem(gwData, "dnsconfig"))
                            dnsConfig = cJSON_GetObjectItem(gwData, "dnsconfig")->valuestring;
                        if(cJSON_GetObjectItem(gwData, "gatewayip"))
                            gatewayIp = cJSON_GetObjectItem(gwData, "gatewayip")->valuestring;
                        if(cJSON_GetObjectItem(gwData, "gatewayipv6"))
                            gatewayIpV6 = cJSON_GetObjectItem(gwData, "gatewayipv6")->valuestring;
                        if(cJSON_GetObjectItem(gwData, "sno"))
                            serialNum = cJSON_GetObjectItem(gwData, "sno")->valuestring;
                        if(cJSON_GetObjectItem(gwData, "ipv6Prefix"))
                            ipv6Prefix = cJSON_GetObjectItem(gwData, "ipv6Prefix")->valuestring;
                        if(cJSON_GetObjectItem(gwData, "DevType"))
                            devType = cJSON_GetObjectItem(gwData, "DevType")->valuestring;

                        if((isgateway) && (g_strrstr(g_strstrip(isgateway),"yes")) && (dnsConfig) && (checkvalidhostname(dnsConfig) == TRUE ) && (gatewayIp) && (checkvalidip(gatewayIp) == TRUE) && (gatewayIpV6) &&(checkvalidip(gatewayIpV6) == TRUE))
                        {
                            GwyDeviceData *gwydata = g_new(GwyDeviceData,1);
                            init_gwydata(gwydata);
                            g_string_assign(gwydata->serial_num,serialNum);
                            g_string_assign(gwydata->gwyip,gatewayIp);
                            g_string_assign(gwydata->gwyipv6,gatewayIpV6);
                            g_string_assign(gwydata->dnsconfig,dnsConfig);
                            g_string_assign(gwydata->ipv6prefix,ipv6Prefix);
                            g_string_assign(gwydata->devicetype,devType);
                            gwList=g_list_append(gwList,gwydata);
                            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] serial_num  %s gwcount %d \n", MODULE_NAME,__FUNCTION__, __LINE__ ,gwydata->serial_num->str,g_list_length(gwList));
                        }
                        else
                        {
                            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] not a valid gateway %s \n", MODULE_NAME,__FUNCTION__, __LINE__,serialNum);
                        }
                    } else {
                        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed to get gateway details,Array item is Null. \n",MODULE_NAME,__FUNCTION__, __LINE__);
                    }
                }
                gwList = g_list_first(gwList);
                if( g_list_length(gwList) == 0)
		{
                    retVal=FALSE;
                    delRouteList();
		}
            }
            else
            {
                checkExistingRouteValid();
                retVal=FALSE;
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] No gwdata to parse gwcount \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            }
            cJSON_Delete(rootJson);
        }
	else
	{
	     retVal=FALSE;
             RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] JSON is empty \n",__FUNCTION__, __LINE__ );
	 
	}
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] gateway list is empty \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        retVal=FALSE;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retVal;
}

gboolean RouteNetworkMgr::getRouteInterface(char * routeIf)
{
    char devFile[] = "//etc//device.properties";
    char mocaIf[10]= {0};
    char wifiIf[10]= {0};
    char ipAddressBuffer[46];
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(readDevFile(devFile,mocaIf,wifiIf))
    {
        int result = getipaddress(mocaIf,ipAddressBuffer,FALSE);
        if (!result)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Could not locate the ipaddress of the broadcast interface %s \n", MODULE_NAME,__FUNCTION__, __LINE__,mocaIf );
            result = getipaddress(wifiIf,ipAddressBuffer,FALSE);
            if (!result)
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Could not locate the ipaddress of the broadcast interface %s \n", MODULE_NAME,__FUNCTION__, __LINE__,wifiIf );
                return FALSE;
            }
            else
            {
                g_stpcpy(routeIf,wifiIf);
            }
        }
        else
        {
            g_stpcpy(routeIf,mocaIf);
        }
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] No route interface available\n", MODULE_NAME,__FUNCTION__, __LINE__);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}

gboolean RouteNetworkMgr::setRoute() {
    gint retType;
    gboolean firstXG1GwData=TRUE;
    gboolean firstXG2GwData=TRUE;
    gboolean setRoute=FALSE;
    GList *element = NULL;
    gboolean retVal=FALSE;
    GwyDeviceData *gwdata;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    element = g_list_first(gwList);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] gw list count %d \n", MODULE_NAME,__FUNCTION__, __LINE__ ,g_list_length(gwList));
    getRouteInterface(routeIf);
    if(!routeIf)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] No Interface available to add route  \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return retVal;
    }
    while(element)
    {
        gwdata=((GwyDeviceData *)element->data);
        element = g_list_next(element);
        if(!gwdata)
        {
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] gwdata NULL \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            return FALSE;
        }
        if(g_strcmp0(gwdata->devicetype->str,"XG2") == 0)
        {
            if(firstXG2GwData == TRUE)
            {
                firstXG2GwData=FALSE;
                setRoute=TRUE;
            }
        }
        else
        {
            if(firstXG1GwData == TRUE)
            {
                firstXG1GwData = FALSE;
                setRoute=TRUE;
            }
        }
        if(setRoute)
        {
            upnpGwyLost=false;
#ifdef ENABLE_NLMONITOR
            if ((!gwSelected) && (!xb3Selected))
            {
               list<std::string> ifcList;
               if (getenv("WIFI_INTERFACE") != NULL)
               {
                  ifcList.push_back(getenv("WIFI_INTERFACE"));
               }

               if (getenv("MOCA_INTERFACE") != NULL)
               {
                  ifcList.push_back(getenv("MOCA_INTERFACE"));
               }
               gwSelected = true;

                for (auto const& i : ifcList)
                {
                  //Call script to disable SLAAC ra support.
                  std::string ifcStr = i;
                  std::string cmd = "/lib/rdk/disableIpv6Autoconf.sh ";
                  cmd += ifcStr;
#ifdef YOCTO_BUILD
                  v_secure_system(cmd.c_str());
#else
                  system(cmd.c_str());
#endif

                  //Invoke API to cleanup Global IPs assigned.
                  NetLinkIfc::get_instance()->deleteinterfaceip(ifcStr,AF_INET6);
                  NetLinkIfc::get_instance()->deleteinterfaceroutes(ifcStr,AF_INET6);
                }
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Gateway Detecetd, SLAAC Support is disabled.\n", MODULE_NAME,__FUNCTION__, __LINE__ );
            }
#endif //ENABLE_NLMONITOR

            GString *GwRouteParam=g_string_new(NULL);
            g_string_printf(GwRouteParam,"%s" ,GW_SETUP_FILE);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->gwyip->str);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->dnsconfig->str);
            g_string_append_printf(GwRouteParam," \"%s\"" ,routeIf);
            g_string_append_printf(GwRouteParam," \"%d\"" ,ROUTE_PRIORITY);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->ipv6prefix->str);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->gwyipv6->str);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->devicetype->str);
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Calling gateway script %s  \n", MODULE_NAME,__FUNCTION__, __LINE__,GwRouteParam->str);
#ifdef YOCTO_BUILD
            retType=v_secure_system(GwRouteParam->str);
#else
            retType=system(GwRouteParam->str);
#endif
            g_string_free(GwRouteParam,TRUE);
        }
        if(retType == SYSTEM_COMMAND_ERROR)
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Error has occured in shell command  \n", MODULE_NAME,__FUNCTION__, __LINE__);
        }
        else if (WEXITSTATUS(retType) == SYSTEM_COMMAND_SHELL_NOT_FOUND)
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] That shell command is not found  \n", MODULE_NAME,__FUNCTION__, __LINE__);
        }
        else if (WEXITSTATUS(retType) == SYSTEM_COMMAND_SHELL_SUCESS)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] system call route set successfully %d  \n", MODULE_NAME,__FUNCTION__, __LINE__,WEXITSTATUS(retType));
            gwdata->isRouteSet=TRUE;
            if(checkIpMode(gwdata->ipv6prefix->str))
            {
                checkAddRouteInfo(gwdata->gwyipv6->str,FALSE,gwdata->ipv6prefix->str);
            }
            else
            {
                checkAddRouteInfo(gwdata->gwyip->str,TRUE,gwdata->ipv6prefix->str);
            }
            retVal=TRUE;
        }
        else
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] no change in routing information %d  \n", MODULE_NAME,__FUNCTION__, __LINE__,WEXITSTATUS(retType));
        }
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retVal;
}

/**
 * @brief This function is used to get the IP address based on IPv6 or IPv4 is enabled.
 *
 * @param[in] ifname Name of the network interface.
 * @param[out] ipAddressBuffer Character buffer to hold the IP address.
 * @param[in] ipv6Enabled Flag to check whether IPV6 is enabled
 *
 * @return Returns an integer value '1' if successfully gets the IP address else returns '0'.
 * @ingroup
 */
int RouteNetworkMgr::getipaddress(char* ifname, char* ipAddressBuffer, gboolean ipv6Enabled)
{
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    getifaddrs(&ifAddrStruct);
    //char addressBuffer[INET_ADDRSTRLEN] = NULL;
    int found=0;
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        if (ipv6Enabled == TRUE)
        {   // check it is IP6
            // is a valid IP6 Address
            if ((strcmp(ifa->ifa_name,ifname)==0) && (ifa ->ifa_addr->sa_family==AF_INET6))
            {
                tmpAddrPtr=&((struct sockaddr_in6  *)ifa->ifa_addr)->sin6_addr;
                inet_ntop(AF_INET6, tmpAddrPtr, ipAddressBuffer, INET6_ADDRSTRLEN);
                //if (strcmp(ifa->ifa_name,"eth0")==0trcmp0(g_strstrip(devConf->mocaMacIf),ifname) == 0)
                if (IN6_IS_ADDR_LINKLOCAL(tmpAddrPtr))
                {
                    found = 1;
                    break;
                }
            }
        }
        else {
            if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
                // is a valid IP4 Address
                tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, tmpAddrPtr, ipAddressBuffer, INET_ADDRSTRLEN);
                //if (strcmp(ifa->ifa_name,"eth0")==0)
                if (strcmp(ifa->ifa_name,ifname)==0)
                {
                    found = 1;
                    break;
                }
            }
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return found;
}

gboolean RouteNetworkMgr::checkAddRouteInfo(char *ipAddr,bool isIPv4,char *ipv6Pfix)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    routeInfo *gwdata=NULL;
    gwRouteInfo=g_list_first(gwRouteInfo);
    GList* tmpGWRouteInfo=g_list_find_custom(gwRouteInfo,ipAddr,(GCompareFunc)g_list_find_ip);
    if(tmpGWRouteInfo)
        gwdata = (routeInfo*)tmpGWRouteInfo->data;
    if(!tmpGWRouteInfo)
    {
        lastRouteSetV4=isIPv4;
        addRouteToList(ipAddr,isIPv4,ipv6Pfix);
    }
    else if ( (!isIPv4) && (gwdata) && (g_strcmp0(g_strstrip(gwdata->ipv6Pfix->str),ipv6Pfix) != 0))
    {
	RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Adding route since prefix changed from %s to %s \n",__FUNCTION__, __LINE__,gwdata->ipv6Pfix->str,ipv6Pfix);
        lastRouteSetV4=isIPv4;
        addRouteToList(ipAddr,isIPv4,ipv6Pfix);
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Route %s is already in the list \n", MODULE_NAME,__FUNCTION__, __LINE__, ipAddr);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}

gboolean RouteNetworkMgr::addRouteToList(char *ipAddr,bool isIPv4,char *ipv6Pfix)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    routeInfo *routeInfoData = g_new(routeInfo,1);
    routeInfoData->isIPv4 = isIPv4;
    routeInfoData->ipStr = g_string_new(NULL);
    routeInfoData->ipv6Pfix = g_string_new(NULL);
    g_string_assign(routeInfoData->ipStr,ipAddr);
    g_string_assign(routeInfoData->ipv6Pfix,ipv6Pfix);
    gwRouteInfo=g_list_prepend(gwRouteInfo,routeInfoData);
    RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] length of route list is %d \n",MODULE_NAME,__FUNCTION__, __LINE__,g_list_length(gwRouteInfo));
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;

}
gboolean RouteNetworkMgr::checkRemoveRouteInfo(char *ipAddr,bool isIPv4)
{
    guint gwRouteLength=0;
//    GList* routeList;
    routeInfo *routeInfoData;
    gint retType;
    gboolean retVal=FALSE;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    gwRouteInfo=g_list_first(gwRouteInfo);
    gwRouteLength = g_list_length(gwRouteInfo);
    if (gwRouteLength > 0)
    {
        gwRouteInfo=g_list_find_custom(gwRouteInfo,ipAddr,(GCompareFunc)g_list_find_ip);
        if( gwRouteInfo != NULL)
        {
            GString* command=g_string_new(NULL);
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Route to be removed  ******* %s ******* \n", MODULE_NAME,__FUNCTION__, __LINE__,ipAddr);
            if (isIPv4)
            {
                g_string_printf(command, "route del default gw %s", ipAddr);
            }
            else
            {
                g_string_printf(command, "ip -6 route del default via %s", ipAddr);
            }
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Remove Route ******* %s ******* \n", MODULE_NAME,__FUNCTION__, __LINE__,command->str);
#ifdef YOCTO_BUILD
            retType=v_secure_system(command->str);
#else
            retType=system(command->str);
#endif
            if(retType == SYSTEM_COMMAND_ERROR)
            {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Error has occured in shell command  \n", MODULE_NAME,__FUNCTION__, __LINE__);
            }
            else if (WEXITSTATUS(retType) == SYSTEM_COMMAND_SHELL_NOT_FOUND)
            {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] That shell command is not found  \n", MODULE_NAME,__FUNCTION__, __LINE__);
            }
            else
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] system call route set successfully %d  \n", MODULE_NAME,__FUNCTION__, __LINE__,WEXITSTATUS(retType));
                retVal=TRUE;
            }
            sendCurrentRouteData();
            routeInfoData=(routeInfo *)gwRouteInfo->data;
            removeRouteFromList(routeInfoData);
            g_string_free(command,TRUE);
        }
        else
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Existing route exist in the new list \n", MODULE_NAME,__FUNCTION__, __LINE__);
        }
    }
    else
    {
      RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Saved gw list is empty \n", MODULE_NAME,__FUNCTION__,__LINE__);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retVal;
}
gboolean RouteNetworkMgr::removeRouteFromList(routeInfo *routeInfoData)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if((routeInfoData) && (gwRouteInfo))
    {
        gwRouteInfo = g_list_first(gwRouteInfo);
        gwRouteInfo = g_list_remove(gwRouteInfo, routeInfoData);
        if(routeInfoData)
        {
            g_string_free(routeInfoData->ipStr,TRUE);
            g_string_free(routeInfoData->ipv6Pfix,TRUE);
            g_free(routeInfoData);
        }
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] route info data is NULL \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;

}

bool checkIpMode(char *v6Prefix)
{
    bool retVal=TRUE;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if (g_strrstr(g_strstrip(v6Prefix),"null") ||  ! *(v6Prefix))
        retVal=FALSE;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retVal;
}

gboolean RouteNetworkMgr::checkExistingRouteValid()
{
    guint gwRouteLength=0;
    guint tempGwRouteLength=0;
    GwyDeviceData *gwdata = NULL;
    routeInfo *routeInfoData=NULL;
    GList* tmpGWList;
    char tempIP[46];
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    gwRouteInfo=g_list_first(gwRouteInfo);
    gwRouteLength = g_list_length(gwRouteInfo);
    tempGwRouteLength = gwRouteLength;
    if (gwRouteLength > 0)
    {
        GList *element = g_list_first(gwRouteInfo);
        while (element && (gwRouteLength > 0))
        {
            routeInfoData = (routeInfo*)element->data;
            element = g_list_next(element);
            gwList=g_list_first(gwList);
            g_stpcpy(tempIP,routeInfoData->ipStr->str);
            tmpGWList=g_list_find_custom(gwList,tempIP,(GCompareFunc)g_list_find_gw);
            if(tmpGWList)
                gwdata = (GwyDeviceData*)tmpGWList->data;
            if(!tmpGWList)
//            if((g_list_find(gwList,tempIP) == NULL))
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] route %s  is not there in new list proceed to remove it \n", MODULE_NAME,__FUNCTION__, __LINE__,routeInfoData->ipStr->str);
                checkRemoveRouteInfo(routeInfoData->ipStr->str,routeInfoData->isIPv4);
            }
	    else if((!routeInfoData->isIPv4) && (gwdata) && (g_strcmp0(g_strstrip(routeInfoData->ipv6Pfix->str),g_strstrip(gwdata->ipv6prefix->str)) != 0))
	    {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] prefix changed from %s to %s \n",__FUNCTION__, __LINE__,routeInfoData->ipv6Pfix->str,gwdata->ipv6prefix->str);
                checkRemoveRouteInfo(routeInfoData->ipStr->str,routeInfoData->isIPv4);
            }
            else
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] route   is there in new list \n", MODULE_NAME,__FUNCTION__, __LINE__);
            }
        }
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if((g_list_length(gwRouteInfo) != 0 ) && (g_list_length(gwRouteInfo) != tempGwRouteLength))
    {
	RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] existing route are fine \n",__FUNCTION__, __LINE__);
	return FALSE;
    }
    return TRUE;
}
gboolean RouteNetworkMgr::printExistingRouteValid()
{
    guint gwRouteLength=0;
    GwyDeviceData *gwdata = NULL;
    routeInfo *routeInfoData=NULL;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    gwRouteLength = g_list_length(gwRouteInfo);
    if (gwRouteLength > 0)
    {
        GList *element = g_list_first(gwRouteInfo);
        while (element && (gwRouteLength > 0))
        {
            routeInfoData = (routeInfo*)element->data;
            element = g_list_next(element);
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] ipstring %s isipv4 %d gwRouteLength %d \n", MODULE_NAME,__FUNCTION__, __LINE__,routeInfoData->ipStr->str,routeInfoData->isIPv4,gwRouteLength);
        }
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] local route list is empty \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}




gboolean RouteNetworkMgr::printGatewayList()
{
    guint gwListLength=0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    gwListLength = g_list_length(gwList);
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] gateway count %d \n", MODULE_NAME,__FUNCTION__, __LINE__,gwListLength);
    if (gwListLength > 0)
    {
        GwyDeviceData *gwdata = NULL;
        GList *element = g_list_first(gwList);
        while (element && (gwListLength > 0))
        {
            gwdata = (GwyDeviceData*)element->data;
            element = g_list_next(element);
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] gateway list %s \n", MODULE_NAME,__FUNCTION__, __LINE__,gwdata->serial_num->str);
        }
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] local gateway list is empty \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}

guint RouteNetworkMgr::g_list_find_ip(routeInfo* gwData, gconstpointer* ip )
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if (g_strcmp0(g_strstrip(gwData->ipStr->str),g_strstrip((gchar *)ip)) == 0)
        return 0;
    else
        return 1;
}
guint RouteNetworkMgr::g_list_find_gw(GwyDeviceData* gwData, gconstpointer* ip )
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if ((g_strcmp0(g_strstrip(gwData->gwyip->str),g_strstrip((gchar *)ip)) == 0) || (g_strcmp0(g_strstrip(gwData->gwyipv6->str),g_strstrip((gchar *)ip)) == 0))
//    if ((g_strcmp0(g_strstrip(gwData->gwyip->str),g_strstrip((gchar *)ip)) == 0) )
        return 0;
    else
        return 1;
}

gboolean RouteNetworkMgr::getCurrentRoute(char * routeIp,gboolean* isIpv4)
{
    FILE *pf;
    char data[100] = {0};
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    GString* command=g_string_new(NULL);
    *isIpv4=lastRouteSetV4;
    if(lastRouteSetV4)
    {
        g_string_printf(command, "route -n | grep 'UG[ \t]' | grep %s | awk '{print $2}' | grep 169.254 | sed -n '1p'", routeIf);
    }
    else
    {
        g_string_printf(command, " ip -6 route | grep %s | awk '/default/ { print $3 }' ", routeIf);
    }
    pf=popen(command->str,"r");
    if(!pf)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] pipe to open route failed %s \n", MODULE_NAME,__FUNCTION__, __LINE__,command->str );
        return FALSE;
    }

    fgets(data, sizeof (data), pf);
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Current Route set is %s \n", MODULE_NAME,__FUNCTION__, __LINE__,data );
    g_stpcpy(routeIp,g_strstrip(data));
    g_string_free(command,TRUE);
    if(pclose(pf))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] pipe command stream not closed properly \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return TRUE;
}

void sendDefaultGatewayRoute()
{
    pthread_t sendDefaultGatewayRouteThread;
    pthread_attr_t attr;
    int rc;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&sendDefaultGatewayRouteThread, &attr, &sendDefaultGatewayRouteThrd, NULL);
    if (rc) {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"ERROR; sendDefaultGatewayRoute return code from pthread_create() is %d\n", rc);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

void* sendDefaultGatewayRouteThrd(void* arg)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    RouteNetworkMgr::sendCurrentRouteData();
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return NULL;
}

gboolean RouteNetworkMgr::sendCurrentRouteData()
{
    routeEventData_t data;
    char routeIp[100]= {0};
    gboolean retVal=FALSE;
    gboolean isIpv4;

    memset(&data,0,sizeof(data));

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(getCurrentRoute(routeIp,&isIpv4))
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] route data to be sent is %s and isIpv4 %d \n", MODULE_NAME,__FUNCTION__, __LINE__,routeIp,isIpv4);
        if(data.routeIp)
            strncpy(data.routeIp,routeIp,sizeof(data.routeIp));
        data.ipv4=isIpv4;
        if(NULL != routeIf)
            strncpy(data.routeIf,routeIf,sizeof(data.routeIf));
    }
    if (IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME, (IARM_EventId_t) IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_DATA, (void *)&data, sizeof(data)) == IARM_RESULT_SUCCESS)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully send IARM_BUS_NETSRVMGR_Route_Event event \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        retVal=TRUE;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] IARM_BUS_NETSRVMGR_Route_Event IARM failed \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retVal;
}

IARM_Result_t RouteNetworkMgr::getCurrentRouteData(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    IARM_Bus_RouteSrvMgr_RouteData_Param_t *param = (IARM_Bus_RouteSrvMgr_RouteData_Param_t *)arg;
    if(getCurrentRoute(param->route.routeIp,&param->route.ipv4))
    {
        param->status = true;
        strncpy(param->route.routeIf,routeIf,sizeof(param->route.routeIf));
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] no current route available \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        param->status = false;
        ret=IARM_RESULT_IPCCORE_FAIL;
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}
