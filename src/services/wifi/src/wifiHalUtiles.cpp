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

#include "wifiHalUtiles.h"
#include "netsrvmgrUtiles.h"

#include <fstream>

#ifdef ENABLE_LOST_FOUND
#include <time.h>
#endif // ENABLE_LOST_FOUND

#ifdef ENABLE_RTMESSAGE
rtConnection con_recv = NULL;
#endif

#define WIFI_HAL_VERSION_SIZE   6
static WiFiStatusCode_t gWifiAdopterStatus = WIFI_UNINSTALLED;
static WiFiConnectionTypeCode_t gWifiConnectionType = WIFI_CON_UNKNOWN;
gchar deviceID[DEVICEID_SIZE];
gchar partnerID[PARTNERID_SIZE];
static char wifiHALVer[WIFI_HAL_VERSION_SIZE]={0};
#ifdef ENABLE_LOST_FOUND
GList *lstLnfPvtResults=NULL;
#define WAIT_TIME_FOR_PRIVATE_CONNECTION 2
pthread_cond_t condLAF = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexLAF = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutexTriggerLAF = PTHREAD_MUTEX_INITIALIZER;
static laf_status_t last_laf_status = {.errcode = 0, .backoff = 0.0f};
static struct timespec last_lnf_server_contact_time;
pthread_t lafConnectThread;
pthread_t lafConnectToPrivateThread;
WiFiLNFStatusCode_t gWifiLNFStatus = LNF_UNITIALIZED;
bool bDeviceActivated=false;
bool bLnfActivationLoop=false;
bool bLNFConnect=false;
bool bStopLNFWhileDisconnected=false;
bool isLAFCurrConnectedssid=false;
bool bIsStopLNFWhileDisconnected=false;
bool bAutoSwitchToPrivateEnabled=true;
bool bSwitch2Private=false;
bool bPrivConnectionLost=false;
#define TOTAL_NO_OF_RETRY 5
#endif
pthread_t wifiStatusMonitorThread;
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
void setLNFState(WiFiLNFStatusCode_t status);

void set_WiFiConnectionType( WiFiConnectionTypeCode_t value);
WiFiConnectionTypeCode_t get_WifiConnectionType();

extern WiFiConnectionStatus savedWiFiConnList;
extern char gWifiMacAddress[MAC_ADDR_BUFF_LEN];
#ifdef USE_RDK_WIFI_HAL
static void wifi_status_action (wifiStatusCode_t , char *, unsigned short );
static wifiStatusCode_t connCode_prev_state;

extern telemetryParams wifiParams_Tele_Period1;
extern telemetryParams wifiParams_Tele_Period2;
static void logs_Period1_Params();
static void logs_Period2_Params();
#endif

#ifdef ENABLE_NLMONITOR
#include "netlinkifc.h"
#endif //ENABLE_NLMONITOR

#define RDK_ASSERT_NOT_NULL(P)          if ((P) == NULL) return EINVAL
#define SECURITY_MODE_WPA_EAP           "WPA-EAP"
#define SECURITY_MODE_WPA_PSK           "WPA-PSK"
#define WIFI_HAL_VERSION                "1.0.0"
#define LFAT_VERSION                    "lfat_version"
#define LFAT_TTL                        "lfat_ttl"
#define LFAT_CONF_FILE                  "/mnt/ramdisk/env/.lnf_conf"
#define DATA_LEN			4096
#define SCRIPT_FILE_TO_GET_DEVICE_ID    "/lib/rdk/getDeviceId.sh"
#define WIFI_CONNECT_TIME               30

typedef struct _wifi_securityModes
{
    SsidSecurity 	securityMode;
    const char          *modeString;
}wifi_securityModes; 

wifi_securityModes wifi_securityModesMap_Netapp[] =
{
    { NET_WIFI_SECURITY_NONE,          		  	"No Security"                   },
    { NET_WIFI_SECURITY_WEP_64, 	          	"WEP (Open & Shared)"        	},
    { NET_WIFI_SECURITY_WPA_PSK_AES, 		  	"WPA-Personal, AES encryp."    	},
    { NET_WIFI_SECURITY_WPA_PSK_TKIP, 		 	"WPA-Personal, TKIP encryp."   	},
    { NET_WIFI_SECURITY_WPA2_PSK_AES,  			"WPA2-Personal, AES encryp."   	},
    { NET_WIFI_SECURITY_WPA2_PSK_TKIP, 			"WPA2-Personal, TKIP encryp."  	},
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_TKIP,		"WPA-ENTERPRISE, TKIP"		},
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_AES,		"WPA-ENTERPRISE, AES"		},
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_TKIP,		"WPA2-ENTERPRISE, TKIP"		},
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES,		"WPA2-ENTERPRISE, AES"		},
    { NET_WIFI_SECURITY_NOT_SUPPORTED, 		  	"Security format not supported" },
};

wifi_securityModes wifi_securityModesMap[] =
{
    { NET_WIFI_SECURITY_NONE,          		  	"None"                   },
    { NET_WIFI_SECURITY_WEP_64, 	          	"WEP"        	},
    { NET_WIFI_SECURITY_WPA_PSK_TKIP, 		 	"WPA"   	},
    { NET_WIFI_SECURITY_WPA_PSK_AES, 		  	"WPA"    	},
    { NET_WIFI_SECURITY_WPA2_PSK_TKIP, 			"WPA2"  	},
    { NET_WIFI_SECURITY_WPA2_PSK_AES,  			"WPA2"   	},
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_TKIP,		"WPA-ENTERPRISE"		},
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_AES,		"WPA-ENTERPRISE"		},
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_TKIP,		"WPA2-ENTERPRISE"		},
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES,		"WPA2-ENTERPRISE"		},
    { NET_WIFI_SECURITY_NOT_SUPPORTED, 		  	"Security format not supported" },
};

#ifdef ENABLE_RTMESSAGE
void rtConnection_init()
{
  rtLog_SetLevel(RT_LOG_INFO);
  rtLog_SetOption(rdkLog);
  rtConnection_Create(&con_recv, "NETSRVMGR_WIFI", SOCKET_ADDRESS);
  rtConnection_AddListener(con_recv, "RDKC.WIFI", onMessage, con_recv);
#ifdef XHB1
  rtConnection_AddListener(con_recv, "RDKC.WIFI.CONNECT", onConnectWifi, con_recv);
#endif
  RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"%s(%d): rtConnection_init done ", __FILE__, __LINE__);
}

void rtConnection_destroy()
{
  rtConnection_Destroy(con_recv);
}

void onMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  rtConnection con_recv = (rtConnection) closure;

  rtMessage req;
  rtMessage_FromBytes(&req, buff, n);

  if (rtMessageHeader_IsRequest(hdr))
  {
    char* buff = NULL;
    uint32_t buff_length = 0;

    rtMessage_ToString(req, &buff, &buff_length);
    rtLog_Info("Req : %.*s", buff_length, buff);
    free(buff);

    // create response
    rtMessage res;
    rtMessage_Create(&res);
    rtMessage_SetString(res, "reply", "Success");
    rtConnection_SendResponse(con_recv, hdr, res, 1000);
    rtMessage_Release(res);
  }
  rtMessage_Release(req);
}

void onConnectWifi(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"%s(%d): Called onConnectWifi ", __FILE__, __LINE__);

  rtConnection con_recv = (rtConnection) closure;

  int ssidIndex = 1;
  char const* ap_SSID = NULL;
  int32_t securityMode = 0;
  char const* ap_security_PreSharedKey = NULL;
  int saveSSID = 1;
  int32_t retVal = 1;

  rtMessage req;
  rtMessage_FromBytes(&req, buff, n);

  if (rtMessageHeader_IsRequest(hdr))
  {
    char* buff = NULL;
    uint32_t buff_length = 0;
    char jbuff[MAX_SSIDLIST_BUF] = {'\0'};
    int jBuffLen = 0;

    rtMessage_ToString(req, &buff, &buff_length);
    rtLog_Info("Req : %.*s", buff_length, buff);
    free(buff);

    rtMessage_GetString(req, "ap_SSID", &ap_SSID);
    rtMessage_GetInt32(req, "securityMode", &securityMode);
    rtMessage_GetString(req, "ap_security_PreSharedKey", &ap_security_PreSharedKey);
    RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"%s(%d): Received ap_SSID : %s securityMode : %d ap_security_PreSharedKey : %s", __FILE__, __LINE__, ap_SSID, securityMode, ap_security_PreSharedKey);

#ifdef USE_RDK_WIFI_HAL
/*    int status = scan_Neighboring_WifiAP(jbuff);
    jBuffLen = strlen(jbuff);
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Scan AP's SSID list buffer size : \n\"%d\"\n", MODULE_NAME,__FUNCTION__, __LINE__, jBuffLen);*/
#endif

    retVal = connect_withSSID(ssidIndex, const_cast<char*>(ap_SSID), (SsidSecurity)6, NULL, const_cast<char*>(ap_security_PreSharedKey), const_cast<char*>(ap_security_PreSharedKey), saveSSID, NULL, NULL, NULL, NULL, WIFI_CON_PRIVATE);

    if (retVal)
    {
      int retry = 0;

      while (false == isWifiConnected() && retry <= WIFI_CONNECT_TIME)
      {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"%s(%d):  Waiting for wifi connection\n", __FILE__, __LINE__);
        sleep(1);
        retry++;
      }

      if (true == isWifiConnected())
      {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"%s(%d): Wifi connection success\n", __FILE__, __LINE__);
        retVal = 0;
      }
      else if (retry >= WIFI_CONNECT_TIME)
      {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"%s(%d): Wifi connection time out\n", __FILE__, __LINE__);
      }
    }

    // create response
    rtMessage res;
    rtMessage_Create(&res);
    rtMessage_SetInt32(res, "reply", retVal);
    rtConnection_SendResponse(con_recv, hdr, res, 1000);
    rtMessage_Release(res);
  }
  rtMessage_Release(req);
}

void* rtMessage_Receive(void*)
{
  RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"%s(%d): rtMessage_Receive\n", __FILE__, __LINE__);
  while (1)
  {
    rtError err = rtConnection_Dispatch(con_recv);
    if (err != RT_OK)
      RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"%s(%d): Dispatch Error: %s", __FILE__, __LINE__, rtStrError(err));
  }
}
#endif


#ifdef USE_RDK_WIFI_HAL
bool getHALVersion()
{
    if(wifiHALVer[0] == 0)
    {
        wifi_getHalVersion(wifiHALVer);
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] WiFi HAL Version is %s \n",MODULE_NAME,__FUNCTION__, __LINE__,wifiHALVer);
    }
}
#endif

SsidSecurity get_wifiSecurityModeFromString(char *secModeString,char *encryptionType)
{
    SsidSecurity mode = NET_WIFI_SECURITY_NOT_SUPPORTED;
    if(!secModeString || !encryptionType) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to NULL. \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return NET_WIFI_SECURITY_NONE;
    }
    if(0 == strcmp(wifiHALVer,WIFI_HAL_VERSION))
    {
        
        int len = sizeof(wifi_securityModesMap_Netapp)/sizeof(wifi_securityModes);
        for(int i = 0; i < len; i++) {
	    if(NULL != strcasestr(secModeString,wifi_securityModesMap_Netapp[i].modeString)) {
                mode = wifi_securityModesMap_Netapp[i].securityMode;
                break;
            }
        }
    }
    else
    {   // If both secModeString and encryptionType are Empty then Security Mode is OPEN/NONE
        if(strlen(secModeString) == 0 && strlen(encryptionType) == 0)
        {
           mode  = NET_WIFI_SECURITY_NONE;
        }
        else
        {
            int len = sizeof(wifi_securityModesMap)/sizeof(wifi_securityModes);
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] securitymode = %s \n", MODULE_NAME,__FUNCTION__, __LINE__,secModeString);
            for(int i = len-1 ; i >= 0; i--) {
                if(NULL != strcasestr(secModeString,wifi_securityModesMap[i].modeString)) {
                    if(((encryptionType != NULL) && (NULL != strcasestr(encryptionType,"AES"))) ||  (NULL != strcasestr(secModeString,"WEP"))) {
                        mode = wifi_securityModesMap[i].securityMode;
                    }
                    else
                    {
                        mode = wifi_securityModesMap[i-1].securityMode;
                    }
                    break;
                }
            }
        }
    }
    return mode;
}

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

unsigned long getUptimeMS(void)
{
    std::ifstream file("/proc/uptime");
    if (file.is_open())
    {
        char buffer[65];
        file.getline(buffer, sizeof buffer - 1);
        file.close();
        double uptime = atof(buffer);
        return uptime*1000;
    }
    return 0;
}

void logMilestone(const char *msg_code)
{
    FILE *fp = NULL;
    fp = fopen("/opt/logs/rdk_milestones.log", "a+");
    if (fp != NULL)
    {
      fprintf(fp, "%s:%ld\n", msg_code, getUptimeMS());
      fclose(fp);
    }
}

#ifdef ENABLE_IARM
static IARM_Result_t WiFi_IARM_Bus_BroadcastEvent(const char *ownerName, IARM_EventId_t eventId, void *data, size_t len)
{
  //  if( !ethernet_on() )
        IARM_Bus_BroadcastEvent(ownerName, eventId, data, len);
}
#endif

static void set_WiFiStatusCode( WiFiStatusCode_t status)
{
    pthread_mutex_lock(&wifiStatusLock);
    if(gWifiAdopterStatus != status)
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Wifi Status changed to %d \n", MODULE_NAME,__FUNCTION__, __LINE__, status );
    gWifiAdopterStatus = status;
    pthread_mutex_unlock(&wifiStatusLock);
}

static WiFiStatusCode_t get_WiFiStatusCode()
{
    return gWifiAdopterStatus;
}

static bool get_WiFiStatusCodeAsString (WiFiStatusCode_t code, char* string)
{
    switch (code)
    {
    case WIFI_UNINSTALLED:
        strcpy (string, "UNINSTALLED");
        break;
    case WIFI_DISABLED:
        strcpy (string, "DISABLED");
        break;
    case WIFI_DISCONNECTED:
        strcpy (string, "DISCONNECTED");
        break;
    case WIFI_PAIRING:
        strcpy (string, "PAIRING");
        break;
    case WIFI_CONNECTING:
        strcpy (string, "CONNECTING");
        break;
    case WIFI_CONNECTED:
        strcpy (string, "CONNECTED");
        break;
    case WIFI_FAILED:
        strcpy (string, "FAILED");
        break;
    default:
        return false;
    }
    return true;
}

#ifdef ENABLE_LOST_FOUND
WiFiLNFStatusCode_t get_WiFiLNFStatusCode()
{
    return gWifiLNFStatus;
}
#endif
void get_CurrentSsidInfo(WiFiConnectionStatus *curSSIDConnInfo)
{
    strncpy(curSSIDConnInfo->ssidSession.ssid, wifiConnData.ssid, SSID_SIZE);
    curSSIDConnInfo->isConnected = (WIFI_CONNECTED == get_WiFiStatusCode())? true: false;
}

