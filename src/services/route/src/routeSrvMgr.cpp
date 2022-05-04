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
#ifdef SAFEC_RDKV
#include "safec_lib.h"
#else
#define MEMCPY_S(dest,dsize,source,ssize)	\
	memcpy(dest,source,ssize);
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
          LOG_INFO("[%s] Preferred Gateway is XB", MODULE_NAME);
       }
       cfgFile.close();
    }


    LOG_TRACE("[%s] Enter", MODULE_NAME);

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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
               LOG_INFO("[%s] Signal to fetch the data from upnp  %d", MODULE_NAME, eventData->data.xupnpData.deviceInfoLength );
            }
            pthread_mutex_unlock(&mutexRoute);
            break;
        }
        }
        LOG_TRACE("[%s] Exit", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    ret = IARM_Malloc(IARM_MEMTYPE_PROCESSLOCAL, sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t) + messageLength + 1, (void**)&param);
    if(ret == IARM_RESULT_SUCCESS)
    {
        param->bufLength = messageLength;

        ret = IARM_Bus_Call(_IARM_XUPNP_NAME,IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo,
                            (void *)param, sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t) + messageLength + 1);

        if(ret == IARM_RESULT_SUCCESS)
        {
            MEMCPY_S(gatewayResults,messageLength+1, ((char *)param + sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t)), param->bufLength);
            gatewayResults[param->bufLength] = '\0';
            returnStatus =  TRUE;
        }
        else
        {
             LOG_ERR("[%s] IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo IARM failed in the fetch", MODULE_NAME);
        }
        LOG_TRACE("[%s] gatewayResults %s", MODULE_NAME, gatewayResults);
        IARM_Free(IARM_MEMTYPE_PROCESSLOCAL, param);
        LOG_TRACE("[%s] Exit", MODULE_NAME);
    }
    else
    {
         LOG_ERR("[%s] IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo , IARM_Malloc Failed", MODULE_NAME);
    }
    return returnStatus;
}

