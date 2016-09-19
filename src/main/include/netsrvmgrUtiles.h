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

namespace netSrvMgrUtiles
{
char* get_IfName_devicePropsFile(void);
bool getMacAddress_IfName(char *ifName_in, char macAddress_out[MAC_ADDR_BUFF_LEN]);
}



#endif /* _NETSRVMGRUTILES_H_ */

/** @} */
/** @} */