WiFiStatusCode_t get_WifiRadioStatus()
{
    bool ret = false;
    WiFiStatusCode_t status = WIFI_UNINSTALLED;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    status = get_WiFiStatusCode();
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit with state %d\n", MODULE_NAME,__FUNCTION__, __LINE__, status );
    return status;
}

void set_WiFiConnectionType( WiFiConnectionTypeCode_t value)
{
    if(value < 0 || value > WIFI_CON_MAX)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Trying to set invalid Wifi Connection Type %d \n", MODULE_NAME,__FUNCTION__, __LINE__, value );
        return;
    }

    if(gWifiConnectionType != value)
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Wifi Connection Type changed to %d ..\n", MODULE_NAME,__FUNCTION__, __LINE__, value );

    gWifiConnectionType = value;
}

WiFiConnectionTypeCode_t get_WifiConnectionType()
{
    return gWifiConnectionType;
}

#ifdef USE_HOSTIF_WIFI_HAL
bool updateWiFiList()
{
    bool ret = false;
#ifdef ENABLE_IARM
    HOSTIF_MsgData_t wifiParam = {0};

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
    strncpy(wifiParam.paramName, WIFI_SSID_ENABLE_PARAM, strlen(WIFI_SSID_ENABLE_PARAM) +1);
    wifiParam.instanceNum = 0;
    wifiParam.paramtype = hostIf_BooleanType;

    memset(&gSsidList, '\0', sizeof(ssidList));
    ret = gpvFromTR069hostif(&wifiParam);

    if(ret)
    {
        bool ssidStatus = get_boolean(wifiParam.paramValue);

        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Retrieved values from tr69hostif as \'%s\':\'%d\'.\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_SSID_ENABLE_PARAM, ssidStatus);

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
                strncpy(gSsidList.ssid, wifiParam.paramValue, sizeof(gSsidList.ssid));
            }

            // Get SSID
            memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
            strncpy(wifiParam.paramName, WIFI_SSID_BSSID_PARAM, strlen(WIFI_SSID_BSSID_PARAM) +1);
            wifiParam.paramtype = hostIf_StringType;
            wifiParam.instanceNum = 0;
            ret = gpvFromTR069hostif(&wifiParam);

            if(ret)
            {
                strncpy(gSsidList.bssid,  wifiParam.paramValue, sizeof(gSsidList.bssid));
            }

            /* Not Implemented in tr69 Hal.*/
            gSsidList.frequency = -1;
            gSsidList.signalstrength = -1;


        }
        else
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] WiFi is disabled. [\'%s\':%d] \n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_SSID_ENABLE_PARAM, ssidStatus);
            ret = false;
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
#endif
    return ret;
}




//---------------------------------------------------------
// generic Api for get HostIf parameters through IARM_TR69Bus
// --------------------------------------------------------
#ifdef ENABLE_IARM
bool gpvFromTR069hostif( HOSTIF_MsgData_t *param)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;

    param->reqType = HOSTIF_GET;
//    HOSTIF_MsgData_t param = &wifiParam;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    /* IARM get Call*/
    ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME, IARM_BUS_TR69HOSTIFMGR_API_GetParams, (void *)param, sizeof(HOSTIF_MsgData_t));

    if(ret != IARM_RESULT_SUCCESS)
    {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%s:%d] Failed with returned \'%d\' for '\%s\''. \n",  MODULE_NAME,__FUNCTION__, __LINE__, ret, param->paramName);
        //ret = ERR_REQUEST_DENIED;
        return false;
    }
    else
    {
        ret = IARM_RESULT_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Successfully returned the value of \'%s\'.\n",  MODULE_NAME,__FUNCTION__, __LINE__, param->paramName);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return true;
}
#endif
static WiFiStatusCode_t getWpaStatus()
{
    WiFiStatusCode_t ret = WIFI_UNINSTALLED;
    FILE *in = NULL;
    char buff[512] = {'\0'};

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    if(!(in = popen("wpa_cli status | grep -i wpa_state | cut -d = -f 2", "r")))
    {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%s:%d] Failed to read wpa_cli status.\n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return ret;
    }

    if(fgets(buff, sizeof(buff), in)!=NULL)
    {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d]  State: %s\n", MODULE_NAME,__FUNCTION__, __LINE__, buff);
//		printf("%s", buff);
    }
    else {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%s:%d]  Failed to get State.\n", MODULE_NAME,__FUNCTION__, __LINE__);
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

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
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
    INT count=1;
#ifdef ENABLE_IARM
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    memset(&eventData, 0, sizeof(eventData));

/*    if(WIFI_CONNECTING == get_WiFiStatusCode()) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Connecting to AP is in Progress..., please try after 120 seconds.\n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return ret;
    }*/

//    if (WIFI_CONNECTED != get_WiFiStatusCode()) {

// we have an issue in states where disconnect state is coming after connecting state so application
// screens are getting messed up so to fix it we want make sure that disconnect is done before sending connecting state.
        if (WIFI_CONNECTED == get_WiFiStatusCode())
        {
            wifi_disconnectEndpoint(1, wifiConnData.ssid);
            usleep(100000);
            while((WIFI_CONNECTED == get_WiFiStatusCode()) && count <= 30)
            {
                usleep(100000);
                count++;
            }
        }

        set_WiFiConnectionType(WIFI_CON_WPS);

        wpsStatus = wifi_setCliWpsButtonPush(ssidIndex);

        if(RETURN_OK == wpsStatus)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Received WPS KeyCode \"%d\";  Successfully called \"wifi_setCliWpsButtonPush(%d)\"; WPS Push button Success. \n",\
                     MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, wpsStatus);
            ret = true;

            /*Generate Event for Connect State
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
            set_WiFiStatusCode(WIFI_CONNECTING);
            wifi_conn_type = WPS_PUSH_CONNECT;*/
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Received WPS KeyCode \"%d\";  Failed in \"wifi_setCliWpsButtonPush(%d)\", WPS Push button press failed with status code (%d) \n",
                     MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, wpsStatus);

            eventData.data.wifiStateChange.state = WIFI_FAILED;
            WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
        }
#if 0
    else if (WIFI_CONNECTED == get_WiFiStatusCode())
    {
        /*If connected, do Disconnect first, the again re-connect to same or different AP.
         * Wait for 60 Sec timeout period if fails to disconnect, then show error.
             * If successfully disconncted, then go ahead and so call connect WpsPush. */
        if(RETURN_OK == wifi_disconnectEndpoint(1, wifiConnData.ssid))
        {
            time_t start_time = time(NULL);
            time_t timeout_period = start_time + confProp.wifiProps.max_timeout/*MAX_TIME_OUT_PERIOD */;

            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Received WPS KeyCode \"%d\"; Already Connected to \"%s\" AP. \"wifi_disconnectEndpointd)\" Successfully on WPS Push. \n",\
                     MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, wifiConnData.ssid, wpsStatus);
            /* Check for status change from Callback function */
            while (start_time < timeout_period)
            {
                if(WIFI_DISCONNECTED == gWifiAdopterStatus)  {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully received Disconnect state \"%d\". \n", MODULE_NAME,__FUNCTION__, __LINE__, gWifiAdopterStatus);
                    set_WiFiConnectionType(WIFI_CON_WPS);
                    ret = (RETURN_OK == wifi_setCliWpsButtonPush(ssidIndex))?true:false;
                    eventData.data.wifiStateChange.state = (ret)? WIFI_CONNECTING: WIFI_FAILED;
                    set_WiFiStatusCode(eventData.data.wifiStateChange.state);
                    WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged' with state as \'%s\'(%d).\n", MODULE_NAME,__FUNCTION__, __LINE__,
                             (ret?"CONNECTING": "FAILED"), eventData.data.wifiStateChange.state);
                    if(ret) {
                        remove(WIFI_BCK_FILENAME);
                        wifi_conn_type = WPS_PUSH_CONNECT;
                    }
                    break;
                }
                else {
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed to Disconnect \"%s\";  wait for %d sec (i.e., loop time : %s) to update disconnect state by wifi_disconnect_callback. (%d) \n",
                             MODULE_NAME,__FUNCTION__, __LINE__, wifiConnData.ssid, MAX_TIME_OUT_PERIOD, ctime(&start_time), gWifiAdopterStatus);
                    sleep(RETRY_TIME_INTERVAL);
                    start_time = time(NULL);
                }
            }
        } else {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Received WPS KeyCode \"%d\";  Failed in \"wifi_disconnectEndpointd(%d)\", WPS Push button press failed with status code (%d) \n",
                     MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, wpsStatus);
        }

        if(false == ret)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Since failed to disconnect, so reconnecintg again. \"%d\". \n", MODULE_NAME,__FUNCTION__, __LINE__);
            set_WiFiConnectionType(WIFI_CON_WPS);
            ret = (RETURN_OK == wifi_setCliWpsButtonPush(ssidIndex))?true:false;
            eventData.data.wifiStateChange.state = (ret)? WIFI_CONNECTING: WIFI_FAILED;
            set_WiFiStatusCode(eventData.data.wifiStateChange.state);
            WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
            if(ret) {
                wifi_conn_type = WPS_PUSH_CONNECT;
            }
        }
    }
#endif
#endif

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}



INT wifi_connect_callback(INT ssidIndex, CHAR *AP_SSID, wifiStatusCode_t *connStatus)
{
    int ret = RETURN_OK;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    wifiStatusCode_t connCode = *connStatus;
    wifi_status_action (connCode, AP_SSID, (unsigned short) ACTION_ON_CONNECT);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

INT wifi_disconnect_callback(INT ssidIndex, CHAR *AP_SSID, wifiStatusCode_t *connStatus)
{
    int ret = RETURN_OK;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    wifiStatusCode_t connCode = *connStatus;
    wifi_status_action (connCode, AP_SSID, (unsigned short)ACTION_ON_DISCONNECT);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}

void wifi_status_action (wifiStatusCode_t connCode, char *ap_SSID, unsigned short action)
{
    const char *connStr = (action == ACTION_ON_CONNECT)?"Connect": "Disconnect";
    char command[128]= {'\0'};
    static unsigned int switchLnf2Priv=0;
    static unsigned int switchPriv2Lnf=0;
    bool retVal = false;
    int radioIndex = 0;
    wifi_sta_stats_t stats;
#ifdef ENABLE_IARM
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_MAX;
    bool notify = false;
    int xreReconnectReason=0;
    memset(&eventData, 0, sizeof(eventData));
#endif

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    switch(connCode) {
    case WIFI_HAL_SUCCESS:
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully %s to AP %s. \n", MODULE_NAME,__FUNCTION__, __LINE__ , connStr, ap_SSID);
        if (ACTION_ON_CONNECT == action) {
            set_WiFiStatusCode(WIFI_CONNECTED);
#ifdef ENABLE_IARM
            notify = true;
#endif
#ifdef ENABLE_LOST_FOUND
            if (! laf_is_lnfssid(ap_SSID))
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] savedWiFiConnList.ssidSession.ssid = %s ap_SSID = %s\n",MODULE_NAME,__FUNCTION__, __LINE__,savedWiFiConnList.ssidSession.ssid,ap_SSID);
                if (strcmp (savedWiFiConnList.ssidSession.ssid, ap_SSID) != 0)
                {
                    storeMfrWifiCredentials();
                    //bounce xre connection if we move from one private ssid to another ssid
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Moving from one SSID = %s  to another SSID = %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,savedWiFiConnList.ssidSession.ssid,ap_SSID);
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
                    xreReconnectReason=20; //RECONNECT_REASON_WIFI_NETWORK_CHANGE
                    if(false == setHostifParam(XRE_REFRESH_SESSION_WITH_RR ,hostIf_IntegerType ,(void *)&xreReconnectReason))
                    {
                        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] refresh xre session failed .\n",__FUNCTION__, __LINE__);
 
                    }
#endif // ENABLE_XCAM_SUPPORT and XHB1 not enabled
#ifdef ENABLE_NLMONITOR
                   char const* intface=getenv("WIFI_INTERFACE");
                   if(intface != NULL)
                   {
                       std::string ifcStr(intface);
                       //cleanup Global IPv6s assigned.
                       NetLinkIfc::get_instance()->deleteinterfaceip(ifcStr,AF_INET6);
                       NetLinkIfc::get_instance()->deleteinterfaceroutes(ifcStr,AF_INET6);
                       RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Clearing ipv6 ip and routes .\n",__FUNCTION__, __LINE__);
                       std::string enableV6 = "/lib/rdk/enableIpv6Autoconf.sh ";
                       enableV6 += ifcStr;  //adding interface
                       enableV6 += " &";
#ifdef YOCTO_BUILD
                       v_secure_system(enableV6.c_str());
#else
                       system(enableV6.c_str());
#endif
                   }
                   else
                       RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] No WiFi interface .\n",__FUNCTION__, __LINE__);
#endif
                }
            }
#endif
#ifdef ENABLE_IARM
            notify = true;
#if 0       /* Do not bounce for any network switch. Do it only from LF to private. */
            if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
            {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] refresh xre session failed .\n", MODULE_NAME,__FUNCTION__, __LINE__);
            }
#endif
#endif
            /* one condition variable is signaled */
#ifndef ENABLE_XCAM_SUPPORT
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d]Trigger DHCP lease for new connection \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            netSrvMgrUtiles::triggerDhcpLease(netSrvMgrUtiles::DHCP_LEASE_RELEASE_AND_RENEW);
#endif
#ifdef ENABLE_LOST_FOUND
            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, sizeof(wifiConnData.ssid));
            wifiConnData.ssid[SSID_SIZE-1] = '\0';
            if (! laf_is_lnfssid(ap_SSID))
            {
                logMilestone("PWIFI_CONNECTED");
                isLAFCurrConnectedssid=false;
                gWifiLNFStatus=CONNECTED_PRIVATE;
                if(bStopLNFWhileDisconnected)
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Enable LNF \n",__FUNCTION__, __LINE__ );
                    bStopLNFWhileDisconnected=false;
                }
                retVal = lastConnectedSSID(&savedWiFiConnList);
                if(!retVal)
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "\n[%s:%s:%d] Last connected ssid fetch failure \n", MODULE_NAME,__FUNCTION__, __LINE__ );
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
                if(!switchPriv2Lnf)
                    switchPriv2Lnf=1;
                if(switchLnf2Priv)
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connecting private ssid. Bouncing xre connection.\n", MODULE_NAME,__FUNCTION__, __LINE__ );
                    xreReconnectReason = 21; // RECONNECT_REASON_WIFI_NETWORK_CHANGE_LNF_TO_PRIVATE
                    if(false == setHostifParam(XRE_REFRESH_SESSION_WITH_RR ,hostIf_IntegerType ,(void *)&xreReconnectReason))
                    {
                        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] refresh xre session failed .\n",__FUNCTION__, __LINE__);
                    }
                    switchLnf2Priv=0;
                }
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:CONNECTED,%s\n",ap_SSID);
                memset(&stats, 0, sizeof(wifi_sta_stats_t));
                wifi_getStats(radioIndex, &stats);
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_STATS:%s,%d,%d,%d,%d,%d,%s,%d\n",stats.sta_SSID, (int)stats.sta_PhyRate, (int)stats.sta_Noise, (int)stats.sta_RSSI,(int)stats.sta_LastDataDownlinkRate,(int)stats.sta_LastDataUplinkRate,stats.sta_BAND,(int)stats.sta_AvgRSSI);
#endif // ENABLE_XCAM_SUPPORT and XHB1 not enabled
            }
            else {
                RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] This is a LNF SSID so no storing \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                isLAFCurrConnectedssid=true;
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
                if(!switchLnf2Priv)
                    switchLnf2Priv=1;
                if(switchPriv2Lnf)
                {
                    switchPriv2Lnf=0;
                }
