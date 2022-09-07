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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#ifdef ENABLE_IARM
#include <signal.h>
#include "libIBus.h"
#include "libIBusDaemon.h"
#include "wifiSrvMgrIarmIf.h"
#include "netsrvmgrIarm.h"
#endif
#include "NetworkMgrMain.h"

#define NM_MGR_WIFI_CLIENT "NetworkMgrWiFiClientApps"
#ifdef SAFEC_RDKV
#include "safec_lib.h"
#else
#define STRCPY_S(dest,size,source)                    \
        strcpy(dest, source);
#endif

int Net_Srv_Reg_Events = false;
int Wifi_Mgr_Reg_Events = false;

typedef enum _NetworkManager_Route_EventId_t {
        IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_DATA=10,
        IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_MAX,
} IARM_Bus_NetworkManager_Route_EventId_t;

enum option {
    Exit,
    Test_getAvailableSSIDs,
    Test_getCurrentState,
    Test_setEnabled,
    Test_getPairedSSID,
    Test_connect,
    Test_initiateWPSPairing,
    Test_initiateWPSPairing2,
    Test_saveSSID,
    Test_clearSSID,
    Test_isPaired,
    Test_getRadioProps,
    Test_getRadioStatsProps,
    Test_getSSIDProps,
#ifdef ENABLE_LOST_FOUND
    Test_getLAFState,
#endif
    Test_getConnectedSSID,
    Test_getEndPointProps,
    Test_getAvailableSSIDsWithName,
    Test_getAvailableSSIDsIncr,
    Test_stopProgressiveScanning,
    Test_disconnectSSID,
    Test_cancelWPSPairing,
    Test_nm_registerForEvents,
    Test_nm_unregisterForEvents,
    Test_wifi_registerForEvents,
    Test_wifi_unregisterForEvents,
    Test_Max_Api,
};

struct { option number; char* name; } options[] =
{
        { Test_getAvailableSSIDs, "getAvailableSSIDs" },
        { Test_getCurrentState, "getCurrentState" },
        { Test_setEnabled, "setEnabled" },
        { Test_getPairedSSID, "getPairedSSID" },
        { Test_connect, "connect" },
        { Test_initiateWPSPairing, "initiateWPSPairing" },
        { Test_initiateWPSPairing2, "initiateWPSPairing2" },
        { Test_saveSSID, "saveSSID" },
        { Test_clearSSID, "clearSSID" },
        { Test_isPaired, "isPaired" },
        { Test_getRadioProps, "getRadioProps" },
        { Test_getRadioStatsProps, "getRadioStatsProps" },
        { Test_getSSIDProps, "getSSIDProps" },
#ifdef ENABLE_LOST_FOUND
        { Test_getLAFState, "getLAFState" },
#endif
        { Test_getConnectedSSID, "getConnectedSSID" },
        { Test_getEndPointProps, "getEndPointProps" },
        { Test_getAvailableSSIDsWithName, "getAvailableSSIDsWithName" },
        { Test_getAvailableSSIDsIncr, "getAvailableSSIDsAsycIncr" },
        { Test_stopProgressiveScanning, "stopProgressiveWifiScanning" },
        { Test_disconnectSSID, "disconnectSSID" },
        { Test_cancelWPSPairing, "cancelWPSPairing" },
        { Test_nm_registerForEvents, "net_srv_mgr_registerForEvents" },
        { Test_nm_unregisterForEvents, "net_srv_mgr_unregisterForEvents" },
        { Test_wifi_registerForEvents, "wifi_mgr_registerForEvents" },
        { Test_wifi_unregisterForEvents, "wifi_mgr_unregisterForEvents" },
        { Exit, "Exit" }
};

static bool user_answers_yes_to(char* question)
{
    char answer;
    std::cout << "\n" << question << " (y/n) ";
    std::cin >> answer;
    return answer == 'y' || answer == 'Y';
}

