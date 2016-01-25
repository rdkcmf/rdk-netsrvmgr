/* COMCAST C O N F I D E N T I A L AND PROPRIETARY
 * ============================================================================
 * This file (and its contents) are the intellectual property of Comcast.  It may
 * not be used, copied, distributed or otherwise  disclosed in whole or in part
 * without the express written permission of Comcast.
 * ============================================================================
 * Copyright (c) 2015 Comcast. All rights reserved.
 * ============================================================================
 */

#include "wifiHalUtiles.h"

static WiFiStatusCode_t gWifiAdopterStatus = WIFI_UNINSTALLED;
#ifdef USE_HOSTIF_WIFI_HAL
static WiFiStatusCode_t getWpaStatus();
#endif
static WiFiConnection wifiConnData;

static eConnMethodType wifi_conn_type;

pthread_mutex_t wifiStatusLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condGo = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexGo = PTHREAD_MUTEX_INITIALIZER;
extern netMgrConfigProps confProp;

WiFiStatusCode_t get_WifiRadioStatus();

extern WiFiConnectionStatus savedWiFiConnList;

#ifdef USE_RDK_WIFI_HAL
static void wifi_status_action (wifiStatusCode_t , char *, unsigned short );
#endif

int get_int(const char* ptr)
{
    int *ret = (int *)ptr;
    return *ret;
}

bool get_boolean(const char *ptr)
{
    bool *ret = (bool *)ptr;
    return *ret;
}


static void set_WiFiStatusCode( WiFiStatusCode_t status)
{
    pthread_mutex_lock(&wifiStatusLock);
    gWifiAdopterStatus = status;
    pthread_mutex_unlock(&wifiStatusLock);
}

static WiFiStatusCode_t get_WiFiStatusCode()
{
    return gWifiAdopterStatus;
}

void get_CurrentSsidInfo(WiFiConnectionStatus *curSSIDConnInfo)
{
    strncpy(curSSIDConnInfo->ssidSession.ssid, wifiConnData.ssid, strlen(wifiConnData.ssid)+1);
    curSSIDConnInfo->isConnected = (WIFI_CONNECTED == get_WiFiStatusCode())? true: false;
}

WiFiStatusCode_t get_WifiRadioStatus()
{
    bool ret = false;
    WiFiStatusCode_t status = WIFI_UNINSTALLED;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

#ifdef USE_HOSTIF_WIFI_HAL
    HOSTIF_MsgData_t wifiParam = {0};
    memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));

    strncpy(wifiParam.paramName, WIFI_ADAPTER_ENABLE_PARAM, strlen(WIFI_ADAPTER_ENABLE_PARAM) +1);
    wifiParam.instanceNum = 1;
    wifiParam.paramtype = hostIf_BooleanType;
    wifiParam.reqType = HOSTIF_GET;

    ret = gpvFromTR069hostif(&wifiParam);
    if(ret)
    {
        bool radioStatus = get_boolean(wifiParam.paramValue);

        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Retrieved values from tr69hostif as \'%s\':\'%d\'.\n", __FUNCTION__, __LINE__, WIFI_ADAPTER_ENABLE_PARAM, radioStatus);
        if(true == radioStatus)
        {
            gWifiAdopterStatus = WIFI_CONNECTED;
            /*Check for Wpa status */
            gWifiAdopterStatus = getWpaStatus();
        }
        else
        {
            gWifiAdopterStatus = (WIFI_CONNECTED == gWifiAdopterStatus)?WIFI_DISCONNECTED:WIFI_DISABLED;
        }
    }

    status = gWifiAdopterStatus;
#else
    status = get_WiFiStatusCode();
#endif
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );

    return status;
}