#endif // ENABLE_XCAM_SUPPORT and XHB1 not enabled
                setLNFState(CONNECTED_LNF);
            }

#endif //  ENABLE_LOST_FOUND
            /*Write into file*/
//            WiFiConnectionStatus wifiParams;

            /*Generate Event for Connect State*/
#ifdef ENABLE_IARM
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTED;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged' with state as \'CONNECTED\'(%d).\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_CONNECTED);
#endif
            pthread_mutex_lock(&mutexGo);
            if(0 == pthread_cond_signal(&condGo)) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Broadcast to monitor. \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            }
            pthread_mutex_unlock(&mutexGo);
        } else if (ACTION_ON_DISCONNECT == action) {
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, sizeof(wifiConnData.ssid));
            wifiConnData.ssid[SSID_SIZE-1] = '\0';
#ifdef ENABLE_IARM
            notify = true;
            /*Generate Event for Disconnect State*/
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_DISCONNECTED;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged' with state as \'DISCONNECTED\'(%d).\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_DISCONNECTED);
#endif
	        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_DISCONNECTED\n");
        }
        break;
    case WIFI_HAL_CONNECTING:
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connecting to AP in Progress... \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            set_WiFiStatusCode(WIFI_CONNECTING);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged' with state as \'CONNECTING\'(%d).\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_CONNECTING);
#endif
        break;

    case WIFI_HAL_DISCONNECTING:
        if(connCode_prev_state != connCode) {
#ifdef ENABLE_IARM
            notify = true;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Disconnecting from AP in Progress... \n", MODULE_NAME,__FUNCTION__, __LINE__ );
#endif
        }
        break;
    case WIFI_HAL_ERROR_NOT_FOUND:
        if(connCode_prev_state != connCode) {
#ifdef ENABLE_IARM
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed in %s with wifiStatusCode %d (i.e., AP not found). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connStr, connCode);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_NO_SSID;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'NO_SSID\'(%d), CurrentState set as %d (DISCONNECTED)\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code, WIFI_DISCONNECTED);
#endif
#ifdef ENABLE_LOST_FOUND
            /* Event Id & Code */
            if(confProp.wifiProps.bEnableLostFound)
            {
                bPrivConnectionLost=true;
                lnfConnectPrivCredentials();
            }
#endif // ENABLE_LOST_FOUND
            set_WiFiStatusCode(WIFI_DISCONNECTED);
        }
        break;

    case WIFI_HAL_ERROR_TIMEOUT_EXPIRED:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed in %s with wifiStatusCode %d (i.e., Timeout expired). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connStr, connCode);
        /* Event Id & Code */
        if(connCode_prev_state != connCode) {
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_UNKNOWN;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'FAILED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
#endif
            set_WiFiStatusCode(WIFI_DISCONNECTED);
        }
        break;

    case WIFI_HAL_ERROR_DEV_DISCONNECT:
        if(connCode_prev_state != connCode) {
#ifdef ENABLE_IARM
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed in %s with wifiStatusCode %d (i.e., Fail in Device/AP Disconnect). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connStr, connCode);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_LOST\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
#endif
            set_WiFiStatusCode(WIFI_DISCONNECTED);
        }
        break;
    /* the SSID of the network changed */
    case WIFI_HAL_ERROR_SSID_CHANGED:
        if(connCode_prev_state != connCode) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to SSID Change (%d) . \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode);
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_LOST_FOUND
            if(confProp.wifiProps.bEnableLostFound)
            {
                bPrivConnectionLost=true;
                lnfConnectPrivCredentials();
            }
#endif //ENABLE_LOST_FOUND
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_SSID_CHANGED;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'SSID_CHANGED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
#endif
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_SSID_CHANGED\n");
        }
        break;
    /* the connection to the network was lost */
    case WIFI_HAL_ERROR_CONNECTION_LOST:
        if(connCode_prev_state != connCode) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to  CONNECTION LOST (%d) from %s. \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode, ap_SSID);
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_LOST\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
#endif
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_LOST\n");
        }
        break;
    /* the connection failed for an unknown reason */
    case WIFI_HAL_ERROR_CONNECTION_FAILED:
        if(connCode_prev_state != connCode)
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Connection Failed (%d) due to unknown reason.. \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode );
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_FAILED;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_FAILED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
#endif
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_FAILED\n");
        }
        break;
    /* the connection was interrupted */
    case WIFI_HAL_ERROR_CONNECTION_INTERRUPTED:
        if(connCode_prev_state != connCode) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to Connection Interrupted (%d). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode );
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_INTERRUPTED;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_INTERRUPTED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
#endif
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_INTERRUPTED\n");
        }
        break;
    /* the connection failed due to invalid credentials */
    case WIFI_HAL_ERROR_INVALID_CREDENTIALS:
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to Invalid Credentials. (%d). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode );
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_LOST_FOUND
            if(confProp.wifiProps.bEnableLostFound)
            {
                bPrivConnectionLost=true;
                lnfConnectPrivCredentials();
            }
#endif //ENABLE_LOST_FOUND
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_INVALID_CREDENTIALS;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'INVALID_CREDENTIALS\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
#endif
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_INVALID_CREDENTIALS\n");
        break;
    /* the connection failed due to authentication failure */
    case WIFI_HAL_ERROR_AUTH_FAILED:
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to Auth Failure (%d)\n", MODULE_NAME, __FUNCTION__, __LINE__, connCode);
            set_WiFiStatusCode (WIFI_DISCONNECTED);
#ifdef ENABLE_LOST_FOUND
            if (confProp.wifiProps.bEnableLostFound)
            {
                bPrivConnectionLost = true;
                lnfConnectPrivCredentials ();
            }
#endif
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_AUTH_FAILED;
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'AUTH_FAILED\'(%d).\n",
                    MODULE_NAME, __FUNCTION__, __LINE__, eventId, eventData.data.wifiError.code);
#endif
            RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_AUTH_FAILED\n");
        break;
    case WIFI_HAL_UNRECOVERABLE_ERROR:
        if(connCode_prev_state != connCode) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to UNRECOVERABLE ERROR. (%d). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode );
            set_WiFiStatusCode(WIFI_FAILED);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_FAILED;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged (%d)' with state as \'FAILED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiStateChange.state);
#endif
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_UNRECOVERABLE_ERROR\n");
        }
        break;
    case WIFI_HAL_ERROR_UNKNOWN:
    default:
        if(connCode_prev_state != connCode) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed in %s with WiFi Error Code %d (i.e., WIFI_HAL_ERROR_UNKNOWN). \n", MODULE_NAME,__FUNCTION__, __LINE__, connStr, connCode );
#ifdef ENABLE_IARM
            notify = true;
            /* Event Id & Code */
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_UNKNOWN;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'UNKNOWN\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
#endif
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_UNKNOWN\n");
        }
        break;
    }

    connCode_prev_state = connCode;
#ifdef ENABLE_IARM
    if(notify && ((eventId >= IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged) && (eventId < IARM_BUS_WIFI_MGR_EVENT_MAX)))
    {
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,
                                     (IARM_EventId_t) eventId,
                                     (void *)&eventData, sizeof(eventData));
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Broadcast Event id \'%d\'. \n", MODULE_NAME,__FUNCTION__, __LINE__, eventId);
    }
#endif
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

/*Connect using SSID Selection */
bool connect_withSSID(int ssidIndex, char *ap_SSID, SsidSecurity ap_security_mode, CHAR *ap_security_WEPKey, CHAR *ap_security_PreSharedKey, CHAR *ap_security_KeyPassphrase,int saveSSID,CHAR * eapIdentity,CHAR * carootcert,CHAR * clientcert,CHAR * privatekey,int conType)
{
    int ret = true;
    wifiSecurityMode_t securityMode;
#ifdef ENABLE_IARM
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
#endif

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    set_WiFiConnectionType((WiFiConnectionTypeCode_t)conType);
    securityMode=(wifiSecurityMode_t)ap_security_mode;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"[%s:%s:%d] ssidIndex %d; ap_SSID : %s; ap_security_mode : %d; saveSSID = %d\n",
             MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, ap_SSID, (int)ap_security_mode, saveSSID );
    if(saveSSID)
    {
        set_WiFiStatusCode(WIFI_CONNECTING);
#ifdef ENABLE_IARM
        eventData.data.wifiStateChange.state = WIFI_CONNECTING;
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
#endif
    }
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR,"[%s:%s:%d] connecting to ssid %s with passphrase %s \n",
            MODULE_NAME,__FUNCTION__, __LINE__, ap_SSID, ap_security_KeyPassphrase);
    ret=wifi_connectEndpoint(ssidIndex, ap_SSID, securityMode, ap_security_WEPKey, ap_security_PreSharedKey, ap_security_KeyPassphrase, saveSSID,eapIdentity,carootcert,clientcert,privatekey);
    if(ret)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] Error in connecting to ssid %s\n",
                 MODULE_NAME,__FUNCTION__, __LINE__, ap_SSID);
#ifdef ENABLE_IARM
        eventData.data.wifiStateChange.state = WIFI_FAILED;
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
#endif
        ret = false;
    }
    else
    {
        ret = true;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}

bool scan_SpecificSSID_WifiAP(char *buffer, const char* SSID, double freq_in)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter..\n", MODULE_NAME,__FUNCTION__, __LINE__ );
#ifndef ENABLE_XCAM_SUPPORT
    wifi_neighbor_ap_t *ssid_ap_array = NULL;
    UINT ssid_ap_array_size = 0;
    WIFI_HAL_FREQ_BAND band = WIFI_HAL_FREQ_BAN_NONE;
    if(freq_in == 2.4)
        band = WIFI_HAL_FREQ_BAND_24GHZ;
    else if(freq_in == 5)
        band = WIFI_HAL_FREQ_BAND_5GHZ;
    else if (freq_in == 0 )
        band = WIFI_HAL_FREQ_BAN_NONE;
    else
        return false;

    bool ret = wifi_getSpecificSSIDInfo (SSID, band, &ssid_ap_array, &ssid_ap_array_size);

    RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"[%s:%s:%d] wifi_get wifi_getSpecificSSIDInfo returned %d, neighbor_ap_array_size %d\n",
                    MODULE_NAME, __FUNCTION__, __LINE__, ret, ssid_ap_array_size);

    if ((RETURN_OK != ret ) && ((NULL == ssid_ap_array) || (0 == ssid_ap_array_size)))
        return false;

    cJSON *rootObj = cJSON_CreateObject();
    if (NULL == rootObj) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] \'Failed to create root json object.\n",  MODULE_NAME,__FUNCTION__, __LINE__);
        return false;
    }

    cJSON *array_obj = NULL, *array_element = NULL;
    char *ssid = NULL;
    int signalStrength = 0;
    double frequency = 0;
    SsidSecurity encrptType = NET_WIFI_SECURITY_NONE;

    cJSON_AddItemToObject(rootObj, "getAvailableSSIDsWithName", array_obj=cJSON_CreateArray());

    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n*********** Start: SSID Scan List **************** \n");
    for (int index = 0; index < ssid_ap_array_size; index++ )
    {
        ssid = ssid_ap_array[index].ap_SSID;
        if (*ssid && (0 != strcmp (ssid, LNF_NON_SECURE_SSID)) && (0 != strcmp (ssid, LNF_SECURE_SSID)))
        {
            signalStrength = ssid_ap_array[index].ap_SignalStrength;
            frequency = strtod(ssid_ap_array[index].ap_OperatingFrequencyBand, NULL);

            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s] [%d] => SSID: %s | SignalStrength: %d | Frequency: %f | EncryptionMode: %s\n",
                             MODULE_NAME, __FUNCTION__, index, ssid, signalStrength, frequency, ssid_ap_array[index].ap_EncryptionMode );

            if(0 == strcmp(wifiHALVer,WIFI_HAL_VERSION))
            {
                /* The type of encryption the neighboring WiFi SSID advertises.*/
                encrptType = get_wifiSecurityModeFromString((char *)ssid_ap_array[index].ap_EncryptionMode,NULL);
            }
            else
            {
                encrptType = get_wifiSecurityModeFromString((char *)ssid_ap_array[index].ap_SecurityModeEnabled,(char *)ssid_ap_array[index].ap_EncryptionMode);
            }
            cJSON_AddItemToArray(array_obj,array_element=cJSON_CreateObject());
            cJSON_AddStringToObject(array_element, "ssid", ssid);
            cJSON_AddNumberToObject(array_element, "security", encrptType);
            cJSON_AddNumberToObject(array_element, "signalStrength", signalStrength);
            cJSON_AddNumberToObject(array_element, "frequency", frequency);
        }
    }
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n***********End: SSID Scan List **************** \n\n");


    char *out = cJSON_PrintUnformatted(rootObj);
    if (out)
    {
        strncpy(buffer, out, strlen(out)+1);
    }

    if(rootObj) cJSON_Delete(rootObj);

    if(out) free(out);

    if(ssid_ap_array)
    {
        RDK_LOG( RDK_LOG_DEBUG,LOG_NMGR, "[%s:%s:%d] malloc allocated = %d \n", MODULE_NAME,__FUNCTION__, __LINE__ , malloc_usable_size(ssid_ap_array));
        free(ssid_ap_array);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return true;
#else    // ENABLE_XCAM_SUPPORT
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
  return false;
#endif   // ENABLE_XCAM_SUPPORT
}

bool scan_Neighboring_WifiAP(char *buffer)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter..\n", MODULE_NAME,__FUNCTION__, __LINE__ );
#ifdef ENABLE_LOST_FOUND

    if((isWifiConnected()) && (isLAFCurrConnectedssid == true) && (RETURN_OK != wifi_disconnectEndpoint(1, wifiConnData.ssid)))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed to  Disconnect in wifi_disconnectEndpoint()", __FUNCTION__, __LINE__);
    }
