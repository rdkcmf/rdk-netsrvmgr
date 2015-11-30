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

#ifdef USE_RDK_WIFI_HAL
static void _irEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
#endif /* USE_RDK_WIFI_HAL */

ssidList gSsidList;

WiFiConnectionStatus gCurWiFiConnStatus;

WiFiNetworkMgr* WiFiNetworkMgr::instance = NULL;
bool WiFiNetworkMgr::instanceIsReady = false;

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

    IARM_Result_t err = IARM_RESULT_IPCCORE_FAIL;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    err = IARM_Bus_Init(IARM_BUS_NM_SRV_MGR_NAME);

    if(IARM_RESULT_SUCCESS != err)
    {
        //LOG("Error initializing IARM.. error code : %d\n",err);
        return err;
    }

    err = IARM_Bus_Connect();

    if(IARM_RESULT_SUCCESS != err)
    {
        //LOG("Error connecting to IARM.. error code : %d\n",err);
        return err;
    }

    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, getAvailableSSIDs);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getCurrentState, getCurrentState);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_setEnabled, setEnabled);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_connect, connect);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getPairedSSID, getPairedSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_saveSSID, saveSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_clearSSID, clearSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_isPaired, isPaired);


#ifdef USE_RDK_WIFI_HAL
    /* Front Panel Event Listner  */
    IARM_Bus_RegisterEventHandler(IARM_BUS_IRMGR_NAME, IARM_BUS_IRMGR_EVENT_IRKEY, _irEventHandler);
#endif

    /* Notification RPC:*/
    IARM_Bus_RegisterEvent(IARM_BUS_WIFI_MGR_EVENT_MAX);

    memset(&gSsidList, '\0', sizeof(ssidList));

#ifdef USE_RDK_WIFI_HAL
    if(wifi_init() == RETURN_OK) {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Successfully wifi_init() done. \n", __FUNCTION__, __LINE__ );
    } else  {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in wifi_init(). \n", __FUNCTION__, __LINE__ );
    }

    /*Register connect and disconnect call back */
    wifi_connectEndpoint_callback_register(wifi_connect_callback);
    wifi_disconnectEndpoint_callback_register(wifi_disconnect_callback);
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
    bool status = false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    param->status = false;

    //const char jsonBuff[] = "\{\"getAvailableSSIDs\"\:\[\{\"ssid\"\:\"xxx\",\"security\"\:1,\"signalStrength\":123,\"frequency\":250\},\{\"ssid\"\:\"xxx\",\"security\":1,\"signalStrength\":123,\"frequency\":250\},\{\"ssid\":\"xxx\",\"security\":1,\"signalStrength\":123,\"frequency\":250\}\]\}";
    char jbuff[MAX_SSIDLIST_BUF] = {'\0'};
    int jBuffLen;

#ifdef USE_HOSTIF_WIFI_HAL
    /* Calling update Wifi list*/
    status = updateWiFiList();
