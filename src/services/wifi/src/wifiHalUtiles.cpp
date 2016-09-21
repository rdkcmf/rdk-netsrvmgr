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
#ifdef ENABLE_LOST_FOUND
#define WAIT_TIME_FOR_PRIVATE_CONNECTION 2
pthread_cond_t condLAF = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexLAF = PTHREAD_MUTEX_INITIALIZER;
WiFiLNFStatusCode_t gWifiLNFStatus = LNF_UNITIALIZED;
bool bDeviceActivated=false;
bool bLNFConnect=false;
bool bWPSPairing=false;
bool isLAFCurrConnectedssid=false;
#define TOTAL_NO_OF_RETRY 5
#endif
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
static wifiStatusCode_t connCode_prev_state;
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

bool ethernet_on()
{
   if(access( "/tmp/wifi-on", F_OK ) != -1 )
      return false;
   else
      return true;
}

static IARM_Result_t WiFi_IARM_Bus_BroadcastEvent(const char *ownerName, IARM_EventId_t eventId, void *data, size_t len)
{
    if( !ethernet_on() )
       IARM_Bus_BroadcastEvent(ownerName, eventId, data, len);
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
#ifdef ENABLE_LOST_FOUND
WiFiLNFStatusCode_t get_WiFiLNFStatusCode()
{
    return gWifiLNFStatus;
}
#endif
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


    bWPSPairing=true;
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
            wifi_conn_type = WPS_PUSH_CONNECT;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Received WPS KeyCode \"%d\";  Failed in \"wifi_setCliWpsButtonPush(%d)\", WPS Push button press failed with status code (%d) \n",
                     __FUNCTION__, __LINE__, ssidIndex, wpsStatus);

            eventData.data.wifiStateChange.state = WIFI_FAILED;
        }
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
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
                    WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
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
            WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
            if(ret) {
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
    bool notify = false;
    char command[128]= {'\0'};
    const char *connStr = (action == ACTION_ON_CONNECT)?"Connect": "Disconnect";
    memset(&eventData, 0, sizeof(eventData));

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    switch(connCode) {
    case WIFI_HAL_SUCCESS:

        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Successfully %s to AP %s. \n", __FUNCTION__, __LINE__ , connStr, ap_SSID);

        if (ACTION_ON_CONNECT == action) {
            set_WiFiStatusCode(WIFI_CONNECTED);
            notify = true;
#if 0       /* Do not bounce for any network switch. Do it only from LF to private. */
            if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
            {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] refresh xre session failed .\n", __FUNCTION__, __LINE__);
            }
#endif
            /* one condition variable is signaled */

            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, strlen(ap_SSID)+1);
            if(strcmp(savedWiFiConnList.ssidSession.ssid, ap_SSID) != 0)
            {
                RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] restart virtual wifi interface since there is a network change. \n", __FUNCTION__, __LINE__ );
                strcpy(command, "systemctl restart virtual-wifi-iface.service & ");
                system(command);
                RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] restart virtual wifi interface is done \n", __FUNCTION__, __LINE__ );
            }
            else
            {
                RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] previous ssid  %s to current ssid %s. \n", __FUNCTION__, __LINE__ , savedWiFiConnList.ssidSession.ssid, ap_SSID);
            }
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] gLAFssid = %s  ap_SSID = %s \n", __FUNCTION__, __LINE__,gLAFssid,ap_SSID);
            if (strcasecmp(gLAFssid, ap_SSID) != 0 )
            {
                isLAFCurrConnectedssid=false;
                if(strcmp(savedWiFiConnList.ssidSession.ssid, ap_SSID) != 0)
                    storeMfrWifiCredentials();
                memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
                strncpy(savedWiFiConnList.ssidSession.ssid, ap_SSID, strlen(ap_SSID)+1);
                strcpy(savedWiFiConnList.ssidSession.passphrase, " ");
                savedWiFiConnList.conn_type = wifi_conn_type;
            }
            else {
                RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] This is a LNF SSID so no storing \n", __FUNCTION__, __LINE__ );
                isLAFCurrConnectedssid=true;
            }


            /*Write into file*/