#endif //ENABLE_LOST_FOUND
    wifi_neighbor_ap_t *neighbor_ap_array = NULL;
    UINT neighbor_ap_array_size = 0;
    bool ret = wifi_getNeighboringWiFiDiagnosticResult (0, &neighbor_ap_array, &neighbor_ap_array_size);

    RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"[%s:%s:%d] wifi_getNeighboringWiFiDiagnosticResult returned %d, neighbor_ap_array_size %d\n",
            MODULE_NAME, __FUNCTION__, __LINE__, ret, neighbor_ap_array_size);

    if ((RETURN_OK != ret ) && ((NULL == neighbor_ap_array) || (0 == neighbor_ap_array_size)))
        return false;

    cJSON *rootObj = cJSON_CreateObject();
    if (NULL == rootObj) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] \'Failed to create root json object.\n",  MODULE_NAME,__FUNCTION__, __LINE__);
        return false;
    }

    cJSON *array_obj = NULL, *array_element = NULL;
    char *ssid = NULL;
    int signalStrength = 0;
    double frequency = 0;
    SsidSecurity encrptType = NET_WIFI_SECURITY_NONE;

    cJSON_AddItemToObject(rootObj, "getAvailableSSIDs", array_obj=cJSON_CreateArray());

    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n*********** Start: SSID Scan List **************** \n");
    for (int index = 0; index < neighbor_ap_array_size; index++ )
    {
        ssid = neighbor_ap_array[index].ap_SSID;
        if (*ssid && (0 != strcmp (ssid, LNF_NON_SECURE_SSID)) && (0 != strcmp (ssid, LNF_SECURE_SSID)))
        {
            signalStrength = neighbor_ap_array[index].ap_SignalStrength;
            frequency = strtod(neighbor_ap_array[index].ap_OperatingFrequencyBand, NULL);

            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s] [%d] => SSID: %s | SignalStrength: %d | Frequency: %f | EncryptionMode: %s\n",
                     MODULE_NAME, __FUNCTION__, index, ssid, signalStrength, frequency, neighbor_ap_array[index].ap_EncryptionMode );

            if(0 == strcmp(wifiHALVer,WIFI_HAL_VERSION))
            {
            /* The type of encryption the neighboring WiFi SSID advertises.*/
                encrptType = get_wifiSecurityModeFromString((char *)neighbor_ap_array[index].ap_EncryptionMode,NULL);
            }
            else
            {
                encrptType = get_wifiSecurityModeFromString((char *)neighbor_ap_array[index].ap_SecurityModeEnabled,(char *)neighbor_ap_array[index].ap_EncryptionMode);
            }
            cJSON_AddItemToArray(array_obj,array_element=cJSON_CreateObject());
            cJSON_AddStringToObject(array_element, "ssid", ssid);
            cJSON_AddNumberToObject(array_element, "security", encrptType);
            cJSON_AddNumberToObject(array_element, "signalStrength", signalStrength);
            cJSON_AddNumberToObject(array_element, "frequency", frequency);
        }
    }
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n***********End: SSID Scan List **************** \n\n");


    char *out = cJSON_PrintUnformatted(rootObj);
    if (out) {
        strncpy(buffer, out, strlen(out)+1);
    }

    if(rootObj) {
        cJSON_Delete(rootObj);
    }
    if(out) free(out);

    if(neighbor_ap_array) {
        RDK_LOG( RDK_LOG_DEBUG,LOG_NMGR, "[%s:%s:%d] malloc allocated = %d \n", MODULE_NAME,__FUNCTION__, __LINE__ , malloc_usable_size(neighbor_ap_array));
        free(neighbor_ap_array);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return true;
}

bool lastConnectedSSID(WiFiConnectionStatus *ConnParams)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    bool ret = false;
    wifi_pairedSSIDInfo_t pairedSSIDInfo = {0};
    if (RETURN_OK != wifi_lastConnected_Endpoint (&pairedSSIDInfo))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] Error in getting last connected SSID \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    else
    {
        snprintf (ConnParams->ssidSession.ssid, sizeof(ConnParams->ssidSession.ssid), "%s", pairedSSIDInfo.ap_ssid);
        snprintf (ConnParams->ssidSession.passphrase, sizeof(ConnParams->ssidSession.passphrase), "%s", pairedSSIDInfo.ap_passphrase);
        snprintf (ConnParams->ssidSession.bssid, sizeof(ConnParams->ssidSession.bssid), "%s", pairedSSIDInfo.ap_bssid);
        snprintf (ConnParams->ssidSession.security, sizeof(ConnParams->ssidSession.security), "%s", pairedSSIDInfo.ap_security);
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR,
                "[%s:%s:%d] last connected  ssid is  %s passphrase is %s  bssid is %s security is %s \n",
                MODULE_NAME, __FUNCTION__, __LINE__,
                ConnParams->ssidSession.ssid, ConnParams->ssidSession.passphrase,
                ConnParams->ssidSession.bssid, ConnParams->ssidSession.security);
        if (strcmp (ConnParams->ssidSession.security, SECURITY_MODE_WPA_PSK) == 0 )
        {
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] SECURITY_MODE_WPA_PSK \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            ConnParams->ssidSession.security_mode=NET_WIFI_SECURITY_WPA2_PSK_AES;
        }
        else if (strcmp (ConnParams->ssidSession.security, SECURITY_MODE_WPA_EAP) == 0)
        {
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] SECURITY_MODE_WPA_EAP \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            ConnParams->ssidSession.security_mode=NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES;
        }
        else
        {
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] SECURITY_MODE_WPA_NONE \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            ConnParams->ssidSession.security_mode=NET_WIFI_SECURITY_NONE;
        }
        ret = true;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
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
        set_WiFiStatusCode(WIFI_DISCONNECTED);
        isCapable = true;
    }
    return isCapable;
}

#ifdef USE_RDK_WIFI_HAL

void *wifiConnStatusThread(void* arg)
{
    int ret = 0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    wifi_sta_stats_t stats;
    WiFiStatusCode_t wifiStatusCode;
    char wifiStatusAsString[32];
    int radioIndex = 0;

    while (true ) {
        pthread_mutex_lock(&mutexGo);

        if(ret = pthread_cond_wait(&condGo, &mutexGo) == 0) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n[%s:%s:%d] ***** Monitor activated by signal ***** \n", MODULE_NAME,__FUNCTION__, __LINE__ );

            pthread_mutex_unlock(&mutexGo);

            while (true) {
                wifiStatusCode = get_WiFiStatusCode();

                if (get_WiFiStatusCodeAsString (wifiStatusCode, wifiStatusAsString)) {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:%s\n", wifiStatusAsString);
                }
                else {
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:Unmappable WiFi status code %d\n", wifiStatusCode);
                }

                if (WIFI_CONNECTED == wifiStatusCode) {
                    //RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n *****Start Monitoring ***** \n");
                    memset(&stats, 0, sizeof(wifi_sta_stats_t));
                    wifi_getStats(radioIndex, &stats);
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_STATS:%s,%d,%d,%d,%d,%d,%s,%d\n",
                            stats.sta_SSID, (int)stats.sta_PhyRate, (int)stats.sta_Noise, (int)stats.sta_RSSI,(int)stats.sta_LastDataDownlinkRate,(int)stats.sta_LastDataUplinkRate,stats.sta_BAND,(int)stats.sta_AvgRSSI);
                    //RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n *****End Monitoring  ***** \n");

                    /*Telemetry Parameter logging*/
                    logs_Period1_Params();
                    logs_Period2_Params();
                }

                sleep(confProp.wifiProps.statsParam_PollInterval);
            }
        }
        else
        {
            pthread_mutex_unlock(&mutexGo);
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

void monitor_WiFiStatus()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&wifiStatusMonitorThread, &attr, wifiConnStatusThread, NULL);
}


bool clearSSID_On_Disconnect_AP()
{
    bool ret = true;
    int status = 0;
    int radioIndex = 1;
    char *ap_ssid = savedWiFiConnList.ssidSession.ssid;
    if(RETURN_OK != wifi_disconnectEndpoint(1, ap_ssid))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed to  Disconnect in wifi_disconnectEndpoint() for AP : \"%s\".\n",\
                 MODULE_NAME,__FUNCTION__, __LINE__, ap_ssid);
        ret = false;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully called \"wifi_disconnectEndpointd()\" for AP: \'%s\'.  \n",\
                MODULE_NAME,__FUNCTION__, __LINE__, ap_ssid);
#ifndef ENABLE_XCAM_SUPPORT
        // Clear wpa_supplicant SSID info
        if(wifi_clearSSIDInfo(radioIndex) == RETURN_OK) {
           RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%s:%d] Successfully cleared SSID info \n", MODULE_NAME,__FUNCTION__, __LINE__ ); 
        }
#endif // ENABLE_XCAM_SUPPORT


#ifdef ENABLE_LOST_FOUND
        if ( false == isLAFCurrConnectedssid )
        {
            memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
            eraseMfrWifiCredentials();
        }
#endif //ENABLE_LOST_FOUND
    }
    return ret;
}

bool disconnectFromCurrentSSID()
{
    bool ret = true;
    int radioIndex = 1;
    char *ap_ssid = savedWiFiConnList.ssidSession.ssid;

    if(RETURN_OK != wifi_disconnectEndpoint(radioIndex, ap_ssid))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed to  Disconnect in wifi_disconnectEndpoint() for AP : \"%s\".\n",\
                 MODULE_NAME,__FUNCTION__, __LINE__, ap_ssid);
        ret = false;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully called \"wifi_disconnectEndpointd()\" for AP: \'%s\'.  \n",\
                MODULE_NAME,__FUNCTION__, __LINE__, ap_ssid);
    }
    return ret;
}

bool cancelWPSPairingOperation()
{
    bool ret = true;
#if !defined (ENABLE_XCAM_SUPPORT) && !defined (XHB1)
    if(wifi_cancelWpsPairing() == RETURN_OK)
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully Cancelled WPS operation.\n",\
                MODULE_NAME,__FUNCTION__, __LINE__);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed to Cancel WPS operation, Looks like no in-progress wps operation.  : \"%s\".\n",\
                MODULE_NAME,__FUNCTION__, __LINE__);
        ret = false;
    }
#endif //ENABLE_LOST_FOUND
    return ret;
}

void getConnectedSSIDInfo(WiFiConnectedSSIDInfo_t *conSSIDInfo)
{
    bool ret = true;
    int radioIndex = 0;
    wifi_sta_stats_t stats;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    memset(&stats, '\0', sizeof(stats));

    wifi_getStats(radioIndex, &stats);
    strncpy((char *)conSSIDInfo->ssid, (const char *)stats.sta_SSID, (size_t)SSID_SIZE);
    strncpy((char *)conSSIDInfo->bssid, (const char *)stats.sta_BSSID, (size_t)BSSID_BUFF);
    conSSIDInfo->rate = stats.sta_PhyRate;
    conSSIDInfo->noise = stats.sta_Noise;
    conSSIDInfo->signalStrength = stats.sta_RSSI;
    conSSIDInfo->avgSignalStrength = stats.sta_AvgRSSI;
    conSSIDInfo->frequency = stats.sta_Frequency;
    strncpy((char *)conSSIDInfo->band,(const char *)stats.sta_BAND,(size_t)BUFF_MIN);
    conSSIDInfo->securityMode = get_wifiSecurityModeFromString(stats.sta_SecMode, stats.sta_SecMode);

    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Connected SSID info: \n \
    		[SSID: \"%s\"| BSSID : \"%s\" | PhyRate : \"%f\" | Noise : \"%f\" | SignalStrength(rssi) : \"%f\" | avgSignalStrengtth : \"%f\" | Frequency : \"%d\" Security Mode : \"%s\"] \n",
            MODULE_NAME,__FUNCTION__, __LINE__,
            stats.sta_SSID, stats.sta_BSSID, stats.sta_PhyRate, stats.sta_Noise, stats.sta_RSSI,conSSIDInfo->avgSignalStrength,conSSIDInfo->frequency,conSSIDInfo->securityMode);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

#endif

#ifdef ENABLE_LOST_FOUND

void doLnFBackoff()
{
    if (last_laf_status.backoff > 0.0f)
    {
        struct timespec current_time;
        clock_gettime (CLOCK_MONOTONIC, &current_time);

        float seconds_elapsed_after_last_lnf_server_contact =
                (current_time.tv_sec - last_lnf_server_contact_time.tv_sec)
                + (((float) ((current_time.tv_nsec - last_lnf_server_contact_time.tv_nsec))) / 1000000000);

        float seconds_remaining_to_wait = last_laf_status.backoff - seconds_elapsed_after_last_lnf_server_contact;
        if (seconds_remaining_to_wait <= 0)
            seconds_remaining_to_wait = 0;

        RDK_LOG (RDK_LOG_INFO, LOG_NMGR,
                "[%s:%s:%d] last requested backoff = %fs, elapsed after last lnf server contact = %fs, waiting for remaining %fs\n",
                MODULE_NAME, __FUNCTION__, __LINE__,
                last_laf_status.backoff, seconds_elapsed_after_last_lnf_server_contact, seconds_remaining_to_wait);

        usleep (seconds_remaining_to_wait * 1000000); // wait out the remainder of the requested backoff period
    }
    else
    {
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] backoff = 0s\n", MODULE_NAME, __FUNCTION__, __LINE__);
    }

    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] backoff wait done.\n", MODULE_NAME, __FUNCTION__, __LINE__);
}

bool triggerLostFound(LAF_REQUEST_TYPE lafRequestType)
{
    laf_conf_t* conf = NULL;
    laf_client_t* clnt = NULL;
    laf_ops_t *ops = NULL;
    laf_device_info_t *dev_info = NULL;
    char currTime[BUFF_LENGTH_32];
    static bool bLastLnfSuccess=false;
    int err;
    bool bRet=true;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter \n", MODULE_NAME,__FUNCTION__, __LINE__ );
#ifdef ENABLE_IARM
    IARM_Bus_MFRLib_GetSerializedData_Param_t param;
#endif
    ops = (laf_ops_t*) malloc(sizeof(laf_ops_t));
    if(!ops)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] laf_ops malloc failed \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return ENOMEM;
    }
    dev_info = (laf_device_info_t*) malloc(sizeof(laf_device_info_t));
    if(!dev_info)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] laf_device_info malloc failed \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        free(ops);
        return ENOMEM;
    }

    memset(dev_info,0,sizeof(laf_device_info_t));
    memset(ops,0,sizeof(laf_ops_t));
    /* set operation parameters */
    ops->laf_log_message = log_message;
    ops->laf_wifi_connect = laf_wifi_connect;
    ops->laf_wifi_disconnect = laf_wifi_disconnect;
    ops->laf_get_lfat = laf_get_lfat;
    ops->laf_set_lfat = laf_set_lfat;

    bRet = getDeviceInfo(dev_info);
    if (bRet == false) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] getDeviceInfo failed\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    /* configure laf */
    err = laf_config_new(&conf, ops, dev_info);
    if(err != 0) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Lost and found client configurtion failed with error code %d \n", MODULE_NAME,__FUNCTION__, __LINE__,err );
        bRet=false;
    }
    else
    {
        /* initialize laf client */
        err = laf_client_new(&clnt, conf);
        if(err != 0) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Lost and found client initialization failed with error code %d \n", MODULE_NAME,__FUNCTION__, __LINE__,err );
            bRet=false;
        }
        else
        {
            /* start laf */
            clnt->req_type = lafRequestType;
            clnt->conf.device.backoff = last_laf_status.backoff;
            err = laf_start(clnt, &last_laf_status);
            if (err != LAF_NO_RESPONSE_FROM_LNF)
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] lnf server requested backoff = %fs\n",
                        MODULE_NAME, __FUNCTION__, __LINE__, last_laf_status.backoff);
                clock_gettime(CLOCK_MONOTONIC, &last_lnf_server_contact_time);
            }

            if(err != 0) {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Error in lost and found client, error code %d \n", MODULE_NAME,__FUNCTION__, __LINE__,err );
                bRet=false;
            }
            if(!bAutoSwitchToPrivateEnabled)
            {
                if((!bRet) && (bLastLnfSuccess))
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] !AutoSwitchToPrivateEnabled + lastLnfSuccessful + currLnfFailure = clr switch2prv Results \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                    clearSwitchToPrivateResults();
                    bLastLnfSuccess=false;
                }
                if(bRet)
                    bLastLnfSuccess=true;
                netSrvMgrUtiles::getCurrentTime(currTime,(char *)TIME_FORMAT);
                addSwitchToPrivateResults(err,currTime);
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Added Time=%s lnf error=%d to list,length of list = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,currTime,err,g_list_length(lstLnfPvtResults));
            }
        }
    }
    free(dev_info);
    free(ops);
    /* destroy config */
    if(conf)
        laf_config_destroy(conf);
    /* destroy lost and found client */
    if(clnt)
        laf_client_destroy(clnt);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return bRet;
}
bool getmacaddress(gchar* ifname,GString *data)
{
    int fd;
    int ioRet;
    struct ifreq ifr;
    unsigned char *mac;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] ERROR opening socket %d \n", MODULE_NAME,__FUNCTION__, __LINE__,fd );
        return false;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name , ifname , IFNAMSIZ-1);
    ioRet=ioctl(fd, SIOCGIFHWADDR, &ifr);
    if (ioRet < 0)
    {
        close(fd);
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] ERROR in ioctl function to retrieve mac address %d\n", MODULE_NAME,__FUNCTION__, __LINE__,fd );
        return false;
    }
    close(fd);
    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    g_string_printf(data,"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return true;
}

