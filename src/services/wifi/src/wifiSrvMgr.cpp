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

#include "wifiSrvMgr.h"
#include "NetworkMgrMain.h"
#include "wifiSrvMgrIarmIf.h"
#include "NetworkMedium.h"
#include "wifiHalUtiles.h"
#include "netsrvmgrUtiles.h"

#ifdef USE_HOSTIF_WIFI_HAL
#ifdef ENABLE_IARM
#include "hostIf_tr69ReqHandler.h"
#endif
#endif

#ifndef ENABLE_XCAM_SUPPORT
// added for create_wpa_supplicant_conf_from_netapp_db
#include <string.h>
#include <unistd.h>
#include "sqlite3.h"
#include "cJSON.h"
#include <stdio.h>
#endif // ENABLE_XCAM_SUPPORT

#define SAVE_SSID 1
#define DELAY_LNF_TIMER_COUNT 5  //Each time wait will be 60 seconds
#ifdef SAFEC_RDKV
#include "safec_lib.h"
#else
#define STRCPY_S(dest,size,source)                    \
        strcpy(dest, source);
#endif

#ifdef USE_RDK_WIFI_HAL
#ifdef ENABLE_IARM
static void _irEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
#endif
#endif /* USE_RDK_WIFI_HAL */

#ifdef ENABLE_LOST_FOUND
extern bool bDeviceActivated;
extern WiFiLNFStatusCode_t gWifiLNFStatus;
#ifdef ENABLE_IARM
static void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
#endif
extern bool bStopLNFWhileDisconnected;
extern bool bIsStopLNFWhileDisconnected;
extern bool bAutoSwitchToPrivateEnabled;
extern bool bSwitch2Private;
#endif
extern WiFiStatusCode_t getWpaStatus();
extern void getWpaSsid(WiFiConnectionStatus *curSSIDConnInfo);
bool bStopProgressiveScanning;
ssidList gSsidList;
extern netMgrConfigProps confProp;
WiFiConnectionStatus savedWiFiConnList;
char gWifiMacAddress[MAC_ADDR_BUFF_LEN] = {'\0'};

#ifdef ENABLE_IARM
IARM_Bus_Daemon_SysMode_t sysModeParam;
#endif

#ifdef ENABLE_RTMESSAGE
#include <pthread.h>
pthread_t wifiMsgThread;
#endif

WiFiNetworkMgr* WiFiNetworkMgr::instance = NULL;
bool WiFiNetworkMgr::instanceIsReady = false;
pthread_mutex_t wpsConnLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wifiScanLock = PTHREAD_MUTEX_INITIALIZER;

#ifdef USE_TELEMETRY_2_0
wifi_telemetry_ops_t wifi_telemetry_ops;
#endif // #ifdef USE_TELEMETRY_2_0

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

#ifndef ENABLE_XCAM_SUPPORT
/**
 * 1. Extract "SSID", "BSSID", "Password", "Security" from specified netapp_db_file.
 * 2. Write into specified wpa_supplicant_conf_file in following format:
 *        network={
 *            ssid="<SSID>"
 *            scan_ssid=1
 *            bssid=<BSSID>
 *            psk="<Password"
 *            key_mgmt=<Security>
 *            auth_alg=OPEN
 *            }
 *    example:
 *        network={
 *            ssid="tukken-axb3-5GHz"
 *            scan_ssid=1
 *            bssid=5C:B0:66:00:4D:10
 *            psk="Comcast2015"
 *            key_mgmt=Wpa2PskAes
 *            auth_alg=OPEN
 *            }
 */
int WiFiNetworkMgr::create_wpa_supplicant_conf_from_netapp_db (const char* wpa_supplicant_conf_file, const char* netapp_db_file)
{
    int ret = 1;
    sqlite3 *db = NULL;

    if (-1 != access (wpa_supplicant_conf_file, F_OK))
    {
        LOG_ERR("wpa_supplicant_conf_file [%s] already exists. Will not overwrite.",
                wpa_supplicant_conf_file);
    }
    else if (SQLITE_OK != sqlite3_open_v2 (netapp_db_file, &db, SQLITE_OPEN_READONLY, NULL))
    {
        LOG_ERR("Failed to open netapp_db_file [%s]. Error = [%s]",
                netapp_db_file, sqlite3_errmsg(db));
    }
    else
    {
        char zSql[256];
        sprintf (zSql, "SELECT value FROM EAV where entity = 'NETAPP_DB_IFACE_WIRELESS' and attribute = 'NETAPP_DB_A_CURRENT_APINFO'");
        LOG_INFO("%s: NetApp DB query = [%s]", zSql);

        sqlite3_stmt *res = NULL;
        char *value = NULL;
        cJSON *root_json = NULL;
        FILE *f = NULL;

        if (SQLITE_OK != sqlite3_prepare_v2 (db, zSql, -1, &res, 0))
        {
            LOG_ERR("NetApp DB query prepare statement error = [%s]", sqlite3_errmsg(db));
        }
        else if (SQLITE_ROW != sqlite3_step (res))
        {
            LOG_ERR("NetApp DB query returned no rows.");
        }
        else if (NULL == (value = (char *) sqlite3_column_text (res, 0)))
        {
            LOG_ERR("NetApp DB query returned NULL value.");
        }
        else if (NULL == (root_json = cJSON_Parse (value)))
        {
            LOG_ERR("NetApp DB query returned non-JSON value = [%s]. Error = [%s]",
                    value, cJSON_GetErrorPtr());
        }
        else if (NULL == (f = fopen (wpa_supplicant_conf_file, "w")))
        {
            LOG_ERR("Error opening wpa_supplicant_conf_file [%s] for write",wpa_supplicant_conf_file);
        }
        else
        {
            LOG_TRACE("NetApp DB query returned value = [%s]",value);

            cJSON *json = NULL;
            const char *ssid = (json = cJSON_GetObjectItem (root_json, "SSID")) ? json->valuestring : "";
            const char *bssid = (json = cJSON_GetObjectItem (root_json, "BSSID")) ? json->valuestring : "";
            const char *password = (json = cJSON_GetObjectItem (root_json, "Password")) ? json->valuestring : "";
            const char *security = (json = cJSON_GetObjectItem (root_json, "Security")) ? json->valuestring : "";

            LOG_TRACE("NetApp DB parameters: ssid = [%s], bssid = [%s], password = [%s], security = [%s]",
                    ssid, bssid, password, security);

            if (!*ssid)     LOG_ERR("TELEMETRY_WIFI_CONF_FROM_DB_SSID_EMPTY");
            if (!*bssid)    LOG_ERR("TELEMETRY_WIFI_CONF_FROM_DB_BSSID_EMPTY");
            if (!*password) LOG_ERR("TELEMETRY_WIFI_CONF_FROM_DB_PASSWORD_EMPTY");
            if (!*security) LOG_ERR("TELEMETRY_WIFI_CONF_FROM_DB_MODE_EMPTY");

            const char *security_mode_map[][2] = {
                {"Open", "NONE"},
                {"Wep", "NONE"},
                {"WpaPskAes", "WPA-PSK"},
                {"WpaPskTkip", "WPA-PSK"},
                {"Wpa2PskAes", "WPA-PSK"},
                {"Wpa2PskTkip", "WPA-PSK"},
                {"WpaEnterprise", "WPA-EAP"},
                {"Wpa2Enterprise", "WPA-EAP"},
                {"SAE", "SAE"},
            };

            const char *wpa_security_mode = ""; // default to empty string
            int n = sizeof (security_mode_map) / sizeof (security_mode_map[0]);
            int i = 0;
            for (; i < n; i++)
            {
                if (0 == strcasecmp (security, security_mode_map[i][0]))
                {
                    wpa_security_mode = security_mode_map[i][1];
                    break;
                }
            }
            if (i == n) // security mode mapping not found
            {
                LOG_ERR("TELEMETRY_WIFI_CONF_FROM_DB_MODE_UNMAPPABLE");
            }

            fprintf (f, "ctrl_interface=/var/run/wpa_supplicant\n");
            fprintf (f, "update_config=1\n");
            fprintf (f, "\n");
            fprintf (f, "network={\n");
            fprintf (f, "    ssid=\"%s\"\n", ssid);
            fprintf (f, "    scan_ssid=1\n");
            fprintf (f, "    bssid=%s\n", bssid);
            fprintf (f, "    psk=\"%s\"\n", password);
            fprintf (f, "    key_mgmt=%s\n", wpa_security_mode);
            fprintf (f, "    auth_alg=OPEN\n");
            fprintf (f, "    }\n");

            fclose(f);

            ret = 0;

            LOG_INFO("%s: Successfully created wpa_supplicant_conf_file [%s].",
                    wpa_supplicant_conf_file);
            LOG_INFO("TELEMETRY_WIFI_CREATED_CONF_FROM_DB");
        }

        if (root_json)
        {
            cJSON_Delete (root_json);
        }

        if (res)
        {
            sqlite3_finalize (res);
        }
    }

    if (db)
    {
        sqlite3_close (db);
    }

    remove(netapp_db_file);

    return ret;
}
#endif // #ifndef ENABLE_XCAM_SUPPORT