//            WiFiConnectionStatus wifiParams;

            /*Generate Event for Connect State*/
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTED;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged' with state as \'CONNECTED\'(%d).\n", __FUNCTION__, __LINE__, WIFI_CONNECTED);
            pthread_mutex_lock(&mutexGo);
            if(0 == pthread_cond_signal(&condGo))
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Broadcast to monitor. \n", __FUNCTION__, __LINE__ );
            pthread_mutex_unlock(&mutexGo);
        } else if (ACTION_ON_DISCONNECT == action) {
            notify = true;
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
        if((connCode_prev_state != connCode) && (!bLNFConnect)) {
            notify = true;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Connecting to AP in Progress... \n", __FUNCTION__, __LINE__ );
            set_WiFiStatusCode(WIFI_CONNECTING);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged' with state as \'CONNECTING\'(%d).\n", __FUNCTION__, __LINE__, WIFI_CONNECTING);
        }
        break;

    case WIFI_HAL_DISCONNECTING:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Disconnecting from AP in Progress... \n", __FUNCTION__, __LINE__ );
        }
        break;
    case WIFI_HAL_ERROR_NOT_FOUND:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., AP not found). \n", __FUNCTION__, __LINE__ , connStr, connCode);
            /* Event Id & Code */
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_NO_SSID;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'NO_SSID\'(%d), CurrentState set as %d (DISCONNECTED)\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code, WIFI_DISCONNECTED);
        }
        break;

    case WIFI_HAL_ERROR_TIMEOUT_EXPIRED:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Timeout expired). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        /* Event Id & Code */
        if(connCode_prev_state != connCode) {
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_UNKNOWN;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'FAILED\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;

    case WIFI_HAL_ERROR_DEV_DISCONNECT:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Fail in Device/AP Disconnect). \n", __FUNCTION__, __LINE__ , connStr, connCode);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_LOST\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;
        /* the SSID of the network changed */
    case WIFI_HAL_ERROR_SSID_CHANGED:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to SSID Change (%d) . \n", __FUNCTION__, __LINE__ , connCode);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_SSID_CHANGED;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            if(confProp.wifiProps.bEnableLostFound)
            {
                bWPSPairing=false;
                lnfConnectPrivCredentials();
            }
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'SSID_CHANGED\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;
        /* the connection to the network was lost */
    case WIFI_HAL_ERROR_CONNECTION_LOST:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to  CONNECTION LOST (%d) from %s. \n", __FUNCTION__, __LINE__ , connCode, ap_SSID);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_LOST\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;
        /* the connection failed for an unknown reason */
    case WIFI_HAL_ERROR_CONNECTION_FAILED:
        if(connCode_prev_state != connCode)
        {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Connection Failed (%d) due to unknown reason. %s. \n", __FUNCTION__, __LINE__ , connCode );
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_FAILED;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_FAILED\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;
        /* the connection was interrupted */
    case WIFI_HAL_ERROR_CONNECTION_INTERRUPTED:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to Connection Interrupted (%d). \n", __FUNCTION__, __LINE__ , connCode );
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_INTERRUPTED;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_INTERRUPTED\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;
        /* the connection failed due to invalid credentials */
    case WIFI_HAL_ERROR_INVALID_CREDENTIALS:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to Invalid Credentials. (%d). \n", __FUNCTION__, __LINE__ , connCode );
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_INVALID_CREDENTIALS;
            set_WiFiStatusCode(WIFI_DISCONNECTED);

            if(confProp.wifiProps.bEnableLostFound)
            {
                bWPSPairing=false;
                lnfConnectPrivCredentials();
            }

            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'INVALID_CREDENTIALS\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;
    case WIFI_HAL_UNRECOVERABLE_ERROR:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to UNRECOVERABLE ERROR. (%d). \n", __FUNCTION__, __LINE__ , connCode );
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_FAILED;
            set_WiFiStatusCode(WIFI_FAILED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onWIFIStateChanged (%d)' with state as \'FAILED\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiStateChange.state);
        }
        break;
    case WIFI_HAL_ERROR_UNKNOWN:
    default:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with WiFi Error Code %d (i.e., WIFI_HAL_ERROR_UNKNOWN). \n", __FUNCTION__, __LINE__, connStr, connCode );
            /* Event Id & Code */
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_UNKNOWN;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Notification on 'onError (%d)' with state as \'UNKNOWN\'(%d).\n", \
                     __FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;
    }

    connCode_prev_state = connCode;
    if(notify && ((eventId >= IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged) && (eventId < IARM_BUS_WIFI_MGR_EVENT_MAX)))
    {
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,
                                (IARM_EventId_t) eventId,
                                (void *)&eventData, sizeof(eventData));
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Broadcast Event id \'%d\'. \n", __FUNCTION__, __LINE__, eventId);
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}

/*Connect using SSID Selection */
bool connect_withSSID(int ssidIndex, char *ap_SSID, SsidSecurity ap_security_mode, CHAR *ap_security_WEPKey, CHAR *ap_security_PreSharedKey, CHAR *ap_security_KeyPassphrase,int saveSSID)
{
    int ret = true;
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    wifiSecurityMode_t securityMode;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    securityMode=(wifiSecurityMode_t)ap_security_mode;

    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR,"[%s:%d] ssidIndex %d; ap_SSID : %s; ap_security_mode : %d; saveSSID = %d  \n",
             __FUNCTION__, __LINE__, ssidIndex, ap_SSID, (int)ap_security_mode, saveSSID );

    ret=wifi_connectEndpoint(ssidIndex, ap_SSID, securityMode, ap_security_WEPKey, ap_security_PreSharedKey, ap_security_KeyPassphrase, saveSSID);

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
        if(!bLNFConnect)
        {
            set_WiFiStatusCode(WIFI_CONNECTING);
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
        }
        ret = true;
    }
    if(!bLNFConnect)
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