#ifdef USE_HOSTIF_WIFI_HAL
bool updateWiFiList()
{
    bool ret = false;
    HOSTIF_MsgData_t wifiParam = {0};

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
    strncpy(wifiParam.paramName, WIFI_SSID_ENABLE_PARAM, strlen(WIFI_SSID_ENABLE_PARAM) +1);
    wifiParam.instanceNum = 0;
    wifiParam.paramtype = hostIf_BooleanType;

    memset(&gSsidList, '\0', sizeof(ssidList));
    ret = gpvFromTR069hostif(&wifiParam);

    if(ret)
    {
        bool ssidStatus = get_boolean(wifiParam.paramValue);

        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Retrieved values from tr69hostif as \'%s\':\'%d\'.\n", __FUNCTION__, __LINE__, WIFI_SSID_ENABLE_PARAM, ssidStatus);

        if(ssidStatus)
        {
            // Get SSID
            memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
            strncpy(wifiParam.paramName, WIFI_SSID_NAME_PARAM, strlen(WIFI_SSID_NAME_PARAM) +1);
            wifiParam.paramtype = hostIf_StringType;
            wifiParam.instanceNum = 0;
            ret = gpvFromTR069hostif(&wifiParam);

            if(ret)
            {
                strncpy(gSsidList.ssid, wifiParam.paramValue, strlen(wifiParam.paramValue) +1);
            }

            // Get SSID
            memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
            strncpy(wifiParam.paramName, WIFI_SSID_BSSID_PARAM, strlen(WIFI_SSID_BSSID_PARAM) +1);
            wifiParam.paramtype = hostIf_StringType;
            wifiParam.instanceNum = 0;
            ret = gpvFromTR069hostif(&wifiParam);

            if(ret)
            {
                strncpy(gSsidList.bssid,  wifiParam.paramValue, strlen(wifiParam.paramValue) +1);
            }

            /* Not Implemented in tr69 Hal.*/
            gSsidList.frequency = -1;
            gSsidList.signalstrength = -1;


        }
        else
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] WiFi is disabled. [\'%s\':%d] \n", __FUNCTION__, __LINE__, WIFI_SSID_ENABLE_PARAM, ssidStatus);
            ret = false;
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );

    return ret;
}




//---------------------------------------------------------
// generic Api for get HostIf parameters through IARM_TR69Bus
// --------------------------------------------------------

bool gpvFromTR069hostif( HOSTIF_MsgData_t *param)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;

    param->reqType = HOSTIF_GET;
//    HOSTIF_MsgData_t param = &wifiParam;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    /* IARM get Call*/
    ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME, IARM_BUS_TR69HOSTIFMGR_API_GetParams, (void *)param, sizeof(HOSTIF_MsgData_t));

    if(ret != IARM_RESULT_SUCCESS)
    {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%d] Failed with returned \'%d\' for '\%s\''. \n",  __FUNCTION__, __LINE__, ret, param->paramName);
        //ret = ERR_REQUEST_DENIED;
        return false;
    }
    else
    {
        ret = IARM_RESULT_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Successfully returned the value of \'%s\'.\n",  __FUNCTION__, __LINE__, param->paramName);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return true;
}

static WiFiStatusCode_t getWpaStatus()
{
    WiFiStatusCode_t ret = WIFI_UNINSTALLED;
    FILE *in = NULL;
    char buff[512] = {'\0'};

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    if(!(in = popen("wpa_cli status | grep -i wpa_state | cut -d = -f 2", "r")))
    {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%d] Failed to read wpa_cli status.\n", __FUNCTION__, __LINE__ );
        return ret;
    }

    if(fgets(buff, sizeof(buff), in)!=NULL)
    {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d]  State: %s\n", __FUNCTION__, __LINE__, buff);
//		printf("%s", buff);
    }
    else {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%d]  Failed to get State.\n", __FUNCTION__, __LINE__);
        pclose(in);
        return ret;
    }

    if(0 == strncasecmp(buff, "DISCONNECTED", strlen("DISCONNECTED")))
    {
        ret = WIFI_DISCONNECTED;
    }
    else if (0 == strncasecmp(buff, "SCANNING", strlen("SCANNING")))
    {
        ret = WIFI_PAIRING;
    }
    else if (0 == strncasecmp(buff, "ASSOCIATED", strlen("ASSOCIATED")))
    {
        ret = WIFI_CONNECTING;
    }
    else if (0 == strncasecmp(buff, "COMPLETED", strlen("COMPLETED")))
    {
        ret = WIFI_CONNECTED;
    }
    else if (0 == strncasecmp(buff, "INTERFACE_DISABLED", strlen("INTERFACE_DISABLED")))
    {
        ret = WIFI_UNINSTALLED;
    }
    else
    {
        ret = WIFI_FAILED;
    }

    pclose(in);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}
#endif /* #ifdef USE_HOSTIF_WIFI_HAL */

#ifdef  USE_RDK_WIFI_HAL