int WiFiNetworkMgr::removeWifiCredsFromNonSecuredPartition(void)
{
    const char *wpaSupplicantConfFile = "/opt/wifi/wpa_supplicant.conf";
    const char *wifiRoamingFile = "/opt/wifi/wifi_roamingControl.json";
    int retValue = 0;

    if(!access(wpaSupplicantConfFile, F_OK))
    {
        if((retValue = remove(wpaSupplicantConfFile)) == -1)
        {
            LOG_ERR("Failed to remove wpa_supplicant.conf [%s]", wpaSupplicantConfFile);
        }
    }

    if(!access(wifiRoamingFile, F_OK))
    {
        if((retValue = remove(wifiRoamingFile)) == -1)
        {
            LOG_ERR("Failed to remove wifi_roamingControl.json [%s]",
            wifiRoamingFile);
        }
    }

    return retValue;
}

int  WiFiNetworkMgr::Init()
{
    LOG_ENTRY_EXIT;

#ifdef ENABLE_IARM
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, getAvailableSSIDs);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, getAvailableSSIDsWithName);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getAvailableSSIDsAsync, getAvailableSSIDsAsync);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getAvailableSSIDsAsyncIncr, getAvailableSSIDsAsyncIncr);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_stopProgressiveWifiScanning, stopProgressiveWifiScanning);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getCurrentState, getCurrentState);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_setEnabled, setEnabled);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_connect, connect);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_initiateWPSPairing, initiateWPSPairing);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_initiateWPSPairing2, initiateWPSPairing2);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getPairedSSID, getPairedSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getPairedSSIDInfo, getPairedSSIDInfo);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_saveSSID, saveSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_clearSSID, clearSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_disconnectSSID, disconnectSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_isPaired, isPaired);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getConnectedSSID, getConnectedSSID);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_cancelWPSPairing, cancelWPSPairing);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getConnectionType, getCurrentConnectionType);
#ifdef  WIFI_CLIENT_ROAMING
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getRoamingCtrls, getRoamingCtrls);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_setRoamingCtrls, setRoamingCtrls);
#endif
#ifdef ENABLE_LOST_FOUND
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getLNFState, getLNFState);
    IARM_Bus_RegisterEventHandler(IARM_BUS_AUTHSERVICE_NAME, IARM_BUS_AUTHSERVICE_EVENT_SWITCH_TO_PRIVATE, _eventHandler);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SWITCH_TO_PRIVATE, _eventHandler);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_STOP_LNF_WHILE_DISCONNECTED, _eventHandler);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_AUTO_SWITCH_TO_PRIVATE_ENABLED, _eventHandler);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_isStopLNFWhileDisconnected, isStopLNFWhileDisconnected);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_isAutoSwitchToPrivateEnabled, isAutoSwitchToPrivateEnabled);
    IARM_Bus_RegisterCall(IARM_BUS_WIFI_MGR_API_getSwitchToPrivateResults, getSwitchToPrivateResults);
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
#endif // #ifdef ENABLE_IARM

    memset(&gSsidList, '\0', sizeof(ssidList));
    /*Check for WiFi Capability */
    isWiFiCapable();

    /*Get WiFi interface Mac*/
    {
        // get wifi mac address
        char *ifName = getenv("WIFI_INTERFACE");
        LOG_INFO("The interface  use is '%s'", ifName);
        if (netSrvMgrUtiles::getMacAddress_IfName(ifName, gWifiMacAddress)) {
            LOG_INFO("The '%s' Mac Addr :%s", ifName, gWifiMacAddress);
        }
        else {
           LOG_WARN("Failed to get wifi mac address.");
        }
    }

#ifndef ENABLE_XCAM_SUPPORT
    create_wpa_supplicant_conf_from_netapp_db ("/opt/secure/wifi/wpa_supplicant.conf", "/opt/secure/wifi/NetApp.db");
#endif // ENABLE_XCAM_SUPPORT

    /* Remove the WiFi credentials from the non-secured partition */
    removeWifiCredsFromNonSecuredPartition();

#ifdef USE_TELEMETRY_2_0
    wifi_telemetry_ops.init = telemetry_init;
    wifi_telemetry_ops.event_s = telemetry_event_s;
    wifi_telemetry_ops.event_d = telemetry_event_d;
    wifi_telemetry_callback_register (&wifi_telemetry_ops);
    telemetry_init("wifihal");
#endif // #ifdef USE_TELEMETRY_2_0

    return 0;
}

#ifdef USE_RDK_WIFI_HAL
bool WiFiNetworkMgr::setWifiEnabled (bool newState)
{
    static bool bWiFiEnabled = false; // TODO: assumes WiFi is disabled when netsrvmgr starts. correct assumption?

    LOG_ENTRY_EXIT;

    LOG_INFO("WiFi state: current [%d] requested [%d]", bWiFiEnabled, newState);

    if (!bWiFiEnabled == !newState)
    {
        LOG_INFO("Already in requested state. Nothing to do.");
        return true;
    }

    bWiFiEnabled = newState; // change WiFi state
    if (bWiFiEnabled)
    {
        WiFiNetworkMgr::getInstance()->Start();
        LOG_INFO("TELEMETRY_NETWORK_MANAGER_ENABLE_WIFI.");
    }
    else
    {
        WiFiNetworkMgr::getInstance()->Stop();
        LOG_INFO("TELEMETRY_NETWORK_MANAGER_DISABLE_WIFI.");
    }
    return true;
}
#endif // USE_RDK_WIFI_HAL

int  WiFiNetworkMgr::Start()
{
    LOG_ENTRY_EXIT;

    bool retVal=false;

#ifdef ENABLE_RTMESSAGE
    rtConnection_init();
#if defined(XHB1) || defined(XHC3)
    if( 0 != pthread_create(&wifiMsgThread, NULL, &rtMessage_Receive, NULL))
    {
      LOG_ERR("Can't create thread.");
    }
    else
      LOG_INFO("Thread created successfully and waiting for message.");
#endif
#endif

#ifdef USE_RDK_WIFI_HAL
    monitor_WiFiStatus();

    if(wifi_init() == RETURN_OK) {
        LOG_INFO("[%s] Successfully wifi_init() done", MODULE_NAME);
    } else  {
        LOG_ERR("[%s] Failed in wifi_init(). ", MODULE_NAME);
    }

    getHALVersion();
    /*Register connect and disconnect call back */
    wifi_connectEndpoint_callback_register(wifi_connect_callback);
    wifi_disconnectEndpoint_callback_register(wifi_disconnect_callback);
#endif

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
    WiFiConnectionStatus tmpWiFiConnList;
    memset(&tmpWiFiConnList, '\0', sizeof(tmpWiFiConnList));
#ifdef  USE_RDK_WIFI_HAL
    retVal = lastConnectedSSID(&tmpWiFiConnList);
#endif
    if(false == retVal) {
#ifdef ENABLE_IARM
        retVal = connectToMfrWifiCredentials();
        if(true == retVal) {
            LOG_INFO("[%s] Successfully connected to the SSID in the MFR", MODULE_NAME);
        }
        else {
            LOG_INFO("[%s] Failed to connect to the SSID in the MFR", MODULE_NAME);
        }
#endif
    }
#endif // ENABLE_XCAM_SUPPORT

#ifdef ENABLE_LOST_FOUND
    if(confProp.wifiProps.bEnableLostFound)
    {
        if(getLAFssid())
        {
            lafConnectToPrivate();
            
            if(false == isWifiConnected())
            {
#ifdef ENABLE_XCAM_SUPPORT
                int retry = 0;
                while (false == isWifiConnected() && retry < DELAY_LNF_TIMER_COUNT)
                {
                  sleep(60);
                  retry++;
                }
                if (false == isWifiConnected())
                {
                  connectToLAF();
                }
#else
                connectToLAF();
#endif
            }
        }
        else
        {
            LOG_ERR("[%s] lfssid fetch failure !!!!!!! ", MODULE_NAME);
        }
    }
#endif

    return 0;
}

