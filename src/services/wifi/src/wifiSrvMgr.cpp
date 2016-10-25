/*
 * ============================================================================
 * COMCAST C O N F I D E N T I A L AND PROPRIETARY
 * ============================================================================
 * This file (and its contents) are the intellectual property of Comcast.  It may
 * not be used, copied, distributed or otherwise  disclosed in whole or in part
 * without the express written permission of Comcast.
 * ============================================================================
 * Copyright (c) 2015 Comcast. All rights reserved.
 * ============================================================================
 */

#include "wifiSrvMgr.h"
#include "NetworkMgrMain.h"
#include "wifiSrvMgrIarmIf.h"
#include "NetworkMedium.h"
#include "wifiHalUtiles.h"


#ifdef USE_HOSTIF_WIFI_HAL
#include "hostIf_tr69ReqHandler.h"
#endif

#define SAVE_SSID 1

#ifdef USE_RDK_WIFI_HAL
static void _irEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
#endif /* USE_RDK_WIFI_HAL */
#ifdef ENABLE_LOST_FOUND
extern bool bDeviceActivated;
extern WiFiLNFStatusCode_t gWifiLNFStatus;
static void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
#endif
ssidList gSsidList;
extern netMgrConfigProps confProp;
WiFiConnectionStatus savedWiFiConnList;
IARM_Bus_Daemon_SysMode_t sysModeParam;

WiFiNetworkMgr* WiFiNetworkMgr::instance = NULL;
bool WiFiNetworkMgr::instanceIsReady = false;
pthread_mutex_t wpsConnLock = PTHREAD_MUTEX_INITIALIZER;

WiFiNetworkMgr::WiFiNetworkMgr() {}
WiFiNetworkMgr::~WiFiNetworkMgr() { }

WiFiNetworkMgr* WiFiNetworkMgr::getInstance()
{
    if (instance == NULL)
    {
        instance = new WiFiNetworkMgr();
        instanceIsReady = true;
    }
    return instance;
}


int  WiFiNetworkMgr::Start()
{
    bool retVal=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, getAvailableSSIDs);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getCurrentState, getCurrentState);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_setEnabled, setEnabled);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_connect, connect);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_initiateWPSPairing, initiateWPSPairing);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getPairedSSID, getPairedSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_saveSSID, saveSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_clearSSID, clearSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_isPaired, isPaired);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getConnectedSSID, getConnectedSSID);

#ifdef ENABLE_LOST_FOUND
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getLNFState, getLNFState);
    IARM_Bus_RegisterEventHandler(IARM_BUS_AUTHSERVICE_NAME, IARM_BUS_AUTHSERVICE_EVENT_SWITCH_TO_PRIVATE, _eventHandler);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SWITCH_TO_PRIVATE, _eventHandler);
#endif
    /* Diagnostic Api's*/
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getRadioProps, getRadioProps);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_setRadioProps, setRadioProps);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getRadioStatsProps, getRadioStatsProps);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getSSIDProps, getSSIDProps);
    IARM_Bus_RegisterCall(IARM_BUS_COMMON_API_SysModeChange,sysModeChange);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getEndPointProps, getEndPointProps);
#ifdef USE_RDK_WIFI_HAL
    /* Front Panel Event Listner  */
    IARM_Bus_RegisterEventHandler(IARM_BUS_IRMGR_NAME, IARM_BUS_IRMGR_EVENT_IRKEY, _irEventHandler);
#endif


    /* Notification RPC:*/
    IARM_Bus_RegisterEvent(IARM_BUS_WIFI_MGR_EVENT_MAX);

    memset(&gSsidList, '\0', sizeof(ssidList));
    /*Check for WiFi Capability */
    isWiFiCapable();

