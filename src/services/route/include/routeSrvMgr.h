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

#include "sysMgr.h"
#include "libIBus.h"
#include "libIARM.h"
#include "libIARMCore.h"
#define MODULE_NAME "ROUTE_MODULE"

typedef struct _gwyDeviceData {
    GString* serial_num;
    GString* gwyip;
    GString* gwyipv6;
    GString* ipv6prefix;
    GString* devicetype;
    GString* dnsconfig;
    gboolean isRouteSet;
} GwyDeviceData;

typedef struct _routeInfo
{
    bool isIPv4;
    GString* ipStr;
    GString* ipv6Pfix;
} routeInfo;

class RouteNetworkMgr
{
public:
    static RouteNetworkMgr* getInstance();
    int Start();
    static gboolean storeRouteDetails(unsigned int messageLength);
    static gboolean checkExistingRouteValid();
    static gboolean delRouteList();
    static gboolean setRoute();
    static gboolean sendCurrentRouteData();

private:
    RouteNetworkMgr();
    virtual ~RouteNetworkMgr();
    static bool instanceIsReady;
    static RouteNetworkMgr* instance;
    static bool getGatewayResults(char* gatewayResults, unsigned int messageLength);
    static gboolean init_gwydata(GwyDeviceData* gwydata);
    static gboolean free_gwydata(GwyDeviceData* gwydata);
    static gboolean delGatewayList();
    static gboolean printGatewayList();
    static gboolean parse_store_gateway_data(char *array);
    static gboolean getRouteInterface(char * routeIf);
    static gboolean readDevFile(char *deviceFile,char *mocaIface,char *wifiIface);
    static int getipaddress(char* ifname, char* ipAddressBuffer, gboolean ipv6Enabled);
    static gboolean checkAddRouteInfo(char *ipAddr,bool isIPv4,char *ipv6Pfix);
    static gboolean addRouteToList(char *ipAddr,bool isIPv4,char *ipv6Pfix);
    static gboolean checkRemoveRouteInfo(char *ipAddr,bool isIPv4);
    static gboolean removeRouteFromList(routeInfo *routeInfoData);
    static gboolean printExistingRouteValid();
    static guint g_list_find_ip(routeInfo* gwData, gconstpointer* ip );
    static guint g_list_find_gw(GwyDeviceData* gwData, gconstpointer* ip );
    static gboolean getCurrentRoute(char * routeIp,gboolean* isIpv4);
    static IARM_Result_t getCurrentRouteData(void *arg);
};


#define IARM_BUS_ROUTE_MGR_API_getCurrentRouteData "getCurrentRouteData"
typedef struct _routeEventData_t {
        char routeIp[46];
        gboolean ipv4;
        char routeIf[10];
} routeEventData_t;

typedef struct _IARM_Bus_RouteSrvMgr_RouteData_Param_t {
    routeEventData_t route;
    bool status;
} IARM_Bus_RouteSrvMgr_RouteData_Param_t;

void sendDefaultGatewayRoute();
void* sendDefaultGatewayRouteThrd(void* arg);
void getGatewayRouteData();
static void _evtHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
void* getGatewayRouteDataThrd(void* arg);
gboolean checkvalidip( char* ipAddress);
gboolean checkvalidhostname( char* hostname);
bool checkIpMode(char *v6Prefix);
typedef enum _NetworkManager_Route_EventId_t {
        IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_DATA=10,
        IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_MAX,
} IARM_Bus_NetworkManager_Route_EventId_t;

// xupnp dependency

#define  _IARM_XUPNP_NAME							"XUPnP" /*!< Method to Get the Xupnp Info */
#define  IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo		"GetXUPNPDeviceInfo" /*!< Method to Get the Xupnp Info */