int  WiFiNetworkMgr::Stop()
{
    LOG_ENTRY_EXIT;

    wpsConnLock = PTHREAD_MUTEX_INITIALIZER;
    shutdownWifi();
    #ifdef ENABLE_RTMESSAGE
    #if defined(XHB1) || defined(XHC3)
      pthread_kill(wifiMsgThread, 0);
    #endif
      rtConnection_destroy();
    #endif
    return 0;
 }

#ifdef ENABLE_IARM
IARM_Result_t WiFiNetworkMgr::getAvailableSSIDsWithName(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    bool status = true;
    char SSID[64] = {'\0'};
    double frequency;
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    IARM_Bus_WiFiSrvMgr_SpecificSsidList_Param_t *param = (IARM_Bus_WiFiSrvMgr_SpecificSsidList_Param_t *)arg;

    char jbuff[MAX_SSIDLIST_BUF] = {'\0'};
    int jBuffLen = 0;

#ifdef USE_RDK_WIFI_HAL
    if(param != NULL && param->SSID != NULL)
         strncpy(SSID,param->SSID,63);
    if(param != NULL && param->frequency != NULL)
         frequency = param->frequency;
    status = scan_SpecificSSID_WifiAP(jbuff,(const char*)SSID,frequency);
    jBuffLen = strlen(jbuff);
    LOG_DBG("[%s] Scan AP's SSID list buffer size : \"%d\"", MODULE_NAME, jBuffLen);
#endif
    if(status == false)
    {
        param->status = false;
        LOG_ERR("[%s] No SSID available.", MODULE_NAME);
        return ret;
    }
    LOG_DBG("[%s] json Message length : [%d].", MODULE_NAME, jBuffLen);

    if(jBuffLen)
    {
        strncpy(param->curSsids.jdata, jbuff, sizeof(param->curSsids.jdata));
        param->curSsids.jdataLen = jBuffLen;
        param->status = true;
        ret = IARM_RESULT_SUCCESS;
    }
    else
    {
        param->status = false;
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}

IARM_Result_t WiFiNetworkMgr::getAvailableSSIDs(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    bool status = true;
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    IARM_Bus_WiFiSrvMgr_SsidList_Param_t *param = (IARM_Bus_WiFiSrvMgr_SsidList_Param_t *)arg;

    char jbuff[MAX_SSIDLIST_BUF] = {'\0'};
    int jBuffLen = 0;

#ifdef USE_RDK_WIFI_HAL
    status = scan_Neighboring_WifiAP(jbuff);
    jBuffLen = strlen(jbuff);
    LOG_DBG("[%s] Scan AP's SSID list buffer size : \n\"%d\"", MODULE_NAME, jBuffLen);
#endif
    if(status == false)
    {
        param->status = false;
        LOG_ERR("[%s] No SSID connected or SSID available.", MODULE_NAME);
        return ret;
    }


    LOG_DBG("[%s] json Message length : [%d].", MODULE_NAME, jBuffLen);

    if(jBuffLen) {
        strncpy(param->curSsids.jdata, jbuff, sizeof(param->curSsids.jdata));
        param->curSsids.jdataLen = jBuffLen;
        param->status = true;
        ret = IARM_RESULT_SUCCESS;
    }
    else {
        param->status = false;
    }

//    LOG_DBG("[%s] SSID List : [%s].", MODULE_NAME, param->curSsids.jdata);

    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}

void *getAvailableSSIDsThread(void* arg)
{
   LOG_TRACE("[%s] Enter", MODULE_NAME);

   IARM_Bus_WiFiSrvMgr_SsidList_Param_t param = {0};

   IARM_Result_t ret = WiFiNetworkMgr::getAvailableSSIDs(&param);

   if(ret == IARM_RESULT_SUCCESS)
   {
      LOG_DBG("[%s] SSID List : [%s]", MODULE_NAME, param.curSsids.jdata);

      
      IARM_BUS_WiFiSrvMgr_EventData_t eventData;
      
      strncpy( eventData.data.wifiSSIDList.ssid_list, param.curSsids.jdata, MAX_SSIDLIST_BUF );
      eventData.data.wifiSSIDList.ssid_list[MAX_SSIDLIST_BUF-1] = 0;

      IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,
                                     IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDs,
                                     (void *)&eventData, sizeof(eventData));
   }
   else
       LOG_ERR("[%s] Failed to get Available SSID", MODULE_NAME);

   LOG_TRACE("[%s] Exit", MODULE_NAME);
   return NULL;
}

IARM_Result_t WiFiNetworkMgr::getAvailableSSIDsAsync(void *arg)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    pthread_t getAvailableSSIDsPThread;
    pthread_attr_t attr;
    int rc;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&getAvailableSSIDsPThread, &attr, getAvailableSSIDsThread, NULL);
    if (rc) {
        LOG_ERR("[%s] Thread creation failed with rc = %d\n", rc);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return IARM_RESULT_SUCCESS;
}
/**
 * @brief Get Current time
 *
 * @param[in] Time spec timer
 */
void getCurrentTime(struct timespec *timer)
{
    clock_gettime(CLOCK_REALTIME, timer);
}

void getTimeValDiff(struct timespec *start, struct timespec *stop,
                    struct timespec *timeDiff)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        timeDiff->tv_sec = stop->tv_sec - start->tv_sec - 1;
        timeDiff->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        timeDiff->tv_sec = stop->tv_sec - start->tv_sec;
        timeDiff->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
    return;
}
static IARM_Result_t scanAndBroadcastResults(bool moreData)
{
   IARM_Bus_WiFiSrvMgr_SsidList_Param_t param = {0};
   IARM_Result_t ret = WiFiNetworkMgr::getAvailableSSIDs(&param);
   if(ret == IARM_RESULT_SUCCESS)
   {
      LOG_DBG("[%s] SSID List : [%s]", MODULE_NAME, param.curSsids.jdata);
      IARM_BUS_WiFiSrvMgr_EventData_t eventData; 
      
      strncpy( eventData.data.wifiSSIDList.ssid_list, param.curSsids.jdata, MAX_SSIDLIST_BUF );
      eventData.data.wifiSSIDList.ssid_list[MAX_SSIDLIST_BUF-1] = 0;
      eventData.data.wifiSSIDList.more_data = moreData;

      IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,
                                     IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDsIncr,
                                     (void *)&eventData, sizeof(eventData));
   }
   else
      LOG_ERR("[%s] Failed to broadcast SSID list", MODULE_NAME);
   return ret;
}