#ifdef ENABLE_IARM
bool getMfrData(GString* mfrDataStr,mfrSerializedType_t mfrType)
{
    bool bRet;
    IARM_Bus_MFRLib_GetSerializedData_Param_t param;
    IARM_Result_t iarmRet = IARM_RESULT_IPCCORE_FAIL;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    memset(&param, 0, sizeof(param));
    param.type = mfrType;
    iarmRet = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_GetSerializedData, &param, sizeof(param));
    if(iarmRet == IARM_RESULT_SUCCESS)
    {
        if(param.buffer && param.bufLen)
        {
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] serialized data %s for mfrtype %d \n", MODULE_NAME,__FUNCTION__, __LINE__,param.buffer,mfrType );
            g_string_assign(mfrDataStr,param.buffer);
            bRet = true;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] serialized data is empty for mfrtype %d \n", MODULE_NAME,__FUNCTION__, __LINE__,mfrType );
            bRet = false;
        }
    }
    else
    {
        bRet = false;
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] IARM CALL failed  for mfrtype %d \n", MODULE_NAME,__FUNCTION__, __LINE__,mfrType );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return bRet;
}

bool getDeviceInfo(laf_device_info_t *dev_info)
{
    bool bRet = true;
    GString* mfrSerialNum = g_string_new(NULL);
    GString* mfrMake = g_string_new(NULL);
    GString* mfrModelName = g_string_new(NULL);
    GString* deviceMacAddr = g_string_new(NULL);
    mfrSerializedType_t mfrType;

    mfrType = mfrSERIALIZED_TYPE_SERIALNUMBER;
    if(partnerID && !partnerID[0])
    {
      if(getDeviceActivationState() == false)
      {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Partner ID Missing in url May be not activated \n", MODULE_NAME,__FUNCTION__, __LINE__);
      }
      else
      {
        g_stpcpy(dev_info->partnerId,partnerID);
      }
    }
    else
    {
      g_stpcpy(dev_info->partnerId,partnerID);
    }
    if(getMfrData(mfrSerialNum,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] mfrSerialNum = %s mfrType = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,mfrSerialNum->str,mfrType );
        strcpy(dev_info->serial_num,mfrSerialNum->str);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] getting serial num from mfr failed \n", MODULE_NAME,__FUNCTION__, __LINE__);
        bRet = false;
        goto out;
    }
    mfrType = mfrSERIALIZED_TYPE_DEVICEMAC;
    if(getMfrData(deviceMacAddr,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] deviceMacAddr = %s mfrType = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,deviceMacAddr->str,mfrType );
        strcpy(dev_info->mac,deviceMacAddr->str);

    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] getting device mac addr from mfr failed \n", MODULE_NAME,__FUNCTION__, __LINE__);
        bRet = false;
        goto out;
    }
    mfrType = mfrSERIALIZED_TYPE_MODELNAME;
    if(getMfrData(mfrModelName,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] mfrModelName = %s mfrType = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,mfrModelName->str,mfrType );
        strcpy(dev_info->model,mfrModelName->str);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] getting model name from mfr failed \n", MODULE_NAME,__FUNCTION__, __LINE__);
        bRet = false;
        goto out;
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] dummy auth token update \n", MODULE_NAME,__FUNCTION__, __LINE__);
    memset(dev_info->auth_token, 0, MAX_AUTH_TOKEN_LEN+1);
    strcpy(dev_info->auth_token,"xact_token");

out:
    g_string_free(mfrSerialNum,TRUE);
    g_string_free(mfrMake,TRUE);
    g_string_free(mfrModelName,TRUE);
    g_string_free(deviceMacAddr,TRUE);

    return bRet;
}

#else // ENABLE_IARM

#if defined (ENABLE_XCAM_SUPPORT) || defined (XHB1)
char* getlfatfilename()
{
    static char *lfat_file = NULL;
    lfat_file =  getenv("LFAT_TOKEN_FILE");
    RDK_LOG(RDK_LOG_INFO,LOG_NMGR,"[%s:%d]lfat file name=%s\n", __FUNCTION__, __LINE__, lfat_file);
    return lfat_file;
}

int get_token_length(unsigned int &tokenlength)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    FILE* fp_token;
    int tokensize;
    char* tokenfilename = getlfatfilename();
    fp_token = fopen(tokenfilename,"rb");
    if(NULL == fp_token) {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"get_token_length - Error in opening lfat token file\n");
        return EBADF;
    }else{
        fseek(fp_token, 0L, SEEK_END);
        tokensize = ftell(fp_token);
        fseek(fp_token, 0, SEEK_SET);
        tokenlength = tokensize;
        RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "get_token_length - token size is - %d\n",tokenlength);
        if(NULL != fp_token){
            fclose(fp_token);
            fp_token = NULL;
        }
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
    return 0;
}

int get_laf_token(char* token,unsigned int tokenlength)
{
    char* tokenfilename = getlfatfilename();
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    FILE* fp_token;
    fp_token = fopen(tokenfilename,"rb");
    if(NULL == fp_token) {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"get_laf_token - Error in opening lfat token file");
        return EBADF;
    }else{
        if(NULL != token)
        {
            fread(token,tokenlength,1,fp_token);
        }else {
            RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"get_laf_token - Token NULL");
        }
        if(NULL != fp_token){
            fclose(fp_token);
            fp_token = NULL;
        }
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
    return 0;
}
#endif // ENABLE_XCAM_SUPPORT or XHB1

#ifdef XHB1
int getMfrDeviceInfo(mfrSerializedType_t stdatatype, char* data)
{
        mfrSerializedData_t stdata = {NULL, 0, NULL};

        if(NULL == data)
        {
                RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Accessing ivalid memory locatio\n", MODULE_NAME, __FUNCTION__, __LINE__);
                return 1;
        }

        if(mfrGetSerializedData(stdatatype, &stdata) == mfrERR_NONE)
        {
                strncpy(data,stdata.buf,stdata.bufLen);
                if (stdata.freeBuf != NULL)
                {
                        stdata.freeBuf(stdata.buf);
                        stdata.buf = NULL;
                }
        }
        else
        {
                RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Error retriving required device info.\n", MODULE_NAME, __FUNCTION__, __LINE__);
                return 1;
        }
        return 0;
}
#endif

bool getDeviceInfo( laf_device_info_t *dev_info )
{
    bool ret = false;
    unsigned int tokenlength=0;
    int readtoken=0;
#ifdef ENABLE_XCAM_SUPPORT
    struct basic_info xcam_dev_info;
    memset((void*)&xcam_dev_info, 0, sizeof(struct basic_info));
    if (!rdkc_get_device_basic_info(&xcam_dev_info))
    {
        strncpy(dev_info->serial_num, xcam_dev_info.serial_number, MAX_DEV_SERIAL_LEN-1);
        strncpy(dev_info->mac, xcam_dev_info.mac_addr, MAX_DEV_MAC_LEN);
        strncpy(dev_info->model, xcam_dev_info.model, MAX_DEV_MODEL_LEN);
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] serial_no=%s\n", MODULE_NAME, __FUNCTION__, __LINE__, dev_info->serial_num);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] rdkc_get_device_basic_info failed\n", MODULE_NAME, __FUNCTION__, __LINE__);
        return ret;
    }
#elif XHB1
    if(1 == getMfrDeviceInfo(mfrSERIALIZED_TYPE_SERIALNUMBER, dev_info->serial_num))
    {
       RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Error reteiving device serial number.\n", MODULE_NAME, __FUNCTION__, __LINE__);
    }
    if(1 == getMfrDeviceInfo(mfrSERIALIZED_TYPE_MODELNAME, dev_info->model))
    {
       RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Error reteiving device model.\n", MODULE_NAME, __FUNCTION__, __LINE__);
    }

    if(1 == getMfrDeviceInfo(mfrSERIALIZED_TYPE_DEVICEMAC, dev_info->mac))
    {
      RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d]Error reteiving device MAC.\n", MODULE_NAME, __FUNCTION__, __LINE__);
    }
#endif
#if defined(ENABLE_XCAM_SUPPORT) || defined(XHB1)
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] auth token update for xcam\n", MODULE_NAME,__FUNCTION__, __LINE__);
    memset(dev_info->auth_token, 0, MAX_AUTH_TOKEN_LEN+1);
    readtoken = get_token_length(tokenlength);
    if(readtoken != 0) {
        RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "getDeviceInfo - Error in token read \n");
        return false;
    }
    readtoken = get_laf_token(dev_info->auth_token,tokenlength);
    if(readtoken != 0 ) {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] getDeviceInfo - Token read error\n", MODULE_NAME, __FUNCTION__, __LINE__);
        return false;
    }
    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] getDeviceInfo - token is : %s\n", MODULE_NAME, __FUNCTION__, __LINE__,dev_info->auth_token);
    ret = true;
#endif // ENABLE_XCAM_SUPPORT and XHB1 defined
    return ret;
}

#endif // ENABLE_IARM

/* Callback function to connect to wifi */
int laf_wifi_connect(laf_wifi_ssid_t* const wificred)
{
    /* call api to connect to wifi */
    int retry = 0;
    bool retVal=false;
    int ssidIndex=1;
    bool notify = false;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if (laf_is_lnfssid(wificred->ssid) && (true == isLAFCurrConnectedssid) && (WIFI_CONNECTED == get_WifiRadioStatus()))
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already connected to LF ssid Ignoring the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return 0;
    }
    if(bStopLNFWhileDisconnected)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already WPS flow has intiated so skipping the request the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }
    if (laf_is_lnfssid(wificred->ssid))
    {
        bLNFConnect=true;
    }
    else
    {
        bLNFConnect=false;
        retVal=lastConnectedSSID(&savedWiFiConnList);
        if (retVal && savedWiFiConnList.ssidSession.ssid[0] != '\0')
        {
          if((strcmp(wificred->ssid, savedWiFiConnList.ssidSession.ssid) != 0) && (strcmp(wificred->passphrase, savedWiFiConnList.ssidSession.passphrase) != 0))
          {
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Lnf response status Credentials changed : BOTH \n", MODULE_NAME,__FUNCTION__, __LINE__ );
          }
          else if(strcmp(wificred->ssid, savedWiFiConnList.ssidSession.ssid) != 0)
          {
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Lnf response status Credentials changed : SSID \n", MODULE_NAME,__FUNCTION__, __LINE__ );
          }
          else if(strcmp(wificred->passphrase, savedWiFiConnList.ssidSession.passphrase) != 0)
          {
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Lnf response status Credentials changed : KeyPassphrase \n", MODULE_NAME,__FUNCTION__, __LINE__ );
          }
        }
    }
#ifdef USE_RDK_WIFI_HAL
    if(gWifiLNFStatus != CONNECTED_PRIVATE)
    {
         setLNFState(LNF_IN_PROGRESS);
         if((bLNFConnect == true) && (RETURN_OK != wifi_disconnectEndpoint(1, wifiConnData.ssid)))
         {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed to  Disconnect in wifi_disconnectEndpoint()", __FUNCTION__, __LINE__);
         }
        retVal=connect_withSSID(ssidIndex, wificred->ssid, (SsidSecurity)wificred->security_mode, NULL, wificred->psk, wificred->passphrase, (int)(!bLNFConnect),wificred->identity,wificred->carootcert,wificred->clientcert,wificred->privatekey,bLNFConnect ? WIFI_CON_LNF : WIFI_CON_PRIVATE);
    }
        //    retVal=connect_withSSID(ssidIndex, wificred->ssid, NET_WIFI_SECURITY_NONE, NULL, NULL, wificred->psk);
//  }
#endif
    if(false == retVal)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] connect with ssid %s failed \n",
                MODULE_NAME, __FUNCTION__, __LINE__, wificred->ssid);
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] connect with ssid %s failed (passphrase %s)\n",
                MODULE_NAME, __FUNCTION__, __LINE__, wificred->ssid, wificred->passphrase );
        return EPERM;
    }
    while(get_WifiRadioStatus() != WIFI_CONNECTED && retry <= confProp.wifiProps.lnfRetryCount) {
        retry++; //max wait for 180 seconds
        if(bStopLNFWhileDisconnected)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already WPS flow has intiated so skipping the request the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            return EPERM;
        }
        sleep(1);
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Device not connected to wifi. waiting to connect... \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    if(retry > confProp.wifiProps.lnfRetryCount) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Waited for %d seconds. Failed to connect. Abort \n", MODULE_NAME,__FUNCTION__, __LINE__, confProp.wifiProps.lnfRetryCount);
        return EPERM;
    }

    /* Bounce xre session only if switching from LF to private */
    if(! laf_is_lnfssid(wificred->ssid))
    {
        setLNFState(CONNECTED_PRIVATE);
        notify = true;
//        sleep(10); //waiting for default route before bouncing the xre connection
//        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connecting private ssid. Bouncing xre connection.\n", MODULE_NAME,__FUNCTION__, __LINE__ );
//#ifdef ENABLE_IARM
//        if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
//        {
//            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] refresh xre session failed .\n", MODULE_NAME,__FUNCTION__, __LINE__);
//        }
//#endif
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return 0;
}

/* Callback function to disconenct wifi */
int laf_wifi_disconnect(void)
{
    int retry = 0;
    bool retVal=false;
    int radioIndex = 1;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if ( false == isLAFCurrConnectedssid )
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connected to private ssid Ignoring the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return 0;
    }
    if(bStopLNFWhileDisconnected)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already WPS flow has intiated so skipping the request the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }
#ifdef USE_RDK_WIFI_HAL
    char *ap_ssid = savedWiFiConnList.ssidSession.ssid;
    retVal = wifi_disconnectEndpoint(radioIndex, ap_ssid);