bool scan_Neighboring_WifiAP(char *buffer)
{
    bool ret = false;
    INT radioIndex = 0,  index;
    UINT output_array_size = 0;
    wifi_neighbor_ap_t *neighbor_ap_array = NULL;
    char *ssid = NULL;
    int security = 0;
    int signalStrength = 0;
    double frequency = 0;
    SsidSecurity encrptType = NET_WIFI_SECURITY_NONE;
    cJSON *rootObj = NULL, *array_element = NULL, *array_obj = NULL;
    char *out = NULL;
    char *pFreq = NULL;


    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter..\n", __FUNCTION__, __LINE__ );

    ret = wifi_getNeighboringWiFiDiagnosticResult(radioIndex, &neighbor_ap_array, &output_array_size);

    if((RETURN_OK != ret ) && ((NULL == neighbor_ap_array) || (0 == output_array_size)))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] Failed in \'wifi_getNeighboringWiFiDiagnosticResult()\' with NULL \'neighbor_ap_array\', size %d and returns as %d \n",
                 __FUNCTION__, __LINE__, output_array_size, ret);
        return false;
    }
    else {
        ret = true;
    }

    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR,"[%s:%d] \'wifi_getNeighboringWiFiDiagnosticResult()\' comes with \'neighbor_ap_array\', size %d and returns as %d \n",
             __FUNCTION__, __LINE__, output_array_size, ret);

    rootObj = cJSON_CreateObject();

    if(NULL == rootObj) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] \'Failed to create root json object.\n",  __FUNCTION__, __LINE__);
        return false;
    }

    cJSON_AddItemToObject(rootObj, "getAvailableSSIDs", array_obj=cJSON_CreateArray());

    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n*********** Start: SSID Scan List **************** \n");
    for (index = 0; index < output_array_size; index++ )
    {
        char temp[500] = {'\0'};

        ssid = neighbor_ap_array[index].ap_SSID;
        if(ssid[0] != '\0') {
            signalStrength = neighbor_ap_array[index].ap_SignalStrength;
            frequency = strtod(neighbor_ap_array[index].ap_OperatingFrequencyBand, &pFreq);


            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] [%d] => SSID : \"%s\"  | SignalStrength : \"%d\" | frequency : \"%f\" | EncryptionMode : \"%s\" \n",\
                     __FUNCTION__, __LINE__, index, ssid, signalStrength, frequency, neighbor_ap_array[index].ap_EncryptionMode );

            /* The type of encryption the neighboring WiFi SSID advertises.*/
            if(0 == strncasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WEP", sizeof("WEP"))) {
                encrptType = NET_WIFI_SECURITY_WEP_128;
            }
            else if (0 == strncasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA", sizeof("WPA"))) {
                encrptType = NET_WIFI_SECURITY_WPA_PSK_TKIP;
            }
            else if (0 == strncasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA2", sizeof("WPA2"))) {
                encrptType = NET_WIFI_SECURITY_WPA2_PSK_AES;
            }
            else if (0 == strcasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA-WPA2")) {
                encrptType = NET_WIFI_SECURITY_WPA2_PSK_TKIP;
            }
            else if (0 == strcasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA-Enterprise")) {
                encrptType = NET_WIFI_SECURITY_WPA_ENTERPRISE_TKIP;
            }
            else if (0 == strcasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA2-Enterprise")) {
                encrptType = NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES;
            }
            else if (0 == strcasecmp(neighbor_ap_array[index].ap_EncryptionMode, "WPA-WPA2-Enterprise")) {
                encrptType = NET_WIFI_SECURITY_WPA2_ENTERPRISE_TKIP;
            }
            else {
                encrptType = NET_WIFI_SECURITY_NOT_SUPPORTED;
            }

            cJSON_AddItemToArray(array_obj,array_element=cJSON_CreateObject());
            cJSON_AddStringToObject(array_element, "ssid", ssid);
            cJSON_AddNumberToObject(array_element, "security", encrptType);
            cJSON_AddNumberToObject(array_element, "signalStrength", signalStrength);
            cJSON_AddNumberToObject(array_element, "frequency", frequency);
        }
    }
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n***********End: SSID Scan List **************** \n\n");


    out = cJSON_PrintUnformatted(rootObj);

    if(out) {
        strncpy(buffer, out, strlen(out)+1);
    }

    if(rootObj) {
        cJSON_Delete(rootObj);
    }
    if(out) free(out);

    if(neighbor_ap_array) {
        RDK_LOG( RDK_LOG_DEBUG,LOG_NMGR, "[%s:%d] malloc allocated = %d \n", __FUNCTION__, __LINE__ , malloc_usable_size(neighbor_ap_array));
        free(neighbor_ap_array);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}
bool lastConnectedSSID(WiFiConnectionStatus *ConnParams)
{
    char ap_ssid[SSID_SIZE];
    char ap_passphrase[PASSPHRASE_BUFF];
    bool ret = true;
    int retVal;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    retVal=wifi_lastConnected_Endpoint(ap_ssid,ap_passphrase);
    if(retVal != RETURN_OK )
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] Error in getting lost connected SSID \n", __FUNCTION__, __LINE__);
        ret = false;
    }
    else
    {
        strncpy(ConnParams->ssidSession.ssid, ap_ssid, sizeof(ConnParams->ssidSession.ssid)-1);
        ConnParams->ssidSession.ssid[sizeof(ConnParams->ssidSession.ssid)-1]='\0';
        strncpy(ConnParams->ssidSession.passphrase, ap_passphrase, sizeof(ConnParams->ssidSession.passphrase)-1);
        ConnParams->ssidSession.passphrase[sizeof(ConnParams->ssidSession.passphrase)-1]='\0';
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d] last connected  ssid is  %s   \n", __FUNCTION__, __LINE__,ConnParams->ssidSession.ssid);
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

            pthread_mutex_unlock(&mutexGo);

            while(WIFI_CONNECTED == get_WiFiStatusCode()) {
                //RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n *****Start Monitoring ***** \n");
                wifi_getStats(1, NULL);
                //RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n *****End Monitoring  ***** \n");
                sleep(confProp.wifiProps.statsParam_PollInterval);
            }
        }
        else
        {
            pthread_mutex_unlock(&mutexGo);
        }
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