static void WIFI_MGR_API_getAvailableSSIDsWithName()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    char ssid[64] = {'\0'};
    double freq ;
    int timeout = 10000;
    IARM_Bus_WiFiSrvMgr_SpecificSsidList_Param_t param;
    memset(&param, 0, sizeof(param));
    printf("[%s] Entering...\r\n", __FUNCTION__);
    printf("\nEnter SSID to get the info :");
    scanf("%s",ssid);
    printf("\nEnter the band to scan 2.4 / 5 / 0 :");
    scanf("%lf",&freq);
    STRCPY_S(param.SSID, sizeof(param.SSID), ssid);
    param.frequency = freq;
    retVal = IARM_Bus_Call_with_IPCTimeout(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, (void *)&param, sizeof(IARM_Bus_WiFiSrvMgr_SpecificSsidList_Param_t),timeout);
    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    if(retVal == IARM_RESULT_SUCCESS && param.status)
    {
        printf("[\n[%s (with Message size: %d)]:\n %s \n '. \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, param.curSsids.jdataLen, param.curSsids.jdata);
    }
    else
    {
       printf("%s : \"Empty\" with message size: \'%d\'). \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, param.curSsids.jdataLen);
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}
static void WIFI_MGR_API_stopProgressiveScanning()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME,IARM_BUS_WIFI_MGR_API_stopProgressiveWifiScanning, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_stopProgressiveWifiScanning, ((retVal == IARM_RESULT_SUCCESS)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif

}

static void WIFI_MGR_API_disconnectSSID()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_disconnectSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_disconnectSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif

}

static void WIFI_MGR_API_cancelWPSPairing()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_cancelWPSPairing, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_cancelWPSPairing, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif

}