/*Connect using WPS Push */
bool connect_WpsPush()
{
    bool ret = false;
    INT ssidIndex = 1;
    INT wpsStatus = RETURN_OK;
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    memset(&eventData, 0, sizeof(eventData));

    if(WIFI_CONNECTING == get_WiFiStatusCode()) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Connecting to AP is in Progress..., please try after 120 seconds.\n", __FUNCTION__, __LINE__ );
        return ret;
    }


    if (WIFI_CONNECTED != get_WiFiStatusCode()) {

        wpsStatus = wifi_setCliWpsButtonPush(ssidIndex);

        if(RETURN_OK == wpsStatus)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Received WPS KeyCode \"%d\";  Successfully called \"wifi_setCliWpsButtonPush(%d)\"; WPS Push button Success. \n",\
                     __FUNCTION__, __LINE__, ssidIndex, wpsStatus);
            ret = true;

            /*Generate Event for Connect State*/
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
            set_WiFiStatusCode(WIFI_CONNECTING);
            remove(WIFI_BCK_FILENAME);
            wifi_conn_type = WPS_PUSH_CONNECT;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Received WPS KeyCode \"%d\";  Failed in \"wifi_setCliWpsButtonPush(%d)\", WPS Push button press failed with status code (%d) \n",
                     __FUNCTION__, __LINE__, ssidIndex, wpsStatus);

            eventData.data.wifiStateChange.state = WIFI_FAILED;
        }
        IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
    }
    else if (WIFI_CONNECTED == get_WiFiStatusCode())
    {
        /*If connected, do Disconnect first, the again re-connect to same or different AP.
         * Wait for 60 Sec timeout period if fails to disconnect, then show error.
             * If successfully disconncted, then go ahead and so call connect WpsPush. */
        if(RETURN_OK == wifi_disconnectEndpoint(1, wifiConnData.ssid))
        {
            time_t start_time = time(NULL);
            time_t timeout_period = start_time + confProp.wifiProps.max_timeout/*MAX_TIME_OUT_PERIOD */;

            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Received WPS KeyCode \"%d\"; Already Connected to \"%s\" AP. \"wifi_disconnectEndpointd)\" Successfully on WPS Push. \n",\
                     __FUNCTION__, __LINE__, ssidIndex, wifiConnData.ssid, wpsStatus);

            /* Check for status change from Callback function */
            while (start_time < timeout_period)
            {
                if(WIFI_DISCONNECTED == gWifiAdopterStatus)  {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Successfully received Disconnect state \"%d\". \n", __FUNCTION__, __LINE__, gWifiAdopterStatus);
                    ret = (RETURN_OK == wifi_setCliWpsButtonPush(ssidIndex))?true:false;
                    eventData.data.wifiStateChange.state = (ret)? WIFI_CONNECTING: WIFI_FAILED;
                    set_WiFiStatusCode(eventData.data.wifiStateChange.state);
                    IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged' with state as \'%s\'(%d).\n", __FUNCTION__, __LINE__,
                             (ret?"CONNECTING": "FAILED"), eventData.data.wifiStateChange.state);
                    if(ret) {
                        remove(WIFI_BCK_FILENAME);
                        wifi_conn_type = WPS_PUSH_CONNECT;
                    }
                    break;
                }
                else {
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed to Disconnect \"%s\";  wait for %d sec (i.e., loop time : %s) to update disconnect state by wifi_disconnect_callback. (%d) \n",
                             __FUNCTION__, __LINE__, wifiConnData.ssid, MAX_TIME_OUT_PERIOD, ctime(&start_time), gWifiAdopterStatus);
                    sleep(RETRY_TIME_INTERVAL);
                    start_time = time(NULL);
                }
            }
        } else {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Received WPS KeyCode \"%d\";  Failed in \"wifi_disconnectEndpointd(%d)\", WPS Push button press failed with status code (%d) \n",
                     __FUNCTION__, __LINE__, ssidIndex, wpsStatus);
        }

        if(false == ret)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Since failed to disconnect, so reconnecintg again. \"%d\". \n", __FUNCTION__, __LINE__);
            ret = (RETURN_OK == wifi_setCliWpsButtonPush(ssidIndex))?true:false;
            eventData.data.wifiStateChange.state = (ret)? WIFI_CONNECTING: WIFI_FAILED;
            set_WiFiStatusCode(eventData.data.wifiStateChange.state);
            IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
            if(ret) {
                remove(WIFI_BCK_FILENAME);
                wifi_conn_type = WPS_PUSH_CONNECT;
            }
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}