void *getAvailableSSIDsIncrThread(void* arg)
{
   LOG_TRACE("[%s] Enter", MODULE_NAME );
   int radioIndex = 0;
   bool more_data = true;
   struct timespec start, end,timeDiff;
   bStopProgressiveScanning = false;
   const char* freq_list = "5785 5180 5220 5240 5805 5745 5200 5500 5825";

   // Scan for High priority 5GHz channels
   LOG_INFO("Starting scan for 5GHz preferred channels...");
   getCurrentTime(&start);
   wifi_setRadioScanningFreqList(radioIndex, freq_list);
   IARM_Result_t ret = scanAndBroadcastResults(more_data);
   if(ret == IARM_RESULT_SUCCESS)
   {
      getCurrentTime(&end);
      getTimeValDiff(&start,&end,&timeDiff);
      LOG_INFO("Successfully broadcasted scan results of 5GHz preferred channels in %ld.%09ld seconds.",(long)timeDiff.tv_sec,timeDiff.tv_nsec);
   }
   else
   {
      LOG_ERR("Failed to broadcast 5GHz preferred scan results");
   }
   pthread_mutex_lock(&wifiScanLock);
   if(bStopProgressiveScanning == true)
   {
      LOG_INFO("Progressive scanning stopped, Skipping further scanning...");
      pthread_mutex_unlock(&wifiScanLock);
      goto exit_scan;
   }
   pthread_mutex_unlock(&wifiScanLock);

   // Scan for 2.4GHz channels based on the dual-band capability
   if(wifi_getDualBandSupport() == true)
   {
      LOG_INFO("Starting scan for 2.4GHz channels...");
      getCurrentTime(&start);
      freq_list = "2412 2417 2422 2427 2432 2437 2442 2447 2452 2457 2462";
      wifi_setRadioScanningFreqList(radioIndex, freq_list);
      ret = scanAndBroadcastResults(more_data);
      if(ret == IARM_RESULT_SUCCESS)
      {
         getCurrentTime(&end);
         getTimeValDiff(&start,&end,&timeDiff);
         LOG_INFO("Successfully broadcasted scan results of 2.4GHz channels in %ld.%09ld seconds.",(long)timeDiff.tv_sec,timeDiff.tv_nsec);
      }
      else
      {
         LOG_ERR("Failed to broadcast 2.4GHz scan results");
      }
   }

   pthread_mutex_lock(&wifiScanLock);
   if(bStopProgressiveScanning == true)
   {
      LOG_INFO("Progressive scanning stopped, Skipping further scanning..");
      pthread_mutex_unlock(&wifiScanLock);
      goto exit_scan;
   }
   pthread_mutex_unlock(&wifiScanLock);

   // Scan for Low priority 5GHz channels
   LOG_INFO("Starting scan for 5GHz low priority channels...\n");
   getCurrentTime(&start);
   freq_list = "5260 5280 5300 5320 5520 5540 5560 5580 5600 5620 5640 5660 5680 5700 5720 5765";
   wifi_setRadioScanningFreqList(radioIndex, freq_list);
   more_data = false;
   ret = scanAndBroadcastResults(more_data);
   if(ret == IARM_RESULT_SUCCESS)
   {
      getCurrentTime(&end);
      getTimeValDiff(&start,&end,&timeDiff);
      LOG_INFO("Successfully broadcasted scan results of 5GHz Non-preferred channels in %ld.%09ld seconds.",(long)timeDiff.tv_sec,timeDiff.tv_nsec);
   }
   else
   {
      LOG_ERR("Failed to broadcast 5GHz Non-preferred scan results");
   }

exit_scan:
   // reset all scan frequency selection
   wifi_setRadioScanningFreqList(radioIndex, (const char*)"0");
   return NULL;
}

IARM_Result_t WiFiNetworkMgr::stopProgressiveWifiScanning(void *arg)
{
   IARM_Result_t ret = IARM_RESULT_SUCCESS;
   pthread_mutex_lock(&wifiScanLock);
   bStopProgressiveScanning = true;
   pthread_mutex_unlock(&wifiScanLock);
   return ret;
}