bool clearSSID_On_Disconnect_AP()
{
    bool ret = true;
    int status = 0;
    char *ap_ssid = savedWiFiConnList.ssidSession.ssid;
    if(RETURN_OK != wifi_disconnectEndpoint(1, ap_ssid))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed to  Disconnect in wifi_disconnectEndpoint() for AP : \"%s\".\n",\
                 __FUNCTION__, __LINE__, ap_ssid);
        ret = false;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Successfully called \"wifi_disconnectEndpointd()\" for AP: \'%s\'.  \n",\
                __FUNCTION__, __LINE__, ap_ssid);
        if ( false == isLAFCurrConnectedssid )
        {
            memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
            eraseMfrWifiCredentials();
        }
    }
    return ret;
}

void getConnectedSSIDInfo(WiFiConnectedSSIDInfo_t *conSSIDInfo)
{
    bool ret = true;
    int radioIndex = 1;
    wifi_sta_stats_t stats;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    memset(&stats, '\0', sizeof(stats));

    wifi_getStats(radioIndex, &stats);
    strncpy((char *)conSSIDInfo->ssid, (const char *)stats.sta_SSID, (size_t)BUFF_LENGTH_64);
    strncpy((char *)conSSIDInfo->bssid, (const char *)stats.sta_BSSID, (size_t)BUFF_LENGTH_64);
    conSSIDInfo->rate = stats.sta_PhyRate;
    conSSIDInfo->noise = stats.sta_Noise;
    conSSIDInfo->signalStrength = stats.sta_RSSI;

    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Connected SSID info: \n \
    		[SSID: \"%s\"| BSSID : \"%s\" | PhyRate : \"%f\" | Noise : \"%f\" | SignalStrength(rssi) : \"%f\"] \n",
            __FUNCTION__, __LINE__,
            stats.sta_SSID, stats.sta_BSSID, stats.sta_PhyRate, stats.sta_Noise, stats.sta_RSSI);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}

#endif

#ifdef ENABLE_LOST_FOUND
bool triggerLostFound(LAF_REQUEST_TYPE lafRequestType)
{
    laf_conf_t* conf = NULL;
    laf_client_t* clnt = NULL;
    laf_ops_t *ops = NULL;
    laf_device_info_t *dev_info = NULL;
    int err;
    bool bRet=true;
    mfrSerializedType_t mfrType;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter \n", __FUNCTION__, __LINE__ );
    IARM_Bus_MFRLib_GetSerializedData_Param_t param;
    ops = (laf_ops_t*) malloc(sizeof(laf_ops_t));
    if(!ops)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] laf_ops malloc failed \n", __FUNCTION__, __LINE__ );
        return ENOMEM;
    }
    dev_info = (laf_device_info_t*) malloc(sizeof(laf_device_info_t));
    if(!dev_info)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] laf_device_info malloc failed \n", __FUNCTION__, __LINE__ );
        free(ops);
        return ENOMEM;
    }
    GString* mfrSerialNum = g_string_new(NULL);
    GString* mfrMake = g_string_new(NULL);
    GString* mfrModelName = g_string_new(NULL);
    GString* deviceMacAddr = g_string_new(NULL);
    memset(dev_info,0,sizeof(laf_device_info_t));
    memset(ops,0,sizeof(laf_ops_t));
    /* set operation parameters */
    ops->laf_log_message = log_message;
    ops->laf_wifi_connect = laf_wifi_connect;
    ops->laf_wifi_disconnect = laf_wifi_disconnect;
    /* set device info */
    mfrType = mfrSERIALIZED_TYPE_SERIALNUMBER;
    if(getMfrData(mfrSerialNum,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] mfrSerialNum = %s mfrType = %d \n", __FUNCTION__, __LINE__,mfrSerialNum->str,mfrType );
        strcpy(dev_info->serial_num,mfrSerialNum->str);

    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] getting serial num from mfr failed \n", __FUNCTION__, __LINE__);
    }
    mfrType = mfrSERIALIZED_TYPE_DEVICEMAC;
    if(getMfrData(deviceMacAddr,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] deviceMacAddr = %s mfrType = %d \n", __FUNCTION__, __LINE__,deviceMacAddr->str,mfrType );
        strcpy(dev_info->mac,deviceMacAddr->str);

    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] getting device mac addr from mfr failed \n", __FUNCTION__, __LINE__);
    }
    mfrType = mfrSERIALIZED_TYPE_MODELNAME;
    if(getMfrData(mfrModelName,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] mfrModelName = %s mfrType = %d \n", __FUNCTION__, __LINE__,mfrModelName->str,mfrType );
        strcpy(dev_info->model,mfrModelName->str);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] getting model name from mfr failed \n", __FUNCTION__, __LINE__);
    }
    /* configure laf */
    err = laf_config_new(&conf, ops, dev_info);
    if(err != 0) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Lost and found client configurtion failed with error code %d \n", __FUNCTION__, __LINE__,err );
        bRet=false;
    }
    else
    {
        /* initialize laf client */
        err = laf_client_new(&clnt, conf);
        if(err != 0) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Lost and found client initialization failed with error code %d \n", __FUNCTION__, __LINE__,err );
            bRet=false;
        }
        else
        {
            /* start laf */
            clnt->req_type = lafRequestType;
            err = laf_start(clnt);
            if(err != 0) {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Error in lost and found client, error code %d \n", __FUNCTION__, __LINE__,err );
                bRet=false;
            }
        }
    }
    g_string_free(mfrSerialNum,TRUE);
    g_string_free(mfrMake,TRUE);
    g_string_free(mfrModelName,TRUE);
    g_string_free(deviceMacAddr,TRUE);
    free(dev_info);
    free(ops);
    /* destroy config */
    if(conf)
        laf_config_destroy(conf);
    /* destroy lost and found client */
    if(clnt)
        laf_client_destroy(clnt);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return bRet;
}
bool getmacaddress(gchar* ifname,GString *data)
{
    int fd;
    int ioRet;
    struct ifreq ifr;
    unsigned char *mac;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] ERROR opening socket %d \n", __FUNCTION__, __LINE__,fd );
        return false;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name , ifname , IFNAMSIZ-1);
    ioRet=ioctl(fd, SIOCGIFHWADDR, &ifr);
    if (ioRet < 0)
    {
        close(fd);
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] ERROR in ioctl function to retrieve mac address %d\n", __FUNCTION__, __LINE__,fd );
        return false;
    }
    close(fd);
    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    g_string_printf(data,"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return true;
}