INT wifi_connect_callback(INT ssidIndex, CHAR *AP_SSID, wifiStatusCode_t *connStatus)
{
    int ret = RETURN_OK;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    wifiStatusCode_t connCode = *connStatus;
    wifi_status_action (connCode, AP_SSID, (unsigned short) ACTION_ON_CONNECT);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}

INT wifi_disconnect_callback(INT ssidIndex, CHAR *AP_SSID, wifiStatusCode_t *connStatus)
{
    int ret = RETURN_OK;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    wifiStatusCode_t connCode = *connStatus;
    wifi_status_action (connCode, AP_SSID, (unsigned short)ACTION_ON_DISCONNECT);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

void wifi_status_action (wifiStatusCode_t connCode, char *ap_SSID, unsigned short action)
{
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_MAX;

    const char *connStr = (action == ACTION_ON_CONNECT)?"Connect": "Disconnect";
    memset(&eventData, 0, sizeof(eventData));

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    switch(connCode) {
    case WIFI_HAL_SUCCESS:

        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Successfully %s to AP %s. \n", __FUNCTION__, __LINE__ , connStr, ap_SSID);

        if (ACTION_ON_CONNECT == action) {
            set_WiFiStatusCode(WIFI_CONNECTED);

            /* one condition variable is signaled */
            pthread_mutex_lock(&mutexGo);
            if(0 == pthread_cond_signal(&condGo))
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Broadcast to monitor. \n", __FUNCTION__, __LINE__ );
            pthread_mutex_unlock(&mutexGo);

            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, strlen(ap_SSID)+1);

            /*Write into file*/
//            WiFiConnectionStatus wifiParams;
            memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
            strncpy(savedWiFiConnList.ssidSession.ssid, ap_SSID, strlen(ap_SSID)+1);
            strcpy(savedWiFiConnList.ssidSession.passphrase, " ");
            savedWiFiConnList.conn_type = wifi_conn_type;
            write_WiFiConnStatusInfo_To_File(&savedWiFiConnList);

            /*Generate Event for Connect State*/
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTED;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged' with state as \'CONNECTED\'(%d).\n", __FUNCTION__, __LINE__, WIFI_CONNECTED);
        } else if (ACTION_ON_DISCONNECT == action) {
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, strlen(ap_SSID)+1);

            /*Generate Event for Disconnect State*/
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_DISCONNECTED;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged' with state as \'DISCONNECTED\'(%d).\n", __FUNCTION__, __LINE__, WIFI_DISCONNECTED);
        }
        break;
    case WIFI_HAL_CONNECTING:
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Connecting to AP in Progress... \n", __FUNCTION__, __LINE__ );
        set_WiFiStatusCode(WIFI_CONNECTING);
        eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
        eventData.data.wifiStateChange.state = WIFI_CONNECTING;
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged' with state as \'CONNECTING\'(%d).\n", __FUNCTION__, __LINE__, WIFI_CONNECTING);
        break;

    case WIFI_HAL_DISCONNECTING:
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Disconnecting from AP in Progress... \n", __FUNCTION__, __LINE__ );
        break;

    case WIFI_HAL_ERROR_NOT_FOUND:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., AP not found). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        /* Event Id & Code */
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_NO_SSID;
        set_WiFiStatusCode(WIFI_FAILED);
        break;

    case WIFI_HAL_ERROR_TIMEOUT_EXPIRED:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Timeout expired). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        /* Event Id & Code */
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_UNKNOWN;
        set_WiFiStatusCode(WIFI_FAILED);
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged' with state as \'FAILED\'(%d).\n", __FUNCTION__, __LINE__, WIFI_FAILED);
        break;

    case WIFI_HAL_ERROR_DEV_DISCONNECT:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Fail in Device/AP Disconnect). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
        break;

        /* the SSID of the network changed */
    case WIFI_HAL_ERROR_SSID_CHANGED:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to SSID Change (%d) . %s. \n", __FUNCTION__, __LINE__ , connCode);
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_SSID_CHANGED;
        break;
        /* the connection to the network was lost */
    case WIFI_HAL_ERROR_CONNECTION_LOST:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to  CONNECTION LOST (%d) from %s. \n", __FUNCTION__, __LINE__ , connCode, ap_SSID);
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
        break;
        /* the connection failed for an unknown reason */
    case WIFI_HAL_ERROR_CONNECTION_FAILED:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Connection Failed (%d) due to unknown reason. %s. \n", __FUNCTION__, __LINE__ , connCode );
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_CONNECTION_FAILED;
        break;
        /* the connection was interrupted */
    case WIFI_HAL_ERROR_CONNECTION_INTERRUPTED:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to Connection Interrupted (%d). \n", __FUNCTION__, __LINE__ , connCode );
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_CONNECTION_INTERRUPTED;
        break;
        /* the connection failed due to invalid credentials */
    case WIFI_HAL_ERROR_INVALID_CREDENTIALS:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to Invalid Credentials. (%d). \n", __FUNCTION__, __LINE__ , connCode );
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_INVALID_CREDENTIALS;
        break;

    case WIFI_HAL_ERROR_UNKNOWN:
    default:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Error Unknown). \n", __FUNCTION__, __LINE__, connStr, connCode );
        /* Event Id & Code */
        eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
        eventData.data.wifiError.code = WIFI_UNKNOWN;
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged' with state as \'FAILED\'(%d).\n", __FUNCTION__, __LINE__, WIFI_FAILED);
        break;
    }

    if((eventId >= IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged) && (eventId < IARM_BUS_WIFI_MGR_EVENT_MAX))
    {
        IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,
                                (IARM_EventId_t) eventId,
                                (void *)&eventData, sizeof(eventData));
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Broadcast Event id \'%d\'. \n", __FUNCTION__, __LINE__, eventId);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}