static void WIFI_MGR_API_getAvailableSSIDsIncr()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_SsidList_Param_t param;
    memset(&param, 0, sizeof(param));
    int timeout = 5000;
    printf("[%s] Entering...\r\n", __FUNCTION__);
    retVal = IARM_Bus_Call_with_IPCTimeout(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDsAsyncIncr, (void *)&param, sizeof(IARM_Bus_WiFiSrvMgr_SsidList_Param_t),timeout);

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getAvailableSSIDsAsyncIncr, ((retVal == IARM_RESULT_SUCCESS)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


static void WIFI_MGR_API_getAvailableSSIDs()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_SsidList_Param_t param;
    memset(&param, 0, sizeof(param));
    int timeout = 10000;
    printf("[%s] Entering...\r\n", __FUNCTION__);
    retVal = IARM_Bus_Call_with_IPCTimeout(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, (void *)&param, sizeof(IARM_Bus_WiFiSrvMgr_SsidList_Param_t),timeout);

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    if(retVal == IARM_RESULT_SUCCESS && param.status) {
        printf("[\n[%s (with Message size: %d)]:\n %s \n '. \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, param.curSsids.jdataLen, param.curSsids.jdata);
    }
    else {
        printf("%s : \"Empty\" with message size: \'%d\'). \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, param.curSsids.jdataLen);
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_getCurrentState()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getCurrentState, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getCurrentState, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    if(retVal == IARM_RESULT_SUCCESS && param.status) {
        printf(" \"getCurrentState\" : %d \n", param.data.wifiStatus );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

#ifdef ENABLE_LOST_FOUND
static void WIFI_MGR_API_getLAFState()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getLNFState, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getLNFState, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    if(retVal == IARM_RESULT_SUCCESS && param.status) {
        printf(" \"getLAFState\" : %d \n", param.data.wifiLNFStatus );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}
#endif
static void WIFI_MGR_API_setEnabled()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    param.data.setwifiadapter.enable = user_answers_yes_to("Press Y/y for setEnabled(true), anything else for setEnabled(false)");
    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_setEnabled, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)    {
        printf("[%s] to \'%d\'.  \n", IARM_BUS_WIFI_MGR_API_setEnabled, param.data.setwifiadapter.enable );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


static void WIFI_MGR_API_getPairedSSID()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    char *ssid = NULL;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getPairedSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getPairedSSID, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    ssid = param.data.getPairedSSID.ssid;
    if(retVal == IARM_RESULT_SUCCESS) {
        printf("\nSSID: %s \n",  (ssid)?ssid:"No ssid is assigned.");
    }

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_connect()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    std::string ssid, passphrase, ans;
    int securitymode;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\"", IARM_BUS_WIFI_MGR_API_connect);
    printf("\n***********************************\n");

    printf( "Would like to connect using ssid & passphrase. (Y/N)?" );
    std::cin >> ans;

    if(!strcasecmp(ans.c_str(),"Y"))
    {
        std::cout<< "	Enter ssid	:	";
        std::cin >> ssid;
        std::cout <<"	Enter passphrase	:	" ;
        std::cin >> passphrase;
        std::cout <<"	Enter securitymode	:	" ;
        std::cin >> securitymode;
        STRCPY_S (param.data.connect.ssid, sizeof(param.data.connect.ssid), ssid.c_str());
        STRCPY_S (param.data.connect.passphrase, sizeof(param.data.connect.passphrase), passphrase.c_str());
        param.data.connect.security_mode=(SsidSecurity)securitymode;
	printf("\n ssid = %s  passphrase = %s  security mode = %d",param.data.connect.ssid,param.data.connect.passphrase,param.data.connect.security_mode);
    }
    else {
        printf( "\nNot selected any ssid & passphrase. so, connect\n" );
    }

    retVal = IARM_Bus_Call( IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_connect, (void *)&param, sizeof(param));

    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_connect, ((retVal == IARM_RESULT_SUCCESS && param.status)?"Success":"Failure"));

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_initiateWPSPairing()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    printf("[%s] Entering...\r\n", __FUNCTION__);

    IARM_Bus_Call( IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_initiateWPSPairing, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_initiateWPSPairing, ((param.status)?"true":"false"));
    printf("\n***********************************\n");
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_initiateWPSPairing2()
{
    printf("[%s] Entering...\r\n", __FUNCTION__);
#ifdef ENABLE_IARM
    IARM_Bus_WiFiSrvMgr_WPS_Parameters_t wps_parameters = {0};
    if (user_answers_yes_to("PBC ?"))
        wps_parameters.pbc = true;
    else if (user_answers_yes_to("Auto-generate PIN ?"))
        wps_parameters.pin[0] = '\0';
    else if (user_answers_yes_to("Use serialized PIN ?"))
        strcpy(wps_parameters.pin, "xxxxxxxx");
    else
    {
        std::string pin;
        std::cout << "\nEnter PIN (8 digits): ";
        std::cin >> pin;
        strncpy(wps_parameters.pin, pin.c_str(), 8);
    }

    IARM_Bus_Call( IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_initiateWPSPairing2, (void *)&wps_parameters, sizeof(wps_parameters));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_initiateWPSPairing2, wps_parameters.status ? "true" : "false");
    if (wps_parameters.status == true && !wps_parameters.pbc)
        printf("\n \"%s\", PIN = \"%s\"", IARM_BUS_WIFI_MGR_API_initiateWPSPairing2, wps_parameters.pin);
    printf("\n***********************************\n");
#endif
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}


static void WIFI_MGR_API_saveSSID()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    std::string ssid, passphrase, ans;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));
    printf("\n***********************************");
    printf("\n \"%s\"", IARM_BUS_WIFI_MGR_API_saveSSID);
    printf("\n***********************************\n");

    printf( "This would saves the ssid and passphrase for future sessions.\n" );
    std::cout << "Enter ssid :" ;
    std::cin >>ssid;
    std::cout << "\nEnter passphrase	:" ;
    std::cin >> passphrase;
    STRCPY_S (param.data.connect.ssid, sizeof(param.data.connect.ssid), ssid.c_str());
    STRCPY_S (param.data.connect.passphrase, sizeof(param.data.connect.passphrase), passphrase.c_str());

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_saveSSID, (void *)&param, sizeof(param));

    printf("\n%s : %s \n", IARM_BUS_WIFI_MGR_API_saveSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"Successfully saved the SSID":"failed to save SSID"));
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_clearSSID()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_clearSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_clearSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_isPaired()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_isPaired, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_isPaired, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");
    printf("%s \n", (param.data.isPaired)?"Paied.":"Not paired.");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_getConnectedSSID() {
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getConnectedSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getConnectedSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("Connected SSID info: \n \
		\tSSID: \"%s\"\n \
			\tBSSID : \"%s\"\n \
			\tPhyRate : \"%f\"\n \
			\tNoise : \"%f\" \n \
                        \tBand : \"%s\"\n \
			\tSignalStrength(rssi) : \"%f\" \n \
                        \tFrequency : \"%d\"\n \
                        \tSecurityMode : \"%d\"\n  ",
           param.data.getConnectedSSID.ssid, \
           param.data.getConnectedSSID.bssid, \
           param.data.getConnectedSSID.rate, \
           param.data.getConnectedSSID.noise, \
           param.data.getConnectedSSID.band, \
           param.data.getConnectedSSID.signalStrength, \
           param.data.getConnectedSSID.frequency, \
           param.data.getConnectedSSID.securityMode);

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_getEndPointProps() 
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getEndPointProps, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getEndPointProps, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("\n Profile : \"EndPoint.1.\": \n \
		[Enable : \"%d\"| Status : \"%s\" | SSIDReference : \"%s\" ] \n",
           param.data.endPointInfo.enable, param.data.endPointInfo.status, param.data.endPointInfo.SSIDReference);

    printf(" \n Profile : \"EndPoint.1.Stats.\": \n \
		[SignalStrength : \"%d\"| Retransmissions : \"%d\" | LastDataUplinkRate : \"%d\" | LastDataDownlinkRate : \" %d\" ] \n",
           param.data.endPointInfo.stats.signalStrength, param.data.endPointInfo.stats.retransmissions,
           param.data.endPointInfo.stats.lastDataDownlinkRate, param.data.endPointInfo.stats.lastDataUplinkRate);

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


static void WIFI_MGR_API_getSSIDProps()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getSSIDProps, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getSSIDProps, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");
    std::cout << "enable : " << param.data.ssid.params.enable << std::endl;
    std::cout << "status : " << param.data.ssid.params.status << std::endl;
    std::cout << "name : " << param.data.ssid.params.name << std::endl;
    std::cout << "bssid : " << param.data.ssid.params.bssid << std::endl;
    std::cout << "macaddr : " <<  param.data.ssid.params.macaddr << std::endl;
    std::cout << "ssid : " << param.data.ssid.params.ssid << std::endl;

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


static void WIFI_MGR_API_getRadioProps()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getRadioProps, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getRadioProps, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");
    std::cout << "enable : " << param.data.radio.params.enable << std::endl;
    std::cout << "status : " << param.data.radio.params.status << std::endl;
    std::cout << "name : " << param.data.radio.params.name << std::endl;
    std::cout << "maxBitRate : " << param.data.radio.params.maxBitRate << std::endl;
    std::cout << "supportedFrequencyBands : " <<  param.data.radio.params.supportedFrequencyBands << std::endl;
    std::cout << "operatingFrequencyBand : "<< param.data.radio.params.operatingFrequencyBand << std::endl;
    std::cout << "autoChannelEnable : "<< param.data.radio.params.autoChannelEnable << std::endl;
    std::cout << "autoChannelRefreshPeriod : "<< param.data.radio.params.autoChannelRefreshPeriod << std::endl;
    std::cout << "autoChannelSupported : "<< param.data.radio.params.autoChannelSupported << std::endl;
    std::cout << "channelsInUse : " << param.data.radio.params.channelsInUse << std::endl;
    std::cout << "channel : " << param.data.radio.params.channel << std::endl;
    std::cout << "extensionChannel : " << param.data.radio.params.extensionChannel << std::endl;
    std::cout << "guardInterval : " << param.data.radio.params.guardInterval << std::endl;
    std::cout << "mcs : " << param.data.radio.params.mcs << std::endl;
    std::cout << "operatingChannelBandwidth : " << param.data.radio.params.operatingChannelBandwidth << std::endl;
    std::cout << "operatingStandards : " << param.data.radio.params.operatingStandards << std::endl;
    std::cout << "possibleChannels : " << param.data.radio.params.possibleChannels << std::endl;
    std::cout << "regulatoryDomain : " << param.data.radio.params.regulatoryDomain << std::endl;

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_getRadioStatsProps()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getRadioStatsProps, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getRadioStatsProps, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");
    std::cout << "BytesSent : " << param.data.radio_stats.params.bytesSent << std::endl;
    std::cout << "BytesReceived : " << param.data.radio_stats.params.bytesReceived << std::endl;
    std::cout << "PacketsSent : " << param.data.radio_stats.params.packetsSent << std::endl;
    std::cout << "PacketsReceived : " << param.data.radio_stats.params.packetsReceived << std::endl;
    std::cout << "ErrorsSent : " <<  param.data.radio_stats.params.errorsSent << std::endl;
    std::cout << "ErrorsReceived : "<< param.data.radio_stats.params.errorsReceived << std::endl;
    std::cout << "DiscardPacketsSent : " << param.data.radio_stats.params.discardPacketsSent << std::endl;
    std::cout << "DiscardPacketsReceived : " << param.data.radio_stats.params.discardPacketsReceived << std::endl;
    std::cout << "PLCPErrorCount : " << param.data.radio_stats.params.plcErrorCount << std::endl;
    std::cout << "FCSErrorCount : " << param.data.radio_stats.params.fcsErrorCount << std::endl;
    std::cout << "InvalidMACCount : " << param.data.radio_stats.params.invalidMACCount << std::endl;
    std::cout << "PacketsOtherReceived : " << param.data.radio_stats.params.packetsOtherReceived << std::endl;

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}
#ifdef ENABLE_IARM
#define IARM_CHECK(FUNC) { \
    if ((res = FUNC) != IARM_RESULT_SUCCESS) { \
        printf("IARM %s: %s\n", #FUNC, \
            res == IARM_RESULT_INVALID_PARAM ? "invalid param" : ( \
            res == IARM_RESULT_INVALID_STATE ? "invalid state" : ( \
            res == IARM_RESULT_IPCCORE_FAIL ? "ipcore fail" : ( \
            res == IARM_RESULT_OOM ? "oom" : "unknown")))); \
    } \
    else \
    { \
        printf("IARM %s: success\n", #FUNC); \
    } \
}
#endif //#ifdef ENABLE_IARM

