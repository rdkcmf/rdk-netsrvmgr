/*
 * ============================================================================
 * COMCAST C O N F I D E N T I A L AND PROPRIETARY
 * ============================================================================
 * This file (and its contents) are the intellectual property of Comcast.  It may
 * not be used, copied, distributed or otherwise  disclosed in whole or in part
 * without the express written permission of Comcast.
 * ============================================================================
 * Copyright (c) 2015 Comcast. All rights reserved.
 * ============================================================================
 */

#include "sysMgr.h"
#include "libIBus.h"
#include "libIARM.h"
#include "libIARMCore.h"

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
} routeInfo;

class RouteNetworkMgr
{
public:
    static RouteNetworkMgr* getInstance();
    int Start();
    static bool getGatewayResults(char* gatewayResults, unsigned int messageLength);
    static gboolean storeRouteDetails(unsigned int messageLength);
    static gboolean init_gwydata(GwyDeviceData* gwydata);
    static gboolean free_gwydata(GwyDeviceData* gwydata);
    static gboolean delGatewayList();
    static gboolean delRouteList();
    static gboolean printGatewayList();
    static gboolean parse_store_gateway_data(char *array);
    static gboolean setRoute();
    static gboolean getRouteInterface(char * routeIf);
    static gboolean readDevFile(char *deviceFile,char *mocaIface,char *wifiIface);
    static int getipaddress(char* ifname, char* ipAddressBuffer, gboolean ipv6Enabled);
    static gboolean checkAddRouteInfo(char *ipAddr,bool isIPv4);
    static gboolean addRouteToList(char *ipAddr,bool isIPv4);
    static gboolean checkRemoveRouteInfo(char *ipAddr,bool isIPv4);
    static gboolean removeRouteFromList(routeInfo *routeInfoData);
    static gboolean checkExistingRouteValid();
    static gboolean printExistingRouteValid();
    static guint g_list_find_ip(routeInfo* gwData, gconstpointer* ip );
    static guint g_list_find_gw(GwyDeviceData* gwData, gconstpointer* ip );

private:
    RouteNetworkMgr();
    virtual ~RouteNetworkMgr();
    static bool instanceIsReady;
    static RouteNetworkMgr* instance;
};






void getGatewayRouteData();
static void _evtHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
void* getGatewayRouteDataThrd(void* arg);
gboolean checkvalidip( char* ipAddress);
gboolean checkvalidhostname( char* hostname);
bool checkIpMode(char *v6Prefix);

// xupnp dependency

#define  _IARM_XUPNP_NAME							"XUPnP" /*!< Method to Get the Xupnp Info */
#define  IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo		"GetXUPNPDeviceInfo" /*!< Method to Get the Xupnp Info */