/*Connect using SSID Selection */
bool connect_withSSID(int ssidIndex, char *ap_SSID, char *ap_security_mode, CHAR *ap_security_WEPKey, CHAR *ap_security_PreSharedKey, CHAR *ap_security_KeyPassphrase)
{
    int ret = true;
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );

    ret=wifi_connectEndpoint(ssidIndex, ap_SSID, ap_security_mode, ap_security_WEPKey, ap_security_PreSharedKey, ap_security_KeyPassphrase);

    if(ret)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] Error in connecting to ssid %s  with passphrase %s \n",
                 __FUNCTION__, __LINE__, ap_SSID, ap_security_KeyPassphrase);
        eventData.data.wifiStateChange.state = WIFI_FAILED;
        ret = false;
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"[%s:%d] connecting to ssid %s  with passphrase %s \n",
                 __FUNCTION__, __LINE__, ap_SSID, ap_security_KeyPassphrase);
        set_WiFiStatusCode(WIFI_CONNECTING);
        eventData.data.wifiStateChange.state = WIFI_CONNECTING;
    }

    IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

bool scan_Neighboring_WifiAP(char *buffer)
{
    bool ret = true;
    INT radioIndex = 0,  index;
    UINT output_array_size = 0;
    wifi_neighbor_ap_t *neighbor_ap_array = NULL;
    char *ssid = NULL;
    int security = 0;
    int signalStrength = 0;
    double frequency = 0;
    SsidSecurity encrptType = NONE;

    cJSON *rootObj = NULL, *array_element = NULL, *array_obj = NULL;
    char *out = NULL;
    char *pFreq = NULL;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter..\n", __FUNCTION__, __LINE__ );

    ret = wifi_getNeighboringWiFiDiagnosticResult(radioIndex, &neighbor_ap_array, &output_array_size);

    if(NULL == neighbor_ap_array || output_array_size)
    {
        return false;
    }

    rootObj = cJSON_CreateObject();

    cJSON_AddItemToObject(rootObj, "getAvailableSSIDs", array_obj=cJSON_CreateArray());

    for (index = 0; index < output_array_size; index++ )
    {
        ssid = neighbor_ap_array[index].ap_SSID;
        signalStrength = neighbor_ap_array[index].ap_SignalStrength;
        frequency = strtod(neighbor_ap_array[index].ap_OperatingFrequencyBand, &pFreq);

        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] SSID : \"%s\"  | SignalStrength : \"%d\" | frequency : \"%f\" | EncryptionMode : \"%s\" \n",
                 __FUNCTION__, __LINE__, ssid, signalStrength, frequency, neighbor_ap_array[index].ap_EncryptionMode );

        /* The type of encryption the neighboring WiFi SSID advertises.*/
        if(0 == strncasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WEP", sizeof("WEP"))) {
            encrptType = WEP;
        }
        else if (0 == strncasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA", sizeof("WPA"))) {
            encrptType = WEP;
        }
        else if (0 == strncasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA2", sizeof("WPA2"))) {
            encrptType = WPA2;
        }
        else if (0 == strcasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA-WPA2")) {
            encrptType = WPA_WPA2;
        }
        else if (0 == strcasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA-Enterprise")) {
            encrptType = WPA_Enterprise;
        }
        else if (0 == strcasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA2-Enterprise")) {
            encrptType = WPA2_Enterprise;
        }
        else if (0 == strcasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA-WPA2-Enterprise")) {
            encrptType = WPA_WPA2_Enterprise;
        }
        else {
            encrptType = NONE;
        }

        cJSON_AddItemToArray(array_obj,array_element=cJSON_CreateObject());
        cJSON_AddStringToObject(array_element, "ssid", ssid);
        cJSON_AddNumberToObject(array_element, "security", encrptType);
        cJSON_AddNumberToObject(array_element, "signalStrength", signalStrength);
        cJSON_AddNumberToObject(array_element, "frequency", frequency);
    }

    out = cJSON_PrintUnformatted(rootObj);
    if(out) {
        strncpy(buffer, out, strlen(out)+1);
    }
    if(rootObj) {
        cJSON_Delete(rootObj);
    }
    if(out) free(out);

    if(neighbor_ap_array) {
        RDK_LOG( RDK_LOG_DEBUG,LOG_NMGR, "[%s:%d] malloc allocated = %d ", __FUNCTION__, __LINE__ , malloc_usable_size(neighbor_ap_array));
        free(neighbor_ap_array);
        RDK_LOG( RDK_LOG_DEBUG,LOG_NMGR, "[%s:%d] malloc allocated = %d ",__FUNCTION__, __LINE__ , malloc_usable_size(neighbor_ap_array));
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;

}