#ifdef ENABLE_IARM
static void eventHandler_nm_mgr(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    if (strcmp(owner, IARM_BUS_NM_SRV_MGR_NAME) != 0)
    {
        printf ("ERROR - nm_mgr unexpected event: owner %s, eventId: %d, data: %p, size: %d.\n", owner, (int)eventId, data, len);
        return;
    }
    if (data == nullptr || len == 0)
    {
        printf ("ERROR - event with NO DATA: eventId: %d, data: %p, size: %d.\n", (int)eventId, data, len);
        return;
    }

    switch (eventId)
    {
        case IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_ENABLED_STATUS:
        {
            IARM_BUS_NetSrvMgr_Iface_EventInterfaceEnabledStatus_t *e = (IARM_BUS_NetSrvMgr_Iface_EventInterfaceEnabledStatus_t*) data;
            printf ("IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_ENABLED_STATUS interface = %s, enabled = %d\n",
                    e->interface, e->status);
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_CONNECTION_STATUS:
        {
            IARM_BUS_NetSrvMgr_Iface_EventInterfaceConnectionStatus_t *e = (IARM_BUS_NetSrvMgr_Iface_EventInterfaceConnectionStatus_t*) data;
            printf ("IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_CONNECTION_STATUS interface = %s, connected = %d\n",
                    e->interface, e->status);
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_IPADDRESS:
        {
            IARM_BUS_NetSrvMgr_Iface_EventInterfaceIPAddress_t *e = (IARM_BUS_NetSrvMgr_Iface_EventInterfaceIPAddress_t*) data;
            printf ("IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_IPADDRESS interface = %s, ip_address = %s, acquired = %d\n",
                    e->interface, e->ip_address, e->acquired);
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_DEFAULT_INTERFACE:
        {
            IARM_BUS_NetSrvMgr_Iface_EventDefaultInterface_t *e = (IARM_BUS_NetSrvMgr_Iface_EventDefaultInterface_t*) data;
            printf ("IARM_BUS_NETWORK_MANAGER_EVENT_DEFAULT_INTERFACE oldInterface = %s newInterface = %s\n",
                    e->oldInterface, e->newInterface);
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED:
        {
            IARM_BUS_NetSrvMgr_Iface_EventData_t iarmData = { 0 };
            printf ("IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED = %d \n", iarmData.isInterfaceEnabled);
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE:
        {
            IARM_BUS_NetSrvMgr_Iface_EventData_t iarmData = { 0 };
            printf ("IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE = %d for interface %s\n", iarmData.isInterfaceEnabled,
                    iarmData.setInterface);
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_WIFI_INTERFACE_STATE:
        {
            IARM_BUS_NetSrvMgr_Iface_EventData_t iarmData = { 0 };
            printf ("IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED = %d \n", iarmData.isInterfaceEnabled);
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_SWITCH_TO_PRIVATE:
        {
            printf("IARM_BUS_NETWORK_MANAGER_EVENT_SWITCH_TO_PRIVATE broadcast!");
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_STOP_LNF_WHILE_DISCONNECTED:
        {
            printf("IARM_BUS_NETWORK_MANAGER_EVENT_STOP_LNF_WHILE_DISCONNECTED event!");
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_AUTO_SWITCH_TO_PRIVATE_ENABLED:
        {
            printf("IARM_BUS_NETWORK_MANAGER_EVENT_AUTO_SWITCH_TO_PRIVATE_ENABLED  event!");
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_DATA:
        {
            printf("IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_DATA  event!");
            break;
        }
    }
}
#endif //#ifdef ENABLE_IARM

#ifdef ENABLE_IARM
static void eventHandler_wifi_mgr(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    if (strcmp(owner, IARM_BUS_NM_SRV_MGR_NAME) != 0)
    {
        printf ("ERROR - wifi_mgr unexpected event: owner %s, eventId: %d, data: %p, size: %d.\n", owner, (int)eventId, data, len);
        return;
    }
    if (data == nullptr || len == 0)
    {
        printf ("ERROR - event with NO DATA: eventId: %d, data: %p, size: %d.\n", (int)eventId, data, len);
        return;
    }

    switch (eventId)
    {
        case IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDs:
        {
            IARM_BUS_WiFiSrvMgr_EventData_t *e = (IARM_BUS_WiFiSrvMgr_EventData_t *)data;
            printf ("IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDs data.wifiSSIDList.ssid_list = %s\n",
            e->data.wifiSSIDList.ssid_list);
            break;
        }
        case IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDsIncr:
        {
            IARM_BUS_WiFiSrvMgr_EventData_t *e = (IARM_BUS_WiFiSrvMgr_EventData_t*) data;
            printf ("IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDsIncr data.wifiSSIDList.more_data = %d data.wifiSSIDList.ssid_list = %s\n",
                    e->data.wifiSSIDList.more_data, e->data.wifiSSIDList.ssid_list);
            break;
        }
        case IARM_BUS_WIFI_MGR_EVENT_onError:
        {
            IARM_BUS_WiFiSrvMgr_EventData_t *e = (IARM_BUS_WiFiSrvMgr_EventData_t*) data;
            printf ("IARM_BUS_WIFI_MGR_EVENT_onError data.wifiError.code = %d\n",
                     e->data.wifiError.code);
            break;
        }
        case IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged:
        {
            IARM_BUS_WiFiSrvMgr_EventData_t *e = (IARM_BUS_WiFiSrvMgr_EventData_t*) data;
            printf ("IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged data.wifiStateChange.state = %d\n",
                     e->data.wifiStateChange.state);
            break;
        }
        case IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged:
        {
            IARM_BUS_WiFiSrvMgr_EventData_t *e = (IARM_BUS_WiFiSrvMgr_EventData_t*) data;
            printf ("IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged data.wifiSSIDList.ssid_list = %s data.wifiStateChange.state = %d \n",
            e->data.wifiSSIDList.ssid_list, e->data.wifiStateChange.state);
            break;
        }
    }
}
#endif //#ifdef ENABLE_IARM

static void NET_MGR_registerForEvents()
{
    #ifdef ENABLE_IARM
    printf("[%s] Entering...\r\n", __FUNCTION__);

    if (Net_Srv_Reg_Events)
    {
        printf("[%s] Already registered\n", __FUNCTION__);
    }
    else
    {
        Net_Srv_Reg_Events = true;

        IARM_Result_t res;
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_ENABLED_STATUS, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_CONNECTION_STATUS, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_IPADDRESS, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_DEFAULT_INTERFACE, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_WIFI_INTERFACE_STATE , eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SWITCH_TO_PRIVATE, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_STOP_LNF_WHILE_DISCONNECTED, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_AUTO_SWITCH_TO_PRIVATE_ENABLED, eventHandler_nm_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_DATA, eventHandler_nm_mgr) );

    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_registerForEvents()
{
#ifdef ENABLE_IARM
    printf("[%s] Entering...\r\n", __FUNCTION__);

    if (Wifi_Mgr_Reg_Events)
    {
        printf("[%s] Already registered\n", __FUNCTION__);
    }
    else
    {
        Wifi_Mgr_Reg_Events = true;

        IARM_Result_t res;
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDs , eventHandler_wifi_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDsIncr, eventHandler_wifi_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onError, eventHandler_wifi_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged, eventHandler_wifi_mgr) );
        IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged, eventHandler_wifi_mgr) );

    }

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void NET_MGR_unregisterForEvents()
{
#ifdef ENABLE_IARM
    printf("[%s] Entering...\r\n", __FUNCTION__);

    if (!Net_Srv_Reg_Events)
    {
        printf("[%s] Already unregistered\n", __FUNCTION__);
    }
    else
    {
        IARM_Result_t res;
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_ENABLED_STATUS) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_CONNECTION_STATUS) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_IPADDRESS) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_DEFAULT_INTERFACE) );

        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_WIFI_INTERFACE_STATE ) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SWITCH_TO_PRIVATE ) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_STOP_LNF_WHILE_DISCONNECTED) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_AUTO_SWITCH_TO_PRIVATE_ENABLED) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_ROUTE_DATA) );

        Net_Srv_Reg_Events = false;
    }

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_unregisterForEvents()
{
#ifdef ENABLE_IARM
    printf("[%s] Entering...\r\n", __FUNCTION__);

    if (!Wifi_Mgr_Reg_Events)
    {
        printf("[%s] Already unregistered\n", __FUNCTION__);
    }
    else
    {
        IARM_Result_t res;
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDs) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDsIncr) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onError) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged) );
        IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged) );

        Wifi_Mgr_Reg_Events = false;
    }

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void cleanup()
{
#ifdef ENABLE_IARM
    if (Wifi_Mgr_Reg_Events)
    {
        WIFI_MGR_unregisterForEvents();
    }
    if (Net_Srv_Reg_Events)
    {
        NET_MGR_unregisterForEvents();
    }
#endif //#ifdef ENABLE_IARM
    return;
}