#endif
    setLNFState(LNF_IN_PROGRESS);
    if(retVal == false) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Tried to disconnect before connect and it failed \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }

    while(get_WifiRadioStatus() != WIFI_DISCONNECTED && retry <= 30) {
        if(bStopLNFWhileDisconnected)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already WPS flow has intiated so skipping the request the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            return EPERM;
        }
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Device is connected to wifi. waiting to disconnect... \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        retry++; //max wait for 180 seconds
        sleep(1);
    }
    if(retry > 30) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Waited for 30seconds. Failed to disconnect. Abort \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
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
    RDK_LOG( map_to_rdkloglevel(level), LOG_NMGR, "[%s:%s:%s:%d] %s\n",MODULE_NAME,SUB_MODULE_NAME, function, line, msg );
}

void setLNFState(WiFiLNFStatusCode_t status)
{
    if(gWifiLNFStatus != status)
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] LNF Status changed to %d\n", MODULE_NAME,__FUNCTION__, __LINE__, status );

    gWifiLNFStatus = status;
}

void *lafConnPrivThread(void* arg)
{
    int ret = 0;
    int counter=0;
    bool retVal;
    int ssidIndex=1;
    LAF_REQUEST_TYPE reqType;
    bool lnfReturnStatus=false;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n",MODULE_NAME, __FUNCTION__, __LINE__ );

    while (true ) {
        pthread_mutex_lock(&mutexLAF);
        if(ret = pthread_cond_wait(&condLAF, &mutexLAF) == 0) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n[%s:%s:%d] Starting the LAF Connect private SSID \n",MODULE_NAME, __FUNCTION__, __LINE__ );
            if(gWifiLNFStatus != CONNECTED_LNF)
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n [%s:%s:%d] WifiLNFStatus = %d .Setting LNF state as in progress. \n",MODULE_NAME, __FUNCTION__, __LINE__,gWifiLNFStatus );
                setLNFState(LNF_IN_PROGRESS);
            }
            pthread_mutex_unlock(&mutexLAF);
            if(bPrivConnectionLost)
            {
                bPrivConnectionLost=false;
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n[%s:%s:%d] Wait for 10 sec before starting lost and found \n",MODULE_NAME, __FUNCTION__, __LINE__ );
                sleep(10);
            }
            do
            {
                if((bAutoSwitchToPrivateEnabled) || (bSwitch2Private))
                {
                    if(bSwitch2Private == true)
                    {
                        reqType = LAF_REQUEST_SWITCH_TO_PRIVATE;
                        bSwitch2Private=false;
                    }
                    else
                    {
                        reqType = LAF_REQUEST_CONNECT_TO_PRIV_WIFI;
                    }
                }
                else
                {
                    reqType = LAF_REQUEST_CONNECT_TO_LFSSID;
                    setLNFState(LNF_IN_PROGRESS);
                }
                if (true == bStopLNFWhileDisconnected)
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", __FUNCTION__, __LINE__ );
                    setLNFState(LNF_UNITIALIZED);
                    break;
                }

                if((gWifiAdopterStatus == WIFI_CONNECTED) && (false == isLAFCurrConnectedssid))
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] connection status %d is LAF current ssid % \n", __FUNCTION__, __LINE__,gWifiAdopterStatus,isLAFCurrConnectedssid );
                    setLNFState(CONNECTED_PRIVATE);
                    break;
                }

                bIsStopLNFWhileDisconnected=false;
                pthread_mutex_lock(&mutexTriggerLAF);
                doLnFBackoff();
                if ((gWifiLNFStatus == CONNECTED_PRIVATE) && (gWifiAdopterStatus == WIFI_CONNECTED))
                {
                    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connected to Private SSID already. Aborting requested LNF.\n",
                            MODULE_NAME, __FUNCTION__, __LINE__);
                    pthread_mutex_unlock(&mutexTriggerLAF);
                    break;
                }
                else
                {
                    if (gWifiLNFStatus == LNF_UNITIALIZED)
                        setLNFState(LNF_IN_PROGRESS);
                    lnfReturnStatus = triggerLostFound(reqType);
                }
                pthread_mutex_unlock(&mutexTriggerLAF);
                if (false == lnfReturnStatus)
//                else if (!triggerLostFound(LAF_REQUEST_CONNECT_TO_PRIV_WIFI) && counter < TOTAL_NO_OF_RETRY)
                {
                    if (true == bStopLNFWhileDisconnected)
                    {
                        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", __FUNCTION__, __LINE__ );
                        setLNFState(LNF_UNITIALIZED);
                        break;
                    }
//                  counter++;
                    retVal=false;
                    retVal=lastConnectedSSID(&savedWiFiConnList);
                    if (retVal && savedWiFiConnList.ssidSession.ssid[0] != '\0')
                    {
                        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Trying Manual Connect with ssid %s security %d since not able to connect through LNF \n",MODULE_NAME, __FUNCTION__, __LINE__,savedWiFiConnList.ssidSession.ssid,savedWiFiConnList.ssidSession.security_mode);
                        retVal=connect_withSSID(ssidIndex, savedWiFiConnList.ssidSession.ssid,savedWiFiConnList.ssidSession.security_mode, NULL, savedWiFiConnList.ssidSession.passphrase, savedWiFiConnList.ssidSession.passphrase,true,NULL,NULL,NULL,NULL,WIFI_CON_PRIVATE);
                        if(false == retVal)
                        {
                            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] connect with ssid %s  failed \n",MODULE_NAME, __FUNCTION__, __LINE__,savedWiFiConnList.ssidSession.ssid );
                        }
                    }
                    else
                    {
                        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed to get previous SSID, Manual connect failed.  \n",MODULE_NAME, __FUNCTION__, __LINE__ );
                    }
                    if (last_laf_status.backoff == 0.0f)
                    {
                        last_laf_status.backoff = (float)confProp.wifiProps.lnfRetryInSecs;
                        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Added default lnf back off time %fs \n",MODULE_NAME, __FUNCTION__, __LINE__,last_laf_status.backoff );
                    }
                }
                else
                {
                    if (LAF_REQUEST_CONNECT_TO_LFSSID == reqType)
                    {
                        setLNFState(CONNECTED_LNF);
                        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Connected to LNF\n",MODULE_NAME, __FUNCTION__, __LINE__ );
                    }
                    else
                    {
                        if (gWifiLNFStatus == CONNECTED_PRIVATE)
                        {
                            setLNFState(CONNECTED_PRIVATE);
                            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] pressed coming out of LNF since box is connected to private \n", __FUNCTION__, __LINE__ );
                        }
                    }
                    break;
                }
                sleep(1);
            }while ((gWifiLNFStatus != CONNECTED_PRIVATE) && (bAutoSwitchToPrivateEnabled));
            bIsStopLNFWhileDisconnected=true;
	    sleep(1);
     }
     else
     {
         pthread_mutex_unlock(&mutexLAF);
     }
     sleep(1);
   }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}
void *lafConnThread(void* arg)
{
    int ret = 0;
    bool lnfReturnStatus=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    setLNFState(LNF_IN_PROGRESS);
    while (false == bDeviceActivated) {
        bLnfActivationLoop=true;
        while(gWifiLNFStatus != CONNECTED_LNF)
        {
            bIsStopLNFWhileDisconnected=false;
            if (gWifiAdopterStatus == WIFI_CONNECTED)
            {
                if (false == isLAFCurrConnectedssid)
                {
                    setLNFState(CONNECTED_PRIVATE);
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connected through non LAF path\n", MODULE_NAME,__FUNCTION__, __LINE__ );
                    bIsStopLNFWhileDisconnected=true;
                    bLnfActivationLoop=false;
                    return NULL;
                }
                break;
            }
            if (true == bStopLNFWhileDisconnected)
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                bIsStopLNFWhileDisconnected=true;
                bLnfActivationLoop=false;
                setLNFState(LNF_UNITIALIZED);
                return NULL;
            }
#if defined(ENABLE_XCAM_SUPPORT) || defined(XHB1)
            int readtoken=0;
            unsigned int tokenlength=0;
            readtoken = get_token_length(tokenlength);
            if(readtoken != 0) {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] LNF TOKEN ERROR \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                setLNFState(CONNECTED_PRIVATE);
                break;
            }
#endif
            pthread_mutex_lock(&mutexTriggerLAF);
            doLnFBackoff();
            if (gWifiLNFStatus == CONNECTED_PRIVATE)
            {
                RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connected to Private SSID already. Aborting requested LNF.\n",
                        MODULE_NAME, __FUNCTION__, __LINE__);
                pthread_mutex_unlock(&mutexTriggerLAF);
                break;
            }
            else
            {
                lnfReturnStatus = triggerLostFound(LAF_REQUEST_CONNECT_TO_LFSSID);
            }
            pthread_mutex_unlock(&mutexTriggerLAF);
            if (false == lnfReturnStatus)
            {

                if (true == bStopLNFWhileDisconnected)
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                    bIsStopLNFWhileDisconnected=true;
                    bLnfActivationLoop=false;
                    return NULL;
                }
                sleep(confProp.wifiProps.lnfRetryInSecs);
                continue;
            }
            else
            {
                setLNFState(CONNECTED_LNF);
                RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Connection to LAF success\n", MODULE_NAME,__FUNCTION__, __LINE__ );
                break;
            }
        }
        bIsStopLNFWhileDisconnected=false;
        sleep(1);
        if((gWifiLNFStatus != CONNECTED_PRIVATE) && (false == isLAFCurrConnectedssid) && (gWifiAdopterStatus == WIFI_CONNECTED))
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] isLAFCurrConnectedssid = %d gWifiAdopterStatus = %d\n", MODULE_NAME,__FUNCTION__, __LINE__,isLAFCurrConnectedssid,gWifiAdopterStatus );
            gWifiLNFStatus=CONNECTED_PRIVATE;
        }
        if (true == bStopLNFWhileDisconnected)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            bLnfActivationLoop=false;
            return NULL;
        }
//        getDeviceActivationState();
    }
    bLnfActivationLoop=false;
    if(gWifiLNFStatus == CONNECTED_LNF)
    {
        bPrivConnectionLost=false;
        lnfConnectPrivCredentials();
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n",MODULE_NAME, __FUNCTION__, __LINE__ );
    pthread_exit(NULL);
}

void connectToLAF()
{
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    pthread_attr_t attr;
    bool retVal=false;
    if((getDeviceActivationState() == false)
#ifdef ENABLE_IARM
        && (IARM_BUS_SYS_MODE_WAREHOUSE != sysModeParam)
#endif
)
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&lafConnectThread, &attr, lafConnThread, NULL);
    }
    else
    {

#ifdef ENABLE_XCAM_SUPPORT
        //check wifi status
        WiFiHalStatus_t wifistatus = getwifiStatusCode();
        if( WIFISTATUS_HAL_COMPLETED == wifistatus )  {
            set_WiFiStatusCode(WIFI_CONNECTED);
            //signal to monitor wifi status
            pthread_mutex_lock(&mutexGo);
            if(0 == pthread_cond_signal(&condGo)) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Broadcast to monitor. \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            }
            pthread_mutex_unlock(&mutexGo);
        } else {
            set_WiFiStatusCode(WIFI_DISCONNECTED);
        }
#elif XHB1
        set_WiFiStatusCode(WIFI_CONNECTED);
        //signal to monitor wifi status
        pthread_mutex_lock(&mutexGo);
        if(0 == pthread_cond_signal(&condGo)) {
          RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Broadcast to monitor. \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        }
        pthread_mutex_unlock(&mutexGo);
#endif
        retVal=lastConnectedSSID(&savedWiFiConnList);
        if (savedWiFiConnList.ssidSession.ssid[0] != '\0')
        {
            sleep(confProp.wifiProps.lnfStartInSecs);
        }

        /* If Device is activated and already connected to Private or any network,
            but defineltly not LF SSID. - No need to trigger LNF*/
        /* Here, 'getDeviceActivationState == true'*/
        lastConnectedSSID(&savedWiFiConnList);

        if((! laf_is_lnfssid(savedWiFiConnList.ssidSession.ssid)) &&  (WIFI_CONNECTED == get_WifiRadioStatus())) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Now connected to Private SSID %s\n", MODULE_NAME,__FUNCTION__, __LINE__ , savedWiFiConnList.ssidSession.ssid);
        }
        else {
#ifdef ENABLE_IARM
            if(IARM_BUS_SYS_MODE_WAREHOUSE != sysModeParam)
#endif
            {
                pthread_mutex_lock(&mutexLAF);
                if(0 == pthread_cond_signal(&condLAF))
                {
                    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Signal to start LAF private SSID \n", __FUNCTION__, __LINE__ );
                }
                pthread_mutex_unlock(&mutexLAF);
                bPrivConnectionLost=true;
            }
        }
    }
}


void lafConnectToPrivate()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&lafConnectToPrivateThread, &attr, lafConnPrivThread, NULL);
}


bool getLAFssid()
{
    char lafSsid[SSID_SIZE];
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    memset(lafSsid, 0, SSID_SIZE);
    laf_get_lfssid(lafSsid);
    if(lafSsid)
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] lfssid is %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,lafSsid);
        return true;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] lfssid is empty \n", MODULE_NAME,__FUNCTION__, __LINE__);
        return false;
    }
}
#if 0
bool isLAFCurrConnectedssid()
{
    bool retVal=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(laf_is_lnfssid(savedWiFiConnList.ssidSession.ssid))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] LAF is the current connected ssid \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        retVal=true;
    }
    else
    {
        retVal=false;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retVal;

}
#endif
bool getDeviceActivationState()
{
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
    char scriptOutput[BUFFER_SIZE_SCRIPT_OUTPUT] = {'\0'};
    unsigned int count=0;
    std::string str;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    while(deviceID && !deviceID[0])
    {
        if (access(SCRIPT_FILE_TO_GET_DEVICE_ID, F_OK ) != -1)
        {
            if (!netSrvMgrUtiles::getCommandOutput(SCRIPT_FILE_TO_GET_DEVICE_ID, scriptOutput, sizeof (scriptOutput)))
            {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Error in running device id script. \n", MODULE_NAME,__FUNCTION__, __LINE__);
                return bDeviceActivated;
            }
            g_stpcpy(deviceID,scriptOutput);
        }
        else
        {
            if(! confProp.wifiProps.authServerURL)
            {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Authserver URL is NULL \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                return bDeviceActivated;
            }
            str.assign(confProp.wifiProps.authServerURL,strlen(confProp.wifiProps.authServerURL));
            CurlObject authServiceURL(str);
            g_stpcpy(deviceID,authServiceURL.getCurlData());
        }
        if ((!deviceID && deviceID[0]) || (count >= 3))
            break;
        sleep(5);
        count++;
    }
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] device id string is %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,deviceID);
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
                g_stpcpy(deviceID, g_strstrip(tokens[loopvar+3]));
                if(deviceID[0] != '\0')
                {
                    bDeviceActivated = true;
                }
            }
        }
      }
      for (loopvar=0; loopvar<tokLength; loopvar++)
      {
        if (g_strrstr(g_strstrip(tokens[loopvar]), "partnerId"))
        {
            //"deviceId": "T00xxxxxxx" so omit 3 tokens ":" fromDeviceId
            if ((loopvar+3) < tokLength )
            {
                g_stpcpy(partnerID, g_strstrip(tokens[loopvar+3]));
                if(partnerID[0] != '\0')
                {
                  RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] partner id is %s \n", MODULE_NAME,__FUNCTION__, __LINE__,partnerID);
                }
            }
        }
      }
    if(tokens)
        g_strfreev(tokens);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return bDeviceActivated;