#ifdef USE_RDK_WIFI_HAL
    if(wifi_init() == RETURN_OK) {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Successfully wifi_init() done. \n", __FUNCTION__, __LINE__ );
    } else  {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in wifi_init(). \n", __FUNCTION__, __LINE__ );
    }

    /*Register connect and disconnect call back */
    wifi_connectEndpoint_callback_register(wifi_connect_callback);
    wifi_disconnectEndpoint_callback_register(wifi_disconnect_callback);


    monitor_WiFiStatus();
#endif


#ifdef ENABLE_LOST_FOUND
    if(confProp.wifiProps.bEnableLostFound)
    {
        if(getLAFssid())
        {
            if(false == isWifiConnected())
            {
                lafConnectToPrivate();
                connectToLAF();
            }
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] lfssid fetch failure !!!!!!! \n", __FUNCTION__, __LINE__ );
        }
    }
#endif


    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}

int  WiFiNetworkMgr::Stop()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    IARM_Bus_Disconnect();
    IARM_Bus_Term();
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}


IARM_Result_t WiFiNetworkMgr::getAvailableSSIDs(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    bool status = true;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_SsidList_Param_t *param = (IARM_Bus_WiFiSrvMgr_SsidList_Param_t *)arg;

    char jbuff[MAX_SSIDLIST_BUF] = {'\0'};
    int jBuffLen = 0;

#ifdef USE_RDK_WIFI_HAL
    status = scan_Neighboring_WifiAP(jbuff);
    jBuffLen = strlen(jbuff);
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Scan AP's SSID list buffer size : \n\"%d\"\n", __FUNCTION__, __LINE__, jBuffLen);
#endif
    if(status == false)
    {
        param->status = false;
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] No SSID connected or SSID available.\n", __FUNCTION__, __LINE__);
        return ret;
    }


    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] json Message length : [%d].\n", __FUNCTION__, __LINE__, jBuffLen);

    if(jBuffLen) {
        strncpy(param->curSsids.jdata, jbuff, jBuffLen);
        param->curSsids.jdataLen = jBuffLen;
        param->status = true;
        ret = IARM_RESULT_SUCCESS;
    }
    else {
        param->status = false;
    }

//    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID List : [%s].\n", __FUNCTION__, __LINE__, param->curSsids.jdata);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::getCurrentState(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = true;

    //if the ethernet is plugged in then report disabled
    if( !ethernet_on() )
        param->data.wifiStatus = get_WifiRadioStatus();
    else
        param->data.wifiStatus = WIFI_DISABLED;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}
#ifdef ENABLE_LOST_FOUND
IARM_Result_t WiFiNetworkMgr::getLNFState(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = true;
    param->data.wifiLNFStatus = get_WiFiLNFStatusCode();
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}
#endif
IARM_Result_t WiFiNetworkMgr::setEnabled(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    bool wifiadapterEnableState = false;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    wifiadapterEnableState = param->data.setwifiadapter.enable;

    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] IARM_BUS_WIFI_MGR_API_setEnabled to %d\n", __FUNCTION__, __LINE__, wifiadapterEnableState);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );

    return ret;
}