bool write_WiFiConnStatusInfo_To_File(WiFiConnectionStatus *ConnParams)
{
    FILE *wifiFile = NULL;
    cJSON *rootObj = NULL, *childObj = NULL;
    char *outBuffer = NULL;
    int prebuf = 0;
    bool ret = true;
    int jbuffLen = 0;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    rootObj = cJSON_CreateObject();

    if(rootObj) {
        cJSON_AddItemToObject(rootObj, WIFI_CONN_DETAILS, childObj=cJSON_CreateObject());
        cJSON_AddStringToObject(childObj, SSID_STR, ConnParams->ssidSession.ssid);
        cJSON_AddStringToObject(childObj, PSK_STR, ConnParams->ssidSession.passphrase);
        cJSON_AddNumberToObject(childObj, CONN_TYPE, ConnParams->conn_type);

        outBuffer = cJSON_PrintBuffered(rootObj, prebuf, 1);
        jbuffLen = (int)strlen(outBuffer);
        if(outBuffer) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Json Buffer: [\n%s].\n", __FUNCTION__, __LINE__, outBuffer);

            if(0 != mkdir (WIFI_BCK_PATHNAME, 0777))	{
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in mkdir with [%d (%s)].\n", __FUNCTION__, __LINE__, errno, strerror(errno));
                ret = false;
            }
            wifiFile = fopen (WIFI_BCK_FILENAME, "w");
            if(wifiFile) {
                fwrite (outBuffer , sizeof(char), jbuffLen, wifiFile);
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Write into file [%d (%s)].\n", __FUNCTION__, __LINE__, errno, strerror(errno));
                fclose (wifiFile);
            }
            else {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed [%d (%s)] to open file.\n", __FUNCTION__, __LINE__, errno, strerror(errno));
            }
            free(outBuffer);
        }
        cJSON_Delete(rootObj);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

bool read_WiFiConnStatusInfo_From_File(WiFiConnectionStatus *ConnParams)
{
    char *readBuff = NULL;
    FILE *wifiFile = NULL;
    cJSON *request_msg = NULL;
    bool ret = true;
    char filePath[100] = {'\0'};

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    wifiFile = fopen(WIFI_BCK_FILENAME, "r");

    if (NULL == wifiFile) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed [%d (%s)] to open file.\n", __FUNCTION__, __LINE__, errno, strerror(errno));
        return false;
    }
    else {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Successfully to open file.\n", __FUNCTION__, __LINE__);
    }

    if (fseek(wifiFile, 0L, SEEK_END) == 0) {
        long jbuffsize = ftell(wifiFile);
        if (jbuffsize == -1) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed with error code : %d (%s) to get the size of the file.\n", __FUNCTION__, __LINE__, errno, strerror(errno));
            ret = false;
        }
        else {
            readBuff = (char *)malloc(sizeof(char) * (jbuffsize + 1));
            if(NULL == readBuff) {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed with error code : %d (%s) to allocate.\n", __FUNCTION__, __LINE__, errno, strerror(errno));
                ret = false;
            }
        }

        if (ret) {
            if (fseek(wifiFile, 0L, SEEK_SET) == 0) {
                size_t buffLen = fread(readBuff, sizeof(char), jbuffsize, wifiFile);

                if (buffLen == 0) {
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed with errno : %d (%s).\n", __FUNCTION__, __LINE__, errno, strerror(errno));
                    ret = false;
                } else {
                    readBuff[++buffLen] = '\0';
                }
            }
        }
    }
    fclose(wifiFile);

    if(ret) {
        request_msg = cJSON_Parse(readBuff);

        if (!request_msg) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed on cJSON_Parse with cJSON_GetErrorPtr: %s\n", __FUNCTION__, __LINE__, cJSON_GetErrorPtr());
            ret = false;
        }
        else
        {
            char *ssid = NULL;
            char *psk = NULL;
            cJSON *param = NULL;

            param = cJSON_GetObjectItem(request_msg, WIFI_CONN_DETAILS);

            if(param)
            {
                /* Get SSID */
                ssid = cJSON_GetObjectItem(request_msg->child, SSID_STR)->valuestring;
                if(ssid) strncpy(ConnParams->ssidSession.ssid, ssid, strlen(ssid));

                /* Get Password para phrase */
                psk = cJSON_GetObjectItem(request_msg->child, PSK_STR)->valuestring;
                if(psk)	strncpy(ConnParams->ssidSession.passphrase, psk, strlen(psk));

                /* Get connection type as WPS or SSID selection. */
                ConnParams->conn_type = (eConnMethodType)cJSON_GetObjectItem(request_msg->child, CONN_TYPE)->valueint;

                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Successfully reading wifi properties from %s file.\n \
                		\'%s\': %s \n \'%s\' : %s\n \'%s\' : %d\n", __FUNCTION__, __LINE__, WIFI_BCK_FILENAME,
                         SSID_STR, ConnParams->ssidSession.ssid,\
                         PSK_STR, ConnParams->ssidSession.passphrase,\
                         CONN_TYPE, ConnParams->conn_type);
            }
            cJSON_Delete(request_msg);
        }
    }

    if(readBuff) {
        free(readBuff);
        readBuff = NULL;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

#endif

bool isWiFiCapable()
{
    bool isCapable = false;
    unsigned long numRadioEntries = 0;
#ifdef  USE_RDK_WIFI_HAL
    wifi_getRadioNumberOfEntries(&numRadioEntries);
    /* Default set to 1 for Xi5, it will change for other devices.*/
    numRadioEntries = 1;
#endif

    if(numRadioEntries)	{
        gWifiAdopterStatus = WIFI_DISCONNECTED;
        isCapable = true;
    }
    return isCapable;
}

#ifdef USE_RDK_WIFI_HAL

void *wifiConnStatusThread(void* arg)
{
    int ret = 0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    while (true ) {
        pthread_mutex_lock(&mutexGo);

        if(ret = pthread_cond_wait(&condGo, &mutexGo) == 0) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n[%s:%d] ***** Monitor activated by signal ***** \n", __FUNCTION__, __LINE__ );

            while(WIFI_CONNECTED == get_WiFiStatusCode()) {
                //RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n *****Start Monitoring ***** \n");
                wifi_getStats();
                //RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n *****End Monitoring  ***** \n");
                sleep(confProp.wifiProps.statsParam_PollInterval);
            }
        }
        pthread_mutex_unlock(&mutexGo);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}

void monitor_WiFiStatus()
{
    pthread_t wifiStatusMonitorThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&wifiStatusMonitorThread, &attr, wifiConnStatusThread, NULL);
}


#endif