void getGatewayRouteData()
{
    pthread_t getGatewayRouteDataThread;
    pthread_attr_t attr;
    int rc;

    LOG_TRACE("[%s] Enter", MODULE_NAME);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&getGatewayRouteDataThread, &attr, &getGatewayRouteDataThrd, NULL);
    if (rc) {
        LOG_ERR("ERROR; getGatewayRouteData return code from pthread_create() is %d", rc);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    memset(&upnpResults,0,sizeof(upnpResults));
    if(getGatewayResults(&upnpResults[0], messageLength))
    {
        if(parse_store_gateway_data(&upnpResults[0]))
            retVal=TRUE;
    }
    else
    {
        LOG_ERR("Failure in getting gateway results.");
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return retVal;
}

void* getGatewayRouteDataThrd(void* arg)
{
    int ret = 0;
    int msgLength;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    while (true ) {
        pthread_mutex_lock(&mutexRoute);
        while(signalUpnpDataReady == false) {
            pthread_cond_wait(&condRoute, &mutexRoute);
        }
        LOG_DBG("[%s] ***** Started fetching gateway data from upnp msg len = %d  *****", MODULE_NAME,messageLength );
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
                    LOG_INFO("[%s] Triggering dhcp lease since no XG gateway", MODULE_NAME);
                    netSrvMgrUtiles::triggerDhcpRenew();
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
               char* interface;
               if ((interface = getenv("WIFI_INTERFACE")) != NULL && *interface != NULL)
                  ifcList.push_back(interface);
               if ((interface = getenv("MOCA_INTERFACE")) != NULL && *interface != NULL)
                  ifcList.push_back(interface);
               gwSelected = false;
               for (auto const& i : ifcList)
               {
                  std::string ifcStr = i;
                  //Invoke API to cleanup Global IPs assigned.
                  NetLinkIfc::get_instance()->deleteinterfaceip(ifcStr,AF_INET6);
                  NetLinkIfc::get_instance()->deleteinterfaceroutes(ifcStr,AF_INET6);
                  //Call script to enable SLAAC ra support.
#ifdef YOCTO_BUILD
                  v_secure_system("/lib/rdk/enableIpv6Autoconf.sh %s", ifcStr.c_str());
#else
                  std::string cmd = "/lib/rdk/enableIpv6Autoconf.sh ";
                  cmd += ifcStr;
                  system(cmd.c_str());
#endif
               }
               LOG_INFO("[%s] No Gateway Detected, SLAAC Support is enabled", MODULE_NAME);
            }
#endif//ENABLE_NLMONITOR
        }
//        }
//	else
//	{
//           LOG_DBG("[%s] No Gateway in UPNP list  msgLength = %d msglenlocal = %d  ***** ", MODULE_NAME, messageLength, msgLength );
//	}
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    int validip4 = inet_pton(AF_INET, ipAddress, &addr4);
    int validip6 = inet_pton(AF_INET6, ipAddress, &addr6);
    if (g_strrstr(g_strstrip(ipAddress),"null") || ! *ipAddress )
    {
         LOG_ERR("[%s] ipaddress are empty %s", MODULE_NAME, ipAddress);
        return TRUE;
    }
    if ((validip4 == 1 ) || (validip6 == 1 ))
    {
        return TRUE;
    }
    else
    {
         LOG_ERR("[%s] Not a valid ip address %s", MODULE_NAME, ipAddress);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if (g_strrstr(g_strstrip(hostname),"null") || ! *hostname )
    {
         LOG_ERR("[%s] hostname  values are empty %s", MODULE_NAME, hostname);
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
     LOG_ERR("[%s] no valid ip so rejecting the dns values %s", MODULE_NAME, hostname);
    if(tokens)
        g_strfreev(tokens); /*CID-24000*/
    return FALSE;
}

gboolean RouteNetworkMgr::init_gwydata(GwyDeviceData* gwydata)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    gwydata->serial_num = g_string_new(NULL);
    gwydata->gwyip = g_string_new(NULL);
    gwydata->gwyipv6 = g_string_new(NULL);
    gwydata->ipv6prefix = g_string_new(NULL);
    gwydata->isRouteSet=FALSE;
    gwydata->dnsconfig = g_string_new(NULL);
    gwydata->devicetype = g_string_new(NULL);
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;
}

gboolean RouteNetworkMgr::free_gwydata(GwyDeviceData* gwydata)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if(gwydata)
    {
        g_string_free(gwydata->serial_num, TRUE);
        g_string_free(gwydata->gwyip, TRUE);
        g_string_free(gwydata->gwyipv6, TRUE);
        g_string_free(gwydata->dnsconfig, TRUE);
        g_string_free(gwydata->ipv6prefix, TRUE);
        g_string_free(gwydata->devicetype, TRUE);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;
}
gboolean RouteNetworkMgr::readDevFile(char *deviceFile,char *mocaIface,char *wifiIface)
{
    GError                  *error=NULL;
    gboolean                result = FALSE;
    gchar* devfilebuffer = NULL;
    gchar counter=0;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if (deviceFile == NULL)
    {
         LOG_ERR("[%s] device properties file not found", MODULE_NAME);
        return result;
    }
    result = g_file_get_contents (deviceFile, &devfilebuffer, NULL, &error);
    if (result == FALSE)
    {
         LOG_ERR("[%s] No contents in device properties", MODULE_NAME);
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
        LOG_ERR("[%s] MOCA_INTERFACE  and WIFI_INTERFACE  not found in %s", MODULE_NAME, deviceFile);
    }
    else
        result = TRUE;
    if(error)
    {
        /* g_clear_error() frees the GError *error memory and reset pointer if set in above operation */
        g_clear_error(&error);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return result;
}
gboolean RouteNetworkMgr::delGatewayList()
{
    guint gwListLength=0;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
       LOG_INFO("[%s] NO gateway data available", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;
}

gboolean RouteNetworkMgr::delRouteList()
{
    guint gwRouteLength=0;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
       LOG_INFO("[%s] NO gateway data available", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;
}

gboolean  RouteNetworkMgr::parse_store_gateway_data(char *array)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
		LOG_INFO("gateway count in json %d", gwCount);
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
                            LOG_TRACE("[%s] serial_num  %s gwcount %d", MODULE_NAME,gwydata->serial_num->str,g_list_length(gwList));
                        }
                        else
                        {
                             LOG_ERR("[%s] not a valid gateway %s", MODULE_NAME, serialNum);
                        }
                    } else {
                         LOG_ERR("[%s] Failed to get gateway details,Array item is Null", MODULE_NAME);
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
               LOG_INFO("[%s] No gwdata to parse gwcount ", MODULE_NAME);
            }
            cJSON_Delete(rootJson);
        }
	else
	{
	     retVal=FALSE;
            LOG_INFO("JSON is empty");

	}
    }
    else
    {
         LOG_ERR("[%s] gateway list is empty", MODULE_NAME);
        retVal=FALSE;
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return retVal;
}