IARM_Result_t WiFiNetworkMgr::connect(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    param->status = false;
    int ssidIndex = 1;
    SsidSecurity securityMode;
    securityMode=param->data.connect.security_mode;
    char *ssid = param->data.connect.ssid;
    short ssid_len = strlen(param->data.connect.ssid);
    char *pass = param->data.connect.passphrase;
    short pass_len = strlen (param->data.connect.passphrase);

    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Connect with SSID (%s) & Passphrase (%s) security mode (%d)\n", __FUNCTION__, __LINE__, ssid, pass,securityMode);
    /* If param data receives as Empty, then use the saved SSIDConnection */
    if (!ssid_len)
    {
        if((savedWiFiConnList.ssidSession.ssid[0] != '\0') && (savedWiFiConnList.ssidSession.passphrase[0] != '\0'))
        {
            /*Now try to connect using saved SSID & PSK */
#ifdef USE_RDK_WIFI_HAL
            connect_withSSID(ssidIndex, savedWiFiConnList.ssidSession.ssid, securityMode, NULL, NULL, savedWiFiConnList.ssidSession.passphrase,SAVE_SSID);
#endif
            param->status = true;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed, Empty saved SSID & Passphrase.\n", __FUNCTION__, __LINE__);
            param->status = false;
        }
    }
    /* If param data receives with SSID and Pass, then use these to compare with saved SSIDConnection & connect*/
    else
    {
        if(ssid_len && pass_len)
        {
            /*Connect with Saved SSID */
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Received valid SSID (%s) & Passphrase (%s).\n", __FUNCTION__, __LINE__, ssid, pass);
#ifdef USE_RDK_WIFI_HAL
            connect_withSSID(ssidIndex, ssid, securityMode, NULL, NULL, pass,SAVE_SSID);
#endif
            param->status = true;
        }
        /* Passphrase can be null when the network security is NONE. */
        else if (ssid_len && (0 == pass_len) && securityMode == NET_WIFI_SECURITY_NONE)
        {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Received valid SSID (%s) with Empty Passphrase.\n", __FUNCTION__, __LINE__, ssid);
#ifdef USE_RDK_WIFI_HAL
            connect_withSSID(ssidIndex, ssid, securityMode, NULL, NULL, savedWiFiConnList.ssidSession.passphrase,SAVE_SSID);
#endif
            param->status = true;
        }
        else {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Invalid SSID & Passphrase.\n", __FUNCTION__, __LINE__);
        }
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::initiateWPSPairing(void* arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = false;
#ifdef USE_RDK_WIFI_HAL
    pthread_mutex_lock(&wpsConnLock);
    param->status = connect_WpsPush()? true: false;
    pthread_mutex_unlock(&wpsConnLock);
#endif
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::saveSSID(void* arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    bool retval = false;
    param->status = false;
    char *ssid = param->data.connect.ssid;
    short ssid_len = strlen(param->data.connect.ssid);
    char *psk = param->data.connect.passphrase;
    short psk_len = strlen (param->data.connect.passphrase);


    /* Saves the ssid and passphrase for future sessions.
     * If an SSID was previously saved, the new SSID and passphrase will overwrite the existing one.
     * Returns false if the passphrase was not able to be saved, and true if the save was successful.*/

    if(ssid_len & psk_len)
    {
        memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
        strncpy(savedWiFiConnList.ssidSession.ssid, ssid, ssid_len+1);
        strncpy(savedWiFiConnList.ssidSession.passphrase, psk, psk_len+1);
        savedWiFiConnList.conn_type = SSID_SECLECTION_CONNECT;
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] %s to file, SSID (%s) & Passphrase (%s).\n", __FUNCTION__, __LINE__, retval? "Successfully Saved": "Failed to Save", ssid, psk);
        param->status = true;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Empty data for SaveSSID\n", __FUNCTION__, __LINE__);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::clearSSID(void* arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = false;

#ifdef USE_RDK_WIFI_HAL
    param->status = clearSSID_On_Disconnect_AP();
#endif

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}
/* returns the SSID if one was previously saved through saveSSID,
 * otherwise returns NULL.*/
IARM_Result_t WiFiNetworkMgr::getPairedSSID(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal = false;
//    WiFiConnectionStatus currSsidInfo;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

//    memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

//    get_CurrentSsidInfo(&currSsidInfo);

//    if (currSsidInfo.ssidSession.ssid[0] != '\0')
#ifdef USE_RDK_WIFI_HAL
    retVal=lastConnectedSSID();
#endif
    if( retVal == true )
    {
        char *ssid = savedWiFiConnList.ssidSession.ssid;
//        memcpy(param->data.getPairedSSID.ssid, currSsidInfo.ssidSession.ssid, SSID_SIZE);
        memcpy(param->data.getPairedSSID.ssid, ssid, strlen(ssid));
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] getPairedSSID SSID (%s).\n", __FUNCTION__, __LINE__, ssid);
        param->status = true;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Error in getting last ssid .\n", __FUNCTION__, __LINE__);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

/*returns true if this device has been previously paired
 * ( calling saveSSID marks this device as paired )*/
IARM_Result_t WiFiNetworkMgr::isPaired(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal=false;
//    WiFiConnectionStatus currSsidInfo;
//    memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

//    get_CurrentSsidInfo(&currSsidInfo);
#ifdef USE_RDK_WIFI_HAL
    retVal=lastConnectedSSID();
#endif

    int ssid_len = strlen(savedWiFiConnList.ssidSession.ssid);

    param->data.isPaired = (ssid_len) ? true : false; /*currSsidInfo.isConnected*/;

    if(param->data.isPaired) {
        RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] This is Paired with \"%s\".\n", __FUNCTION__, __LINE__, savedWiFiConnList.ssidSession.ssid);
    }
    else {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] This is Not Paired with \"%s\".\n", __FUNCTION__, __LINE__, savedWiFiConnList.ssidSession.ssid);
    }
    param->status = true;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

/*
 *  The properties of the currently connected SSID
 */
IARM_Result_t WiFiNetworkMgr::getConnectedSSID(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    param->status = true;
    memset(&param->data.getConnectedSSID, '\0', sizeof(WiFiConnectedSSIDInfo_t));

#ifdef USE_RDK_WIFI_HAL
    getConnectedSSIDInfo(&param->data.getConnectedSSID);
#endif

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}



#ifdef USE_RDK_WIFI_HAL
static void _irEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    /* By default, the WPS shall be handled through XRE,
     * if "DISABLE_WPS_XRE" flag is set to '1' in /etc/netsrvmgr.conf, then
     * it will enable through netsrvmgr, else handled by xre  */
    if(confProp.wifiProps.disableWpsXRE)
    {
        IARM_Bus_IRMgr_EventData_t *irEventData = (IARM_Bus_IRMgr_EventData_t*)data;

        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
        int keyCode = irEventData->data.irkey.keyCode;
        int keyType = irEventData->data.irkey.keyType;
        int isFP = irEventData->data.irkey.isFP;

        {
            if (keyType == KET_KEYDOWN && keyCode == KED_WPS)
            {
                pthread_mutex_lock(&wpsConnLock);
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Received Key info [Type : %d; Code : %d; isFP : %d ] \n", __FUNCTION__, __LINE__, keyType, keyCode, isFP );
                connect_WpsPush();
                pthread_mutex_unlock(&wpsConnLock);
            }
        }

    }
    else {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Disabled WPS event by default. Now, WPS key functionality handled by XRE and disabled in netsrvmgr.\n", __FUNCTION__, __LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}
#endif


IARM_Result_t WiFiNetworkMgr::getRadioProps(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
#ifdef USE_RDK_WIFI_HAL
    unsigned char output_bool[BUFF_MIN] = {'\0'};
    char output_string[BUFF_MAX] = {'\0'};
    int output_INT = 0;
    unsigned long output_ulong = 0;
    int radioIndex=1;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_BUS_WiFi_DiagsPropParam_t *param = (IARM_BUS_WiFi_DiagsPropParam_t *)arg;

    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioEnable(radioIndex,  output_bool) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio enable is %s .\n", __FUNCTION__, __LINE__, output_bool);
//        param->data.radio.params.enable= (output_bool == (unsigned char*)"true" ? true : false);
        param->data.radio.params.enable=*output_bool;
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio enable is %s .\n", __FUNCTION__, __LINE__, param->data.radio.params.enable ? "true" : "false");
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioEnable FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioStatus( radioIndex,  output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio status is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.status,BUFF_MIN,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioStatus FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioIfName( radioIndex,  output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio ifname  is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.name,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioIfName FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioMaxBitRate( radioIndex,  output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] max bit rate  is %s .\n", __FUNCTION__, __LINE__, output_string);
        param->data.radio.params.maxBitRate=atoi(output_string);

    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioMaxBitRate FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioAutoChannelSupported( radioIndex,  output_bool) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] auto channel supported  is %s .\n", __FUNCTION__, __LINE__, output_bool);
        param->data.radio.params.autoChannelSupported=output_bool == (unsigned char*)"true";
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioAutoChannelSupported FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioAutoChannelEnable( radioIndex,  output_bool) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] auto channel enable  is %s .\n", __FUNCTION__, __LINE__, output_bool);
        param->data.radio.params.autoChannelEnable=output_bool == (unsigned char*)"true";
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioAutoChannelEnable FAILURE \n", __FUNCTION__, __LINE__);
    }
    if (wifi_getRadioAutoChannelRefreshPeriod( radioIndex, &output_ulong) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] auto channel refresh period  is %lu .\n", __FUNCTION__, __LINE__, output_ulong);
        param->data.radio.params.autoChannelRefreshPeriod=output_ulong;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioAutoChannelRefreshPeriod FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioExtChannel( radioIndex,  output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio ext channel  is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.extensionChannel,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioExtChannel FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioGuardInterval( radioIndex,  output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio guard is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.guardInterval,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioGuarderval FAILURE \n", __FUNCTION__, __LINE__);
    }
    output_INT = 0;
    if (wifi_getRadioMCS( radioIndex, &output_INT) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio ext channel  is %d .\n", __FUNCTION__, __LINE__, output_INT);
        param->data.radio.params.mcs=output_INT;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioMCS FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioTransmitPowerSupported( radioIndex,  output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio transmit power support  is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.transmitPowerSupported,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioTransmitPowerSupported FAILURE \n", __FUNCTION__, __LINE__);
    }
    output_INT = 0;
    if (wifi_getRadioTransmitPower( radioIndex,  &output_INT) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio transmit power   is %lu .\n", __FUNCTION__, __LINE__, output_ulong);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioTransmitPower FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioIEEE80211hSupported( radioIndex,  output_bool) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] IEEE80211h Supp   is %lu .\n", __FUNCTION__, __LINE__, output_bool);
        //param->data.radio.params.IEEE80211hSupported=*output_bool;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioIEEE80211hSupported FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioIEEE80211hEnabled( radioIndex,  output_bool) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] IEEE80211hEnabled   is %s .\n", __FUNCTION__, __LINE__, output_bool);
        //param->data.radio.params.IEEE80211hEnabled=*output_bool;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioIEEE80211hEnabled FAILURE \n", __FUNCTION__, __LINE__);
    }

    if (wifi_getRadioChannel(radioIndex, &output_ulong) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio channel  is %lu .\n", __FUNCTION__, __LINE__, output_ulong);
        param->data.radio.params.channel=output_ulong;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioChannel FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioChannelsInUse(radioIndex, output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio channels in use  is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.channelsInUse,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioChannelsInUse FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioOperatingChannelBandwidth(radioIndex, output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] operating channel bandwith  is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.operatingChannelBandwidth,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioOperatingChannelBandwidth FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioOperatingFrequencyBand(radioIndex, output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] operating frequency band  is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.operatingFrequencyBand,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioOperatingFrequencyBand FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioStandard(radioIndex, output_string,NULL,NULL,NULL) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio standards  is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.operatingStandards,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioStandard FAILURE \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioPossibleChannels(radioIndex, output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] radio possible channels is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.possibleChannels,BUFF_LENGTH_64,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioPossibleChannels FAILURE \n", __FUNCTION__, __LINE__);
    }
    output_INT = 0;
    if (wifi_getRadioTransmitPower( radioIndex, &output_INT) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Radio TransmitPower %lu .\n", __FUNCTION__, __LINE__, output_INT);
        param->data.radio.params.transmitPower = output_INT;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed to get HAL wifi_getRadioTransmitPower. \n", __FUNCTION__, __LINE__);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRegulatoryDomain(radioIndex, output_string) == RETURN_OK) {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Radio wifi_getRegulatoryDomain is %s .\n", __FUNCTION__, __LINE__, output_string);
        snprintf(param->data.radio.params.regulatoryDomain,BUFF_LENGTH_4,output_string);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed to get wifi_getRegulatoryDomain.\n", __FUNCTION__, __LINE__);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
#endif
    return ret;
}


IARM_Result_t WiFiNetworkMgr::setRadioProps(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::getRadioStatsProps(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t *param = (IARM_BUS_WiFi_DiagsPropParam_t *)arg;
#ifdef USE_RDK_WIFI_HAL
    WiFi_Radio_Stats_Diag_Params params;
    param->status = getRadioStats(&param->data.radio_stats.params);
#endif
    return ret;
}

IARM_Result_t WiFiNetworkMgr::getSSIDProps(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
#ifdef USE_RDK_WIFI_HAL
    unsigned long output_ulong;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_BUS_WiFi_DiagsPropParam_t *param = (IARM_BUS_WiFi_DiagsPropParam_t *)arg;
    WiFiStatusCode_t status = WIFI_DISCONNECTED;
    WiFiConnectionStatus currSsidInfo;
    char output_string[BUFF_MAX];
    int ssidIndex=1;
    memset(output_string,0, BUFF_MAX);

    if(param->numEntry == IARM_BUS_WIFI_MGR_SSIDEntry)
    {
        if(wifi_getSSIDNumberOfEntries(&output_ulong) == RETURN_OK) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID Entries is %lu .\n", __FUNCTION__, __LINE__, output_ulong);
            param->data.ssidNumberOfEntries=(unsigned int)output_ulong;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID Entries param->data.ssidNumberOfEntries is %d .\n", __FUNCTION__, __LINE__, param->data.ssidNumberOfEntries);

        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getSSIDEntries FAILURE \n", __FUNCTION__, __LINE__);
        }
    }
    else if(param->numEntry == IARM_BUS_WIFI_MGR_RadioEntry)
    {
        if(wifi_getRadioNumberOfEntries(&output_ulong) == RETURN_OK) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Radio Entries is %lu .\n", __FUNCTION__, __LINE__, output_ulong);
            param->data.radioNumberOfEntries=(unsigned int)output_ulong;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Radio Entries param->data.radioNumberOfEntries is %d .\n", __FUNCTION__, __LINE__, param->data.ssidNumberOfEntries);

        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getRadioNumberOfEntries FAILURE \n", __FUNCTION__, __LINE__);
        }
    }
    else
    {
        if(wifi_getSSIDName(ssidIndex,output_string) == RETURN_OK) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID Name is %s .\n", __FUNCTION__, __LINE__, output_string);
            snprintf(param->data.ssid.params.name,BUFF_LENGTH_32,output_string);
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID Name is %s .\n", __FUNCTION__, __LINE__, param->data.ssid.params.name);

        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getSSIDName FAILURE \n", __FUNCTION__, __LINE__);
        }

        memset(output_string,0, BUFF_MAX);
        if (wifi_getBaseBSSID(ssidIndex, output_string) == RETURN_OK) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] BSSID  is %s .\n", __FUNCTION__, __LINE__, output_string);
            snprintf(param->data.ssid.params.bssid,BUFF_MAC,output_string);
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] BSSID  is %s .\n", __FUNCTION__, __LINE__, param->data.ssid.params.bssid);
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getBaseBSSID FAILURE \n", __FUNCTION__, __LINE__);
        }
        memset(output_string,0, BUFF_MAX);
        if (wifi_getSSIDMACAddress(ssidIndex, output_string) == RETURN_OK) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID MAC address is %s .\n", __FUNCTION__, __LINE__, output_string);
            snprintf(param->data.ssid.params.macaddr,BUFF_MAC,output_string);
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID MAC address is %s .\n", __FUNCTION__, __LINE__, param->data.ssid.params.macaddr);
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] HAL wifi_getSSIDMACAddress FAILURE \n", __FUNCTION__, __LINE__);
        }
        memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
        get_CurrentSsidInfo(&currSsidInfo);
        memcpy(param->data.ssid.params.ssid, currSsidInfo.ssidSession.ssid, SSID_SIZE);
        if(currSsidInfo.ssidSession.ssid[0] != '\0')
        {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] get current SSID (%s).\n", __FUNCTION__, __LINE__,
                     currSsidInfo.ssidSession.ssid);
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Empty SSID \n", __FUNCTION__, __LINE__);
        }

        status = get_WifiRadioStatus();
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] getPairedSSID SSID (%s).\n", __FUNCTION__, __LINE__,
                 currSsidInfo.ssidSession.ssid);
        if(WIFI_CONNECTED == status )
        {
            param->data.ssid.params.enable = true;
            snprintf(param->data.ssid.params.status,BUFF_MIN,"UP");
        }
        else
        {
            param->data.ssid.params.enable = false;
            snprintf(param->data.ssid.params.status,BUFF_MIN,"DOWN");
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
#endif
    return ret;
}