bool getMfrData(GString* mfrDataStr,mfrSerializedType_t mfrType)
{
    bool bRet;
    IARM_Bus_MFRLib_GetSerializedData_Param_t param;
    IARM_Result_t iarmRet = IARM_RESULT_IPCCORE_FAIL;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    memset(&param, 0, sizeof(param));
    param.type = mfrType;
    iarmRet = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_GetSerializedData, &param, sizeof(param));
    if(iarmRet == IARM_RESULT_SUCCESS)
    {
        if(param.buffer && param.bufLen)
        {
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] serialized data %s for mfrtype %d \n", __FUNCTION__, __LINE__,param.buffer,mfrType );
            g_string_assign(mfrDataStr,param.buffer);
            bRet = true;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] serialized data is empty for mfrtype %d \n", __FUNCTION__, __LINE__,mfrType );
            bRet = false;
        }
    }
    else
    {
        bRet = false;
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] IARM CALL failed  for mfrtype %d \n", __FUNCTION__, __LINE__,mfrType );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return bRet;
}


/* Callback function to connect to wifi */
int laf_wifi_connect(laf_wifi_ssid_t* const wificred)
{
    /* call api to connect to wifi */
    int retry = 0;
    bool retVal=false;
    int ssidIndex=1;
    bool notify = false;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] new ssid = (%s) gLAFssid = (%s) is laf current ssid %d \n", __FUNCTION__, __LINE__,wificred->ssid,gLAFssid,isLAFCurrConnectedssid );
    if ((strcmp(wificred->ssid, gLAFssid) == 0) && (true == isLAFCurrConnectedssid) && (WIFI_CONNECTED == get_WifiRadioStatus()))
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Already connected to LF ssid Ignoring the request \n", __FUNCTION__, __LINE__ );
        return 0;
    }
    if(bWPSPairing)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Already WPS flow has intiated so skipping the request the request \n", __FUNCTION__, __LINE__ );
        return 0;
    }
    if (strcmp(wificred->ssid, gLAFssid) == 0)
    {
        bLNFConnect=true;
    }
    else
    {
        bLNFConnect=false;
    }
#ifdef USE_RDK_WIFI_HAL
    if(wificred->psk[0] == '\0')
        retVal=connect_withSSID(ssidIndex, wificred->ssid, NET_WIFI_SECURITY_NONE, NULL, NULL, wificred->passphrase,(int)(!bLNFConnect));
    else
        retVal=connect_withSSID(ssidIndex, wificred->ssid, NET_WIFI_SECURITY_NONE, NULL, wificred->psk, NULL,(int)(!bLNFConnect));