#endif
    if(status == false)
    {
        param->status = false;
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] No SSID connected or SSID available.\n", __FUNCTION__, __LINE__);
        return ret;
    }

    sprintf(jbuff, (const char*)"\{\"getAvailableSSIDs\"\:\[\{\"ssid\"\:\"%s\", \"security\"\:%d,\"signalStrength\":%d,\"frequency\":%f\}\]\}", \
            gSsidList.ssid, gSsidList.security, gSsidList.signalstrength, gSsidList.frequency );

    jBuffLen = strlen(jbuff) +1;

    memcpy(param->data.curSsids.jdata, jbuff, jBuffLen);
    param->data.curSsids.jdataLen = jBuffLen;
    param->status = true;
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID List : [%s].\n", __FUNCTION__, __LINE__, param->data.curSsids.jdata);

    ret = IARM_RESULT_SUCCESS;
    //ssidlist();
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::getCurrentState(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = true;

    param->data.wifiStatus = get_WifiRadioStatus();

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

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

    char *ssid = param->data.connect.ssid;
    short ssid_len = strlen(param->data.connect.ssid);
    char *psk = param->data.connect.passphrase;
    short psk_len = strlen (param->data.connect.passphrase);

    /* If param data receives as Empty, then use the saved SSIDConnection */
    if (!ssid_len)
    {
        if((gCurWiFiConnStatus.ssidSession.ssid[0] != '\0') && (gCurWiFiConnStatus.ssidSession.passphrase[0] != '\0'))
        {
            /*Now try to connect using saved SSID & PSK */
            param->status = true;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed, Empty saved SSID & Passphrase.\n", __FUNCTION__, __LINE__);
            param->status = false;
        }
    }
    /* If param data receives with SSID and PSK, then use these to compare with saved SSIDConnection & connect*/
    else
    {
        if(ssid_len & psk_len)
        {
            /*Connect with Saved SSID */
            if ((0 == strncmp(ssid, gCurWiFiConnStatus.ssidSession.ssid, ssid_len)) &&
                    (0 == strncmp(psk, gCurWiFiConnStatus.ssidSession.passphrase, psk_len)))
            {
                RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Received valid SSID (%s) & Passphrase (%s).\n", __FUNCTION__, __LINE__, ssid, psk);
                gCurWiFiConnStatus.isConnected = true;
                param->status = true;
            }
            else {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed to set due to Invalid SSID (%s) & Passphrase (%s).\n", __FUNCTION__, __LINE__, ssid, psk);
            }
        }
        /* Passphrase can be null when the network security is NONE. */
        else if (ssid_len && (0 == psk_len))
        {
            if ((0 == strncmp(ssid, gCurWiFiConnStatus.ssidSession.ssid, ssid_len)))
            {
                RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Received valid SSID (%s) with Empty Passphrase.\n", __FUNCTION__, __LINE__, ssid);

                gCurWiFiConnStatus.isConnected = true;
                param->status = true;
            }
        }
        else {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Invalid SSID & Passphrase.\n", __FUNCTION__, __LINE__);
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::saveSSID(void* arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

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
        memset(&gCurWiFiConnStatus, 0 ,sizeof(gCurWiFiConnStatus));
        strncpy(gCurWiFiConnStatus.ssidSession.ssid, ssid, ssid_len+1);
        strncpy(gCurWiFiConnStatus.ssidSession.passphrase, psk, psk_len+1);

        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Saving SSID (%s) & Passphrase (%s).\n", __FUNCTION__, __LINE__, ssid, psk);
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
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    param->status = false;

    /* Clears the saved SSID.
    * Returns false if the could not be cleared, and true if the clear operation was successful.*/
    char *ssid = param->data.connect.ssid;
    short ssid_len = strlen(param->data.connect.ssid);
    char *psk = param->data.connect.passphrase;
    short psk_len = strlen (param->data.connect.passphrase);

    if(ssid_len & psk_len)
    {
        /* Clear the Saved SSID */
        //
        memset(&gCurWiFiConnStatus, 0 ,sizeof(gCurWiFiConnStatus));
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Clearing SSID (%s) & Passphrase (%s).\n", __FUNCTION__, __LINE__, ssid, psk);
        param->status = true;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Empty SSID & Passphrase.\n", __FUNCTION__, __LINE__);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}
/* returns the SSID if one was previously saved through saveSSID,
 * otherwise returns NULL.*/
IARM_Result_t WiFiNetworkMgr::getPairedSSID(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    WiFiConnectionStatus currSsidInfo;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    get_CurrentSsidInfo(&currSsidInfo);

    if (currSsidInfo.ssidSession.ssid[0] != '\0')
    {
        memcpy(param->data.getPairedSSID.ssid, currSsidInfo.ssidSession.ssid, SSID_SIZE);
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] getPairedSSID SSID (%s) & Passphrase (%s).\n", __FUNCTION__, __LINE__,
                 currSsidInfo.ssidSession.ssid, currSsidInfo.ssidSession.passphrase);
        param->status = true;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Empty SSID & Passphrase.\n", __FUNCTION__, __LINE__);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

/*returns true if this device has been previously paired
 * ( calling saveSSID marks this device as paired )*/
IARM_Result_t WiFiNetworkMgr::isPaired(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    WiFiConnectionStatus currSsidInfo;
    memset(&currSsidInfo, '\0', sizeof(currSsidInfo));

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    get_CurrentSsidInfo(&currSsidInfo);
    param->status = currSsidInfo.isConnected;

    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] This is \"%s\" with \" %s \" .\'\'\n", __FUNCTION__, __LINE__,\
             ((param->status)?"Paired":"Not Paired"), currSsidInfo.ssidSession.ssid );

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

#ifdef USE_RDK_WIFI_HAL
static void _irEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    IARM_Bus_IRMgr_EventData_t *irEventData = (IARM_Bus_IRMgr_EventData_t*)data;
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    int keyCode = irEventData->data.irkey.keyCode;
    int keyType = irEventData->data.irkey.keyType;
    int isFP = irEventData->data.irkey.isFP;

    {
        if (keyType == KET_KEYDOWN && keyCode == KED_WPS)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Received Key info [Type : %d; Code : %d; isFP : %d ] \n", __FUNCTION__, __LINE__, keyType, keyCode, isFP );
            connect_WpsPush();
        }
    }
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}
#endif