gboolean RouteNetworkMgr::getRouteInterface(char * routeIf)
{
    char devFile[] = "//etc//device.properties";
    char mocaIf[10]= {0};
    char wifiIf[10]= {0};
    char ipAddressBuffer[46];
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if(readDevFile(devFile,mocaIf,wifiIf))
    {
        int result = getipaddress(mocaIf,ipAddressBuffer,FALSE);
        if (!result)
        {
            LOG_INFO("[%s] Could not locate the ipaddress of the broadcast interface %s", MODULE_NAME,mocaIf );
            result = getipaddress(wifiIf,ipAddressBuffer,FALSE);
            if (!result)
            {
                LOG_INFO("[%s] Could not locate the ipaddress of the broadcast interface %s", MODULE_NAME, wifiIf );
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
        LOG_ERR("[%s] No route interface available", MODULE_NAME);
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    element = g_list_first(gwList);
    LOG_TRACE("[%s] gw list count %d", MODULE_NAME, g_list_length(gwList));
    getRouteInterface(routeIf);
    if(!routeIf)
    {
        LOG_ERR("[%s] No Interface available to add route", MODULE_NAME);
        return retVal;
    }
    while(element)
    {
        gwdata=((GwyDeviceData *)element->data);
        element = g_list_next(element);
        if(!gwdata)
        {
            LOG_TRACE("[%s] gwdata NULL", MODULE_NAME);
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
               char* interface;
               if ((interface = getenv("WIFI_INTERFACE")) != NULL && *interface != NULL)
                  ifcList.push_back(interface);
               if ((interface = getenv("MOCA_INTERFACE")) != NULL && *interface != NULL)
                  ifcList.push_back(interface);
               gwSelected = true;

                for (auto const& i : ifcList)
                {
                  //Call script to disable SLAAC ra support.
                  std::string ifcStr = i;
#ifdef YOCTO_BUILD
                  v_secure_system("/lib/rdk/disableIpv6Autoconf.sh %s", ifcStr.c_str());
#else
                  std::string cmd = "/lib/rdk/disableIpv6Autoconf.sh ";
                  cmd += ifcStr;
                  system(cmd.c_str());
#endif

                  //Invoke API to cleanup Global IPs assigned.
                  NetLinkIfc::get_instance()->deleteinterfaceip(ifcStr,AF_INET6);
                  NetLinkIfc::get_instance()->deleteinterfaceroutes(ifcStr,AF_INET6);
                }
               LOG_INFO("[%s] Gateway Detecetd, SLAAC Support is disabled.", MODULE_NAME);
            }
#endif //ENABLE_NLMONITOR

#ifdef YOCTO_BUILD
           LOG_INFO("[%s] Calling gateway script with arguments %s %s %s %s %d %s %s %s ", MODULE_NAME,
                                    GW_SETUP_FILE, gwdata->gwyip->str, gwdata->dnsconfig->str, routeIf, ROUTE_PRIORITY,
                                    gwdata->ipv6prefix->str, gwdata->gwyipv6->str, gwdata->devicetype->str);
            retType=v_secure_system(GW_SETUP_FILE " %s %s %s %d %s %s %s ",
                                    gwdata->gwyip->str, gwdata->dnsconfig->str, routeIf, ROUTE_PRIORITY,
                                    gwdata->ipv6prefix->str, gwdata->gwyipv6->str, gwdata->devicetype->str);
            //Ignoring WEXITSTATUS(retType) in v_secure_system() as it is already taken care in secure_wrapper.c
#else
            GString *GwRouteParam=g_string_new(NULL);
            g_string_printf(GwRouteParam,"%s" ,GW_SETUP_FILE);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->gwyip->str);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->dnsconfig->str);
            g_string_append_printf(GwRouteParam," \"%s\"" ,routeIf);
            g_string_append_printf(GwRouteParam," \"%d\"" ,ROUTE_PRIORITY);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->ipv6prefix->str);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->gwyipv6->str);
            g_string_append_printf(GwRouteParam," \"%s\"" ,gwdata->devicetype->str);
            LOG_INFO("[%s] Calling gateway script %s", MODULE_NAME,GwRouteParam->str);
            retType=system(GwRouteParam->str);
            g_string_free(GwRouteParam,TRUE);
            if(retType != SYSTEM_COMMAND_ERROR)
            {
                retType = WEXITSTATUS(retType);
            }
#endif
        }
        if(retType == SYSTEM_COMMAND_ERROR)
        {
            LOG_ERR("[%s] Error has occured in shell command", MODULE_NAME);
        }
        else if (retType == SYSTEM_COMMAND_SHELL_NOT_FOUND)
        {
            LOG_ERR("[%s] That shell command is not found", MODULE_NAME);
        }
        else if (retType == SYSTEM_COMMAND_SHELL_SUCESS)
        {
           LOG_INFO("[%s] system call route set successfully %d", MODULE_NAME, retType);
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
           LOG_INFO("[%s] no change in routing information %d", MODULE_NAME, retType);
        }
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return found;
}