#else
    return true;
#endif // ENABLE_XCAM_SUPPORT and XHB1
}
bool isWifiConnected()
{
    return (get_WiFiStatusCode() == WIFI_CONNECTED);
}

void lnfConnectPrivCredentials()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(bLnfActivationLoop)
    {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Still in LnF Activation Loop so discarding getting private credentials \n",MODULE_NAME,__FUNCTION__, __LINE__ );
        return;
    }
    pthread_mutex_lock(&mutexLAF);
    if(0 == pthread_cond_signal(&condLAF))
    {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Signal to start LAF private SSID \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    pthread_mutex_unlock(&mutexLAF);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}
#endif // ENABLE_LOST_FOUND

#ifdef ENABLE_IARM
bool setHostifParam (char *name, HostIf_ParamType_t type, void *value)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    bool retValue=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    HOSTIF_MsgData_t param = {0};

    if(NULL == name || value == NULL) {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d]  Null pointer caught for Name/Value \n", MODULE_NAME,__FUNCTION__ ,__LINE__ );
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
        case  hostIf_IntegerType:
        case hostIf_UnsignedIntType:
            put_int(param.paramValue, *(int*)value);
            param.paramtype = hostIf_IntegerType;
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
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] Error executing Set parameter call from tr69Client, error code \n",MODULE_NAME,__FUNCTION__,__LINE__ );
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG,LOG_NMGR,"[%s:%s:%d] sent iarm message to set hostif parameter\n",MODULE_NAME,__FUNCTION__,__LINE__ );
        retValue=true;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retValue;
}
#endif // ENABLE_IARM

void put_boolean(char *ptr, bool val)
{
    bool *tmp = (bool *)ptr;
    *tmp = val;
}

void put_int(char *ptr, int val)
{
    int *tmp = (int *)ptr;
    *tmp = val;
}

#ifdef ENABLE_IARM
bool connectToMfrWifiCredentials()
{
    int ssidIndex=1;
    bool retVal=false;
    IARM_BUS_MFRLIB_API_WIFI_Credentials_Param_t param = {0};

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    param.requestType=WIFI_GET_CREDENTIALS;
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        RDK_LOG(RDK_LOG_INFO,LOG_NMGR,"[%s:%s:%d] IARM success in retrieving the stored wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
        retVal = connect_withSSID(ssidIndex, param.wifiCredentials.cSSID, NET_WIFI_SECURITY_WPA_PSK_AES, NULL, param.wifiCredentials.cPassword, param.wifiCredentials.cPassword, true, NULL, NULL, NULL, NULL, WIFI_CON_PRIVATE);
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO,LOG_NMGR,"[%s:%s:%d] IARM failure in retrieving the stored wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
    }
    return retVal;
}
#endif

bool storeMfrWifiCredentials(void)
{
    bool retVal=false;
#ifdef USE_RDK_WIFI_HAL
#ifdef ENABLE_IARM
    IARM_BUS_MFRLIB_API_WIFI_Credentials_Param_t param = {0};
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    retVal=lastConnectedSSID(&savedWiFiConnList);
    if(retVal == false)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "\n[%s:%s:%d] Last connected ssid fetch failure \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return retVal;
    }
    else
    {
        retVal=false;
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] fetched ssid details  %s \n",MODULE_NAME,__FUNCTION__,__LINE__,savedWiFiConnList.ssidSession.ssid);
    }
    param.requestType=WIFI_GET_CREDENTIALS;
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] IARM success in retrieving the stored wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
        if ((strcmp (param.wifiCredentials.cSSID, savedWiFiConnList.ssidSession.ssid) == 0) &&
                (strcmp (param.wifiCredentials.cPassword, savedWiFiConnList.ssidSession.passphrase) == 0))
        {
            RDK_LOG(RDK_LOG_INFO,LOG_NMGR,"[%s:%s:%d] Same ssid info not storing it stored ssid %s new ssid %s \n",
                    MODULE_NAME, __FUNCTION__, __LINE__, param.wifiCredentials.cSSID, savedWiFiConnList.ssidSession.ssid);
            return true;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO,LOG_NMGR,"[%s:%s:%d] ssid info is different continue to store ssid %s new ssid %s \n",
                    MODULE_NAME, __FUNCTION__, __LINE__, param.wifiCredentials.cSSID, savedWiFiConnList.ssidSession.ssid);
        }
    }
    memset(&param,0,sizeof(param));
    param.requestType=WIFI_SET_CREDENTIALS;
    strcpy(param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid);
    strcpy(param.wifiCredentials.cPassword,savedWiFiConnList.ssidSession.passphrase);
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] IARM success in storing wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
        memset(&param,0,sizeof(param));
        param.requestType=WIFI_GET_CREDENTIALS;
        if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
        {
            RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] IARM success in retrieving the stored wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
            if ((strcmp (param.wifiCredentials.cSSID, savedWiFiConnList.ssidSession.ssid) == 0) &&
                    (strcmp (param.wifiCredentials.cPassword, savedWiFiConnList.ssidSession.passphrase) == 0))
            {
                retVal=true;
                RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] Successfully stored the credentails and verified stored ssid %s current ssid %s \n",MODULE_NAME,__FUNCTION__,__LINE__,param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] failure in storing  wifi credentials   \n",MODULE_NAME,__FUNCTION__,__LINE__ );
            }

        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] IARM error in retrieving stored wifi credentials mfr error code %d \n",MODULE_NAME,__FUNCTION__,__LINE__,param.returnVal );
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] IARM error in storing wifi credentials mfr error code \n",MODULE_NAME,__FUNCTION__,__LINE__ ,param.returnVal);
    }
#endif // ENABLE_IARM
#endif // USE_RDK_WIFI_HAL
    return retVal;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}


bool eraseMfrWifiCredentials(void)
{
    bool retVal=false;
#ifdef USE_RDK_WIFI_HAL
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
#ifdef ENABLE_IARM
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_EraseAllData,0,0))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] IARM success in erasing wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
        retVal=true;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] failure in erasing  wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
#endif
#endif
    return retVal;
}


void getEndPointInfo(WiFi_EndPoint_Diag_Params *endPointInfo)
{
    int radioIndex = 0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
#ifdef USE_RDK_WIFI_HAL
    wifi_sta_stats_t stats;
    memset(&stats, '\0', sizeof(stats));

    wifi_getStats(radioIndex, &stats);

    bool enable = (WIFI_CONNECTED == get_WiFiStatusCode())? true: false;
    if (enable)
    {
        strncpy((char *)endPointInfo->status, "Enabled", (size_t)BUFF_LENGTH_64);
        endPointInfo->enable = enable;
        strncpy((char *)endPointInfo->SSIDReference, (const char *)stats.sta_SSID, (size_t)BUFF_LENGTH_64);

        endPointInfo->stats.signalStrength = stats.sta_RSSI;
        endPointInfo->stats.retransmissions = stats.sta_Retransmissions;
        endPointInfo->stats.lastDataDownlinkRate = stats.sta_LastDataDownlinkRate;
        endPointInfo->stats.lastDataUplinkRate = stats.sta_LastDataUplinkRate;
        /*endPointInfo->security.modesSupported = ;*/
    }
    else
    {
        strncpy((char *)endPointInfo->status, "Disabled", (size_t)BUFF_LENGTH_64);
        endPointInfo->enable = enable;
    }

    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] \n Profile : \"EndPoint.1.\": \n \
    		[Enable : \"%d\"| Status : \"%s\" | SSIDReference : \"%s\" ] \n",
            MODULE_NAME,__FUNCTION__, __LINE__, endPointInfo->enable, endPointInfo->status, endPointInfo->SSIDReference);

    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] \n Profile : \"EndPoint.1.Stats.\": \n \
    		[SignalStrength : \"%d\"| Retransmissions : \"%d\" | LastDataUplinkRate : \"%d\" | LastDataDownlinkRate : \" %d\" ] \n",
            MODULE_NAME,__FUNCTION__, __LINE__, endPointInfo->stats.signalStrength, endPointInfo->stats.retransmissions,
            endPointInfo->stats.lastDataDownlinkRate, endPointInfo->stats.lastDataUplinkRate);
#endif // USE_RDK_WIFI_HAL
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}


#ifdef USE_RDK_WIFI_HAL
#ifdef WIFI_CLIENT_ROAMING
bool getRoamingConfigInfo(WiFi_RoamingCtrl_t *param)
{
   int ssidIndex = 0;
   bool retStatus = false;
   wifi_roamingCtrl_t  out_param;
   memset(&out_param,0,sizeof(out_param));
   int status = wifi_getRoamingControl(ssidIndex,&out_param);
   if(status ==  0) {
       param->roamingEnable = (out_param.roamingEnable==1?true:false);
       param->preassnBestThreshold = out_param.preassnBestThreshold;
       param->preassnBestDelta = out_param.preassnBestDelta;
       param->status = ROAM_PARAM_SUCCESS;
       param->selfSteerOverride = (out_param.selfSteerOverride==1?true:false);
       param->postAssnLevelDeltaConnected = out_param.postAssnLevelDeltaConnected;
       param->postAssnLevelDeltaDisconnected = out_param.postAssnLevelDeltaDisconnected;
       param->postAssnSelfSteerThreshold = out_param.postAssnSelfSteerThreshold;
       param->postAssnSelfSteerTimeframe = out_param.postAssnSelfSteerTimeframe;
       param->postAssnBackOffTime = out_param.postAssnBackOffTime;
       //param->postAssnSelfSteerBeaconsMissedTime = out_param.postAssnSelfSteerBeaconsMissedTime;
       param->postAssnAPcontrolThresholdLevel = out_param.postAssnAPctrlThreshold;
       param->postAssnAPcontrolTimeframe = out_param.postAssnAPctrlTimeframe;
       param->roaming80211kvrEnable = out_param.roam80211kvrEnable;
       RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s] Successfully set Roaming param- [roamingEnable=%d,preassnBestThreshold=%d,preassnBestDelta=%d]\n", MODULE_NAME,param->roamingEnable,param->preassnBestThreshold,param->preassnBestDelta);
       retStatus = true;
    } else if(status == -1) {
        param->status = ROAM_PARAM_FAILURE;
    } else if(status == -2) {
        param->status = ROAM_PARAM_DISABLED;
    } else
      param->status = ROAM_PARAM_FAILURE;

   return retStatus;
}

bool setRoamingConfigInfo(WiFi_RoamingCtrl_t *param)
{
   int ssidIndex = 0;
   bool retStatus = false;
   wifi_roamingCtrl_t  in_param;
   memset(&in_param,0,sizeof(in_param));

   in_param.roamingEnable = (param->roamingEnable==true?1:0);
   in_param.preassnBestThreshold = param->preassnBestThreshold;
   in_param.preassnBestDelta = param->preassnBestDelta;
   in_param.selfSteerOverride = (param->selfSteerOverride==true?1:0);
   in_param.postAssnLevelDeltaConnected = param->postAssnLevelDeltaConnected;
   in_param.postAssnLevelDeltaDisconnected = param->postAssnLevelDeltaDisconnected;
   in_param.postAssnSelfSteerThreshold = param->postAssnSelfSteerThreshold;
   in_param.postAssnSelfSteerTimeframe = param->postAssnSelfSteerTimeframe;
   in_param.postAssnBackOffTime = param->postAssnBackOffTime;
   //in_param.postAssnSelfSteerBeaconsMissedTime = param->postAssnSelfSteerBeaconsMissedTime;
   in_param.postAssnAPctrlThreshold = param->postAssnAPcontrolThresholdLevel;
   in_param.postAssnAPctrlTimeframe = param->postAssnAPcontrolTimeframe;
   in_param.roam80211kvrEnable = param->roaming80211kvrEnable;

   int status = wifi_setRoamingControl(ssidIndex,&in_param);
   if(status == 0) {
       param->status = ROAM_PARAM_SUCCESS;
       retStatus = true;
       RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s] Successfully set Roaming param- [roamingEnable=%d,preassnBestThreshold=%d,preassnBestDelta=%d]\n", MODULE_NAME,in_param.roamingEnable,in_param.preassnBestThreshold,in_param.preassnBestDelta);
   } else if(status == -1)
        param->status = ROAM_PARAM_FAILURE;
     else if(status == -2)
        param->status = ROAM_PARAM_DISABLED;
     else
        param->status = ROAM_PARAM_FAILURE;

   return retStatus;
}
#endif
bool getRadioStats(WiFi_Radio_Stats_Diag_Params *params)
{
    int radioIndex=0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    wifi_radioTrafficStats_t *trafficStats =(wifi_radioTrafficStats_t *) malloc(sizeof(wifi_radioTrafficStats_t));
    if(trafficStats == NULL)
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s]Malloc Memory allocation failure\n", MODULE_NAME,__FUNCTION__);
        return false;
    }
    printf("malloc allocated = %d ", malloc_usable_size(trafficStats));
    if (wifi_getRadioTrafficStats(radioIndex, trafficStats) == RETURN_OK) {
        params->bytesSent = trafficStats->radio_BytesSent;
        params->bytesReceived  = trafficStats->radio_BytesReceived;
        params->packetsSent = trafficStats->radio_PacketsSent;
        params->packetsReceived = trafficStats->radio_PacketsReceived;
        params->errorsSent = trafficStats->radio_ErrorsSent;
        params->errorsReceived = trafficStats->radio_ErrorsReceived;
        params->discardPacketsSent  = trafficStats->radio_DiscardPacketsSent;
        params->discardPacketsReceived  = trafficStats->radio_DiscardPacketsReceived;
        params->plcErrorCount = trafficStats->radio_DiscardPacketsReceived;
        params->fcsErrorCount = trafficStats->radio_FCSErrorCount;
        params->invalidMACCount = trafficStats->radio_InvalidMACCount;
        params->packetsOtherReceived = trafficStats->radio_PacketsOtherReceived;
        params->noiseFloor = trafficStats->radio_NoiseFloor;

    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] HAL wifi_getRadioTrafficStats FAILURE \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    free(trafficStats);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return true;
}