void signal_handler (int sigNum)
{
#ifdef ENABLE_IARM
    printf ("%s(): Received signal %d \n",__FUNCTION__, sigNum);

    cleanup();
    
    exit(0);

#endif //#ifdef ENABLE_IARM
}

int main()
{
#ifdef ENABLE_IARM
    int input;
    bool loop = true;
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
//    printf("[%s] Starting \'%s\' Client.\r\n", __FUNCTION__, IARM_BUS_NM_SRV_MGR_NAME);

        /* Signal handler */
    signal (SIGHUP, signal_handler);
    signal (SIGINT, signal_handler);
    signal (SIGQUIT, signal_handler);
    signal (SIGTERM, signal_handler);

    IARM_Bus_Init(NM_MGR_WIFI_CLIENT);
    IARM_Bus_Connect();

    do
    {
        printf("\n==================================================================\n");
        printf("*****	Network Mgr: Execute WiFi Manager Api's		****	");
        printf("\n==================================================================\n");
        for (int n = sizeof(options)/sizeof(options[0]), i = 0; i < n; i++)
            printf( "%d. %s\n", options[i].number, options[i].name);
        printf( "\n==================================================================\n");
        printf( "\n Selection: " );
        std::cin >> input;

        /* Get/Set RPC handler*/
        switch (input) {
        case Test_getAvailableSSIDs:
            WIFI_MGR_API_getAvailableSSIDs();
            break;
        case Test_getAvailableSSIDsWithName:
            WIFI_MGR_API_getAvailableSSIDsWithName();
            break;
        case Test_getCurrentState:
            WIFI_MGR_API_getCurrentState();
            break;
        case Test_setEnabled:
            WIFI_MGR_API_setEnabled();
            break;
        case Test_getPairedSSID:
            WIFI_MGR_API_getPairedSSID();
            break;
        case Test_connect:
            WIFI_MGR_API_connect();
            break;
        case Test_initiateWPSPairing:
            WIFI_MGR_API_initiateWPSPairing();
            break;
        case Test_initiateWPSPairing2:
            WIFI_MGR_API_initiateWPSPairing2();
            break;
        case Test_saveSSID:
            WIFI_MGR_API_saveSSID();
            break;
        case Test_clearSSID:
            WIFI_MGR_API_clearSSID();
            break;
        case Test_isPaired:
            WIFI_MGR_API_isPaired();
            break;
        case Test_getRadioProps:
            WIFI_MGR_API_getRadioProps();
            break;
        case Test_getRadioStatsProps:
            WIFI_MGR_API_getRadioStatsProps();
            break;
        case Test_getSSIDProps:
            WIFI_MGR_API_getSSIDProps();
            break;
#ifdef ENABLE_LOST_FOUND
        case Test_getLAFState:
            WIFI_MGR_API_getLAFState();
            break;
#endif
        case Test_getConnectedSSID:
            WIFI_MGR_API_getConnectedSSID();
            break;
        case Test_getEndPointProps:
            WIFI_MGR_API_getEndPointProps();
            break;
        case Test_getAvailableSSIDsIncr:
            WIFI_MGR_API_getAvailableSSIDsIncr();
            break;
        case Test_stopProgressiveScanning:
            WIFI_MGR_API_stopProgressiveScanning();
            break;
        case Test_disconnectSSID:
            WIFI_MGR_API_disconnectSSID();
            break;
        case Test_cancelWPSPairing:
            WIFI_MGR_API_cancelWPSPairing();
            break;
        case Test_nm_registerForEvents:
            NET_MGR_registerForEvents();
            break;
        case Test_nm_unregisterForEvents:
            NET_MGR_unregisterForEvents();
            break;
        case Test_wifi_registerForEvents:
            WIFI_MGR_registerForEvents();
            break;
        case Test_wifi_unregisterForEvents:
            WIFI_MGR_unregisterForEvents();
            break;
        default:
            loop = false;
            printf( "Wrong Input..., try again.\n" );
            break;
        }
    } while (loop);

    cleanup();

    IARM_Bus_Disconnect();
    IARM_Bus_Term();
//    printf("[%s] Exiting... \'%s\' Client Exiting\r\n", __FUNCTION__, IARM_BUS_NM_SRV_MGR_NAME );

#endif
}
