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
typedef enum _NetworkManager_MoCA_EventId_t {
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG=20,
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION,
        IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_MAX,
} IARM_Bus_NetworkManager_MoCA_EventId_t;

typedef enum _NetworkManager_EventId_t {
        IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED=50,
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

