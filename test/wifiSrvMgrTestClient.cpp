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
#include "wifiSrvMgrIarmIf.h"

#define NM_MGR_WIFI_CLIENT "NetworkMgrWiFiClientApps"

static void WIFI_MGR_API_getAvailableSSIDs()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
#if 0
    NM_WiFi_getAvailableSSID_Args param;
    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
        printf("[%s] : \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDs);
    }
#endif
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_getCurrentState()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    WiFiStatusCode_t wifiStatusCode;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_getCurrentState, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
        wifiStatusCode = param.data.wifiStatus;
        printf("[%s] : Param.status : %d WiFiStatusCode : %d \n", IARM_BUS_WIFI_MGR_API_getCurrentState,
               param.status,	wifiStatusCode );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_setEnabled()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    param.data.setwifiadapter.enable = true;

    retVal = IARM_Bus_Call(IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_setEnabled, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
       printf("[%s] to \'%d\'.  \n", IARM_BUS_WIFI_MGR_API_setEnabled, param.data.setwifiadapter.enable );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}


static void WIFI_MGR_API_getPairedSSID()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_getPairedSSID, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
        printf("[%s] : Status : %d & Paied SSID: %s \n", IARM_BUS_WIFI_MGR_API_getPairedSSID,
               param.status,
               param.data.getPairedSSID.ssid);
    }

    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_connect()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    strcpy (param.data.connect.ssid, "HOME-E137-5");
    strcpy (param.data.connect.passphrase, "JCHNANCAANJ773CP");

    retVal = IARM_Bus_Call( IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_connect, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
        printf("[%s] : status : %d \n", IARM_BUS_WIFI_MGR_API_connect, param.status);
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_saveSSID()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;

    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    strcpy (param.data.connect.ssid, "HOME-E137-5");
    strcpy (param.data.connect.passphrase, "JCHNANCAANJ773CP");

    retVal = IARM_Bus_Call(IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_saveSSID, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
        printf("[%s] : Status : %s \n", IARM_BUS_WIFI_MGR_API_saveSSID,
               ((param.status)?"Successfully saved the SSID":"failed to save SSID"));
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_clearSSID()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    strcpy (param.data.connect.ssid, "HOME-E137-5");
    strcpy (param.data.connect.passphrase, "JCHNANCAANJ773CP");

    retVal = IARM_Bus_Call(IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_clearSSID, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
        printf("[%s] : ClearSSID Status : \'%s\'.\n", IARM_BUS_WIFI_MGR_API_clearSSID,
               ((param.status)?"cleared":"failed"));
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_isPaired()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_MGR_NAME, IARM_BUS_WIFI_MGR_API_isPaired, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)
    {
        printf("[%s]: status: %s & SSID: \'%s\'\n", IARM_BUS_WIFI_MGR_API_isPaired,
               ((param.status)?"true":"false"),
               param.data.getPairedSSID.ssid  );
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
    std::cout << "==========================================\n";
    WIFI_MGR_API_getCurrentState();
    std::cout << "==========================================\n";
	WIFI_MGR_API_setEnabled();
    std::cout << "==========================================\n";
    WIFI_MGR_API_getPairedSSID();
    std::cout << "==========================================\n";
    WIFI_MGR_API_connect();
    std::cout << "==========================================\n";
    WIFI_MGR_API_saveSSID();
    std::cout << "==========================================\n";
    WIFI_MGR_API_clearSSID();
    std::cout << "==========================================\n";
    WIFI_MGR_API_isPaired();
    std::cout << "==========================================\n";

    IARM_Bus_Disconnect();
    IARM_Bus_Term();
    printf("[%s] Exiting... \'%s\' Client Exiting\r\n", __FUNCTION__, IARM_BUS_NM_MGR_NAME );

}

