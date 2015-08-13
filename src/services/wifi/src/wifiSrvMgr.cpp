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
#include "hostIf_tr69ReqHandler.h"


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

    err = IARM_Bus_Init(IARM_BUS_NM_MGR_NAME);

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
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

//    NM_WiFi_getAvailableSSID_Args *param = (NM_WiFi_getAvailableSSID_Args *)arg;

    /* Query from hostif */
//    unsigned int num_ssids = 0;
//    strcpy(param->ssid, "HOME-E137-2.4");
//    param->security = 1;
//    param->signalstrength = 100;;
//    param->frequency = 150;

    /*    HOSTIF_MsgData_t param = {0};
        strncpy(param.paramName, "Device.WiFi.SSIDNumberOfEntries", strlen("Device.WiFi.SSIDNumberOfEntries")+1)
        param.reqType = HOSTIF_GET;
        param.paramtype = hostIf_UnsignedIntType;
    */
//    getParamInfoFromHostIf();

    /*
     * retCode = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME,
                            IARM_BUS_TR69HOSTIFMGR_API_GetParams,
                            (void *)&param,
                            sizeof(HOSTIF_MsgData_t));
     */
//    num_ssids = (unsigned int) param.paramvalue;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::getCurrentState(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    param->data.wifiStatus = WIFI_UNINSTALLED;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::setEnabled(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
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

IARM_Result_t WiFiNetworkMgr::getPairedSSID(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    /* returns the SSID if one was previously saved through saveSSID, otherwise returns NULL.*/
    if (gCurWiFiConnStatus.ssidSession.ssid[0] != '\0')
    {
        memcpy(param->data.getPairedSSID.ssid, gCurWiFiConnStatus.ssidSession.ssid, SSID_SIZE);
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] getPairedSSID SSID (%s) & Passphrase (%s).\n", __FUNCTION__, __LINE__,
                 gCurWiFiConnStatus.ssidSession.ssid, gCurWiFiConnStatus.ssidSession.passphrase);
        param->status = true;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Empty SSID & Passphrase.\n", __FUNCTION__, __LINE__);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::isPaired(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    /*returns true if this device has been previously paired ( calling saveSSID marks this device as paired )*/
    param->status = gCurWiFiConnStatus.isConnected;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

//---------------------------------------------------------
// generic Api for get HostIf parameters through IARM_TR69Bus
// --------------------------------------------------------
int getParamInfoFromHostIf ()
{
//    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
#if 0
    HOSTIF_MsgData_t param = {0};

    /* Initialize hostIf get structure */
    strncpy(param.paramName, name, strlen(name)+1);
    param.reqType = HOSTIF_GET;

    /* IARM get Call*/
    ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME, IARM_BUS_TR69HOSTIFMGR_API_GetParams, (void *)&param, sizeof(HOSTIF_MsgData_t));

    if(ret != IARM_RESULT_SUCCESS)
    {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%s():%d] Failed in IARM_Bus_Call(), with return value: %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
        return ERR_REQUEST_DENIED;
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] The value of param: %s paramLen : %d\n", __FILE__, __FUNCTION__, __LINE__, name, param.paramLen);
    }
#endif
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return WiFiResult_ok;
}
