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

#define IARM_BUS_NM_SRV_MGR_NAME "NET_SRV_MGR"
#define INTERFACE_SIZE 10
#define INTERFACE_LIST 50
#define IARM_BUS_NETSRVMGR_API_getActiveInterface "getActiveInterface"
#define IARM_BUS_NETSRVMGR_API_getNetworkInterfaces "getNetworkInterfaces"
#define IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogStatus "getTelemetryLogStatus"
#define IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogDuration "getTelemetryLogDuration"
#define IARM_BUS_NETSRVMGR_API_isInterfaceEnabled "isInterfaceEnabled"
#define IARM_BUS_NETSRVMGR_API_getInterfaceControlPersistence  "getInterfaceControlPersistence"

typedef enum _NetworkManager_MoCA_EventId_t {
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG=20,
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION,
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_MAX,
} IARM_Bus_NetworkManager_MoCA_EventId_t;

typedef enum _NetworkManager_EventId_t {
        IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED=50,
        IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE,
        IARM_BUS_NETWORK_MANAGER_MAX,
} IARM_Bus_NetworkManager_EventId_t;
 
typedef struct _IARM_BUS_NetSrvMgr_Iface_EventData_t {
   union {
        char activeIface[INTERFACE_SIZE];
        char allNetworkInterfaces[INTERFACE_LIST];
        char setInterface[INTERFACE_SIZE];
	};
   char interfaceCount;
   bool isInterfaceEnabled;
} IARM_BUS_NetSrvMgr_Iface_EventData_t;
#endif /* _NETSRVMGRIARM_H_ */

/** @} */
/** @} */