gboolean RouteNetworkMgr::checkAddRouteInfo(char *ipAddr,bool isIPv4,char *ipv6Pfix)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
	LOG_INFO("Adding route since prefix changed from %s to %s ", gwdata->ipv6Pfix->str, ipv6Pfix);
        lastRouteSetV4=isIPv4;
        addRouteToList(ipAddr,isIPv4,ipv6Pfix);
    }
    else
    {
       LOG_INFO("[%s] Route %s is already in the list", MODULE_NAME, ipAddr);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;
}

gboolean RouteNetworkMgr::addRouteToList(char *ipAddr,bool isIPv4,char *ipv6Pfix)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    routeInfo *routeInfoData = g_new(routeInfo,1);
    routeInfoData->isIPv4 = isIPv4;
    routeInfoData->ipStr = g_string_new(NULL);
    routeInfoData->ipv6Pfix = g_string_new(NULL);
    g_string_assign(routeInfoData->ipStr,ipAddr);
    g_string_assign(routeInfoData->ipv6Pfix,ipv6Pfix);
    gwRouteInfo=g_list_prepend(gwRouteInfo,routeInfoData);
    LOG_INFO("[%s] length of route list is %d",MODULE_NAME,g_list_length(gwRouteInfo));
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;

}
gboolean RouteNetworkMgr::checkRemoveRouteInfo(char *ipAddr,bool isIPv4)
{
    guint gwRouteLength=0;
//    GList* routeList;
    routeInfo *routeInfoData;
    gint retType;
    gboolean retVal=FALSE;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    gwRouteInfo=g_list_first(gwRouteInfo);
    gwRouteLength = g_list_length(gwRouteInfo);
    if (gwRouteLength > 0)
    {
        gwRouteInfo=g_list_find_custom(gwRouteInfo,ipAddr,(GCompareFunc)g_list_find_ip);
        if( gwRouteInfo != NULL)
        {
#ifndef YOCTO_BUILD
            GString* command=g_string_new(NULL);
#endif
	    LOG_INFO("[%s] Route to be removed  ******* %s ******* ", MODULE_NAME, ipAddr);
            if (isIPv4)
            {
#ifdef YOCTO_BUILD
                retType=v_secure_system("route del default gw %s", ipAddr);
                //Ignoring WEXITSTATUS(retType) in v_secure_system() as it is already taken care in secure_wrapper.c
               LOG_INFO("[%s] Remove Route ******* route del default gw %s ******* ", MODULE_NAME, ipAddr);
#else
                g_string_printf(command, "route del default gw %s", ipAddr);
#endif
            }
            else
            {
#ifdef YOCTO_BUILD
                retType=v_secure_system("ip -6 route del default via %s", ipAddr);
                //Ignoring WEXITSTATUS(retType) in v_secure_system() as it is already taken care in secure_wrapper.c
               LOG_INFO("[%s] Remove Route ******* ip -6 route del default via %s *******", MODULE_NAME,ipAddr);
#else
                g_string_printf(command, "ip -6 route del default via %s", ipAddr);
#endif
            }

#ifndef YOCTO_BUILD
            retType=system(command->str);
            if(retType != SYSTEM_COMMAND_ERROR)
            {
                retType = WEXITSTATUS(retType);
            }
            LOG_INFO( "[%s] Remove Route ******* %s *******", MODULE_NAME, command->str);
#endif
            if(retType == SYSTEM_COMMAND_ERROR)
            {
                LOG_ERR("[%s] Error has occured in shell command", MODULE_NAME);
            }
            else if (retType == SYSTEM_COMMAND_SHELL_NOT_FOUND)
            {
                LOG_ERR("[%s] That shell command is not found", MODULE_NAME);
            }
            else
            {
                LOG_INFO("[%s] system call route set successfully %d", MODULE_NAME, retType);
                retVal=TRUE;
            }
            sendCurrentRouteData();
            routeInfoData=(routeInfo *)gwRouteInfo->data;
            removeRouteFromList(routeInfoData);
#ifndef YOCTO_BUILD
            g_string_free(command,TRUE);
#endif
        }
        else
        {
           LOG_INFO("[%s] Existing route exist in the new list", MODULE_NAME);
        }
    }
    else
    {
     LOG_INFO("[%s] Saved gw list is empty ", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return retVal;
}
gboolean RouteNetworkMgr::removeRouteFromList(routeInfo *routeInfoData)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
       LOG_INFO("[%s] route info data is NULL ", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;

}

bool checkIpMode(char *v6Prefix)
{
    bool retVal=TRUE;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if (g_strrstr(g_strstrip(v6Prefix),"null") ||  ! *(v6Prefix))
        retVal=FALSE;
    LOG_TRACE("[%s] Exit", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
                LOG_INFO("[%s] route %s  is not there in new list proceed to remove it", MODULE_NAME, routeInfoData->ipStr->str);
                checkRemoveRouteInfo(routeInfoData->ipStr->str,routeInfoData->isIPv4);
            }
	    else if((!routeInfoData->isIPv4) && (gwdata) && (g_strcmp0(g_strstrip(routeInfoData->ipv6Pfix->str),g_strstrip(gwdata->ipv6prefix->str)) != 0))
	    {
                LOG_INFO("prefix changed from %s to %s ", routeInfoData->ipv6Pfix->str, gwdata->ipv6prefix->str);
                checkRemoveRouteInfo(routeInfoData->ipStr->str,routeInfoData->isIPv4);
            }
            else
            {
               LOG_INFO("[%s] route is there in new list", MODULE_NAME);
            }
        }
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    if((g_list_length(gwRouteInfo) != 0 ) && (g_list_length(gwRouteInfo) != tempGwRouteLength))
    {
	LOG_INFO("existing route are fine");
	return FALSE;
    }
    return TRUE;
}
gboolean RouteNetworkMgr::printExistingRouteValid()
{
    guint gwRouteLength=0;
    GwyDeviceData *gwdata = NULL;
    routeInfo *routeInfoData=NULL;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    gwRouteLength = g_list_length(gwRouteInfo);
    if (gwRouteLength > 0)
    {
        GList *element = g_list_first(gwRouteInfo);
        while (element && (gwRouteLength > 0))
        {
            routeInfoData = (routeInfo*)element->data;
            element = g_list_next(element);
            LOG_TRACE("[%s:%s] ipstring %s isipv4 %d gwRouteLength %d", MODULE_NAME,routeInfoData->ipStr->str,routeInfoData->isIPv4,gwRouteLength);
        }
    }
    else
    {
       LOG_INFO("[%s] local route list is empty ", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;
}




gboolean RouteNetworkMgr::printGatewayList()
{
    guint gwListLength=0;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    gwListLength = g_list_length(gwList);
    LOG_INFO("[%s] gateway count %d ", MODULE_NAME, gwListLength);
    if (gwListLength > 0)
    {
        GwyDeviceData *gwdata = NULL;
        GList *element = g_list_first(gwList);
        while (element && (gwListLength > 0))
        {
            gwdata = (GwyDeviceData*)element->data;
            element = g_list_next(element);
            LOG_INFO("[%s] gateway list %s", MODULE_NAME, gwdata->serial_num->str);
        }
    }
    else
    {
       LOG_INFO("[%s] local gateway list is empty", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;
}

guint RouteNetworkMgr::g_list_find_ip(routeInfo* gwData, gconstpointer* ip )
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if (g_strcmp0(g_strstrip(gwData->ipStr->str),g_strstrip((gchar *)ip)) == 0)
        return 0;
    else
        return 1;
}
guint RouteNetworkMgr::g_list_find_gw(GwyDeviceData* gwData, gconstpointer* ip )
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
        LOG_ERR("[%s] pipe to open route failed %s", MODULE_NAME, command->str );
        return FALSE;
    }

    fgets(data, sizeof (data), pf);
    LOG_INFO("[%s] Current Route set is %s", MODULE_NAME, data );
    g_stpcpy(routeIp,g_strstrip(data));
    g_string_free(command,TRUE);
    if(pclose(pf))
    {
        LOG_ERR("[%s] pipe command stream not closed properly", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return TRUE;
}

void sendDefaultGatewayRoute()
{
    pthread_t sendDefaultGatewayRouteThread;
    pthread_attr_t attr;
    int rc;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&sendDefaultGatewayRouteThread, &attr, &sendDefaultGatewayRouteThrd, NULL);
    if (rc) {
       LOG_ERR("ERROR; sendDefaultGatewayRoute return code from pthread_create() is %d", rc);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
}

void* sendDefaultGatewayRouteThrd(void* arg)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    RouteNetworkMgr::sendCurrentRouteData();
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return NULL;
}

gboolean RouteNetworkMgr::sendCurrentRouteData()
{
    routeEventData_t data;
    char routeIp[100]= {0};
    gboolean retVal=FALSE;
    gboolean isIpv4;

    memset(&data,0,sizeof(data));

    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if(getCurrentRoute(routeIp,&isIpv4))
    {
       LOG_INFO("[%s] route data to be sent is %s and isIpv4 %d", MODULE_NAME, routeIp, isIpv4);
        if(data.routeIp)
            strncpy(data.routeIp,routeIp,sizeof(data.routeIp));
        data.ipv4=isIpv4;
        if(NULL != routeIf)
            strncpy(data.routeIf,routeIf,sizeof(data.routeIf));
    }
    if (IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME, (IARM_EventId_t) IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_DATA, (void *)&data, sizeof(data)) == IARM_RESULT_SUCCESS)
    {
       LOG_INFO("[%s] Successfully send IARM_BUS_NETSRVMGR_Route_Event event", MODULE_NAME);
        retVal=TRUE;
    }
    else
    {
        LOG_ERR("[%s] IARM_BUS_NETSRVMGR_Route_Event IARM failed ", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return retVal;
}

IARM_Result_t RouteNetworkMgr::getCurrentRouteData(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    IARM_Bus_RouteSrvMgr_RouteData_Param_t *param = (IARM_Bus_RouteSrvMgr_RouteData_Param_t *)arg;
    if(getCurrentRoute(param->route.routeIp,&param->route.ipv4))
    {
        param->status = true;
        strncpy(param->route.routeIf,routeIf,sizeof(param->route.routeIf));
    }
    else
    {
        LOG_INFO("[%s] no current route available", MODULE_NAME);
        param->status = false;
        ret=IARM_RESULT_IPCCORE_FAIL;
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}