IARM_Result_t WiFiNetworkMgr::getAvailableSSIDsAsyncIncr(void *arg)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME );

    pthread_t getAvailableSSIDsIncrPThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&getAvailableSSIDsIncrPThread, &attr, getAvailableSSIDsIncrThread, NULL);

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t WiFiNetworkMgr::getCurrentState(void *arg)
{
    LOG_ENTRY_EXIT;

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = true;

    bool enabled;
    if (false == isInterfaceEnabled("WIFI", enabled)) // WIFI interface does not exist
        param->data.wifiStatus = WIFI_UNINSTALLED;
    else if (!enabled)
        param->data.wifiStatus = WIFI_DISABLED;
    else
        param->data.wifiStatus = getWpaStatus();

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t WiFiNetworkMgr::getCurrentConnectionType(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    LOG_TRACE("[%s] Enter", MODULE_NAME );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = true;

    //if the ethernet is plugged in then report disabled
    if( !ethernet_on() )
        param->data.connectionType = get_WifiConnectionType();
    else
        param->data.connectionType = WIFI_CON_UNKNOWN;

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}

#ifdef ENABLE_LOST_FOUND
IARM_Result_t WiFiNetworkMgr::isAutoSwitchToPrivateEnabled(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    LOG_TRACE("[%s] Enter", MODULE_NAME );

    bool *param = (bool *)arg;
    *param=bAutoSwitchToPrivateEnabled;
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::getSwitchToPrivateResults(void *arg)
{
  IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
  bool status = true;
  LOG_TRACE("[%s] Enter", MODULE_NAME );

  IARM_Bus_WiFiSrvMgr_SwitchPrivateResults_Param_t *param = (IARM_Bus_WiFiSrvMgr_SwitchPrivateResults_Param_t *)arg;

  char jbuff[MAX_SSIDLIST_BUF] = {'\0'};
  int jBuffLen = 0;

  status = convertSwitchToPrivateResultsToJson(jbuff);
  jBuffLen = strlen(jbuff);
  LOG_DBG("[%s] Switch Private Results list buffer size : \n\"%d\"", MODULE_NAME,jBuffLen);
  if(status == false)
  {
      param->status = false;
      LOG_ERR("[%s] No Switch private Results.", MODULE_NAME);
      return ret;
  }


  LOG_DBG("[%s] json Message length : [%d].", MODULE_NAME, jBuffLen);

  if(jBuffLen) {
      strncpy(param->switchPvtResults.jdata, jbuff, MAX_SSIDLIST_BUF);
      param->switchPvtResults.jdataLen = jBuffLen;
      param->status = true;
      ret = IARM_RESULT_SUCCESS;
  }
  else {
      param->status = false;
  }
  LOG_DBG("[%s] json Data : %s", MODULE_NAME,param->switchPvtResults.jdata);

//    LOG_DBG("[%s] SSID List : [%s].\n", MODULE_NAME, param->curSsids.jdata);

  LOG_TRACE("[%s] Exit", MODULE_NAME );
  return ret;
}
IARM_Result_t WiFiNetworkMgr::isStopLNFWhileDisconnected(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    LOG_TRACE("[%s] Enter", MODULE_NAME );
    bool *param = (bool *)arg;
    *param=bIsStopLNFWhileDisconnected;
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}
IARM_Result_t WiFiNetworkMgr::getLNFState(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    LOG_TRACE("[%s] Enter", MODULE_NAME );
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = true;
    param->data.wifiLNFStatus = get_WiFiLNFStatusCode();
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}
#endif // ENABLE_LOST_FOUND
IARM_Result_t WiFiNetworkMgr::setEnabled(void *arg)
{
    LOG_ENTRY_EXIT;

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    bool wifiadapterEnableState = param->data.setwifiadapter.enable;

    LOG_INFO("IARM_BUS_WIFI_MGR_API_setEnabled %d", wifiadapterEnableState);

#ifdef USE_RDK_WIFI_HAL
    WiFiNetworkMgr::getInstance()->setWifiEnabled(wifiadapterEnableState);
#endif // USE_RDK_WIFI_HAL

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t WiFiNetworkMgr::connect(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    LOG_TRACE("[%s] Enter", MODULE_NAME );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    param->status = false;
    int ssidIndex = 1;
    SsidSecurity securityMode;
    securityMode = param->data.connect.security_mode;
    char *ssid = param->data.connect.ssid;
    short ssid_len = strlen(param->data.connect.ssid);
    char *pass = param->data.connect.passphrase;
    short pass_len = strlen (param->data.connect.passphrase);
    char *eapIden = param->data.connect.eapIdentity;
    char * carootcert = param->data.connect.carootcert;
    char * clientcert = param->data.connect.clientcert;
    char * privatekey = param->data.connect.privatekey;

    LOG_DBG("[%s] Connect with SSID (%s) & Passphrase (%s) security mode (%d)", MODULE_NAME, ssid, pass,securityMode);
    /* If param data receives as Empty, then use the saved SSIDConnection */
    if (!ssid_len)
    {
        if((savedWiFiConnList.ssidSession.ssid[0] != '\0') && (savedWiFiConnList.ssidSession.passphrase[0] != '\0'))
        {
            /*Now try to connect using saved SSID & PSK */
#ifdef USE_RDK_WIFI_HAL
            connect_withSSID(ssidIndex, savedWiFiConnList.ssidSession.ssid, savedWiFiConnList.ssidSession.security_mode, NULL,savedWiFiConnList.ssidSession.passphrase, savedWiFiConnList.ssidSession.passphrase,SAVE_SSID,eapIden,carootcert,clientcert,privatekey,WIFI_CON_MANUAL);
#endif
            param->status = true;
        }
        else
        {
            LOG_ERR("[%s] Failed, Empty saved SSID & Passphrase.", MODULE_NAME);
            param->status = false;
        }
    }
    /* If param data receives with SSID and Pass, then use these to compare with saved SSIDConnection & connect*/
    else
    {
        if(ssid_len && pass_len)
        {
            /*Connect with Saved SSID */
            LOG_DBG("[%s] Received valid SSID (%s) & Passphrase (%s).", MODULE_NAME, ssid, pass);
#ifdef USE_RDK_WIFI_HAL
            connect_withSSID(ssidIndex, ssid, securityMode, NULL, pass, pass,SAVE_SSID,eapIden,carootcert,clientcert,privatekey,WIFI_CON_MANUAL);
#endif
            param->status = true;
        }
        /* Passphrase can be null when the network security is NONE. */
        else if (ssid_len && (0 == pass_len) && securityMode == NET_WIFI_SECURITY_NONE)
        {
            LOG_DBG("[%s] Received valid SSID (%s) with Empty Passphrase.", MODULE_NAME, ssid);
#ifdef USE_RDK_WIFI_HAL
            connect_withSSID(ssidIndex, ssid, securityMode, NULL,savedWiFiConnList.ssidSession.passphrase , savedWiFiConnList.ssidSession.passphrase,SAVE_SSID,eapIden,carootcert,clientcert,privatekey,WIFI_CON_MANUAL);
#endif
            param->status = true;
        }
        else {
            LOG_ERR("[%s] Invalid SSID & Passphrase.", MODULE_NAME);
        }
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::initiateWPSPairing(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = false;
#ifdef USE_RDK_WIFI_HAL
    pthread_mutex_lock(&wpsConnLock);
    param->status = connect_WpsPush();
    pthread_mutex_unlock(&wpsConnLock);
#endif
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t WiFiNetworkMgr::initiateWPSPairing2(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_Bus_WiFiSrvMgr_WPS_Parameters_t *params = (IARM_Bus_WiFiSrvMgr_WPS_Parameters_t*) arg;
    params->status = false;
#ifdef USE_RDK_WIFI_HAL
    pthread_mutex_lock(&wpsConnLock);
    params->status = params->pbc ? connect_WpsPush() : connect_WpsPin(params->pin);
    pthread_mutex_unlock(&wpsConnLock);
#endif
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t WiFiNetworkMgr::saveSSID(void* arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    LOG_TRACE("[%s] Enter", MODULE_NAME );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    bool retval = false;
    param->status = false;
    SsidSecurity securityMode;
    securityMode = param->data.connect.security_mode;
    char *ssid = param->data.connect.ssid;
    short ssid_len = strlen(param->data.connect.ssid);
    char *psk = param->data.connect.passphrase;
    short psk_len = strlen (param->data.connect.passphrase);

    /* Saves the ssid and passphrase for future sessions.
     * If an SSID was previously saved, the new SSID and passphrase will overwrite the existing one.
     * Returns false if the passphrase was not able to be saved, and true if the save was successful.*/

    if(ssid_len && psk_len)
    {
        memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
        strncpy(savedWiFiConnList.ssidSession.ssid, ssid, ssid_len+1);
        strncpy(savedWiFiConnList.ssidSession.passphrase, psk, psk_len+1);
        savedWiFiConnList.conn_type = SSID_SECLECTION_CONNECT;
        savedWiFiConnList.ssidSession.security_mode = (SsidSecurity)securityMode;
        retval = true;
        LOG_INFO("[%s] %s to file, SSID (%s) & Passphrase (%s) SecurityMode (%d).", MODULE_NAME, retval? "Successfully Saved": "Failed to Save",
           savedWiFiConnList.ssidSession.ssid, savedWiFiConnList.ssidSession.passphrase, savedWiFiConnList.ssidSession.security_mode);
        param->status = retval;
    }
    else
    {
        LOG_ERR("[%s] Empty data for SaveSSID", MODULE_NAME);
        ret = IARM_RESULT_INVALID_PARAM;
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}

IARM_Result_t WiFiNetworkMgr::disconnectSSID(void* arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    param->status = false;
#ifdef USE_RDK_WIFI_HAL
    param->status = disconnectFromCurrentSSID();
#endif
    LOG_TRACE("[%s] Exit", MODULE_NAME );
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

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}
/* returns the SSID if one was previously saved through saveSSID,
 * otherwise returns NULL.*/
IARM_Result_t WiFiNetworkMgr::getPairedSSID(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal = false;
//    WiFiConnectionStatus currSsidInfo;
    WiFiConnectionStatus tmpSavedWiFiConnList;
    memset(&tmpSavedWiFiConnList, '\0', sizeof(tmpSavedWiFiConnList));

    LOG_TRACE("[%s] Enter", MODULE_NAME );

//    memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

//    get_CurrentSsidInfo(&currSsidInfo);

//    if (currSsidInfo.ssidSession.ssid[0] != '\0')
#ifdef USE_RDK_WIFI_HAL
    retVal=lastConnectedSSID(&tmpSavedWiFiConnList);
#endif
    if( retVal == true )
    {
        char *ssid = tmpSavedWiFiConnList.ssidSession.ssid;
        STRCPY_S(param->data.getPairedSSID.ssid, sizeof(param->data.getPairedSSID.ssid), ssid);
        LOG_INFO("[%s] getPairedSSID SSID (%s).", MODULE_NAME, ssid);
        param->status = true;
    }
    else
    {
        LOG_ERR("[%s] Error in getting last ssid ", MODULE_NAME);
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}

/**
 *   Convert Security mode to String based on the Broadband specification
 *   Supported Security modes are
 *          None
 *          WEP-64
 *          WEP-128
 *          WPA-Personal
 *          WPA2-Personal
 *          WPA-WPA2-Personal
 *          WPA-Enterprise
 *          WPA2-Enterprise
 *          WPA-WPA2-Enterprise
 */
bool convertSecurityModeToString(char* securityModeStr,SsidSecurity sec_mode)
{
   bool ret = true;

   if(!securityModeStr)
   {
      LOG_ERR("securityModeStr is NULL, Failed to get Security mode string.");
      ret = false;
   }
   else
   {
      switch(sec_mode)
      {
         case NET_WIFI_SECURITY_WPA2_PSK_AES:
            strncpy(securityModeStr,"WPA2-Personal",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA2_PSK_TKIP:
            strncpy(securityModeStr,"WPA2-Personal",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA_PSK_AES:
            strncpy(securityModeStr,"WPA-Personal",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA_PSK_TKIP:
            strncpy(securityModeStr,"WPA-Personal",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES:
            strncpy(securityModeStr,"WPA2-Enterprise",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA2_ENTERPRISE_TKIP:
            strncpy(securityModeStr,"WPA2-Enterprise",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA_ENTERPRISE_AES:
            strncpy(securityModeStr,"WPA-Enterprise",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA_ENTERPRISE_TKIP:
            strncpy(securityModeStr,"WPA-Enterprise",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WEP_64:
            strncpy(securityModeStr,"WEP-64",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WEP_128:
            strncpy(securityModeStr,"WEP-128",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA3_PSK_AES:
            strncpy(securityModeStr,"WPA2-WPA3",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA3_SAE:
            strncpy(securityModeStr,"WPA3",BUFF_LENGTH_32-1);
            break;
         case NET_WIFI_SECURITY_WPA_WPA2_PSK:
	    strncpy(securityModeStr,"WPA-WPA2-Personal",BUFF_LENGTH_32-1);
	    break;
	 case NET_WIFI_SECURITY_WPA_WPA2_ENTERPRISE:
	    strncpy(securityModeStr,"WPA-WPA2-Enterprise",BUFF_LENGTH_32-1);
	    break;
	 case NET_WIFI_SECURITY_NONE:
            strncpy(securityModeStr,"None",BUFF_LENGTH_32-1);
            break;
         default:
            strncpy(securityModeStr,"None",BUFF_LENGTH_32-1);
            break;
      }
   }
   return ret;
}

IARM_Result_t WiFiNetworkMgr::getPairedSSIDInfo(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    bool retVal = false;
    char securityModeString[BUFF_LENGTH_32];
    WiFiConnectionStatus tmpSavedWiFiConnList;
    memset(&tmpSavedWiFiConnList, '\0', sizeof(tmpSavedWiFiConnList));
    LOG_TRACE("[%s] Enter", MODULE_NAME );
    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;
    memset(&param->data.getPairedSSIDInfo, '\0', sizeof(WiFiPairedSSIDInfo_t));
#ifdef USE_RDK_WIFI_HAL
    retVal=lastConnectedSSID(&tmpSavedWiFiConnList);
#endif
    if( retVal == true )
    {
        char *ssid = tmpSavedWiFiConnList.ssidSession.ssid;
        STRCPY_S(param->data.getPairedSSIDInfo.ssid, sizeof(param->data.getPairedSSIDInfo.ssid), ssid);
        char *bssid = tmpSavedWiFiConnList.ssidSession.bssid;
        STRCPY_S(param->data.getPairedSSIDInfo.bssid, sizeof(param->data.getPairedSSIDInfo.bssid), bssid);
        memset(securityModeString,0,BUFF_LENGTH_32);
        convertSecurityModeToString(securityModeString,tmpSavedWiFiConnList.ssidSession.security_mode);
        strncpy(param->data.getPairedSSIDInfo.security,securityModeString,BUFF_LENGTH_32-1);
        LOG_INFO("[%s] getPairedSSIDInfo SSID (%s) : BSSID (%s).", MODULE_NAME, ssid, bssid);
        param->status = true;
        ret = IARM_RESULT_SUCCESS;
    }
    else
    {
        LOG_ERR("[%s] Error in getting last ssid .", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}

/*returns true if this device has been previously paired
 * ( calling saveSSID marks this device as paired )*/
IARM_Result_t WiFiNetworkMgr::isPaired(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal=false;
    WiFiConnectionStatus tmpSavedWiFiConnList;
    memset(&tmpSavedWiFiConnList, '\0', sizeof(tmpSavedWiFiConnList));
//    WiFiConnectionStatus currSsidInfo;
//    memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

//    get_CurrentSsidInfo(&currSsidInfo);
#ifdef USE_RDK_WIFI_HAL
    retVal=lastConnectedSSID(&tmpSavedWiFiConnList);
#endif

    int ssid_len = strlen(tmpSavedWiFiConnList.ssidSession.ssid);

    param->data.isPaired = (ssid_len) ? true : false; /*currSsidInfo.isConnected*/;

    if(param->data.isPaired) {
        LOG_TRACE("[%s] This is Paired with \"%s\".", MODULE_NAME, tmpSavedWiFiConnList.ssidSession.ssid);
    }
    else {
        LOG_ERR("[%s] This is Not Paired with \"%s\".", MODULE_NAME, tmpSavedWiFiConnList.ssidSession.ssid);
    }
    param->status = true;

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}

/*
 *  The properties of the currently connected SSID
 */
IARM_Result_t WiFiNetworkMgr::getConnectedSSID(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal=false;
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

    param->status = true;
    memset(&param->data.getConnectedSSID, '\0', sizeof(WiFiConnectedSSIDInfo_t));

#ifdef USE_RDK_WIFI_HAL
    getConnectedSSIDInfo(&param->data.getConnectedSSID);
#endif

    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}

/*
 *  Cancel WPS pairing operation
 */
IARM_Result_t WiFiNetworkMgr::cancelWPSPairing (void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal=false;
    LOG_TRACE("[%s] Enter", MODULE_NAME );

    IARM_Bus_WiFiSrvMgr_Param_t *param = (IARM_Bus_WiFiSrvMgr_Param_t *)arg;

#ifdef USE_RDK_WIFI_HAL
    retVal = cancelWPSPairingOperation();
#endif
    param->status = retVal;

    LOG_TRACE("[%s] Exit", MODULE_NAME );
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

        LOG_DBG("[%s] Enter", MODULE_NAME);
        int keyCode = irEventData->data.irkey.keyCode;
        int keyType = irEventData->data.irkey.keyType;
        int isFP = irEventData->data.irkey.isFP;

        {
            if (keyType == KET_KEYDOWN && keyCode == KED_WPS)
            {
                pthread_mutex_lock(&wpsConnLock);
                LOG_INFO("[%s] Received Key info [Type : %d; Code : %d; isFP : %d ]", MODULE_NAME, keyType, keyCode, isFP );
                connect_WpsPush();
                pthread_mutex_unlock(&wpsConnLock);
            }
        }

    }
    else {
        LOG_DBG("[%s] Disabled WPS event by default. Now, WPS key functionality handled by XRE and disabled in netsrvmgr.", MODULE_NAME );
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
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
    int radioIndex=0;
    char freqBand[BUFF_MIN] = {'\0'};

    LOG_TRACE("[%s] Enter", MODULE_NAME );

    IARM_BUS_WiFi_DiagsPropParam_t *param = (IARM_BUS_WiFi_DiagsPropParam_t *)arg;

    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioEnable(radioIndex,  output_bool) == RETURN_OK) {
        LOG_DBG("[%s] radio enable is %s .", MODULE_NAME, output_bool);
//        param->data.radio.params.enable= (output_bool == (unsigned char*)"true" ? true : false);
        param->data.radio.params.enable=*output_bool;
        LOG_DBG("[%s] radio enable is %s .", MODULE_NAME, param->data.radio.params.enable ? "true" : "false");
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioEnable FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioStatus( radioIndex,  output_string) == RETURN_OK) {
        LOG_DBG("[%s] radio status is %s ", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.status,BUFF_MIN,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioStatus FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioIfName( radioIndex,  output_string) == RETURN_OK) {
        LOG_DBG("[%s] radio ifname  is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.name,BUFF_LENGTH_24,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioIfName FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioMaxBitRate( radioIndex,  output_string) == RETURN_OK) {
        LOG_DBG("[%s] max bit rate  is %s ", MODULE_NAME, output_string);
        param->data.radio.params.maxBitRate=atoi(output_string);

    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioMaxBitRate FAILURE ", MODULE_NAME);
    }
    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioAutoChannelSupported( radioIndex,  output_bool) == RETURN_OK) {
        LOG_DBG("[%s] auto channel supported  is %s .", MODULE_NAME, output_bool);
        param->data.radio.params.autoChannelSupported=output_bool == (unsigned char*)"true";
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioAutoChannelSupported FAILURE ", MODULE_NAME);
    }
    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioAutoChannelEnable( radioIndex,  output_bool) == RETURN_OK) {
        LOG_DBG("[%s] auto channel enable  is %s ", MODULE_NAME, output_bool);
        param->data.radio.params.autoChannelEnable=output_bool == (unsigned char*)"true";
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioAutoChannelEnable FAILURE ", MODULE_NAME);
    }
    if (wifi_getRadioAutoChannelRefreshPeriod( radioIndex, &output_ulong) == RETURN_OK) {
        LOG_DBG("[%s] auto channel refresh period  is %lu ", MODULE_NAME, output_ulong);
        param->data.radio.params.autoChannelRefreshPeriod=output_ulong;
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioAutoChannelRefreshPeriod FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioExtChannel( radioIndex,  output_string) == RETURN_OK) {
        LOG_DBG("[%s] radio ext channel  is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.extensionChannel,BUFF_LENGTH_24,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioExtChannel FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioGuardInterval( radioIndex,  output_string) == RETURN_OK) {
        LOG_DBG("[%s] radio guard is %s ", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.guardInterval,BUFF_LENGTH_24,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioGuarderval FAILURE ", MODULE_NAME);
    }
    output_INT = 0;
    if (wifi_getRadioMCS( radioIndex, &output_INT) == RETURN_OK) {
        LOG_DBG("[%s] radio ext channel  is %d ", MODULE_NAME, output_INT);
        param->data.radio.params.mcs=output_INT;
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioMCS FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if (wifi_getRadioTransmitPowerSupported( radioIndex,  output_string) == RETURN_OK) {
        LOG_DBG("[%s] radio transmit power support  is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.transmitPowerSupported,BUFF_LENGTH_64,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioTransmitPowerSupported FAILURE", MODULE_NAME);
    }
    output_INT = 0;
    if (wifi_getRadioTransmitPower( radioIndex,  &output_INT) == RETURN_OK) {
        LOG_DBG("[%s] radio transmit power   is %lu .", MODULE_NAME, output_ulong);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioTransmitPower FAILURE", MODULE_NAME);
    }
    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioIEEE80211hSupported( radioIndex,  output_bool) == RETURN_OK) {
        LOG_DBG("[%s] IEEE80211h Supp   is %lu ", MODULE_NAME, output_bool);
        //param->data.radio.params.IEEE80211hSupported=*output_bool;
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioIEEE80211hSupported FAILURE", MODULE_NAME);
    }
    memset(output_bool,0,BUFF_MIN);
    if (wifi_getRadioIEEE80211hEnabled( radioIndex,  output_bool) == RETURN_OK) {
        LOG_DBG("[%s] IEEE80211hEnabled   is %s ", MODULE_NAME, output_bool);
        //param->data.radio.params.IEEE80211hEnabled=*output_bool;
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioIEEE80211hEnabled FAILURE", MODULE_NAME);
    }

    if (wifi_getRadioChannel(radioIndex, &output_ulong) == RETURN_OK) {
        LOG_DBG("[%s] radio channel  is %lu .", MODULE_NAME, output_ulong);
        param->data.radio.params.channel=output_ulong;
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioChannel FAILURE", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioChannelsInUse(radioIndex, output_string) == RETURN_OK) {
        LOG_DBG("[%s] radio channels in use  is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.channelsInUse,BUFF_LENGTH_24,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioChannelsInUse FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioOperatingChannelBandwidth(radioIndex, output_string) == RETURN_OK) {
        LOG_DBG("[%s] operating channel bandwith  is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.operatingChannelBandwidth,BUFF_LENGTH_24,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioOperatingChannelBandwidth FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if(wifi_getRadioSupportedFrequencyBands(radioIndex, output_string) == RETURN_OK) {
        LOG_DBG("[%s] Supported frequency band  is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.supportedFrequencyBands,BUFF_LENGTH_24,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioSupportedFrequencyBands FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    memset(freqBand,0,BUFF_MIN);
    if ( wifi_getRadioOperatingFrequencyBand(radioIndex, output_string) == RETURN_OK) {
        LOG_DBG("[%s] operating frequency band  is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.operatingFrequencyBand,BUFF_LENGTH_24,output_string);
        strncpy(freqBand,output_string,BUFF_MIN-1);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioOperatingFrequencyBand FAILURE", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    // Make sure that we are passing the correct Radio Index to wifi_hal
    int actualRadioIndex = 0;
    if(strncmp(freqBand,"5GHz",BUFF_MIN-1) == 0)
        actualRadioIndex = 1;

    if ( wifi_getRadioSupportedStandards(actualRadioIndex, output_string) == RETURN_OK) {
        LOG_DBG("[%s] radio Supported standards  are %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.supportedStandards,BUFF_LENGTH_24,output_string);
    }
    else {
        LOG_ERR("[%s] HAL wifi_getRadioSupportedStandards FAILURE ", MODULE_NAME);
    }

    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioStandard(actualRadioIndex, output_string,NULL,NULL,NULL) == RETURN_OK) {
        LOG_DBG("[%s] radio standards  is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.operatingStandards,BUFF_MIN,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioStandard FAILURE ", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRadioPossibleChannels(actualRadioIndex, output_string) == RETURN_OK) {
        LOG_DBG("[%s] radio possible channels is %s ", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.possibleChannels,BUFF_LENGTH_256,output_string);
    }
    else
    {
        LOG_ERR("[%s] HAL wifi_getRadioPossibleChannels FAILURE", MODULE_NAME);
    }
    output_INT = 0;
    if (wifi_getRadioTransmitPower( radioIndex, &output_INT) == RETURN_OK) {
        LOG_DBG("[%s] Radio TransmitPower %lu .", MODULE_NAME, output_INT);
        param->data.radio.params.transmitPower = output_INT;
    }
    else
    {
        LOG_ERR("[%s] Failed to get HAL wifi_getRadioTransmitPower.", MODULE_NAME);
    }
    memset(output_string,0,BUFF_MAX);
    if ( wifi_getRegulatoryDomain(radioIndex, output_string) == RETURN_OK) {
        LOG_DBG("[%s] Radio wifi_getRegulatoryDomain is %s .", MODULE_NAME, output_string);
        snprintf(param->data.radio.params.regulatoryDomain,BUFF_LENGTH_4,output_string);
    }
    else
    {
        LOG_ERR("[%s] Failed to get wifi_getRegulatoryDomain.", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
#endif
    param->status = true;
    return ret;
}


IARM_Result_t WiFiNetworkMgr::setRadioProps(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    LOG_TRACE("[%s] Enter", MODULE_NAME );

    LOG_TRACE("[%s] Exit", MODULE_NAME );
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
    LOG_TRACE("[%s] Enter", MODULE_NAME );

    IARM_BUS_WiFi_DiagsPropParam_t *param = (IARM_BUS_WiFi_DiagsPropParam_t *)arg;
    WiFiStatusCode_t status = WIFI_DISCONNECTED;
    WiFiConnectionStatus currSsidInfo;
    char output_string[BUFF_MAX];
    int ssidIndex=0;
    memset(output_string,0, BUFF_MAX);

    if(param->numEntry == IARM_BUS_WIFI_MGR_SSIDEntry)
    {
        if(wifi_getSSIDNumberOfEntries(&output_ulong) == RETURN_OK) {
            LOG_DBG("[%s] SSID Entries is %lu ", MODULE_NAME, output_ulong);
            param->data.ssidNumberOfEntries=(unsigned int)output_ulong;
            LOG_DBG("[%s] SSID Entries param->data.ssidNumberOfEntries is %d .", MODULE_NAME, param->data.ssidNumberOfEntries);

        }
        else
        {
            LOG_ERR("[%s] HAL wifi_getSSIDEntries FAILURE ", MODULE_NAME);
        }
    }
    else if(param->numEntry == IARM_BUS_WIFI_MGR_RadioEntry)
    {
        if(wifi_getRadioNumberOfEntries(&output_ulong) == RETURN_OK) {
            LOG_DBG("[%s] Radio Entries is %lu .", MODULE_NAME, output_ulong);
            param->data.radioNumberOfEntries=(unsigned int)output_ulong;
            LOG_DBG("[%s] Radio Entries param->data.radioNumberOfEntries is %d .", MODULE_NAME, param->data.ssidNumberOfEntries);

        }
        else
        {
            LOG_ERR("[%s] HAL wifi_getRadioNumberOfEntries FAILURE", MODULE_NAME);
        }
    }
    else
    {
        if(wifi_getSSIDName(ssidIndex,output_string) == RETURN_OK) {
            LOG_DBG("[%s] SSID Name is %s .", MODULE_NAME, output_string);
            snprintf(param->data.ssid.params.name,BUFF_LENGTH_32,output_string);
            LOG_DBG("[%s] SSID Name is %s .", MODULE_NAME, param->data.ssid.params.name);

        }
        else
        {
            LOG_ERR("[%s] HAL wifi_getSSIDName FAILURE", MODULE_NAME);
        }

        memset(output_string,0, BUFF_MAX);
        if (wifi_getBaseBSSID(ssidIndex, output_string) == RETURN_OK) {
            LOG_DBG("[%s] BSSID  is %s .", MODULE_NAME, output_string);
            snprintf(param->data.ssid.params.bssid,BUFF_MAC,output_string);
            LOG_DBG("[%s] BSSID  is %s .", MODULE_NAME, param->data.ssid.params.bssid);
        }
        else
        {
            LOG_ERR("[%s] HAL wifi_getBaseBSSID FAILURE ", MODULE_NAME);
        }
        memset(output_string,0, BUFF_MAX);
        if (gWifiMacAddress[0] != '\0') {
            LOG_DBG("[%s] SSID MAC address is %s ",MODULE_NAME, gWifiMacAddress);
            snprintf(param->data.ssid.params.macaddr,BUFF_MAC, gWifiMacAddress);
            LOG_DBG("[%s] SSID MAC address is %s .",MODULE_NAME, param->data.ssid.params.macaddr);
        }
        else
        {
            LOG_ERR("[%s] HAL wifi_getSSIDMACAddress FAILURE", MODULE_NAME);
        }
        memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
        get_CurrentSsidInfo(&currSsidInfo);
        STRCPY_S(param->data.ssid.params.ssid, sizeof(param->data.ssid.params.ssid), currSsidInfo.ssidSession.ssid);
        if(currSsidInfo.ssidSession.ssid[0] != '\0')
        {
            LOG_DBG("[%s] get current SSID (%s).", MODULE_NAME, currSsidInfo.ssidSession.ssid);
        }
        else
        {
	    memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
            getWpaSsid(&currSsidInfo);
            STRCPY_S(param->data.ssid.params.ssid, sizeof(param->data.ssid.params.ssid), currSsidInfo.ssidSession.ssid);

           if(currSsidInfo.ssidSession.ssid[0] != '\0')
	   {
               LOG_DBG("[%s] get current SSID (%s).", MODULE_NAME, currSsidInfo.ssidSession.ssid);
	   }
	   else
	   {
               LOG_ERR("[%s] Empty SSID", MODULE_NAME);
	   }
        }

        status = get_WifiRadioStatus();
        LOG_DBG("[%s] getPairedSSID SSID (%s).", MODULE_NAME, currSsidInfo.ssidSession.ssid);
        if(WIFI_CONNECTED == status )
        {
            param->data.ssid.params.enable = true;
            snprintf(param->data.ssid.params.status,BUFF_MIN,"UP");
        }
        else
        {
            status = getWpaStatus();
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
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME );
#endif
    return ret;
}

#ifdef ENABLE_LOST_FOUND
#ifdef ENABLE_IARM
static void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME );
    if (strcmp(owner, IARM_BUS_AUTHSERVICE_NAME)  == 0) {
        switch (eventId) {
        case IARM_BUS_AUTHSERVICE_EVENT_SWITCH_TO_PRIVATE:
        {
            IARM_BUS_AuthService_EventData_t *param = (IARM_BUS_AuthService_EventData_t *)data;
            if (param->value == DEVICE_ACTIVATED)
            {
                LOG_INFO("[%s] Auth service msg  Box Activated ", MODULE_NAME);
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
            if ((!bDeviceActivated)&&(param->value == DEVICE_ACTIVATED))
            {
                LOG_INFO("[%s] Service Manager msg Box Activated ", MODULE_NAME);
                bDeviceActivated = true;
            }
            if(!bAutoSwitchToPrivateEnabled)
            {
              LOG_INFO("[%s] explicit call to switch to private", MODULE_NAME);
              bSwitch2Private=true;
	      lnfConnectPrivCredentials();
            }
        }
        break;
        case IARM_BUS_NETWORK_MANAGER_EVENT_STOP_LNF_WHILE_DISCONNECTED:
        {
            bool *param = (bool *)data;
            if(*param == bStopLNFWhileDisconnected)
            {
                LOG_INFO("[%s] Discarding stopLNFWhileDisconnected event since there is no change in value existing %d new %d ", MODULE_NAME,bStopLNFWhileDisconnected,*param);
                break;
            }
            LOG_INFO("[%s] event handler of stopLNFWhileDisconnected old value %d current value %d", MODULE_NAME, bStopLNFWhileDisconnected,*param);
            bStopLNFWhileDisconnected=*param;
            if((!bStopLNFWhileDisconnected) && (confProp.wifiProps.bEnableLostFound) && (gWifiLNFStatus != CONNECTED_PRIVATE))
            {
               lnfConnectPrivCredentials();
               LOG_INFO("[%s] starting LnF since stopLNFWhileDisconnected value is %d.",MODULE_NAME,bStopLNFWhileDisconnected);
            }
        }
        break;
        case IARM_BUS_NETWORK_MANAGER_EVENT_AUTO_SWITCH_TO_PRIVATE_ENABLED:
        {
            bool *param = (bool *)data;
            if(*param == bAutoSwitchToPrivateEnabled)
            {
                LOG_INFO("[%s]  Discarding  event bAutoSwitchToPrivateEnabled  since there is no change in value existing %d new %d", MODULE_NAME,bAutoSwitchToPrivateEnabled,*param);
                break;
            }
            LOG_INFO("[%s]  event handler  of bAutoSwitchToPrivateEnabled old value %d current value %d ", MODULE_NAME,bAutoSwitchToPrivateEnabled,*param);
            bAutoSwitchToPrivateEnabled=*param;
            if((gWifiLNFStatus != CONNECTED_PRIVATE) && bAutoSwitchToPrivateEnabled)
            {
               lnfConnectPrivCredentials();
               LOG_INFO("[%s] starting LnF since AutoSwitchToPrivateEnabled value is %d.", MODULE_NAME,bAutoSwitchToPrivateEnabled);

            }
        }
        break;
        default:
            break;
        }
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
}
#endif
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
    LOG_TRACE("[%s] Enter", MODULE_NAME );
    IARM_Bus_CommonAPI_SysModeChange_Param_t *param = (IARM_Bus_CommonAPI_SysModeChange_Param_t *)arg;
    LOG_INFO("[%s] Sys Mode Change::New mode --> %d, Old mode --> %d", MODULE_NAME, param->newMode,param->oldMode);
    sysModeParam=param->newMode;
    if(IARM_BUS_SYS_MODE_WAREHOUSE == sysModeParam)
    {
        LOG_INFO("[%s] Trigger Dhcp lease since we are in warehouse mode",MODULE_NAME );
        netSrvMgrUtiles::triggerDhcpRenew();
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    bool retVal=false;
    //LOG_TRACE("[%s] Enter", MODULE_NAME );

    IARM_BUS_WiFi_DiagsPropParam_t *param = (IARM_BUS_WiFi_DiagsPropParam_t *)arg;

    param->status = true;
    memset(&param->data.endPointInfo, '\0', sizeof(param->data.endPointInfo));

#ifdef USE_RDK_WIFI_HAL
    getEndPointInfo(&param->data.endPointInfo);
#endif

    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return IARM_RESULT_SUCCESS;
}


#ifdef  WIFI_CLIENT_ROAMING
IARM_Result_t WiFiNetworkMgr::getRoamingCtrls(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal=false;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    WiFi_RoamingCtrl_t *param = (WiFi_RoamingCtrl_t *)arg;
    memset(param,0,sizeof(WiFi_RoamingCtrl_t));
    retVal = getRoamingConfigInfo(param);
    ret = (retVal==true?IARM_RESULT_SUCCESS:IARM_RESULT_IPCCORE_FAIL);
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}


IARM_Result_t WiFiNetworkMgr::setRoamingCtrls(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool retVal=0;
    LOG_TRACE("[%s] Enter", MODULE_NAME );
    WiFi_RoamingCtrl_t *param = (WiFi_RoamingCtrl_t *)arg;
    retVal = setRoamingConfigInfo(param);
    ret = (retVal==true?IARM_RESULT_SUCCESS:IARM_RESULT_IPCCORE_FAIL);
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}
#endif
#endif