//    retVal=connect_withSSID(ssidIndex, wificred->ssid, NET_WIFI_SECURITY_NONE, NULL, NULL, wificred->psk);
#endif
    if(false == retVal)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] connect with ssid %s passphrase %s failed \n", __FUNCTION__, __LINE__,wificred->ssid,wificred->passphrase );
        return EPERM;
    }
    while(get_WifiRadioStatus() != WIFI_CONNECTED && retry <= 30) {
        retry++; //max wait for 180 seconds
        sleep(1);
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Device not connected to wifi. waiting to connect... \n", __FUNCTION__, __LINE__ );
    }
    if(retry > 30) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Waited for 30seconds. Failed to connect. Abort \n", __FUNCTION__, __LINE__ );
        return EPERM;
    }

    /* Bounce xre session only if switching from LF to private */
    if(strcmp(wificred->ssid, gLAFssid) != 0)
    {
        notify = true;
        sleep(10); //waiting for default route before bouncing the xre connection
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Connecting private ssid. Bouncing xre connection.\n", __FUNCTION__, __LINE__ );
        
        if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] refresh xre session failed .\n", __FUNCTION__, __LINE__);
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return 0;
}

/* Callback function to disconenct wifi */
int laf_wifi_disconnect(void)
{
    int retry = 0;
    bool retVal=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    if ( false == isLAFCurrConnectedssid )
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Connected to private ssid Ignoring the request \n", __FUNCTION__, __LINE__ );
        return 0;
    }
#ifdef USE_RDK_WIFI_HAL
    retVal = clearSSID_On_Disconnect_AP();
#endif
    if(retVal == false) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Tried to disconnect before connect and it failed \n", __FUNCTION__, __LINE__ );
        return EPERM;
    }

    while(get_WifiRadioStatus() != WIFI_DISCONNECTED && retry <= 30) {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Device is connected to wifi. waiting to disconnect... \n", __FUNCTION__, __LINE__ );
        retry++; //max wait for 180 seconds
        sleep(1);
    }
    if(retry > 30) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Waited for 30seconds. Failed to disconnect. Abort \n", __FUNCTION__, __LINE__ );
        return EPERM;
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return 0;
}

/* log level to display */
static rdk_LogLevel map_to_rdkloglevel(int level)
{
    rdk_LogLevel rdklevel;

    switch(level)
    {
    case LAF_LOG_DEBUG:
        rdklevel = RDK_LOG_DEBUG;
        break;
    case LAF_LOG_INFO:
        rdklevel = RDK_LOG_INFO;
        break;
    case LAF_LOG_WARNING:
        rdklevel = RDK_LOG_WARN;
        break;
    case LAF_LOG_ERROR:
        rdklevel = RDK_LOG_ERROR;
        break;
    default:
        rdklevel = RDK_LOG_INFO;
        break;
    }
    return rdklevel;
}
/* callback function to log messages from lost and found */
void log_message(laf_loglevel_t level, char const* function, int line, char const* msg)
{
    RDK_LOG( map_to_rdkloglevel(level), LOG_NMGR, "[%s:%d] %s\n", function, line, msg );
}
void *lafConnPrivThread(void* arg)
{
    int ret = 0;
    int counter=0;
    bool retVal;
    int ssidIndex=1;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    while (true ) {
        pthread_mutex_lock(&mutexLAF);
        if(ret = pthread_cond_wait(&condLAF, &mutexLAF) == 0) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n[%s:%d] Starting the LAF Connect private SSID \n", __FUNCTION__, __LINE__ );
            retVal=false;
            retVal=lastConnectedSSID(&savedWiFiConnList);
            if(retVal == false)
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "\n[%s:%d] Last connected ssid fetch failure \n", __FUNCTION__, __LINE__ );
            gWifiLNFStatus=LNF_IN_PROGRESS;
            pthread_mutex_unlock(&mutexLAF);
            while (gWifiLNFStatus != CONNECTED_PRIVATE)
            {
                if (true == bWPSPairing)
                {
                    bWPSPairing=false;
                    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] WPS pairing pressed coming out of LNF \n", __FUNCTION__, __LINE__ );
                    break;
                }
                if((gWifiAdopterStatus == WIFI_CONNECTED) && (false == isLAFCurrConnectedssid))
                {
                    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] connection status %d is LAF current ssid % \n", __FUNCTION__, __LINE__,gWifiAdopterStatus,isLAFCurrConnectedssid );
                    gWifiLNFStatus=CONNECTED_PRIVATE;
                    break;
                }
                else if (!triggerLostFound(LAF_REQUEST_CONNECT_TO_PRIV_WIFI))
