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