void logs_Period1_Params()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(NULL != wifiParams_Tele_Period1.paramlist)
    {
        WiFi_EndPoint_Diag_Params endPointInfo;
        getEndPointInfo(&endPointInfo);
        WiFi_Radio_Stats_Diag_Params params;
        getRadioStats(&params);

        GList *iter = g_list_first(wifiParams_Tele_Period1.paramlist);

        while(iter)
        {
            /* Device.WiFi.EndPoint.{i}.Stats.LastDataDownlinkRate*/
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "LastDataDownlinkRate")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, endPointInfo.stats.lastDataDownlinkRate);
            }
            /* Device.WiFi.EndPoint.{i}.Stats.LastDataUplinkRate */
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "LastDataUplinkRate ")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, endPointInfo.stats.lastDataUplinkRate);
            }

            /* Device.WiFi.EndPoint.{i}.Stats.Retransmissions*/
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Retransmissions")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, endPointInfo.stats.retransmissions);
            }

            /*Device.WiFi.Endpoint.{i}.Stats.SignalStrength*/
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "SignalStrength")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%d \n",  (char *)iter->data, endPointInfo.stats.signalStrength);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Channel")) {
                unsigned long output_ulong = 0;
                int radioIndex = 0;
                if (wifi_getRadioChannel(radioIndex, &output_ulong) == RETURN_OK) {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n", (char *)iter->data , output_ulong);
                }
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.FCSErrorCount")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%u \n",  (char *)iter->data, params.fcsErrorCount);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.Noise")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%u \n",  (char *)iter->data, params.noiseFloor);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "TransmitPower"))
            {
                INT output_INT = 0;
                int radioIndex = 0;
                if (wifi_getRadioTransmitPower( radioIndex,  &output_INT) == RETURN_OK) {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%d \n", (char *)iter->data , output_INT);
                }
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.ErrorsReceived")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%u \n",  (char *)iter->data, params.errorsReceived);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.ErrorsSent")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%u \n",  (char *)iter->data, params.errorsSent);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.PacketsReceived")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, params.packetsReceived);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.PacketsSent")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, params.packetsSent);
            }

            iter = g_list_next(iter);
        }

    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}


void logs_Period2_Params()
{
    static bool print_flag = true;
    time_t start_t;
    static time_t lastExec_t;
    double diff_t;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    time(&start_t);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] print_flag : %d\n", __FUNCTION__, __LINE__, print_flag);
    if(print_flag)
    {
        if(NULL != wifiParams_Tele_Period2.paramlist)
        {
            char bssid_string[BUFF_MAX];
            int ssidIndex = 0;
            memset(bssid_string,0, BUFF_MAX);
            char output_string[BUFF_MAX];
            wifi_getBaseBSSID(ssidIndex, bssid_string);

            WiFiConnectionStatus currSsidInfo;
            memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
            get_CurrentSsidInfo(&currSsidInfo);

            GList *iter = g_list_first(wifiParams_Tele_Period2.paramlist);
            while(iter)
            {
                RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d]%s \n",__FUNCTION__, __LINE__, (char *)iter->data);
                /* Device.WiFi.Endpoint.{i}.NumberOfEntries*/
                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "NumberOfEntries")) {
                    int noOfEndpoint = 1;
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%d \n",  (char *)iter->data, noOfEndpoint);
                }

                if((g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.EndPoint.1.Profile.1.SSID"))
                        || (g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.SSID.1.SSID"))
                        || (g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.EndPoint.1.SSIDReference")))
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%s \n",  (char *)iter->data, currSsidInfo.ssidSession.ssid);
                }

                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "BSSID")) {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%s \n",  (char *)iter->data, bssid_string);
                }

                /* Device.WiFi.EndPoint.{i}.Profile.{i}.SSID*/
                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Name")) {
                    memset(output_string,0, BUFF_MAX);
                    if(wifi_getSSIDName(ssidIndex, output_string) == RETURN_OK) {
                        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%s \n",  (char *)iter->data, output_string);
                    }
                }

                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.SSID.1.MACAddress")) {
                    if(gWifiMacAddress[0] != '\0') {
                        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%s \n",  (char *)iter->data, gWifiMacAddress);
                    }
                }

                iter = g_list_next(iter);
            }
        }
    }
    diff_t = difftime(start_t, lastExec_t);

    if (diff_t >= wifiParams_Tele_Period2.timePeriod) {
        time(&lastExec_t);
        print_flag = true;
    }
    else
    {
        print_flag = false;
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}


void readValues(FILE *pFile, char *pToken, char *data)
{
    char buffer[DATA_LEN];
    char *keyValue;
    if(pFile == NULL)
      return;
    /* Search the token in the config txt file and copy the value */
    fseek(pFile, 0, SEEK_SET);
    while (fgets(buffer, sizeof (buffer), pFile) != NULL)
    {
        keyValue = strtok( buffer, "=" );
        if(!(strcmp(keyValue, pToken)))
        {
          keyValue = strtok(NULL, "\n" );
          if(keyValue != NULL)
          {
            strncpy(data, keyValue,strlen(keyValue));
            data[strlen(keyValue)] = '\0'; // Adding '\0' is required if strncpy is used
          }
          break;
        }
    }
}

#ifdef ENABLE_LOST_FOUND
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
/* get lfat from auth service */
int laf_get_lfat(laf_lfat_t *lfat)
{
    long          http_err_code;
    cJSON         *response;
    std::string   str;
    gchar         *retStr;

    str.assign(confProp.wifiProps.getLfatUrl,strlen(confProp.wifiProps.getLfatUrl));

    CurlObject getLfatUrl(str);
    retStr  = getLfatUrl.getCurlData();
    http_err_code = getLfatUrl.gethttpcode();

    if((http_err_code != 200) || (strlen(retStr ) <= 1)) {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "get lfat from auth service failed with error code %d ", http_err_code);
        if(http_err_code == 0)
            http_err_code = 404;
        return http_err_code;
    }

    /* parse json response */
    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "Response from authservice to get lfat %s", retStr);
    response = cJSON_Parse(retStr);
    RDK_ASSERT_NOT_NULL(response);
    RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "version"));
    strcpy(lfat->version, cJSON_GetObjectItem(response, "version")->valuestring);
    RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "expires"));
    lfat->ttl = cJSON_GetObjectItem(response, "expires")->valueint;
    RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "LFAT"));
    lfat->len = strlen(cJSON_GetObjectItem(response, "LFAT")->valuestring);
    lfat->token = (char *) malloc(lfat->len+1);
    if(lfat->token == NULL)
        return ENOMEM;
    strcpy(lfat->token, cJSON_GetObjectItem(response, "LFAT")->valuestring);
    lfat->token[lfat->len] = '\0';
    cJSON_Delete(response);
    return 0;
}
#else // ENABLE_XCAM_SUPPORT or XHB1
int laf_get_lfat(laf_lfat_t *lfat)
{
	RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
	int ret=0;
	unsigned int tokenlength=0;
        FILE *fd = NULL;
        char version[MAX_VERSION_LEN];
        char ttl[MAX_VERSION_LEN];
        long value = 0;
        char *eptr;

	ret = get_token_length(tokenlength);
	if(ret != 0) {
		RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "laf_get_lfat - Error in token read \n");
		return ret;
	}
	lfat->token = (char *) malloc(tokenlength+1);
	if(lfat->token == NULL)
		return ENOMEM;
	ret = get_laf_token(lfat->token,tokenlength);
	lfat->len = tokenlength;
	lfat->token[tokenlength] = '\0';
	RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "laf_get_lfat - token size is - %d\n",lfat->len);
	RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "laf_get_lfat - token is - %s\n",lfat->token);

        //Read lfat version and ttl from /mnt/ramdisk/env/.lnf_conf
        if (0 == access(LFAT_CONF_FILE, F_OK)) {
          memset(version,'\0', MAX_VERSION_LEN);
          memset(ttl, '\0', MAX_VERSION_LEN);
          fd = fopen(LFAT_CONF_FILE, "r");
          if (fd != NULL) {
            readValues(fd, LFAT_VERSION, version);
            if(version != NULL){
              strcpy(lfat->version, version);
            }else{
              strcpy(lfat->version, "1.1");
            }

            readValues(fd, LFAT_TTL, ttl);
            if(ttl != NULL){
              value = strtol(ttl, &eptr, 10);
              lfat->ttl = value;
            }else{
              lfat->ttl = 31536000;
            }
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "lfat_version : %s, lfat_ttl : %s\n",version, ttl);
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "lfat_version : %s, lfat_ttl : %ld\n",lfat->version, lfat->ttl);
            fclose(fd);
          }else{
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "Unable to open lfat_conf file\n");
          }
        }
        else //If .lnf_conf file is not present
        { 
          // Read from netsrvmgr.conf 
          strcpy(lfat->version, confProp.wifiProps.lfatVersion);
          lfat->ttl = confProp.wifiProps.lfatTTL;
          RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "lfat_version : %s, lfat_ttl : %ld\n",lfat->version, lfat->ttl);
        }

	RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
	return ret;
}
#endif // ENABLE_XCAM_SUPPORT

#if !defined (ENABLE_XCAM_SUPPORT) && !defined(XHB1)
/* store lfat with auth service */
int laf_set_lfat(laf_lfat_t* const lfat)
{
    cJSON         *req_payload;
    char          *data;
    long          http_err_code;
    std::string   str;
    gchar         *retStr;

    str.assign(confProp.wifiProps.setLfatUrl,strlen(confProp.wifiProps.setLfatUrl));
    req_payload = cJSON_CreateObject();
    RDK_ASSERT_NOT_NULL(req_payload);
    cJSON_AddStringToObject(req_payload, "version", lfat->version);
    cJSON_AddNumberToObject(req_payload, "expires", lfat->ttl);
    cJSON_AddStringToObject(req_payload, "lfat", lfat->token);

    data = cJSON_Print(req_payload);
    RDK_ASSERT_NOT_NULL(data);
    CurlObject setLfatUrl(str,data);
    retStr  = setLfatUrl.getCurlData();
    http_err_code = setLfatUrl.gethttpcode();

    if(http_err_code != 200) {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,  "set lfat to auth service failed with error code %d message %s", http_err_code);
        cJSON_Delete(req_payload);
        if(data)
            free(data);
        if(http_err_code == 0)
            http_err_code = 404;
        return http_err_code;
    }

    cJSON_Delete(req_payload);
    if(data)
        free(data);

    return 0;
}
#else // ENABLE_XCAM_SUPPORT or XHB1
int laf_set_lfat(laf_lfat_t* const lfat)
{
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
  char* tokenfilename = getlfatfilename();
  char buffer[512];
  memset(buffer, 0, 512);
  if(NULL != lfat->token){
      RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "Updating lfat token after token expiry\n");
      sprintf(buffer,"echo \"%s\" > %s",lfat->token,tokenfilename);
      system(buffer);
  }else {
       RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,  "LFAT Token is Null");
       return -1;
  }
  return 0;
}
#endif // ENABLE_XCAM_SUPPORT or XHB1

bool addSwitchToPrivateResults(int lnfError,char *currTime)
{
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
  WiFiLnfSwitchPrivateResults_t *WiFiLnfSwitchPrivateResultsData = g_new(WiFiLnfSwitchPrivateResults_t,1);
  WiFiLnfSwitchPrivateResultsData->lnfError = lnfError;
  strcpy((char *)WiFiLnfSwitchPrivateResultsData->currTime,currTime);
  lstLnfPvtResults=g_list_append(lstLnfPvtResults,WiFiLnfSwitchPrivateResultsData);
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Time = %s lnf error = %d length of list = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,WiFiLnfSwitchPrivateResultsData->currTime,WiFiLnfSwitchPrivateResultsData->lnfError,g_list_length(lstLnfPvtResults));
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

bool convertSwitchToPrivateResultsToJson(char *buffer)
{
  cJSON *rootObj = NULL, *array_element = NULL, *array_obj = NULL;
  UINT output_array_size = 0;
  INT index;
  char *out = NULL;
  char privateResultsLength=0;
  WiFiLnfSwitchPrivateResults_t *WiFiLnfSwitchPrivateResultsData=NULL;
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
  rootObj = cJSON_CreateObject();
  if(NULL == rootObj) {
    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] \'Failed to create root json object.\n",  MODULE_NAME,__FUNCTION__, __LINE__);
    return false;
  }
  cJSON_AddItemToObject(rootObj, "results", array_obj=cJSON_CreateArray());
  GList *element = g_list_first(lstLnfPvtResults);
  privateResultsLength=g_list_length(lstLnfPvtResults);

  while ((element)&&(privateResultsLength > 0))
  {
    WiFiLnfSwitchPrivateResultsData = (WiFiLnfSwitchPrivateResults_t*)element->data;
    cJSON_AddItemToArray(array_obj,array_element=cJSON_CreateObject());
    cJSON_AddStringToObject(array_element, "timestamp",(const char*)WiFiLnfSwitchPrivateResultsData->currTime);
    cJSON_AddNumberToObject(array_element, "result", WiFiLnfSwitchPrivateResultsData->lnfError);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Time = %s lnf error = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,WiFiLnfSwitchPrivateResultsData->currTime,WiFiLnfSwitchPrivateResultsData->lnfError );
    element = g_list_next(element);
    privateResultsLength--;
  }
  out = cJSON_PrintUnformatted(rootObj);

  if(out) {
    strncpy(buffer, out, strlen(out)+1);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Buffer = %s \n", MODULE_NAME,__FUNCTION__, __LINE__,buffer );
  }
  if(rootObj) {
    cJSON_Delete(rootObj);
  }
  if(out) free(out);
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
  return true;
}
bool clearSwitchToPrivateResults()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if((lstLnfPvtResults) && (g_list_length(lstLnfPvtResults) != 0))
    {
	g_list_free_full(lstLnfPvtResults,g_free);
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] switch to private results list is empty \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}
#endif // ENABLE_LOST_FOUND
#endif // USE_RDK_WIFI_HAL
bool shutdownWifi()
{
    bool result=true;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if ((wifiStatusMonitorThread) && ( pthread_cancel(wifiStatusMonitorThread) == -1 )) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] wifiStatusMonitorThread cancel failed! \n", MODULE_NAME,__FUNCTION__, __LINE__);
        result=false;
    }
#ifdef ENABLE_LOST_FOUND
    if ((lafConnectThread) && (pthread_cancel(lafConnectThread) == -1 )) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] lafConnectThread cancel failed! \n", MODULE_NAME,__FUNCTION__, __LINE__);
        result=false;
    }
    if ((lafConnectToPrivateThread) && (pthread_cancel(lafConnectToPrivateThread) == -1 )) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] lafConnectToPrivateThread failed! \n", MODULE_NAME,__FUNCTION__, __LINE__);
        result=false;
    }
#endif
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Start WiFi Uninitialization \n", MODULE_NAME,__FUNCTION__, __LINE__);
#ifdef USE_RDK_WIFI_HAL
    wifi_uninit();
#endif
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d]  WiFi Uninitialization done \n", MODULE_NAME,__FUNCTION__, __LINE__);
    gWifiAdopterStatus = WIFI_UNINSTALLED;
#ifdef ENABLE_LOST_FOUND
    condLAF = PTHREAD_COND_INITIALIZER;
    mutexLAF = PTHREAD_MUTEX_INITIALIZER;
    gWifiLNFStatus = LNF_UNITIALIZED;
    bLNFConnect=false;
    isLAFCurrConnectedssid=false;
#endif
    wifiStatusLock = PTHREAD_MUTEX_INITIALIZER;
    condGo = PTHREAD_COND_INITIALIZER;
    mutexGo = PTHREAD_MUTEX_INITIALIZER;
    memset(&gSsidList,0,sizeof gSsidList);
    memset(&savedWiFiConnList,0,sizeof savedWiFiConnList);
    memset(&wifiConnData,0,sizeof wifiConnData);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return result;
}


