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

WiFiStatusCode_t get_WifiRadioStatus();

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
