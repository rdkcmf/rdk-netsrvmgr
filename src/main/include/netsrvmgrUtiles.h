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
char* get_IfName_devicePropsFile(void);
bool getMacAddress_IfName(char *ifName_in, char macAddress_out[MAC_ADDR_BUFF_LEN]);
void triggerDhcpLease(void);
bool getRouteInterface(char* devname);
bool readDevFile(char *deviceName);
char getAllNetworkInterface(char* devAllInterface);
}



#endif /* _NETSRVMGRUTILES_H_ */

/** @} */
/** @} */