//                else if (!triggerLostFound(LAF_REQUEST_CONNECT_TO_PRIV_WIFI) && counter < TOTAL_NO_OF_RETRY)
                {
                    if(bWPSPairing)
                    {
                        bWPSPairing=false;
                        break;

                    }

//                    counter++;
                    if (savedWiFiConnList.ssidSession.ssid[0] != '\0')
                    {
                        retVal=connect_withSSID(ssidIndex, savedWiFiConnList.ssidSession.ssid, NET_WIFI_SECURITY_NONE, NULL, NULL, savedWiFiConnList.ssidSession.passphrase,true);
                        if(false == retVal)
                        {
                            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] connect with ssid %s  failed \n", __FUNCTION__, __LINE__,savedWiFiConnList.ssidSession.ssid );
                        }
                    }
                    sleep(confProp.wifiProps.lnfRetryInSecs);

                }
                else
                {
                    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] pressed coming out of LNF since box is connected to private \n", __FUNCTION__, __LINE__ );
                    break;
                }
            }
        }
        else
        {
            pthread_mutex_unlock(&mutexLAF);
        }
    }
}
void *lafConnThread(void* arg)
{
    int ret = 0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    gWifiLNFStatus=LNF_IN_PROGRESS;
    while (false == bDeviceActivated) {
        while(gWifiLNFStatus != CONNECTED_LNF)
        {
            if (gWifiAdopterStatus == WIFI_CONNECTED)
            {
                if (false == isLAFCurrConnectedssid)
                {
                    gWifiLNFStatus=CONNECTED_PRIVATE;
                    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Connected through non LAF path\n", __FUNCTION__, __LINE__ );
                }
                break;
            }
            if (true == bWPSPairing)
            {
                bWPSPairing=false;
                return NULL;
            }
            if (false == triggerLostFound(LAF_REQUEST_CONNECT_TO_LFSSID))
            {

                sleep(confProp.wifiProps.lnfRetryInSecs);
                continue;
            }
            else
            {
                gWifiLNFStatus=CONNECTED_LNF;
                RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Connection to LAF success\n", __FUNCTION__, __LINE__ );
                break;
            }
        }
        sleep(1);
//        getDeviceActivationState();
    }
    if(gWifiLNFStatus == CONNECTED_LNF)
    {
        lnfConnectPrivCredentials();
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    pthread_exit(NULL);
}

void connectToLAF()
{
    pthread_t lafConnectThread;
    pthread_attr_t attr;
    bool retVal=false;
    char lfssid[33] = {'\0'};
    if((getDeviceActivationState() == false) && (IARM_BUS_SYS_MODE_WAREHOUSE != sysModeParam))
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&lafConnectThread, &attr, lafConnThread, NULL);
    }
    else
    {
        retVal=lastConnectedSSID(&savedWiFiConnList);
        if (savedWiFiConnList.ssidSession.ssid[0] != '\0')
        {
            sleep(confProp.wifiProps.lnfStartInSecs);
        }

        /* If Device is activated and already connected to Private or any network,
            but defineltly not LF SSID. - No need to trigger LNF*/
        /* Here, 'getDeviceActivationState == true'*/
        laf_get_lfssid(lfssid);
        lastConnectedSSID(&savedWiFiConnList);

        if((strcasecmp(lfssid, savedWiFiConnList.ssidSession.ssid)) &&  (WIFI_CONNECTED == get_WifiRadioStatus()))
        {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Now connected to Private SSID \n", __FUNCTION__, __LINE__ , savedWiFiConnList.ssidSession.ssid);
        }
        else {
            if(IARM_BUS_SYS_MODE_WAREHOUSE != sysModeParam)
            {
                pthread_mutex_lock(&mutexLAF);
                if(0 == pthread_cond_signal(&condLAF))
                {
                    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Signal to start LAF private SSID \n", __FUNCTION__, __LINE__ );
                }
                pthread_mutex_unlock(&mutexLAF);
            }
        }
    }
}


void lafConnectToPrivate()
{
    pthread_t lafConnectToPrivateThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&lafConnectToPrivateThread, &attr, lafConnPrivThread, NULL);
}


bool getLAFssid()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    memset( gLAFssid, 0, SSID_SIZE);
    laf_get_lfssid(gLAFssid);
    if(gLAFssid)
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] lfssid is %s \n", __FUNCTION__, __LINE__ ,gLAFssid);
        return true;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] lfssid is empty %s \n", __FUNCTION__, __LINE__ ,gLAFssid);
        return false;
    }
}
#if 0
bool isLAFCurrConnectedssid()
{
    bool retVal=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    if((strcmp(savedWiFiConnList.ssidSession.ssid, gLAFssid) == 0))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] LAF is the current connected ssid \n", __FUNCTION__, __LINE__ );
        retVal=true;
    }
    else
    {
        retVal=false;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return retVal;

}
#endif
bool getDeviceActivationState()
{
    gchar *deviceID="";
    unsigned int count=0;
    std::string str;
    GString* value=g_string_new(NULL);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    if(! confProp.wifiProps.authServerURL)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Authserver URL is NULL \n", __FUNCTION__, __LINE__ );
        return bDeviceActivated;
    }
    str.assign(confProp.wifiProps.authServerURL,strlen(confProp.wifiProps.authServerURL));
    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Authserver URL is %s %s \n", __FUNCTION__, __LINE__,confProp.wifiProps.authServerURL ,str.c_str());

    while(deviceID && !deviceID[0])
    {
        CurlObject authServiceURL(str);
        deviceID=authServiceURL.getCurlData();
        if ((!deviceID && deviceID[0]) || (count >= 3))
            break;
        sleep(5);
        count++;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] device id string is %s \n", __FUNCTION__, __LINE__ ,deviceID);
    gchar **tokens = g_strsplit_set(deviceID,"{}:,\"", -1);
    guint tokLength = g_strv_length(tokens);
    guint loopvar=0;
    for (loopvar=0; loopvar<tokLength; loopvar++)
    {
        if (g_strrstr(g_strstrip(tokens[loopvar]), "deviceId"))
        {
            //"deviceId": "T00xxxxxxx" so omit 3 tokens ":" fromDeviceId
            if ((loopvar+3) < tokLength )
            {
                g_string_assign(value, g_strstrip(tokens[loopvar+3]));
                if(value->str[0] != '\0')
                {
                    bDeviceActivated = true;
                }
            }
        }
    }
    g_free(deviceID);
    if(tokens)
        g_strfreev(tokens);
    g_string_free(value,TRUE);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return bDeviceActivated;

}
bool isWifiConnected()
{
    if (get_WiFiStatusCode() == WIFI_CONNECTED)
    {
        return true;
    }
    return false;
}

