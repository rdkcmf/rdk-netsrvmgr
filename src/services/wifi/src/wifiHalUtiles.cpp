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

static WiFiStatusCode_t gWifiAdopterStatus = WIFI_DISABLED;
static WiFiStatusCode_t getWpaStatus();
static WiFiConnection wifiConnData;

pthread_mutex_t wifiStatusLock = PTHREAD_MUTEX_INITIALIZER;

WiFiStatusCode_t get_WifiRadioStatus();

static void wifi_status_action (wifiStatusCode_t , char *, unsigned short );

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


WiFiStatusCode_t get_WifiRadioStatus()
{
    bool ret = false;
    WiFiStatusCode_t status = WIFI_UNINSTALLED;
    HOSTIF_MsgData_t wifiParam = {0};

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

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
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );

    return status;
}

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
	else{
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


#ifdef  USE_RDK_WIFI_HAL

/*Connect using WPS Push */
bool connect_WpsPush()
{
    bool ret = false;
    INT ssidIndex = 1;
    INT wpsStatus = RETURN_OK;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    if (WIFI_CONNECTED != get_WiFiStatusCode()) {

        wpsStatus = wifi_setCliWpsButtonPush(ssidIndex);

        if(RETURN_OK == wpsStatus)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Received WPS KeyCode \"%d\";  Successfully called \"wifi_setCliWpsButtonPush(%d)\"; WPS Push button Success. \n",\
                     __FUNCTION__, __LINE__, ssidIndex, wpsStatus);
            ret = true;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Received WPS KeyCode \"%d\";  Failed in \"wifi_setCliWpsButtonPush(%d)\", WPS Push button press failed with status code (%d) \n",
                     __FUNCTION__, __LINE__, ssidIndex, wpsStatus);
        }
    }
    else if (WIFI_CONNECTED == get_WiFiStatusCode())
    {
        /*If connected, do Disconnect first, the again re-connect to same or different AP.
         * Wait for 60 Sec timeout period if fails to disconnect, then show error.
             * If successfully disconncted, then go ahead and so call connect WpsPush. */
        if(RETURN_OK == wifi_disconnectEndpoint(1, wifiConnData.ssid))
        {
            time_t start_time = time(NULL);
            time_t timeout_period = start_time + MAX_TIME_OUT_PERIOD;

            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Received WPS KeyCode \"%d\"; Already Connected to \"%s\" AP. \"wifi_disconnectEndpointd)\" Successfully on WPS Push. \n",\
                     __FUNCTION__, __LINE__, ssidIndex, wifiConnData.ssid, wpsStatus);

            /* Check for status change from Callback function */
            while (start_time < timeout_period)
            {
                if(WIFI_DISCONNECTED == gWifiAdopterStatus)  {

                    ret = (RETURN_OK == wifi_setCliWpsButtonPush(ssidIndex))?true:false;
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Successfully received Disconnect state \"%d\". \n", __FUNCTION__, __LINE__, gWifiAdopterStatus);
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
    const char *connStr = (action == ACTION_ON_CONNECT)?"Connect": "Disconnect";

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    switch(connCode) {
    case WIFI_HAL_SUCCESS:
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] %s Successfully to AP %s. \n", __FUNCTION__, __LINE__ , connStr, ap_SSID);
        /*TODO : Action on connect and Disconnect */
        if (ACTION_ON_CONNECT == action) {
	    set_WiFiStatusCode(WIFI_CONNECTED);
            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, strlen(ap_SSID)+1);
        } else if (ACTION_ON_DISCONNECT == action) {
	    set_WiFiStatusCode(WIFI_DISCONNECTED);
            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, strlen(ap_SSID)+1);
        }
        break;
    case WIFI_HAL_ERROR_NOT_FOUND:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., AP not found). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        break;
    case WIFI_HAL_ERROR_NOT_IN_RANGE:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., AP not in Range). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        break;
    case WIFI_HAL_ERROR_TIMEOUT_EXPIRED:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Timeout expired). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        break;
    case WIFI_HAL_ERROR_CON_ACTIVATION:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Fail in connection Activation). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        break;
    case WIFI_HAL_ERROR_CON_DEACTIVATION:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Fail in connection Deactivation). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        break;
    case WIFI_HAL_ERROR_DEV_DISCONNECT:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Fail in Device/AP Disconnect). \n", __FUNCTION__, __LINE__ , connStr, connCode);
        break;
    case WIFI_HAL_ERROR_UNKNOWN:
    default:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in %s with wifiStatusCode %d (i.e., Error Unknown). \n", __FUNCTION__, __LINE__, connStr, connCode );
        break;
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}


#endif

#ifdef WIFI_COMMON_API
int scan_Neighboring_WifiAP(char *buffer)
{
    int ret = 0, i;
    INT radioIndex = 0;
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



    ret = wifi_getNeighboringWiFiDiagnosticResult(radioIndex, &neighbor_ap_array, &output_array_size);

    if(NULL == neighbor_ap_array || output_array_size)
    {
        return -1;
    }

    rootObj = cJSON_CreateObject();

    cJSON_AddItemToObject(rootObj, "getAvailableSSIDs", array_obj=cJSON_CreateArray());

    for (i = 0; i < output_array_size; i++ )
    {
        ssid = neighbor_ap_array[i].ap_SSID;
        signalStrength = neighbor_ap_array[i].ap_SignalStrength;
        frequency = strtod(neighbor_ap_array[i].ap_OperatingFrequencyBand, &pFreq);

        /* The type of encryption the neighboring WiFi SSID advertises.*/
        if(0 == strcasecmp(neighbor_ap_array[i].ap_SecurityModeEnabled, "WEP")) {
            encrptType = WEP;
        }
        else if (0 == strcasecmp(neighbor_ap_array[i].ap_SecurityModeEnabled, "WPA")) {
            encrptType = WEP;
        }
        else if (0 == strcasecmp(neighbor_ap_array[i].ap_SecurityModeEnabled, "WPA2")) {
            encrptType = WPA2;
        }
        else if (0 == strcasecmp(neighbor_ap_array[i].ap_SecurityModeEnabled, "WPA-WPA2")) {
            encrptType = WPA_WPA2;
        }
        else if (0 == strcasecmp(neighbor_ap_array[i].ap_SecurityModeEnabled, "WPA-Enterprise")) {
            encrptType = WPA_Enterprise;
        }
        else if (0 == strcasecmp(neighbor_ap_array[i].ap_SecurityModeEnabled, "WPA2-Enterprise")) {
            encrptType = WPA2_Enterprise;
        }
        else if (0 == strcasecmp(neighbor_ap_array[i].ap_SecurityModeEnabled, "WPA-WPA2-Enterprise")) {
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

    strncpy(buffer, out, strlen(out)+1);

    cJSON_Delete(rootObj);
    free(out);


    return ret;

}

#endif
