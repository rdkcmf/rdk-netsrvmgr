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
 * @defgroup NETSRVMGR Network Service Manager
 * - Network Service Manager dynamically detect Network interfaces, send notifications to subscribers.
 * - Provides 3 major functionalities such as Wi-Fi, Route, MoCA.
 *
 * @defgroup NETSRVMGR_TYPES Network Service Manager Types
 * @ingroup  NETSRVMGR
 *
 * @defgroup NETSRVMGR_APIS Network Service Manager APIs
 * @ingroup  NETSRVMGR
 *
 **/

#ifndef _NETSRVMGRUTILES_H_
#define _NETSRVMGRUTILES_H_
#pragma once

#include <iostream>

/**
 * @addtogroup NETSRVMGR_TYPES
 * @{
 */
#define MAC_ADDR_BUFF_LEN 	18

#define SYSTEM_COMMAND_SHELL_NOT_FOUND 127
#define SYSTEM_COMMAND_SHELL_SUCESS 23
#define SYSTEM_COMMAND_ERROR -1
#define BUFFER_SIZE_SCRIPT_OUTPUT 512

/** @} */  //END OF GROUP NETSRVMGR_TYPES

class EntryExitLogger
{
    const char* func;
    const char* file;
public:
    EntryExitLogger (const char* func, const char* file);
    ~EntryExitLogger ();
};

#define LOG_ENTRY_EXIT EntryExitLogger entry_exit_logger (__FUNCTION__, __FILE__)

namespace netSrvMgrUtiles
{
enum Dhcp_Lease_Operation {
    DHCP_LEASE_RENEW = 0,
    DHCP_LEASE_RELEASE_AND_RENEW
};

/**
 * @addtogroup NETSRVMGR_APIS
 * @{
 */

/**
 * @brief This function gets the interface name based on the device/properties file.
 *
 * @param[in] void.
 *
 * @return  Returns interface name.
 */
char* get_IfName_devicePropsFile(void);

/**
 * @brief This function is used to get the MAC address for the provided interface.
 *
 * @param[in] ifName_in Indicates the interface name which the mac address is required.
 * @param[out] macAddress_out Indicates the mac address of ifname_in interface name.
 *
 * @return  Returns true if successfully gets the mac address of interface provided or else false.
 */
bool getMacAddress_IfName(char *ifName_in, char macAddress_out[MAC_ADDR_BUFF_LEN]);

/**
 * @brief This function trigger the process by which the DHCP client renews or updates its IP address configuration data with the
 * DHCP server.
 *
 * @param[in] op      DHCP lease operation in order to renew/release and renew.
 */
void triggerDhcpLease(Dhcp_Lease_Operation op = DHCP_LEASE_RENEW);

/**
 * @brief This function retrieves information about the active routing interface.
 *
 * @param[out] devname Device name buffer to be filled.
 *
 * @return  Returns true if successfully found the route interface or else false.
 */
bool getRouteInterface(char* devname);

/**
 * @brief This function gets the device interface type(Ethernet/MOCA/WIFI) of the input device name.
 * This function reads the device interface type from device properties file.
 *
 * @param[in/out] Interface deviceName as input and Device type buffer to be filled as output.
 *
 * @return  Returns true if successfully gets the route interface type or else false.
 */
bool readDevFile(char *deviceName);

/**
 * @brief This function parse all the device interface details and gives all the network interface device name in the output buffer.
 * This function also returns the total network interface count.
 *
 * @param[out] devAllInterface Every interface device names.
 *
 * @return  Returns total network interface count.
 */
char getAllNetworkInterface(char* devAllInterface);

/**
 * @brief This function retrieves the current time using the requested format specifier.
 *
 * @param[out] currTime    Current time to be filled.
 * @param[in]  timeFormat    Requested time format to give the current time.
 *
 * @return  Returns true if successfully gets the current time and converted to specified format or else false.
 */
bool getCurrentTime(char* currTime,const char *timeFormat);

/**
 * @brief This function returns the Ethernet active status of the interface, if finds the status from interface status file.
 *
 * @param[out] interfaceName    Interface name for which network status required.
 *
 * @return  Returns true if Ethernet mode is "Up" or else false.
 */
bool checkInterfaceActive(char *interfaceName);

/**
 * @brief This function is used to get STB IP address and its IP version.
 *
 * @param[out] stbip    STB IP.
 * @param[out] isIpv6    Internet Protocol Version.
 *
 * @return  Returns true if successfully gets the IP details, Otherwise returns false.
 */
bool getSTBip(char *stbip,bool *isIpv6);

/**
 * @brief This function is used to get Interface IP address and on which interface.
 *
 * @param[out] Interface IP IP of the Interface .
 * @param[out] netmask   Netmask of the interface
 *
 * @return  Returns true if successfully gets the IP details, Otherwise returns false.
 */
bool getInterfaceConfig(char *ifName, char *interfaceIp, char *netMask);

/**
 * @brief This function is used to get Interface IP address and on which interface.
 *
 * @param[out] Interface IP IP of the Interface .
 * @param[out] interface    Internet Protocol Version.
 *
 * @return  Returns true if successfully gets the IP details, Otherwise returns false.
 */
bool getDNSip(char *primaryDNS, char *secondaryDNS);

/*
 * @brief This function is used to get STB IP address for a given address family.
 *
 * @param[in] family    IP Address family in string format. Valid values are "ipv6" or "ipv4"
 * @param[out] stbip    STB IP.
 *
 * @return  Returns true if successfully gets the IP details, Otherwise returns false.
 */
bool getSTBip_family(char *stbip,char *family);

/**
 * @brief This function gets the active interface device type(Ethernet/MOCA/WIFI).
 *
 * @param[out] devname Device type buffer to be filled.
 *
 * @return  Returns true if successfully gets the route interface type or else false.
 */
bool getRouteInterfaceType(char* devname);

/**
 * @brief This function checks whether IP string(IPv4 and IPv6) is link local address or not.
 *
 * @param[in] stbip    STB IP.
 * @param[in] family    Interface family.
 *
 * @return  Returns false in case of not supported interface family data passed.
 */
bool chk_ipaddr_linklocal(const char *stbip,unsigned int family);

/**
 * @brief This function is used to get the current active interface(WIFI/MOCA).
 *
 * @param[out] currentInterface    Current active Interface.
 *
 * @return  Returns true is successfully gets the current active interface details.
 */
bool currentActiveInterface(char *currentInterface);

/**
 * @brief This function checks whether the input ipv6 address is based on specified mac address.
 *
 * @param[in] ipv6Addr    IP address.
 * @param[in] macAddr    MAC address.
 *
 * @return  Returns true if global ipv6 address is on mac based, Otherwise false.
 */
bool check_global_v6_based_macaddress(std::string ipv6Addr,std::string macAddr);

/**
 * @brief This function checks whether the input ipv6 address is Unique Local Address.
 *
 * @param[in] ipv6Addr    IP address.
 *
 * @return  Returns true if global ipv6 address is ULA, Otherwise false.
 */
bool check_global_v6_ula_address(std::string ipv6Addr);


/**
 * @brief This function is used to get the output of running the specified command.
 *
 * @param[in]  command               command to run.
 * @param[out] output_buffer         output buffer into which to put command's output
 * @param[in]  output_buffer_size    output buffer size
 *
 * @return  Returns true if it gets the command's output successfully, Otherwise false.
 */
bool getCommandOutput(const char *command, char *output_buffer, size_t output_buffer_size);
}


/** @} */  //END OF GROUP NETSRVMGR_APIS
#endif /* _NETSRVMGRUTILES_H_ */