#ifdef ENABLE_LOST_FOUND
static void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    if (strcmp(owner, IARM_BUS_AUTHSERVICE_NAME)  == 0) {
        switch (eventId) {
        case IARM_BUS_AUTHSERVICE_EVENT_SWITCH_TO_PRIVATE:
        {
            IARM_BUS_AuthService_EventData_t *param = (IARM_BUS_AuthService_EventData_t *)data;
            if (param->value == DEVICE_ACTIVATED)
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Auth service msg  Box Activated \n", __FUNCTION__, __LINE__ );
                bDeviceActivated = true;
            }
        }
        break;
        default:
            break;
        }
    }
    else if (strcmp(owner, IARM_BUS_NM_SRV_MGR_NAME)  == 0) {
        switch (eventId) {
        case IARM_BUS_NETWORK_MANAGER_EVENT_SWITCH_TO_PRIVATE:
        {
            IARM_BUS_NetworkManager_EventData_t *param = (IARM_BUS_NetworkManager_EventData_t *)data;
            if (param->value == DEVICE_ACTIVATED)
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]  Service Manager msg Box Activated \n", __FUNCTION__, __LINE__ );
                bDeviceActivated = true;
            }
        }
        break;
        default:
            break;
        }
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}
#endif

/**
 * @brief This function is used to get the system modes (EAS, NORMAL and WAREHOUSE)
 *
 * @param[in] arg Pointer variable of void.
 *
 * @retval IARM_RESULT_SUCCESS By default it return success.
 *
 */
IARM_Result_t WiFiNetworkMgr::sysModeChange(void *arg)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    IARM_Bus_CommonAPI_SysModeChange_Param_t *param = (IARM_Bus_CommonAPI_SysModeChange_Param_t *)arg;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Sys Mode Change::New mode --> %d, Old mode --> %d\n", __FUNCTION__, __LINE__ ,param->newMode,param->oldMode);
    sysModeParam=param->newMode;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return IARM_RESULT_SUCCESS;
}

/**
 * @brief This function is used to get the EndPoint Details
 *
 * @param[in] arg Pointer variable of void.
 *
 * @retval IARM_RESULT_SUCCESS By default it return success.
 *
 */
IARM_Result_t WiFiNetworkMgr::getEndPointProps(void *arg)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    bool retVal=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_BUS_WiFi_DiagsPropParam_t *param = (IARM_BUS_WiFi_DiagsPropParam_t *)arg;

    param->status = true;
    memset(&param->data.endPointInfo, '\0', sizeof(param->data.endPointInfo));

#ifdef USE_RDK_WIFI_HAL
    getEndPointInfo(&param->data.endPointInfo);
#endif

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return IARM_RESULT_SUCCESS;
}