void lnfConnectPrivCredentials()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    pthread_mutex_lock(&mutexLAF);
    if(0 == pthread_cond_signal(&condLAF))
    {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Signal to start LAF private SSID \n", __FUNCTION__, __LINE__ );
    }
    pthread_mutex_unlock(&mutexLAF);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}
#endif
bool setHostifParam (char *name, HostIf_ParamType_t type, void *value)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    bool retValue=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    HOSTIF_MsgData_t param = {0};

    if(NULL == name || value == NULL) {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d]  Null pointer caught for Name/Value \n", __FUNCTION__ ,__LINE__ );
        return retValue;
    }

    /* Initialize hostIf Set structure */
    strncpy(param.paramName, name, strlen(name)+1);
    param.reqType = HOSTIF_SET;

    switch (type) {
    case hostIf_StringType:
    case hostIf_DateTimeType:
        param.paramtype = hostIf_StringType;
        strcpy(param.paramValue, (char *) value);
        break;
        /*    case  hostIf_IntegerType:
            case hostIf_UnsignedIntType:
                put_int(param.paramValue, value);
                param.paramtype = hostIf_IntegerType;*/
    case hostIf_BooleanType:
        put_boolean(param.paramValue,*(bool*)value);
        param.paramtype = hostIf_BooleanType;
        break;
    default:
        break;
    }

    ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME,
                        IARM_BUS_TR69HOSTIFMGR_API_SetParams,
                        (void *)&param,
                        sizeof(param));
    if(ret != IARM_RESULT_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] Error executing Set parameter call from tr69Client, error code \n",__FUNCTION__,__LINE__ );
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG,LOG_NMGR,"[%s:%d] sent iarm message to set hostif parameter\n",__FUNCTION__,__LINE__ );
        retValue=true;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return retValue;
}
void put_boolean(char *ptr, bool val)
{
    bool *tmp = (bool *)ptr;
    *tmp = val;
}

bool storeMfrWifiCredentials(void)
{
    bool retVal=false;
#ifdef USE_RDK_WIFI_HAL
    IARM_BUS_MFRLIB_API_WIFI_Credentials_Param_t param = {0};
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    retVal=lastConnectedSSID(&savedWiFiConnList);
    if(retVal == false)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "\n[%s:%d] Last connected ssid fetch failure \n", __FUNCTION__, __LINE__ );
        return retVal;
    }
    else
    {
        retVal=false;
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%d] fetched ssid details \n",__FUNCTION__,__LINE__ );
    }
    param.requestType=WIFI_GET_CREDENTIALS;
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%d] IARM success in retrieving the stored wifi credentials \n",__FUNCTION__,__LINE__ );
        if((g_strcmp0(param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid) != 0 ) && (g_strcmp0(param.wifiCredentials.cPassword,savedWiFiConnList.ssidSession.passphrase) != 0))
        {
            RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%d] ssid info is different continue to store ssid \n",__FUNCTION__,__LINE__ );
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO,LOG_NMGR,"[%s:%d] Same ssid info not storing it stored ssid %s new ssid %d \n",__FUNCTION__,__LINE__,param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid);
            return true;
        }
    }
    memset(&param,0,sizeof(param));
    param.requestType=WIFI_SET_CREDENTIALS;
    strcpy(param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid);
    strcpy(param.wifiCredentials.cPassword,savedWiFiConnList.ssidSession.passphrase);
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%d] IARM success in storing wifi credentials \n",__FUNCTION__,__LINE__ );
        memset(&param,0,sizeof(param));
        param.requestType=WIFI_GET_CREDENTIALS;
        if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
        {
            RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%d] IARM success in retrieving the stored wifi credentials \n",__FUNCTION__,__LINE__ );
            if((g_strcmp0(param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid) == 0 ) && (g_strcmp0(param.wifiCredentials.cPassword,savedWiFiConnList.ssidSession.passphrase) == 0 ))
            {
                retVal=true;
                RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%d] Successfully stored the credentails and verified stored ssid %s current ssid %s \n",__FUNCTION__,__LINE__,param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] failure in storing  wifi credentials   \n",__FUNCTION__,__LINE__ );
            }

        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] IARM error in retrieving stored wifi credentials mfr error code %d \n",__FUNCTION__,__LINE__,param.returnVal );
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] IARM error in storing wifi credentials mfr error code \n",__FUNCTION__,__LINE__ ,param.returnVal);
    }
#endif
    return retVal;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}


bool eraseMfrWifiCredentials(void)
{
    bool retVal=false;
#ifdef USE_RDK_WIFI_HAL
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_EraseAllData,0,0))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%d] IARM success in erasing wifi credentials \n",__FUNCTION__,__LINE__ );
        retVal=true;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] failure in erasing  wifi credentials \n",__FUNCTION__,__LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
#endif
    return retVal;
}
