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


/**
 * @file netSrvMgrIarm.h
 * @brief The header file provides components netSrvMgrIarm information APIs.
 */
/**
 * @defgroup netSrvMgr Iarm api's.
 * Describe the details about netSrvMgr Iarm api's.
 * @ingroup netsrvmgr
 *
 */

/**
* @defgroup netsrvmgr
* @{
* @defgroup netsermgr
* @{
**/


#ifndef _NETSRVMGRIARM_H_
#define _NETSRVMGRIARM_H_

#include <arpa/inet.h>

/**
 * @addtogroup NETSRVMGR_TYPES
 * @{
 */
#define IARM_BUS_NM_SRV_MGR_NAME "NET_SRV_MGR"
#define INTERFACE_SIZE 10
#define INTERFACE_LIST 50
#define MAX_IP_ADDRESS_LEN 46
#define MAX_IP_FAMILY_SIZE 10
#define MAX_HOST_NAME_LEN 128
#define IARM_BUS_NETSRVMGR_API_getActiveInterface "getActiveInterface"
#define IARM_BUS_NETSRVMGR_API_getNetworkInterfaces "getNetworkInterfaces"
#define IARM_BUS_NETSRVMGR_API_getInterfaceList "getInterfaceList"
#define IARM_BUS_NETSRVMGR_API_getDefaultInterface "getDefaultInterface"
#define IARM_BUS_NETSRVMGR_API_setDefaultInterface "setDefaultInterface"
#define IARM_BUS_NETSRVMGR_API_isInterfaceEnabled "isInterfaceEnabled"
#define IARM_BUS_NETSRVMGR_API_setInterfaceEnabled "setInterfaceEnabled"
#define IARM_BUS_NETSRVMGR_API_getSTBip "getSTBip"
#define IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogStatus "getTelemetryLogStatus"
#define IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogDuration "getTelemetryLogDuration"
#define IARM_BUS_NETSRVMGR_API_setIPSettings "setIPSettings"
#define IARM_BUS_NETSRVMGR_API_getIPSettings "getIPSettings"
#define IARM_BUS_NETSRVMGR_API_getSTBip_family "getSTBip_family"
#define IARM_BUS_NETSRVMGR_API_isConnectedToInternet "isConnectedToInternet"
#define IARM_BUS_NETSRVMGR_API_setConnectivityTestEndpoints "setConnectivityTestEndpoints"
#define IARM_BUS_NETSRVMGR_API_isAvailable "isAvailable"
#define IARM_BUS_NETSRVMGR_API_getPublicIP "getPublicIP"

typedef enum _NetworkManager_MoCA_EventId_t {
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG=20,
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION,
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_MAX,
} IARM_Bus_NetworkManager_MoCA_EventId_t;

typedef enum _NetworkManager_EventId_t {
        IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED=50,
        IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE,
        IARM_BUS_NETWORK_MANAGER_EVENT_WIFI_INTERFACE_STATE,
        IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_ENABLED_STATUS,
        IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_CONNECTION_STATUS,
        IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_IPADDRESS,
        IARM_BUS_NETWORK_MANAGER_EVENT_DEFAULT_INTERFACE,
        IARM_BUS_NETWORK_MANAGER_MAX,
} IARM_Bus_NetworkManager_EventId_t;
 
typedef struct _IARM_BUS_NetSrvMgr_Iface_EventData_t {
   union {
        char activeIface[INTERFACE_SIZE];
        char allNetworkInterfaces[INTERFACE_LIST];
        char setInterface[INTERFACE_SIZE];
	char activeIfaceIpaddr[MAX_IP_ADDRESS_LEN];
	};
   char interfaceCount;
   bool isInterfaceEnabled;
   bool persist;
   char ipfamily[MAX_IP_FAMILY_SIZE];
} IARM_BUS_NetSrvMgr_Iface_EventData_t;

typedef enum _NetworkManager_GetIPSettings_ErrorCode_t
{
  NETWORK_IPADDRESS_ACQUIRED,
  NETWORK_IPADDRESS_NOTFOUND,
  NETWORK_NO_ROUTE_INTERFACE,
  NETWORK_NO_DEFAULT_ROUTE,
  NETWORK_DNS_NOT_CONFIGURED,
  NETWORK_INVALID_IPADDRESS,
} NetworkManager_GetIPSettings_ErrorCode_t;

typedef struct {
    char interface[16];
    char ipversion[16];
    bool autoconfig;
    char ipaddress[INET6_ADDRSTRLEN];
    char netmask[INET6_ADDRSTRLEN];
    char gateway[INET6_ADDRSTRLEN];
    char primarydns[INET6_ADDRSTRLEN];
    char secondarydns[INET6_ADDRSTRLEN];
    bool isSupported;
    NetworkManager_GetIPSettings_ErrorCode_t errCode;
} IARM_BUS_NetSrvMgr_Iface_Settings_t;

typedef struct {
    char name[16];
    char mac[20];
    unsigned int flags;
} NetSrvMgr_Interface_t;

typedef struct {
    unsigned char         size;
    NetSrvMgr_Interface_t interfaces[8];
} IARM_BUS_NetSrvMgr_InterfaceList_t;

typedef struct {
    char interface[16];
    char gateway[MAX_IP_ADDRESS_LEN];
} IARM_BUS_NetSrvMgr_DefaultRoute_t;

typedef struct {
    char interface[16];
    bool status;
} IARM_BUS_NetSrvMgr_Iface_EventInterfaceStatus_t;

typedef IARM_BUS_NetSrvMgr_Iface_EventInterfaceStatus_t IARM_BUS_NetSrvMgr_Iface_EventInterfaceEnabledStatus_t;
typedef IARM_BUS_NetSrvMgr_Iface_EventInterfaceStatus_t IARM_BUS_NetSrvMgr_Iface_EventInterfaceConnectionStatus_t;

typedef struct {
    char interface[16];
    char ip_address[MAX_IP_ADDRESS_LEN];
    bool is_ipv6;
    bool acquired;
} IARM_BUS_NetSrvMgr_Iface_EventInterfaceIPAddress_t;

typedef struct {
    char oldInterface[16];
    char newInterface[16];
} IARM_BUS_NetSrvMgr_Iface_EventDefaultInterface_t;

typedef struct
{
    unsigned char size;
    char          endpoints[5][260]; // domain name max length + ':' + port number max chars + '\0' = 253+1+5+1 = 260
} IARM_BUS_NetSrvMgr_Iface_TestEndpoints_t;

typedef struct
{
    char server[MAX_HOST_NAME_LEN];
    uint16_t port;
    bool ipv6;
    char interface[16];
    uint16_t bind_timeout;
    uint16_t cache_timeout;
    bool sync;
    char public_ip[MAX_IP_ADDRESS_LEN];
} IARM_BUS_NetSrvMgr_Iface_StunRequest_t;

/** @} */  //END OF GROUP NETSRVMGR_TYPES

#endif /* _NETSRVMGRIARM_H_ */

/** @} */
/** @} */

