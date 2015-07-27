/*
 * ============================================================================
 * COMCAST C O N F I D E N T I A L AND PROPRIETARY
 * ============================================================================
 * This file and its contents are the intellectual property of Comcast.  It may
 * not be used, copied, distributed or otherwise  disclosed in whole or in part
 * without the express written permission of Comcast.
 * ============================================================================
 * Copyright (c) 2015 Comcast. All rights reserved.
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include "libIBus.h"
#include "libIBusDaemon.h"
#include "NetworkMgrWiFiIarmIf.h"

#define NM_MGR_WIFI_CLIENT "NetworkMgrWiFiClientApps"

static void WIFI_MGR_API_getAvailableSSIDs()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    NM_WiFi_getAvailableSSID_Args param;
    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
        printf("[%s] : \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDs);
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

int main()
{

    IARM_Result_t retVal = IARM_RESULT_SUCCESS;

    printf("[%s] Starting \'%s\' Client.\r\n", __FUNCTION__, IARM_BUS_NM_MGR_NAME);
    IARM_Bus_Init(NM_MGR_WIFI_CLIENT);
    IARM_Bus_Connect();

    /* Get/Set RPC handler*/
    std::cout << "==========================================\n";
    WIFI_MGR_API_getAvailableSSIDs();

    IARM_Bus_Disconnect();
    IARM_Bus_Term();
    printf("[%s] Exiting... \'%s\' Client Exiting\r\n", __FUNCTION__, IARM_BUS_NM_MGR_NAME );

}

