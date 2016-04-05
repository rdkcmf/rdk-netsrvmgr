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

enum {
    Test_getAvailableSSIDs =1,
    Test_getCurrentState = 2,
    Test_setEnabled = 3,
    Test_getPairedSSID =4,
    Test_connect = 5,
    Test_initiateWPSPairing = 6,
    Test_saveSSID = 7,
    Test_clearSSID =8,
    Test_isPaired =9,
    Test_getRadioProps =10,
    Test_getRadioStatsProps =11,
    Test_getSSIDProps =12,
    Test_Max_Api,
};


static void WIFI_MGR_API_getAvailableSSIDs()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_SsidList_Param_t param;
    memset(&param, 0, sizeof(param));
    printf("[%s] Entering...\r\n", __FUNCTION__);
    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, (void *)&param, sizeof(IARM_Bus_WiFiSrvMgr_SsidList_Param_t));

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
}

static void WIFI_MGR_API_getCurrentState()
{
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
}

static void WIFI_MGR_API_setEnabled()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    param.data.setwifiadapter.enable = true;
    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_setEnabled, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)    {
        printf("[%s] to \'%d\'.  \n", IARM_BUS_WIFI_MGR_API_setEnabled, param.data.setwifiadapter.enable );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}


static void WIFI_MGR_API_getPairedSSID()
{
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
}

static void WIFI_MGR_API_connect()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    std::string ssid, passphrase, ans;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\"", IARM_BUS_WIFI_MGR_API_connect);
    printf("\n***********************************\n");

    printf( "Would like to connect using ssid & passphrase. (Y/N)?" );
    std::cin >> ans;

    if(!strcasecmp(ans.c_str(),"Y"))
    {
        std::cout<< "	Enter ssid 	: 	";
        std::cin >> ssid;
        std::cout <<"	Enter passphrase	:	" ;
        std::cin >> passphrase;
        strcpy (param.data.connect.ssid, ssid.c_str());
        strcpy (param.data.connect.passphrase, passphrase.c_str());
    }
    else {
        printf( "\nNot selected any ssid & passphrase. so, connect\n" );
    }

    retVal = IARM_Bus_Call( IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_connect, (void *)&param, sizeof(param));

    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_connect, ((retVal == IARM_RESULT_SUCCESS && param.status)?"Success":"Failure"));

    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_initiateWPSPairing()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    printf("[%s] Entering...\r\n", __FUNCTION__);

    IARM_Bus_Call( IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_initiateWPSPairing, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_initiateWPSPairing, ((param.status)?"true":"false"));
    printf("\n***********************************\n");
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_saveSSID()
{
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
    strcpy (param.data.connect.ssid, ssid.c_str());
    strcpy (param.data.connect.passphrase, passphrase.c_str());

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_saveSSID, (void *)&param, sizeof(param));

    printf("\n%s : %s \n", IARM_BUS_WIFI_MGR_API_saveSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"Successfully saved the SSID":"failed to save SSID"));
    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_clearSSID()
{
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_clearSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_clearSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
}

static void WIFI_MGR_API_isPaired()
{
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
}

static void WIFI_MGR_API_getSSIDProps()
{
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
}


static void WIFI_MGR_API_getRadioProps()
{
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
}

static void WIFI_MGR_API_getRadioStatsProps()
{
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
}


int main()
{
    int input;
    bool loop = true;
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
//    printf("[%s] Starting \'%s\' Client.\r\n", __FUNCTION__, IARM_BUS_NM_SRV_MGR_NAME);

    IARM_Bus_Init(NM_MGR_WIFI_CLIENT);
    IARM_Bus_Connect();

    do
    {
        printf("\n==================================================================\n");
        printf("*****	Network Mgr: Execute WiFi Manager Api's		**** 	");
        printf("\n==================================================================\n");
        printf( "1. getAvailableSSIDs\n" );
        printf( "2. getCurrentState\n" );
        printf( "3. setEnabled\n" );
        printf( "4. getPairedSSID\n");
        printf( "5. connect\n");
        printf( "6. initiateWPSPairing\n");
        printf( "7. saveSSID\n");
        printf( "8. clearSSID\n");
        printf( "9. isPaired\n");
        printf( "10. getRadioProps\n");
        printf( "11. getRadioStatsProps\n");
        printf( "12. getSSIDProps\n");
        printf( "0. Exit." );
        printf( "\n==================================================================\n");

        printf( "\n Selection: " );
        std::cin >> input;

        /* Get/Set RPC handler*/
        switch (input) {
        case Test_getAvailableSSIDs:
            WIFI_MGR_API_getAvailableSSIDs();
            break;
        case Test_getCurrentState:
            WIFI_MGR_API_getCurrentState();
            break;
        case Test_setEnabled:
            printf( "\'setEnabled\' Not Applicable any more..!!!\n" );
//			WIFI_MGR_API_setEnabled();
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
        default:
            loop = false;
            printf( "Wrong Input..., try again.\n" );
            break;
        }
    } while (loop);

    IARM_Bus_Disconnect();
    IARM_Bus_Term();
//    printf("[%s] Exiting... \'%s\' Client Exiting\r\n", __FUNCTION__, IARM_BUS_NM_SRV_MGR_NAME );


}

