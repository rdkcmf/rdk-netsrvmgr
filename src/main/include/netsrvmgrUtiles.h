/**
 * @file netSrvMgrUtiles.h
 * @brief The header file provides components netSrvMgrUtiles information APIs.
 */
/**
 * @defgroup netSrvMgr Utility api's.
 * Describe the details about netSrvMgr Utility api's.
 * @ingroup netsrvmgr
 *
 */

/**
* @defgroup netsrvmgr
* @{
* @defgroup netsermgr
* @{
**/


#ifndef _NETSRVMGRUTILES_H_
#define _NETSRVMGRUTILES_H_
#pragma once

#include <iostream>

#define MAC_ADDR_BUFF_LEN 	18

#define SYSTEM_COMMAND_SHELL_NOT_FOUND 127
#define SYSTEM_COMMAND_SHELL_SUCESS 23
#define SYSTEM_COMMAND_ERROR -1

namespace netSrvMgrUtiles
{
enum Dhcp_Lease_Operation {
    DHCP_LEASE_RENEW = 0,
    DHCP_LEASE_RELEASE_AND_RENEW
};
char* get_IfName_devicePropsFile(void);
bool getMacAddress_IfName(char *ifName_in, char macAddress_out[MAC_ADDR_BUFF_LEN]);
void triggerDhcpLease(Dhcp_Lease_Operation op = DHCP_LEASE_RENEW);
bool getRouteInterface(char* devname);
bool readDevFile(char *deviceName);
char getAllNetworkInterface(char* devAllInterface);
bool getCurrentTime(char* currTime,const char *timeFormat);
bool checkInterfaceActive(char *interfaceName);
}



#endif /* _NETSRVMGRUTILES_H_ */

/** @} */
/** @} */
