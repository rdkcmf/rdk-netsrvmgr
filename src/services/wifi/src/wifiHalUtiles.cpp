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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
#include <cJSON.h>

#include "sysMgr.h"
#include <libIBus.h>
#include <libIARMCore.h>

#include "xdiscovery.h"
#endif

#ifdef ENABLE_LOST_FOUND
#include <time.h>
#endif // ENABLE_LOST_FOUND

#ifdef ENABLE_RTMESSAGE
rtConnection con_recv = NULL;
#endif

#ifdef YOCTO_BUILD
extern "C" {
#include "secure_wrapper.h"
}
#endif
#ifdef SAFEC_RDKV
#include "safec_lib.h"
#else
#define STRCPY_S(dest,size,source)                    \
        strcpy(dest, source);
#endif

#include <sys/stat.h>

#define WIFI_HAL_VERSION_SIZE   6
#define MAX_WIFI_STATUS_STRING 32
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
#define MAX_DEVICE_INFO_STRING_LENGTH 32
#endif

static WiFiStatusCode_t gWifiAdopterStatus = WIFI_UNINSTALLED;
static WiFiConnectionTypeCode_t gWifiConnectionType = WIFI_CON_UNKNOWN;
gchar deviceID[DEVICEID_SIZE];
gchar partnerID[PARTNERID_SIZE];
static char wifiHALVer[WIFI_HAL_VERSION_SIZE]={0};
#ifdef ENABLE_LOST_FOUND
GList *lstLnfPvtResults=NULL;
#define WAIT_TIME_FOR_PRIVATE_CONNECTION 2
bool startLAF = false;
pthread_mutex_t mutexLAF = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condLAF = PTHREAD_COND_INITIALIZER;
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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
int apDetailsBuffLength;
bool bsignalAPDetailsReady = true;
pthread_cond_t condAPDetails = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexAPDetails = PTHREAD_MUTEX_INITIALIZER;
pthread_t apDetailsCollectionThread;
char deviceModel[MAX_DEVICE_INFO_STRING_LENGTH] = {0};
char deviceMake[MAX_DEVICE_INFO_STRING_LENGTH] = {0};
char deviceMACAddr[MAX_DEVICE_INFO_STRING_LENGTH] = {0};
#endif
bool bShutdownWifi=false;
pthread_t wifiStatusMonitorThread;
WiFiStatusCode_t getWpaStatus();
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
#define SECURITY_MODE_SAE               "SAE"
#define WIFI_HAL_VERSION                "1.0.0"
#define LFAT_VERSION                    "lfat_version"
#define LFAT_TTL                        "lfat_ttl"
#define LFAT_CONF_FILE                  "/mnt/ramdisk/env/.lnf_conf"
#define DATA_LEN                        4096
#define SCRIPT_FILE_TO_GET_DEVICE_ID    "/lib/rdk/getDeviceId.sh"
#define WIFI_CONNECT_TIME               30

typedef struct _wifi_securityModes
{
    SsidSecurity        securityMode;
    const char          *modeString;
}wifi_securityModes;

wifi_securityModes wifi_securityModesMap_Netapp[] =
{
    { NET_WIFI_SECURITY_NONE,                   "No Security"                   },
    { NET_WIFI_SECURITY_WEP_64,                 "WEP (Open & Shared)"           },
    { NET_WIFI_SECURITY_WPA_PSK_AES,            "WPA-Personal, AES encryp."     },
    { NET_WIFI_SECURITY_WPA_PSK_TKIP,           "WPA-Personal, TKIP encryp."    },
    { NET_WIFI_SECURITY_WPA2_PSK_AES,           "WPA2-Personal, AES encryp."    },
    { NET_WIFI_SECURITY_WPA2_PSK_TKIP,          "WPA2-Personal, TKIP encryp."   },
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_TKIP,    "WPA-ENTERPRISE, TKIP"          },
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_AES,     "WPA-ENTERPRISE, AES"           },
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_TKIP,   "WPA2-ENTERPRISE, TKIP"         },
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES,    "WPA2-ENTERPRISE, AES"          },
    { NET_WIFI_SECURITY_WPA_WPA2_PSK,           "WPA-WPA2-Personal"             },
    { NET_WIFI_SECURITY_WPA_WPA2_ENTERPRISE,    "WPA-WPA2-ENTERPRISE"           },
    { NET_WIFI_SECURITY_WPA3_SAE,               "WPA3"                          },
    { NET_WIFI_SECURITY_WPA3_PSK_AES,           "WPA2-WPA3"                     },
    { NET_WIFI_SECURITY_NOT_SUPPORTED,          "Security format not supported" },
};

wifi_securityModes wifi_securityModesMap[] =
{
    { NET_WIFI_SECURITY_NONE,                   "None"                          },
    { NET_WIFI_SECURITY_WEP_64,                 "WEP"                           },
    { NET_WIFI_SECURITY_WPA_PSK_TKIP,           "WPA"                           },
    { NET_WIFI_SECURITY_WPA_PSK_AES,            "WPA"                           },
    { NET_WIFI_SECURITY_WPA2_PSK_TKIP,          "WPA2"                          },
    { NET_WIFI_SECURITY_WPA2_PSK_AES,           "WPA2"                          },
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_TKIP,    "WPA-ENTERPRISE"                },
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_AES,     "WPA-ENTERPRISE"                },
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_TKIP,   "WPA2-ENTERPRISE"               },
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES,    "WPA2-ENTERPRISE"               },
    { NET_WIFI_SECURITY_WPA_WPA2_PSK,           "WPA-WPA2"                      },
    { NET_WIFI_SECURITY_WPA_WPA2_ENTERPRISE,    "WPA-WPA2-ENTERPRISE"           },
    { NET_WIFI_SECURITY_WPA3_SAE,               "WPA3"                          },
    { NET_WIFI_SECURITY_WPA3_PSK_AES,           "WPA2-WPA3"                     },
    { NET_WIFI_SECURITY_NOT_SUPPORTED,          "Security format not supported" },
};

#ifdef ENABLE_RTMESSAGE
void rtConnection_init()
{
  rtLog_SetLevel(RT_LOG_INFO);
  rtLog_SetOption(rdkLog);
  rtConnection_Create(&con_recv, "NETSRVMGR_WIFI", SOCKET_ADDRESS);
  rtConnection_AddListener(con_recv, "RDKC.WIFI", onMessage, con_recv);
#if defined(XHB1) || defined(XHC3)
  rtConnection_AddListener(con_recv, "RDKC.WIFI.CONNECT", onConnectWifi, con_recv);
#endif
 LOG_INFO("rtConnection_init done");
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
 LOG_INFO("Called onConnectWifi");

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
    LOG_INFO("Received ap_SSID : %s securityMode : %d ap_security_PreSharedKey : %s", ap_SSID, securityMode, ap_security_PreSharedKey);

#ifdef USE_RDK_WIFI_HAL
/*    int status = scan_Neighboring_WifiAP(jbuff);
    jBuffLen = strlen(jbuff);
    LOG_INFO( "[%s] Scan AP's SSID list buffer size : \n\"%d\"", MODULE_NAME, jBuffLen);*/
#endif

    retVal = connect_withSSID(ssidIndex, const_cast<char*>(ap_SSID), (SsidSecurity)6, NULL, const_cast<char*>(ap_security_PreSharedKey), const_cast<char*>(ap_security_PreSharedKey), saveSSID, NULL, NULL, NULL, NULL, WIFI_CON_PRIVATE);

    if (retVal)
    {
      int retry = 0;

      while (false == isWifiConnected() && retry <= WIFI_CONNECT_TIME)
      {
       LOG_INFO("Waiting for wifi connection");
        sleep(1);
        retry++;
      }

      if (true == isWifiConnected())
      {
       LOG_INFO("Wifi connection success");
        retVal = 0;
      }
      else if (retry >= WIFI_CONNECT_TIME)
      {
       LOG_INFO("Wifi connection time out");
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
  LOG_INFO("rtMessage_Receive");
  while (1)
  {
    rtError err = rtConnection_Dispatch(con_recv);
    if (err != RT_OK)
    {
      LOG_DBG("Dispatch Error: %s", rtStrError(err));
    }
    //Adding sleep to avoid logs flooding in case of rtrouted bad state
    usleep(10000);
  }
}
#endif


#ifdef USE_RDK_WIFI_HAL
bool getHALVersion()
{
    if(wifiHALVer[0] == 0)
    {
        wifi_getHalVersion(wifiHALVer);
        LOG_INFO("[%s] WiFi HAL Version is %s",MODULE_NAME, wifiHALVer);
    }
    return true;
}
#endif

SsidSecurity get_wifiSecurityModeFromString(char *secModeString, char *encryptionType)
{
    if (!secModeString || !encryptionType)
    {
        LOG_ERR("[%s] Failed due to NULL.", MODULE_NAME);
        return NET_WIFI_SECURITY_NONE;
    }
    LOG_TRACE("[%s] securitymode = %s encryption = %s",
            MODULE_NAME, secModeString, encryptionType);
    if (strlen(secModeString) == 0 && strlen(encryptionType) == 0)
        return NET_WIFI_SECURITY_NONE;
    if (0 == strcmp(wifiHALVer,WIFI_HAL_VERSION))
    {
        int len = sizeof(wifi_securityModesMap_Netapp)/sizeof(wifi_securityModes);
        for (int i = 0; i < len; i++)
        {
            if (strcasestr(secModeString, wifi_securityModesMap_Netapp[i].modeString))
                return wifi_securityModesMap_Netapp[i].securityMode;
        }
    }
    else
    {
        int len = sizeof(wifi_securityModesMap)/sizeof(wifi_securityModes);
        for (int i = len-1; i >= 0; i--)
        {
            if (strcasestr(secModeString, wifi_securityModesMap[i].modeString))
            {
                if (strcasestr(encryptionType, "AES") || strcasestr(secModeString, "WEP"))
                    return wifi_securityModesMap[i].securityMode;
                else
                    return wifi_securityModesMap[i-1].securityMode;
            }
        }
    }
    return NET_WIFI_SECURITY_NOT_SUPPORTED;
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
     return IARM_RESULT_SUCCESS;
}

bool getMfrData(GString* mfrDataStr,mfrSerializedType_t mfrType)
{
    bool bRet;
    IARM_Bus_MFRLib_GetSerializedData_Param_t param;
    IARM_Result_t iarmRet = IARM_RESULT_IPCCORE_FAIL;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    memset(&param, 0, sizeof(param));
    param.type = mfrType;
    iarmRet = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_GetSerializedData, &param, sizeof(param));
    if(iarmRet == IARM_RESULT_SUCCESS)
    {
        if(param.buffer && param.bufLen)
        {
            LOG_TRACE("[%s] serialized data %s for mfrtype %d", MODULE_NAME, param.buffer,mfrType );
            g_string_assign(mfrDataStr,param.buffer);
            bRet = true;
        }
        else
        {
            LOG_ERR("[%s] serialized data is empty for mfrtype %d ",MODULE_NAME, mfrType );
            bRet = false;
        }
    }
    else
    {
        bRet = false;
        LOG_ERR("[%s] IARM CALL failed  for mfrtype %d",MODULE_NAME, mfrType );
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return bRet;
}
#endif

static void set_WiFiStatusCode( WiFiStatusCode_t status)
{
    pthread_mutex_lock(&wifiStatusLock);
    if(gWifiAdopterStatus != status)
        LOG_TRACE("[%s] Wifi Status changed to %d", MODULE_NAME, status );
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
        STRCPY_S (string, MAX_WIFI_STATUS_STRING, "UNINSTALLED");
        break;
    case WIFI_DISABLED:
        STRCPY_S (string, MAX_WIFI_STATUS_STRING, "DISABLED");
        break;
    case WIFI_DISCONNECTED:
        STRCPY_S (string, MAX_WIFI_STATUS_STRING, "DISCONNECTED");
        break;
    case WIFI_PAIRING:
        STRCPY_S (string, MAX_WIFI_STATUS_STRING, "PAIRING");
        break;
    case WIFI_CONNECTING:
        STRCPY_S (string, MAX_WIFI_STATUS_STRING, "CONNECTING");
        break;
    case WIFI_CONNECTED:
        STRCPY_S (string, MAX_WIFI_STATUS_STRING, "CONNECTED");
        break;
    case WIFI_FAILED:
        STRCPY_S (string, MAX_WIFI_STATUS_STRING, "FAILED");
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    status = get_WiFiStatusCode();
    LOG_TRACE("[%s] Exit with state %d", MODULE_NAME, status );
    return status;
}

void set_WiFiConnectionType( WiFiConnectionTypeCode_t value)
{
    if(value < 0 || value > WIFI_CON_MAX)
    {
        LOG_ERR("[%s] Trying to set invalid Wifi Connection Type %d ", MODULE_NAME, value );
        return;
    }

    if(gWifiConnectionType != value)
        LOG_ERR("[%s] Wifi Connection Type changed to %d ..", MODULE_NAME, value );

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

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
    strncpy(wifiParam.paramName, WIFI_SSID_ENABLE_PARAM, strlen(WIFI_SSID_ENABLE_PARAM) +1);
    wifiParam.instanceNum = 0;
    wifiParam.paramtype = hostIf_BooleanType;

    memset(&gSsidList, '\0', sizeof(ssidList));
    ret = gpvFromTR069hostif(&wifiParam);

    if(ret)
    {
        bool ssidStatus = get_boolean(wifiParam.paramValue);

        LOG_DBG("[%s] Retrieved values from tr69hostif as \'%s\':\'%d\'.", MODULE_NAME, WIFI_SSID_ENABLE_PARAM, ssidStatus);

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
            LOG_INFO("[%s] WiFi is disabled. [\'%s\':%d] ", MODULE_NAME, WIFI_SSID_ENABLE_PARAM, ssidStatus);
            ret = false;
        }
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME );
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

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    /* IARM get Call*/
    ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME, IARM_BUS_TR69HOSTIFMGR_API_GetParams, (void *)param, sizeof(HOSTIF_MsgData_t));

    if(ret != IARM_RESULT_SUCCESS)
    {
        LOG_FATAL("[%s] Failed with returned \'%d\' for '\%s\''.",  MODULE_NAME, ret, param->paramName);
        //ret = ERR_REQUEST_DENIED;
        return false;
    }
    else
    {
        ret = IARM_RESULT_SUCCESS;
        LOG_DBG("[%s] Successfully returned the value of \'%s\'.",  MODULE_NAME, param->paramName);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return true;
}
#endif
#endif /* #ifdef USE_HOSTIF_WIFI_HAL */

void getWpaSsid(WiFiConnectionStatus *curSSIDConnInfo) {
    FILE *in = NULL;
    char buff[512] = {'\0'};

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    if ( WIFI_CONNECTED == getWpaStatus())
    {
        if(!(in = popen("wpa_cli status | grep -i '^ssid' | cut -d = -f 2", "r")))
        {
              LOG_FATAL("[%s] Failed to read wpa_cli status.", MODULE_NAME);
              return;
        }

        if(fgets(buff, sizeof(buff), in)!=NULL)
        {
	    LOG_DBG("[%s] SSID: %s", MODULE_NAME, buff);
	    size_t len = strlen(buff);
	    if (len > 0 && buff[len-1] == '\n') {
	        buff[len-1] = '\0';
	    }
	    strncpy(curSSIDConnInfo->ssidSession.ssid, buff, SSID_SIZE);
	    curSSIDConnInfo->isConnected = true;
        }
        else {
	    LOG_FATAL("[%s] Failed to get SSID.", MODULE_NAME);
        }
        pclose(in);

    }
    else {
        curSSIDConnInfo->isConnected = false;
    }
}

WiFiStatusCode_t getWpaStatus()
{
    WiFiStatusCode_t wpa_state = WIFI_DISCONNECTED; // default to this state
    FILE *in = NULL;
    char buff[512] = {'\0'};

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    if(!(in = popen("wpa_cli status | grep -i wpa_state | cut -d = -f 2", "r")))
    {
        LOG_ERR("[%s] Failed to read wpa_cli status.", MODULE_NAME);
        return wpa_state;
    }

    char* c = fgets(buff, sizeof(buff), in);

    pclose(in);

    if (c == NULL)
    {
        LOG_ERR("[%s] Failed to get State.", MODULE_NAME);
        return wpa_state;
    }

    LOG_INFO("[%s] State: %s", MODULE_NAME, buff);

    // possible wpa_state values (as seen in wpa_supplicant_state_txt() function of wpa_supplicant) are:
    // DISCONNECTED, INACTIVE, INTERFACE_DISABLED, SCANNING,
    // AUTHENTICATING, ASSOCIATING, ASSOCIATED, 4WAY_HANDSHAKE, GROUP_HANDSHAKE,
    // COMPLETED, UNKNOWN
    if (0 == strncasecmp(buff, "SCANNING", strlen("SCANNING")))
    {
        wpa_state = WIFI_PAIRING;
    }
    else if (0 == strncasecmp(buff, "AUTHENTICATING", strlen("AUTHENTICATING")) ||
            0 == strncasecmp(buff, "ASSOCIATING", strlen("ASSOCIATING")) ||
            0 == strncasecmp(buff, "ASSOCIATED", strlen("ASSOCIATED")) ||
            0 == strncasecmp(buff, "4WAY_HANDSHAKE", strlen("4WAY_HANDSHAKE")) ||
            0 == strncasecmp(buff, "GROUP_HANDSHAKE", strlen("GROUP_HANDSHAKE")))
    {
        wpa_state = WIFI_CONNECTING;
    }
    else if (0 == strncasecmp(buff, "COMPLETED", strlen("COMPLETED")))
    {
        wpa_state = WIFI_CONNECTED;
    }
    else if (0 == strncasecmp(buff, "INTERFACE_DISABLED", strlen("INTERFACE_DISABLED")))
    {
        wpa_state = WIFI_DISABLED;
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return wpa_state;
}

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

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    memset(&eventData, 0, sizeof(eventData));

/*    if(WIFI_CONNECTING == get_WiFiStatusCode()) {
        LOG_ERR("[%s] Connecting to AP is in Progress..., please try after 120 seconds.", MODULE_NAME);
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
            LOG_INFO("[%s] Received WPS KeyCode \"%d\";  Successfully called \"wifi_setCliWpsButtonPush(%d)\"; WPS Push button Success. ",\
                     MODULE_NAME, ssidIndex, wpsStatus);
            ret = true;

            /*Generate Event for Connect State
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
            set_WiFiStatusCode(WIFI_CONNECTING);
            wifi_conn_type = WPS_PUSH_CONNECT;*/
        }
        else
        {
            LOG_ERR("[%s] Received WPS KeyCode \"%d\";  Failed in \"wifi_setCliWpsButtonPush(%d)\", WPS Push button press failed with status code (%d) ",
                     MODULE_NAME, ssidIndex, wpsStatus);

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

            LOG_INFO("[%s] Received WPS KeyCode \"%d\"; Already Connected to \"%s\" AP. \"wifi_disconnectEndpointd)\" Successfully on WPS Push. ",\
                     MODULE_NAME, ssidIndex, wifiConnData.ssid, wpsStatus);
            /* Check for status change from Callback function */
            while (start_time < timeout_period)
            {
                if(WIFI_DISCONNECTED == gWifiAdopterStatus)  {
                    LOG_INFO("[%s] Successfully received Disconnect state \"%d\". ", MODULE_NAME, gWifiAdopterStatus);
                    set_WiFiConnectionType(WIFI_CON_WPS);
                    ret = (RETURN_OK == wifi_setCliWpsButtonPush(ssidIndex))?true:false;
                    eventData.data.wifiStateChange.state = (ret)? WIFI_CONNECTING: WIFI_FAILED;
                    set_WiFiStatusCode(eventData.data.wifiStateChange.state);
                    WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
                    LOG_ERR("[%s] Notification on 'onWIFIStateChanged' with state as \'%s\'(%d).", MODULE_NAME,
                             (ret?"CONNECTING": "FAILED"), eventData.data.wifiStateChange.state);
                    if(ret) {
                        remove(WIFI_BCK_FILENAME);
                        wifi_conn_type = WPS_PUSH_CONNECT;
                    }
                    break;
                }
                else {
                    LOG_ERR("[%s] Failed to Disconnect \"%s\";  wait for %d sec (i.e., loop time : %s) to update disconnect state by wifi_disconnect_callback. (%d)",
                             MODULE_NAME, wifiConnData.ssid, MAX_TIME_OUT_PERIOD, ctime(&start_time), gWifiAdopterStatus);
                    sleep(RETRY_TIME_INTERVAL);
                    start_time = time(NULL);
                }
            }
        } else {
            LOG_ERR("[%s] Received WPS KeyCode \"%d\";  Failed in \"wifi_disconnectEndpointd(%d)\", WPS Push button press failed with status code (%d)",
                     MODULE_NAME, ssidIndex, wpsStatus);
        }

        if(false == ret)
        {
            LOG_INFO("[%s] Since failed to disconnect, so reconnecintg again. \"%d\". ", MODULE_NAME);
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

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}

static bool get_WpsPIN(char *wps_pin)
{
    LOG_ENTRY_EXIT;

    bool result = false;
#ifdef ENABLE_IARM
    GString *mfrSerialPin = g_string_new(NULL);
    if (getMfrData(mfrSerialPin, mfrSERIALIZED_TYPE_WPSPIN))
    {
        snprintf(wps_pin, 8+1, "%s", mfrSerialPin->str);
        result = true;
    }
    else
    {
        LOG_ERR("[%s] getting serialized pin from mfr failed.");
    }
    g_string_free(mfrSerialPin, TRUE);
#endif
    return result;
}

//Connect using WPS pin
// if wps_pin = “xxxxxxxx" (or some other condition like "wps_pin not 8 digits")
//     get PIN by making a call to Mfr (or some function that gets a PIN)
// else
//     pass wps_pin to wifihal
//     if wps_pin = “” (this check happens in wifihal)
//         have wpa_supplicant auto-generate PIN
//     else
//         have wpa_supplicant use the passed wps_pin
bool connect_WpsPin(char *wps_pin)
{
    LOG_ENTRY_EXIT;

    if (!wps_pin)
        return false;

    LOG_DBG("[%s] wps_pin = %s", wps_pin);

    if (0 == strcmp(wps_pin, "xxxxxxxx") && false == get_WpsPIN(wps_pin))
        return false;

    if (WIFI_CONNECTED == get_WiFiStatusCode())
    {
        wifi_disconnectEndpoint(1, wifiConnData.ssid);
        usleep(100000);
        for (int i = 1; i <= 30 && WIFI_CONNECTED == get_WiFiStatusCode(); i++)
            usleep(100000);
    }

    set_WiFiConnectionType(WIFI_CON_WPS);

    INT wpsStatus = wifi_setCliWpsEnrolleePin(1, wps_pin);
    if (RETURN_OK == wpsStatus)
    {
        LOG_INFO("wifi_setCliWpsEnrolleePin called successfully");
        return true;
    }
    else
    {
        LOG_ERR("wifi_setCliWpsEnrolleePin returned %d", wpsStatus);
#ifdef ENABLE_IARM
        IARM_BUS_WiFiSrvMgr_EventData_t eventData;
        memset(&eventData, 0, sizeof(eventData));
        eventData.data.wifiStateChange.state = WIFI_FAILED;
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,
                IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged,
                (void*) &eventData, sizeof(eventData));
#endif
        return false;
    }
}

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
void _evtHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
   LOG_TRACE("[%s] Enter..");
    if (strcmp(owner, IARM_BUS_SYSMGR_NAME) == 0)
    {
        switch(eventId)
        {
            case IARM_BUS_SYSMGR_EVENT_XUPNP_DATA_UPDATE:
            {
                IARM_Bus_SYSMgr_EventData_t *eventData = (IARM_Bus_SYSMgr_EventData_t*)data;
                pthread_mutex_lock(&mutexAPDetails);
                bsignalAPDetailsReady = true;
                if(0 == pthread_cond_signal(&condAPDetails))
                {
                    apDetailsBuffLength = eventData->data.xupnpData.deviceInfoLength;
                    LOG_INFO("[%s] Signal to fetch the data from upnp  %d", MODULE_NAME, eventData->data.xupnpData.deviceInfoLength );
                }
                pthread_mutex_unlock(&mutexAPDetails);
                break;
            }
        }
        LOG_TRACE("[%s] Exit...", MODULE_NAME);
    }
}

void initializeDiscovery(void)
{
    IARM_Bus_RegisterEventHandler(IARM_BUS_SYSMGR_NAME,IARM_BUS_SYSMGR_EVENT_XUPNP_DATA_UPDATE, _evtHandler);
}

void shutdownDiscovery(void)
{
    IARM_Bus_UnRegisterEventHandler(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_EVENT_XUPNP_DATA_UPDATE);
}
#endif

INT wifi_connect_callback(INT ssidIndex, CHAR *AP_SSID, wifiStatusCode_t *connStatus)
{
    int ret = RETURN_OK;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    wifiStatusCode_t connCode = *connStatus;
    wifi_status_action (connCode, AP_SSID, (unsigned short) ACTION_ON_CONNECT);
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}

INT wifi_disconnect_callback(INT ssidIndex, CHAR *AP_SSID, wifiStatusCode_t *connStatus)
{
    int ret = RETURN_OK;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    wifiStatusCode_t connCode = *connStatus;
    wifi_status_action (connCode, AP_SSID, (unsigned short)ACTION_ON_DISCONNECT);
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return ret;
}

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
bool parseAPDetails(char *array)
{
    guint counter = 0;
    bool retVal = TRUE;
    LOG_TRACE("[%s] Enter..", MODULE_NAME);

    if(array)
    {
        cJSON *rootJson = cJSON_Parse(array);
        if(rootJson)
        {
            cJSON *deviceFullData = cJSON_GetObjectItem(rootJson, "xmediagateways");

            if(deviceFullData)
            {
                guint deviceCount = cJSON_GetArraySize(deviceFullData);
                LOG_INFO("gateway count in json %d", deviceCount);
                char *devType, *devModel, *devMake, *devMACAddr;

                for (counter = 0; counter < deviceCount; counter++)
                {
                    cJSON *deviceData = cJSON_GetArrayItem(deviceFullData, counter);
                    devType = devModel = devMake = devMACAddr = NULL;

                    if(deviceData) {
                        if(cJSON_GetObjectItem(deviceData, "DevType"))
                            devType = cJSON_GetObjectItem(deviceData, "DevType")->valuestring;

                        if(g_strrstr(g_strstrip(devType),"XB"))
                        {
                            LOG_INFO("[%s] Device type  %s Device count %d", MODULE_NAME, devType, counter);
                            if (cJSON_GetObjectItem(deviceData, "modelClass"))
                                devModel = cJSON_GetObjectItem(deviceData, "modelClass")->valuestring;
                            if(cJSON_GetObjectItem(deviceData, "make"))
                                devMake = cJSON_GetObjectItem(deviceData, "make")->valuestring;
                            if(cJSON_GetObjectItem(deviceData, "bcastMacAddress"))
                                devMACAddr = cJSON_GetObjectItem(deviceData, "bcastMacAddress")->valuestring;

                            if(devModel)
                            {
                                STRCPY_S(deviceModel, sizeof(deviceModel), devModel);
                            }

                            if(devMake)
                            {
                                STRCPY_S(deviceMake, sizeof(deviceMake), devMake);
                            }

                            if(devMACAddr)
                            {
                                STRCPY_S(deviceMACAddr, sizeof(deviceMACAddr), devMACAddr);
                            }

                            LOG_INFO("[%s] Device Model: %s Device Make: %s MAC Addr: %s", MODULE_NAME, deviceModel, deviceMake, deviceMACAddr);
                            break;
                        }
                        else
                        {
                           LOG_ERR("[%s] not a valid XB device %s", MODULE_NAME, devType);
                        }
                    } else {
                        LOG_ERR("[%s] Failed to get XB details", MODULE_NAME);
                    }
                }
            }
            else
            {
                retVal = FALSE;
                LOG_INFO("[%s] No AP Details to parse gwcount", MODULE_NAME);
            }
            cJSON_Delete(rootJson);
        }
        else
        {
             retVal = FALSE;
             LOG_INFO("JSON is empty");
        }
    }
    else
    {
        LOG_ERR("[%s] Device list is empty", MODULE_NAME);
        retVal = FALSE;
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return retVal;
}

bool getAPDetails(unsigned int msgLength)
{
    IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t *param = NULL;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool returnStatus = FALSE;

    char apDetails[msgLength + 1];
    memset(&apDetails, 0, sizeof(apDetails));

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    ret = IARM_Malloc(IARM_MEMTYPE_PROCESSLOCAL, sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t) + msgLength + 1, (void**)&param);
    if(ret == IARM_RESULT_SUCCESS)
    {
        param->bufLength = msgLength;

        ret = IARM_Bus_Call(_IARM_XUPNP_NAME,IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo,
                            (void *)param, sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t) + msgLength + 1);

        if(ret == IARM_RESULT_SUCCESS)
        {
            MEMCPY_S(apDetails, msgLength + 1, ((char *)param + sizeof(IARM_Bus_SYSMGR_GetXUPNPDeviceInfo_Param_t)), param->bufLength);
            apDetails[param->bufLength] = '\0';
            returnStatus =  TRUE;
        }
        else
        {
            LOG_ERR("[%s] IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo IARM failed in the fetch ", MODULE_NAME);
        }

        LOG_TRACE("[%s] apDetails %s", MODULE_NAME, apDetails);
        IARM_Free(IARM_MEMTYPE_PROCESSLOCAL, param);

        /* Parse the UPnP device information */
        returnStatus = parseAPDetails(apDetails);
    }
    else
    {
        LOG_ERR("[%s] IARM_BUS_XUPNP_API_GetXUPNPDeviceInfo , IARM_Malloc Failed ", MODULE_NAME);
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return returnStatus;
}

void* getAPMakeAndModel(void *arg)
{
    bool ret = 0;
    int msgLength;

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    while (!bShutdownWifi) {
        pthread_mutex_lock(&mutexAPDetails);
        while(bsignalAPDetailsReady == false) {
            pthread_cond_wait(&condAPDetails, &mutexAPDetails);
        }

        LOG_DBG("[%s] ***** Started fetching device data from upnp msg len = %d  ***** ", MODULE_NAME, apDetailsBuffLength);

        msgLength = apDetailsBuffLength;
        bsignalAPDetailsReady = false;
        ret = getAPDetails(msgLength);
        pthread_mutex_unlock(&mutexAPDetails);

        if(ret && strlen(deviceModel) > 0 && strlen(deviceMake) > 0)
        {
            shutdownDiscovery();
            break;
        }
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return NULL;
}

void collectAPDetails(void)
{
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&apDetailsCollectionThread, &attr, getAPMakeAndModel, NULL);
}
#endif

void wifi_status_action (wifiStatusCode_t connCode, char *ap_SSID, unsigned short action)
{
    const char *connStr = (action == ACTION_ON_CONNECT)?"Connect": "Disconnect";
    char command[128]= {'\0'};
    static unsigned int switchLnf2Priv=0;
    static unsigned int switchPriv2Lnf=0;
    bool retVal = false;
    bool sameSSid = true;
    int radioIndex = 0;
    wifi_sta_stats_t stats;
#ifdef ENABLE_IARM
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_MAX;
    bool notify = false;
    int xreReconnectReason=0;
    memset(&eventData, 0, sizeof(eventData));
#endif

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    if ((strcmp(savedWiFiConnList.ssidSession.ssid, "") != 0) && (strcmp(savedWiFiConnList.ssidSession.ssid, ap_SSID) != 0))
    {
        sameSSid = false;
    }
    switch(connCode) {
    case WIFI_HAL_SUCCESS:
        LOG_INFO("[%s] Successfully %s to AP %s.", MODULE_NAME, connStr, ap_SSID);
        if (ACTION_ON_CONNECT == action) {
            set_WiFiStatusCode(WIFI_CONNECTED);
#ifdef ENABLE_IARM
            notify = true;
#endif
#ifdef ENABLE_LOST_FOUND
            if (! laf_is_lnfssid(ap_SSID))
#endif //ENABLE_LOST_FOUND
            {
                LOG_INFO("[%s] savedWiFiConnList.ssidSession.ssid = %s ap_SSID = %s",MODULE_NAME, savedWiFiConnList.ssidSession.ssid,ap_SSID);
                if (strcmp (savedWiFiConnList.ssidSession.ssid, ap_SSID) != 0)
                {
                    storeMfrWifiCredentials();
#ifdef ENABLE_LOST_FOUND
                    //bounce xre connection if we move from one private ssid to another ssid
                    LOG_INFO("[%s] Moving from one SSID = %s  to another SSID = %s", MODULE_NAME,savedWiFiConnList.ssidSession.ssid,ap_SSID);
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
                    xreReconnectReason=20; //RECONNECT_REASON_WIFI_NETWORK_CHANGE
                    if(false == setHostifParam(XRE_REFRESH_SESSION_WITH_RR ,hostIf_IntegerType ,(void *)&xreReconnectReason))
                    {
                        LOG_ERR("refresh xre session failed ."); 
                    }
#endif // ENABLE_XCAM_SUPPORT, XHC3 and XHB1 not enabled
#ifdef ENABLE_NLMONITOR
                   char const* intface=getenv("WIFI_INTERFACE");
                   if(intface != NULL)
                   {
                       std::string ifcStr(intface);
                       //cleanup Global IPv6s assigned.
                       NetLinkIfc::get_instance()->deleteinterfaceip(ifcStr,AF_INET6);
                       NetLinkIfc::get_instance()->deleteinterfaceroutes(ifcStr,AF_INET6);
                       LOG_INFO("Clearing ipv6 ip and routes .");
                       struct stat stat_buf;
                       if (stat("/tmp/ani_wifi", &stat_buf) == 0) // if active network interface = WIFI
                       {
#ifdef YOCTO_BUILD
                           v_secure_system("/lib/rdk/enableIpv6Autoconf.sh %s &", ifcStr.c_str());
#else
                           std::string enableV6 = "/lib/rdk/enableIpv6Autoconf.sh ";
                           enableV6 += ifcStr;  //adding interface
                           enableV6 += " &";
                           system(enableV6.c_str());
#endif
                       }
                   }
                   else
                       LOG_ERR("No WiFi interface .");
#endif
#endif //ENABLE_LOST_FOUND
                }
            }
#ifdef ENABLE_IARM
            notify = true;
#if 0       /* Do not bounce for any network switch. Do it only from LF to private. */
            if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
            {
                LOG_ERR("[%s] refresh xre session failed .", MODULE_NAME);
            }
#endif
#endif
            /* one condition variable is signaled */
#ifndef ENABLE_XCAM_SUPPORT
            LOG_INFO("[%s] Trigger DHCP lease for new connection", MODULE_NAME);
            if (access("/opt/persistent/ip.wifi.0", F_OK ) != 0)
            {
#if !defined(XHB1) && !defined(XHC3)
                if (!sameSSid)
                {
                    LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:wifi_addr_reset.sh");
#ifdef YOCTO_BUILD
                    v_secure_system("/bin/sh /lib/rdk/wifi_addr_reset.sh");
#else
                    system("/bin/sh /lib/rdk/wifi_addr_reset.sh");
#endif
                }
                else
#endif
                {
                    LOG_INFO( "TELEMETRY_WIFI_CONNECTION_STATUS:triggerDhcpReleaseAndRenew");
                    netSrvMgrUtiles::triggerDhcpReleaseAndRenew(getenv("WIFI_INTERFACE"));
                }
            }
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
                    LOG_INFO("Enable LNF");
                    bStopLNFWhileDisconnected=false;
                }
                retVal = lastConnectedSSID(&savedWiFiConnList);
                if(!retVal)
                    LOG_ERR("[%s] Last connected ssid fetch failure ", MODULE_NAME);
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
                if(!switchPriv2Lnf)
                    switchPriv2Lnf=1;
                if(switchLnf2Priv)
                {
                    LOG_INFO("[%s] Connecting private ssid. Bouncing xre connection.", MODULE_NAME);
                    xreReconnectReason = 21; // RECONNECT_REASON_WIFI_NETWORK_CHANGE_LNF_TO_PRIVATE
                    if(false == setHostifParam(XRE_REFRESH_SESSION_WITH_RR ,hostIf_IntegerType ,(void *)&xreReconnectReason))
                    {
                        LOG_ERR("refresh xre session failed .");
                    }
                    switchLnf2Priv=0;
                }
                LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:CONNECTED,%s", ap_SSID);
                memset(&stats, 0, sizeof(wifi_sta_stats_t));
                wifi_getStats(radioIndex, &stats);

                if(!sameSSid)
                {
                    memset(&deviceModel, 0, sizeof(deviceModel));
                    memset(&deviceMake, 0, sizeof(deviceMake));
                    memset(&deviceMACAddr, 0, sizeof(deviceMACAddr));
                    collectAPDetails();
                    initializeDiscovery();
                }

                if(!strlen(deviceModel) && !strlen(deviceMake))
                {
                    LOG_INFO("TELEMETRY_WIFI_STATS:%s,%s,%d,%d,%d,%d,%d,%s", stats.sta_SSID, stats.sta_BSSID, (int)stats.sta_PhyRate, (int)stats.sta_Noise, (int)stats.sta_RSSI, (int)stats.sta_LastDataDownlinkRate, (int)stats.sta_LastDataUplinkRate, stats.sta_BAND);
                }
                else
                {
                    LOG_INFO("TELEMETRY_WIFI_STATS:%s,%s,%d,%d,%d,%d,%d,%s,%s,%s,%s", stats.sta_SSID, stats.sta_BSSID, (int)stats.sta_PhyRate, (int)stats.sta_Noise, (int)stats.sta_RSSI, (int)stats.sta_LastDataDownlinkRate, (int)stats.sta_LastDataUplinkRate, stats.sta_BAND, deviceMake, deviceModel, deviceMACAddr);
                }
#endif // ENABLE_XCAM_SUPPORT, XHC3 and XHB1 not enabled
            }
            else {
                LOG_TRACE("[%s] This is a LNF SSID so no storing", MODULE_NAME);
                isLAFCurrConnectedssid=true;
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
                if(!switchLnf2Priv)
                    switchLnf2Priv=1;
                if(switchPriv2Lnf)
                {
                    switchPriv2Lnf=0;
                }
#endif // ENABLE_XCAM_SUPPORT, XHC3 and XHB1 not enabled
                setLNFState(CONNECTED_LNF);
            }

#endif //  ENABLE_LOST_FOUND
            /*Write into file*/
//            WiFiConnectionStatus wifiParams;

            /*Generate Event for Connect State*/
#ifdef ENABLE_IARM
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTED;
            LOG_DBG("[%s] Notification on 'onWIFIStateChanged' with state as \'CONNECTED\'(%d).", MODULE_NAME, WIFI_CONNECTED);
#endif
            while(bShutdownWifi!=true){
                LOG_INFO("[%s] attempting Mutex try Lock ", MODULE_NAME );
                if(0== pthread_mutex_trylock(&mutexGo)){
                    if(0 == pthread_cond_signal(&condGo)) {
                        LOG_INFO("[%s] Broadcast to monitor.", MODULE_NAME );
                    }
                    pthread_mutex_unlock(&mutexGo);
                    break;
                }
                else{
                    sleep(1);/*wait for another thread to release lock also check bShutdownWifi*/
                }
            }
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
            LOG_DBG("[%s] Notification on 'onWIFIStateChanged' with state as \'DISCONNECTED\'(%d).", MODULE_NAME, WIFI_DISCONNECTED);
#endif
	        LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_DISCONNECTED");
        }
        break;
    case WIFI_HAL_CONNECTING:
            LOG_INFO("[%s] Connecting to AP in Progress...", MODULE_NAME);
            set_WiFiStatusCode(WIFI_CONNECTING);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
            LOG_DBG("[%s] Notification on 'onWIFIStateChanged' with state as \'CONNECTING\'(%d).", MODULE_NAME, WIFI_CONNECTING);
#endif
        break;

    case WIFI_HAL_DISCONNECTING:
        if(connCode_prev_state != connCode) {
#ifdef ENABLE_IARM
            notify = true;
            LOG_INFO("[%s] Disconnecting from AP in Progress...", MODULE_NAME);
#endif
        }
        break;
    case WIFI_HAL_ERROR_NOT_FOUND:
        if(connCode_prev_state != connCode) {
#ifdef ENABLE_IARM
            notify = true;
            LOG_ERR("[%s] Failed in %s with wifiStatusCode %d (i.e., AP not found). ", MODULE_NAME, connStr, connCode);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_NO_SSID;
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'NO_SSID\'(%d), CurrentState set as %d (DISCONNECTED)", \
                     MODULE_NAME,eventId,  eventData.data.wifiError.code, WIFI_DISCONNECTED);
#endif
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_LOST_FOUND
            /* Event Id & Code */
            if(confProp.wifiProps.bEnableLostFound)
            {
                bPrivConnectionLost=true;
                lnfConnectPrivCredentials();
            }
#endif // ENABLE_LOST_FOUND
        }
        break;

    case WIFI_HAL_ERROR_TIMEOUT_EXPIRED:
        LOG_ERR("[%s] Failed in %s with wifiStatusCode %d (i.e., Timeout expired). ", MODULE_NAME, connStr, connCode);
        /* Event Id & Code */
        if(connCode_prev_state != connCode) {
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_UNKNOWN;
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'FAILED\'(%d).", \
                     MODULE_NAME,eventId,  eventData.data.wifiError.code);
#endif
            set_WiFiStatusCode(WIFI_DISCONNECTED);
        }
        break;

    case WIFI_HAL_ERROR_DEV_DISCONNECT:
        if(connCode_prev_state != connCode) {
#ifdef ENABLE_IARM
            notify = true;
            LOG_ERR("[%s] Failed in %s with wifiStatusCode %d (i.e., Fail in Device/AP Disconnect).", MODULE_NAME, connStr, connCode);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'CONNECTION_LOST\'(%d).", \
                     MODULE_NAME, eventId,  eventData.data.wifiError.code);
#endif
            set_WiFiStatusCode(WIFI_DISCONNECTED);
        }
        break;
    /* the SSID of the network changed */
    case WIFI_HAL_ERROR_SSID_CHANGED:
        if(connCode_prev_state != connCode) {
            LOG_ERR("[%s] Failed due to SSID Change (%d) . ", MODULE_NAME, connCode);
#ifdef USE_TELEMETRY_2_0
            telemetry_event_d("WIFIV_ERR_HAL_SSIDChngd", 1);
#endif // #ifdef USE_TELEMETRY_2_0

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
            eventId = IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged;
            LOG_INFO("[%s] Notification on 'onSSIDsChanged (%d)'.", MODULE_NAME, eventId);

            //Two events need to be generated in this switch case, hence calling WiFi_IARM_Bus_BroadcastEvent() for IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged here itself
            WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,(IARM_EventId_t) eventId,(void *)&eventData,sizeof(eventData));
            LOG_INFO("[%s] Broadcast Event id \'%d\'.", MODULE_NAME, eventId);

            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_SSID_CHANGED;
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'SSID_CHANGED\'(%d).", \
                     MODULE_NAME, eventId,  eventData.data.wifiError.code);
#endif
            LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_SSID_CHANGED");
        }
        break;
    /* the connection to the network was lost */
    case WIFI_HAL_ERROR_CONNECTION_LOST:
        if(connCode_prev_state != connCode) {
            LOG_ERR("[%s] Failed due to  CONNECTION LOST (%d) from %s.", MODULE_NAME, connCode, ap_SSID);
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'CONNECTION_LOST\'(%d).", \
                     MODULE_NAME,eventId,  eventData.data.wifiError.code);
#endif
	    LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_LOST");
        }
        break;
    /* the connection failed for an unknown reason */
    case WIFI_HAL_ERROR_CONNECTION_FAILED:
        if(connCode_prev_state != connCode)
        {
            LOG_ERR("[%s] Connection Failed (%d) due to unknown reason.. ", MODULE_NAME, connCode );
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_FAILED;
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'CONNECTION_FAILED\'(%d).", \
                     MODULE_NAME,eventId,  eventData.data.wifiError.code);
#endif
	    LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_FAILED");
        }
        break;
    /* the connection was interrupted */
    case WIFI_HAL_ERROR_CONNECTION_INTERRUPTED:
        if(connCode_prev_state != connCode) {
            LOG_ERR("[%s] Failed due to Connection Interrupted (%d).", MODULE_NAME, connCode );
            set_WiFiStatusCode(WIFI_DISCONNECTED);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_INTERRUPTED;
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'CONNECTION_INTERRUPTED\'(%d).", \
                     MODULE_NAME,eventId,  eventData.data.wifiError.code);
#endif
            LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_INTERRUPTED");
        }
        break;
    /* the connection failed due to invalid credentials */
    case WIFI_HAL_ERROR_INVALID_CREDENTIALS:
            LOG_ERR("[%s] Failed due to Invalid Credentials. (%d). ", MODULE_NAME, connCode );
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
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'INVALID_CREDENTIALS\'(%d)", \
                     MODULE_NAME,eventId,  eventData.data.wifiError.code);
#endif
            LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_INVALID_CREDENTIALS");
        break;
    /* the connection failed due to authentication failure */
    case WIFI_HAL_ERROR_AUTH_FAILED:
            LOG_ERR("[%s] Failed due to Auth Failure (%d)", MODULE_NAME, connCode);
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
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'AUTH_FAILED\'(%d).",
                    MODULE_NAME, eventId, eventData.data.wifiError.code);
#endif
            LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_AUTH_FAILED");
        break;
    case WIFI_HAL_UNRECOVERABLE_ERROR:
        if(connCode_prev_state != connCode) {
            LOG_ERR("[%s] Failed due to UNRECOVERABLE ERROR. (%d).", MODULE_NAME, connCode );
            set_WiFiStatusCode(WIFI_FAILED);
#ifdef ENABLE_IARM
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_FAILED;
            LOG_ERR("[%s] Notification on 'onWIFIStateChanged (%d)' with state as \'FAILED\'(%d).", \
                     MODULE_NAME, eventId,  eventData.data.wifiStateChange.state);
#endif
	    LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_UNRECOVERABLE_ERROR");
        }
        break;
    case WIFI_HAL_ERROR_UNKNOWN:
    default:
        if(connCode_prev_state != connCode) {
            LOG_ERR("[%s] Failed in %s with WiFi Error Code %d (i.e., WIFI_HAL_ERROR_UNKNOWN).", MODULE_NAME, connStr, connCode );
#ifdef ENABLE_IARM
            notify = true;
            /* Event Id & Code */
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_UNKNOWN;
            LOG_ERR("[%s] Notification on 'onError (%d)' with state as \'UNKNOWN\'(%d).", \
                     MODULE_NAME, eventId,  eventData.data.wifiError.code);
#endif
	    LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_UNKNOWN");
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
        LOG_INFO("[%s] Broadcast Event id \'%d\'. ", MODULE_NAME, eventId);
    }
#endif
    LOG_TRACE("[%s] Exit", MODULE_NAME );
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

    LOG_TRACE("[%s] Enter", MODULE_NAME);
    set_WiFiConnectionType((WiFiConnectionTypeCode_t)conType);
    securityMode=(wifiSecurityMode_t)ap_security_mode;
    LOG_INFO("[%s] ssidIndex %d; ap_SSID : %s; ap_security_mode : %d; saveSSID = %d",
             MODULE_NAME, ssidIndex, ap_SSID, (int)ap_security_mode, saveSSID );
    if(saveSSID)
    {
        set_WiFiStatusCode(WIFI_CONNECTING);
#ifdef ENABLE_IARM
        eventData.data.wifiStateChange.state = WIFI_CONNECTING;
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
#endif
    }
    LOG_DBG("[%s] connecting to ssid %s with passphrase %s ",
            MODULE_NAME, ap_SSID, ap_security_KeyPassphrase);
    ret=wifi_connectEndpoint(ssidIndex, ap_SSID, securityMode, ap_security_WEPKey, ap_security_PreSharedKey, ap_security_KeyPassphrase, saveSSID,eapIdentity,carootcert,clientcert,privatekey);
    if(ret)
    {
        LOG_ERR("[%s] Error in connecting to ssid %s",
                 MODULE_NAME, ap_SSID);
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
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return ret;
}

bool scan_SpecificSSID_WifiAP(char *buffer, const char* SSID, double freq_in)
{
    LOG_TRACE("[%s] Enter..", MODULE_NAME);
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

    LOG_INFO("[%s] wifi_get wifi_getSpecificSSIDInfo returned %d, neighbor_ap_array_size %d",
                    MODULE_NAME, ret, ssid_ap_array_size);

    if ((RETURN_OK != ret ) && ((NULL == ssid_ap_array) || (0 == ssid_ap_array_size)))
        return false;

    cJSON *rootObj = cJSON_CreateObject();
    if (NULL == rootObj) {
        LOG_ERR("[%s] \'Failed to create root json object.",  MODULE_NAME);
        return false;
    }

    cJSON *array_obj = NULL, *array_element = NULL;
    char *ssid = NULL;
    int signalStrength = 0;
    double frequency = 0;
    SsidSecurity encrptType = NET_WIFI_SECURITY_NONE;

    cJSON_AddItemToObject(rootObj, "getAvailableSSIDsWithName", array_obj=cJSON_CreateArray());

    LOG_DBG( "\n*********** Start: SSID Scan List **************** \n");
    for (int index = 0; index < ssid_ap_array_size; index++ )
    {
        ssid = ssid_ap_array[index].ap_SSID;
        if (*ssid && (0 != strcmp (ssid, LNF_NON_SECURE_SSID)) && (0 != strcmp (ssid, LNF_SECURE_SSID)))
        {
            signalStrength = ssid_ap_array[index].ap_SignalStrength;
            frequency = strtod(ssid_ap_array[index].ap_OperatingFrequencyBand, NULL);

           LOG_DBG( "[%s] [%d] => SSID: %s | SignalStrength: %d | Frequency: %f | EncryptionMode: %s",
                             MODULE_NAME, index, ssid, signalStrength, frequency, ssid_ap_array[index].ap_EncryptionMode );

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
   LOG_DBG( "\n***********End: SSID Scan List **************** \n\n");

    bool result = false;
    cJSON *ret_json = NULL;
    char *out = cJSON_PrintUnformatted(rootObj);
    if(out)
    {
        /* Parse and check the SSID Scan List is valid JSON */
        if (NULL == (ret_json = cJSON_Parse (out)))
        {
            LOG_ERR("scan_SpecificSSID_WifiAP query returned non-JSON value after cJSON_parse = [%s]. Error = [%s]",out, cJSON_GetErrorPtr());
            result = false;
        }
        else
        {
            strncpy(buffer, out, strlen(out)+1);
            LOG_DBG("[%s] Out = %s", MODULE_NAME,out);
            LOG_DBG("[%s] Buffer = %s", MODULE_NAME,buffer);
            result = true;
            cJSON_Delete(ret_json);
        }
        free(out);
    }
    else
    {
        LOG_ERR("scan_SpecificSSID_WifiAP query return SSID List is NULL");
        result = false;
    }

    if(rootObj) cJSON_Delete(rootObj);

    if(ssid_ap_array)
    {
        LOG_DBG("[%s] malloc allocated = %d ", MODULE_NAME, malloc_usable_size(ssid_ap_array));
        free(ssid_ap_array);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return result;
#else    // ENABLE_XCAM_SUPPORT
  LOG_TRACE("[%s] Exit", MODULE_NAME);
  return false;
#endif   // ENABLE_XCAM_SUPPORT
}

bool scan_Neighboring_WifiAP(char *buffer)
{
    LOG_TRACE("[%s] Enter..", MODULE_NAME);
#ifdef ENABLE_LOST_FOUND

    if((isWifiConnected()) && (isLAFCurrConnectedssid == true) && (RETURN_OK != wifi_disconnectEndpoint(1, wifiConnData.ssid)))
    {
        LOG_ERR("Failed to  Disconnect in wifi_disconnectEndpoint()");
    }
#endif //ENABLE_LOST_FOUND
    wifi_neighbor_ap_t *neighbor_ap_array = NULL;
    UINT neighbor_ap_array_size = 0;
    bool ret = wifi_getNeighboringWiFiDiagnosticResult (0, &neighbor_ap_array, &neighbor_ap_array_size);

    LOG_INFO("[%s] wifi_getNeighboringWiFiDiagnosticResult returned %d, neighbor_ap_array_size %d",
            MODULE_NAME, ret, neighbor_ap_array_size);

    if ((RETURN_OK != ret ) && ((NULL == neighbor_ap_array) || (0 == neighbor_ap_array_size)))
        return false;

    cJSON *rootObj = cJSON_CreateObject();
    if (NULL == rootObj) {
        LOG_ERR("[%s] \'Failed to create root json object.",  MODULE_NAME);
        return false;
    }

    cJSON *array_obj = NULL, *array_element = NULL;
    char *ssid = NULL;
    int signalStrength = 0;
    double frequency = 0;
    SsidSecurity encrptType = NET_WIFI_SECURITY_NONE;

    cJSON_AddItemToObject(rootObj, "getAvailableSSIDs", array_obj=cJSON_CreateArray());

    LOG_DBG( "\n*********** Start: SSID Scan List **************** \n");
    for (int index = 0; index < neighbor_ap_array_size; index++ )
    {
        ssid = neighbor_ap_array[index].ap_SSID;
        if (*ssid && (0 != strcmp (ssid, LNF_NON_SECURE_SSID)) && (0 != strcmp (ssid, LNF_SECURE_SSID)))
        {
            signalStrength = neighbor_ap_array[index].ap_SignalStrength;
            frequency = strtod(neighbor_ap_array[index].ap_OperatingFrequencyBand, NULL);

            LOG_DBG("[%s] [%d] => SSID: %s | SignalStrength: %d | Frequency: %f | EncryptionMode: %s",
                     MODULE_NAME, index, ssid, signalStrength, frequency, neighbor_ap_array[index].ap_EncryptionMode );

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
    LOG_DBG( "\n***********End: SSID Scan List **************** \n\n");

    bool result = false;
    cJSON *ret_json = NULL;
    char *out = cJSON_PrintUnformatted(rootObj);

    if(out)
    {
        /* Parse and check the SSID Scan List is valid JSON */
        if (NULL == (ret_json = cJSON_Parse (out)))
        {
            LOG_ERR("scan_Neighboring_WifiAP query returned non-JSON value after cJSON_parse = [%s]. Error = [%s]",out, cJSON_GetErrorPtr());
            result = false;
        }
        else
        {
            strncpy(buffer, out, strlen(out)+1);
            LOG_DBG("[%s] Out = %s", MODULE_NAME,out);
            LOG_DBG("[%s] Buffer = %s", MODULE_NAME,buffer);
            result = true;
            cJSON_Delete(ret_json);
        }
        free(out);
    }
    else
    {
        LOG_ERR("scan_Neighboring_WifiAP query return SSID List is NULL");
        result = false;
    }

    if(rootObj) {
        cJSON_Delete(rootObj);
    }

    if(neighbor_ap_array) {
        LOG_DBG( "[%s] malloc allocated = %d ", MODULE_NAME, malloc_usable_size(neighbor_ap_array));
        free(neighbor_ap_array);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return result;
}

bool lastConnectedSSID(WiFiConnectionStatus *ConnParams)
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    bool ret = false;
    wifi_pairedSSIDInfo_t pairedSSIDInfo = {0};
    if (RETURN_OK != wifi_lastConnected_Endpoint (&pairedSSIDInfo))
    {
        LOG_ERR("[%s] Error in getting last connected SSID ", MODULE_NAME);
    }
    else
    {
        snprintf (ConnParams->ssidSession.ssid, sizeof(ConnParams->ssidSession.ssid), "%s", pairedSSIDInfo.ap_ssid);
        snprintf (ConnParams->ssidSession.passphrase, sizeof(ConnParams->ssidSession.passphrase), "%s", pairedSSIDInfo.ap_passphrase);
        snprintf (ConnParams->ssidSession.bssid, sizeof(ConnParams->ssidSession.bssid), "%s", pairedSSIDInfo.ap_bssid);
        snprintf (ConnParams->ssidSession.security, sizeof(ConnParams->ssidSession.security), "%s", pairedSSIDInfo.ap_security);
        snprintf (ConnParams->ssidSession.security_WEPKey, sizeof(ConnParams->ssidSession.security_WEPKey), "%s", pairedSSIDInfo.ap_wep_key);
        if ((strstr (ConnParams->ssidSession.security, SECURITY_MODE_WPA_PSK) != NULL ) ||
            (strstr (ConnParams->ssidSession.security, SECURITY_MODE_SAE) != NULL ))
        {
            LOG_TRACE("[%s] SECURITY_MODE_SAE", MODULE_NAME);
            ConnParams->ssidSession.security_mode=NET_WIFI_SECURITY_WPA3_SAE;//Same security_mode for both wpa2 and wpa3
        }
        else if (strcmp (ConnParams->ssidSession.security, SECURITY_MODE_WPA_EAP) == 0)
        {
            LOG_TRACE("[%s] SECURITY_MODE_WPA_EAP", MODULE_NAME);
            ConnParams->ssidSession.security_mode=NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES;
        }
        else if (ConnParams->ssidSession.security_WEPKey[0]!=NULL)
        {
           LOG_TRACE("[%s] security_WEPKey is %s", MODULE_NAME, ConnParams->ssidSession.security_WEPKey );
           if ( strlen(ConnParams->ssidSession.security_WEPKey) == 26 || strlen(ConnParams->ssidSession.security_WEPKey) == 13 ){
              ConnParams->ssidSession.security_mode = NET_WIFI_SECURITY_WEP_128;
           }
           else{
               ConnParams->ssidSession.security_mode = NET_WIFI_SECURITY_WEP_64;
           }
        }
        else
        {
            LOG_TRACE("[%s] SECURITY_MODE_WPA_NONE", MODULE_NAME);
            ConnParams->ssidSession.security_mode=NET_WIFI_SECURITY_NONE;
        }
        LOG_TRACE("[%s] last connected  ssid is  %s passphrase is %s  bssid is %s security is %s security_mode is %d",
                MODULE_NAME, ConnParams->ssidSession.ssid, ConnParams->ssidSession.passphrase,
                ConnParams->ssidSession.bssid, ConnParams->ssidSession.security,ConnParams->ssidSession.security_mode);
        ret = true;
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    wifi_sta_stats_t stats;
    WiFiStatusCode_t wifiStatusCode;
    char wifiStatusAsString[MAX_WIFI_STATUS_STRING];
    int radioIndex = 0;
    struct timespec condGoTimeout;

    pthread_mutex_lock(&mutexGo);
    pthread_cond_wait(&condGo, &mutexGo);
    pthread_mutex_unlock(&mutexGo);

    LOG_DBG("[%s] ***** Monitor activated by signal ***** ", MODULE_NAME);

    while (!bShutdownWifi) {
        wifiStatusCode = get_WiFiStatusCode();

        if (get_WiFiStatusCodeAsString (wifiStatusCode, wifiStatusAsString)) {
            LOG_INFO("TELEMETRY_WIFI_CONNECTION_STATUS:%s", wifiStatusAsString);
        }
        else {
            LOG_ERR("TELEMETRY_WIFI_CONNECTION_STATUS:Unmappable WiFi status code %d", wifiStatusCode);
        }

        if (WIFI_CONNECTED == wifiStatusCode) {
            //LOG_INFO( "\n *****Start Monitoring ***** \n");
#if !defined (XHB1) && !defined (XHC3)
            memset(&stats, 0, sizeof(wifi_sta_stats_t));
            wifi_getStats(radioIndex, &stats);

#if !defined(ENABLE_XCAM_SUPPORT)
            if((strlen(deviceModel) > 0) && (strlen(deviceMake) > 0))
            {
                LOG_INFO("TELEMETRY_WIFI_STATS:%s,%s,%d,%d,%d,%d,%d,%s,%s,%s,%s", stats.sta_SSID, stats.sta_BSSID, (int)stats.sta_PhyRate, (int)stats.sta_Noise, (int)stats.sta_RSSI, (int)stats.sta_LastDataDownlinkRate, (int)stats.sta_LastDataUplinkRate, stats.sta_BAND, deviceMake, deviceModel, deviceMACAddr);
            }
            else
#endif
            {
                LOG_INFO("TELEMETRY_WIFI_STATS:%s,%s,%d,%d,%d,%d,%d,%s", stats.sta_SSID, stats.sta_BSSID, (int)stats.sta_PhyRate, (int)stats.sta_Noise, (int)stats.sta_RSSI, (int)stats.sta_LastDataDownlinkRate, (int)stats.sta_LastDataUplinkRate, stats.sta_BAND);
            }
#endif
            //LOG_INFO( "\n *****End Monitoring  ***** \n");

            /*Telemetry Parameter logging*/
            logs_Period1_Params();
            logs_Period2_Params();
        }

        pthread_mutex_lock(&mutexGo);
        clock_gettime(CLOCK_REALTIME, &condGoTimeout);
        condGoTimeout.tv_sec += confProp.wifiProps.statsParam_PollInterval;
        ret = 0;
        while (!bShutdownWifi && ret != ETIMEDOUT) {
            ret = pthread_cond_timedwait(&condGo, &mutexGo, &condGoTimeout);
        }
        pthread_mutex_unlock(&mutexGo);
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return NULL;
}

void monitor_WiFiStatus()
{
    bShutdownWifi = false;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
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
        LOG_ERR("[%s] Failed to  Disconnect in wifi_disconnectEndpoint() for AP : \"%s\".",\
                 MODULE_NAME, ap_ssid);
        ret = false;
    }
    else
    {
       LOG_INFO("[%s] Successfully called \"wifi_disconnectEndpointd()\" for AP: \'%s\'.  ",\
                MODULE_NAME, ap_ssid);
#ifndef ENABLE_XCAM_SUPPORT
        // Clear wpa_supplicant SSID info
        if(wifi_clearSSIDInfo(radioIndex) == RETURN_OK) {
          LOG_INFO("[%s] Successfully cleared SSID info", MODULE_NAME);
        }
#endif // ENABLE_XCAM_SUPPORT


#ifdef ENABLE_LOST_FOUND
        if ( false == isLAFCurrConnectedssid )
#endif //ENABLE_LOST_FOUND
        {
            memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
            eraseMfrWifiCredentials();
        }
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
        LOG_ERR("[%s] Failed to  Disconnect in wifi_disconnectEndpoint() for AP : \"%s\".",\
                 MODULE_NAME, ap_ssid);
        ret = false;
    }
    else
    {
       LOG_INFO("[%s] Successfully called \"wifi_disconnectEndpointd()\" for AP: \'%s\'.  ",\
                MODULE_NAME, ap_ssid);
    }
    return ret;
}

bool cancelWPSPairingOperation()
{
    bool ret = true;
#if !defined (ENABLE_XCAM_SUPPORT) && !defined (XHB1) && !defined(XHC3)
    if(wifi_cancelWpsPairing() == RETURN_OK)
    {
       LOG_INFO("[%s] Successfully Cancelled WPS operation.", MODULE_NAME);
    }
    else
    {
        LOG_ERR("[%s] Failed to Cancel WPS operation, Looks like no in-progress wps operation.", MODULE_NAME);
        ret = false;
    }
#endif //ENABLE_LOST_FOUND
    return ret;
}

void getConnectedSSIDInfo(WiFiConnectedSSIDInfo_t *conSSIDInfo)
{
    int radioIndex = 0;
    wifi_sta_stats_t stats;

    LOG_TRACE("[%s] Enter", MODULE_NAME);

    memset(&stats, '\0', sizeof(stats));

    wifi_getStats(radioIndex, &stats);
    strncpy((char *)conSSIDInfo->ssid, (const char *)stats.sta_SSID, (size_t)SSID_SIZE);
    strncpy((char *)conSSIDInfo->bssid, (const char *)stats.sta_BSSID, (size_t)BSSID_BUFF);
    conSSIDInfo->rate = stats.sta_PhyRate;
    conSSIDInfo->noise = stats.sta_Noise;
    conSSIDInfo->signalStrength = stats.sta_RSSI;
    conSSIDInfo->frequency = stats.sta_Frequency;
    strncpy((char *)conSSIDInfo->band,(const char *)stats.sta_BAND,(size_t)BUFF_MIN);
    conSSIDInfo->securityMode = get_wifiSecurityModeFromString(stats.sta_SecMode, stats.sta_Encryption);

    LOG_DBG("[%s] Connected SSID info: \n \
            [SSID: \"%s\"| BSSID : \"%s\" | PhyRate : \"%f\" | Noise : \"%f\" | SignalStrength(rssi) : \"%f\" | Frequency : \"%d\" Security Mode : \"%d\"] \n",
            MODULE_NAME, stats.sta_SSID, stats.sta_BSSID, stats.sta_PhyRate, stats.sta_Noise, stats.sta_RSSI,conSSIDInfo->frequency,conSSIDInfo->securityMode);

    LOG_TRACE("[%s] Exit", MODULE_NAME );
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

        LOG_INFO( "[%s] last requested backoff = %fs, elapsed after last lnf server contact = %fs, waiting for remaining %fs",
                MODULE_NAME, last_laf_status.backoff, seconds_elapsed_after_last_lnf_server_contact, seconds_remaining_to_wait);

        usleep (seconds_remaining_to_wait * 1000000); // wait out the remainder of the requested backoff period
    }
    else
    {
       LOG_INFO("[%s] backoff = 0s", MODULE_NAME);
    }

   LOG_INFO("[%s] backoff wait done.", MODULE_NAME);
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

    LOG_TRACE("[%s] Enter", MODULE_NAME);
#ifdef ENABLE_IARM
    IARM_Bus_MFRLib_GetSerializedData_Param_t param;
#endif
    ops = (laf_ops_t*) malloc(sizeof(laf_ops_t));
    if(!ops)
    {
        LOG_ERR("[%s] laf_ops malloc failed ", MODULE_NAME);
        return ENOMEM;
    }
    dev_info = (laf_device_info_t*) malloc(sizeof(laf_device_info_t));
    if(!dev_info)
    {
        LOG_ERR("[%s] laf_device_info malloc failed ", MODULE_NAME);
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
        LOG_ERR("[%s] getDeviceInfo failed", MODULE_NAME);
    }
    /* configure laf */
    err = laf_config_new(&conf, ops, dev_info);
    if(err != 0) {
        LOG_ERR("[%s] Lost and found client configurtion failed with error code %d ", MODULE_NAME, err );
        bRet=false;
    }
    else
    {
        /* initialize laf client */
        err = laf_client_new(&clnt, conf);
        if(err != 0) {
            LOG_ERR("[%s] Lost and found client initialization failed with error code %d", MODULE_NAME, err );
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
                LOG_INFO("[%s] lnf server requested backoff = %fs",
                        MODULE_NAME, last_laf_status.backoff);
                clock_gettime(CLOCK_MONOTONIC, &last_lnf_server_contact_time);
            }

            if(err != 0) {
                LOG_ERR("[%s] Error in lost and found client, error code %d", MODULE_NAME, err );
                bRet=false;
            }
            if(!bAutoSwitchToPrivateEnabled)
            {
                if((!bRet) && (bLastLnfSuccess))
                {
                    LOG_INFO("[%s] !AutoSwitchToPrivateEnabled + lastLnfSuccessful + currLnfFailure = clr switch2prv Results ", MODULE_NAME);
                    clearSwitchToPrivateResults();
                    bLastLnfSuccess=false;
                }
                if(bRet)
                    bLastLnfSuccess=true;
                netSrvMgrUtiles::getCurrentTime(currTime,(char *)TIME_FORMAT);
                if (addSwitchToPrivateResults(err,currTime) != 0) {
                   LOG_INFO( "[%s] Added Time=%s lnf error=%d to list,length of list = %d", MODULE_NAME,currTime,err,g_list_length(lstLnfPvtResults));
                }
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

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return bRet;
}
bool getmacaddress(gchar* ifname,GString *data)
{
    int fd;
    int ioRet;
    struct ifreq ifr;
    unsigned char *mac;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        LOG_ERR("[%s] ERROR opening socket %d ", MODULE_NAME,fd );
        return false;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name , ifname , IFNAMSIZ-1);
    ioRet=ioctl(fd, SIOCGIFHWADDR, &ifr);
    if (ioRet < 0)
    {
        close(fd);
        LOG_ERR("[%s] ERROR in ioctl function to retrieve mac address %d", MODULE_NAME, fd );
        return false;
    }
    close(fd);
    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    g_string_printf(data,"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return true;
}

#ifdef ENABLE_IARM
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
        LOG_ERR("[%s] Partner ID Missing in url May be not activated ", MODULE_NAME);
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
        LOG_TRACE("[%s] mfrSerialNum = %s mfrType = %d", MODULE_NAME,mfrSerialNum->str,mfrType );
        STRCPY_S(dev_info->serial_num, sizeof(dev_info->serial_num), mfrSerialNum->str);
    }
    else
    {
        LOG_ERR("[%s] getting serial num from mfr failed ", MODULE_NAME);
        bRet = false;
        goto out;
    }
    mfrType = mfrSERIALIZED_TYPE_DEVICEMAC;
    if(getMfrData(deviceMacAddr,mfrType))
    {
        LOG_TRACE("[%s] deviceMacAddr = %s mfrType = %d ", MODULE_NAME, deviceMacAddr->str,mfrType);
        STRCPY_S(dev_info->mac, sizeof(dev_info->mac), deviceMacAddr->str);

    }
    else
    {
        LOG_ERR("[%s] getting device mac addr from mfr failed ", MODULE_NAME);
        bRet = false;
        goto out;
    }
    mfrType = mfrSERIALIZED_TYPE_MODELNAME;
    if(getMfrData(mfrModelName,mfrType))
    {
        LOG_TRACE("[%s] mfrModelName = %s mfrType = %d ", MODULE_NAME, mfrModelName->str,mfrType );
        STRCPY_S(dev_info->model, sizeof(dev_info->model), mfrModelName->str);
    }
    else
    {
        LOG_ERR("[%s] getting model name from mfr failed ", MODULE_NAME);
        bRet = false;
        goto out;
    }

    LOG_TRACE("[%s] dummy auth token update ", MODULE_NAME);
    memset(dev_info->auth_token, 0, MAX_AUTH_TOKEN_LEN+1);
    STRCPY_S(dev_info->auth_token, sizeof(dev_info->auth_token), "xact_token");

out:
    g_string_free(mfrSerialNum,TRUE);
    g_string_free(mfrMake,TRUE);
    g_string_free(mfrModelName,TRUE);
    g_string_free(deviceMacAddr,TRUE);

    return bRet;
}

#else // ENABLE_IARM

#if defined (ENABLE_XCAM_SUPPORT) || defined (XHB1) || defined(XHC3)
char* getlfatfilename()
{
    static char *lfat_file = NULL;
    lfat_file =  getenv("LFAT_TOKEN_FILE");
    LOG_INFO("lfat file name=%s", lfat_file);
    return lfat_file;
}

int get_token_length(unsigned int &tokenlength)
{
    LOG_ENTRY_EXIT;
    FILE* fp_token;
    int tokensize;
    char* tokenfilename = getlfatfilename();
    fp_token = fopen(tokenfilename,"rb");
    if(NULL == fp_token) {
        LOG_ERR("get_token_length - Error in opening lfat token file");
        return EBADF;
    }else{
        fseek(fp_token, 0L, SEEK_END);
        tokensize = ftell(fp_token);
        fseek(fp_token, 0, SEEK_SET);
        tokenlength = tokensize;
        LOG_DBG("get_token_length - token size is - %d",tokenlength);
        if(NULL != fp_token){
            fclose(fp_token);
            fp_token = NULL;
        }
    }
    return 0;
}

int get_laf_token(char* token,unsigned int tokenlength)
{
    LOG_ENTRY_EXIT;
    char* tokenfilename = getlfatfilename();
    FILE* fp_token;
    fp_token = fopen(tokenfilename,"rb");
    if(NULL == fp_token) {
        LOG_ERR("get_laf_token - Error in opening lfat token file");
        return EBADF;
    }else{
        if(NULL != token)
        {
            fread(token,tokenlength,1,fp_token);
        }else {
            LOG_ERR("get_laf_token - Token NULL");
        }
        if(NULL != fp_token){
            fclose(fp_token);
            fp_token = NULL;
        }
    }
    return 0;
}
#endif // ENABLE_XCAM_SUPPORT or XHB1 or XHC3

#if defined(XHB1) || defined(XHC3)
int getMfrDeviceInfo(mfrSerializedType_t stdatatype, char* data)
{
        mfrSerializedData_t stdata = {NULL, 0, NULL};

        if(NULL == data)
        {
                LOG_ERR("[%s] Accessing ivalid memory locatio", MODULE_NAME);
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
                LOG_ERR("[%s] Error retriving required device info.", MODULE_NAME);
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
    char* partnerIdFile = "/opt/usr_config/partnerid.txt";
#ifdef ENABLE_XCAM_SUPPORT
    struct basic_info xcam_dev_info;
    memset((void*)&xcam_dev_info, 0, sizeof(struct basic_info));
    if (!rdkc_get_device_basic_info(&xcam_dev_info))
    {
        strncpy(dev_info->serial_num, xcam_dev_info.serial_number, MAX_DEV_SERIAL_LEN-1);
        strncpy(dev_info->mac, xcam_dev_info.mac_addr, MAX_DEV_MAC_LEN);
        strncpy(dev_info->model, xcam_dev_info.model, MAX_DEV_MODEL_LEN);
        LOG_INFO("[%s] serial_no=%s", MODULE_NAME, dev_info->serial_num);
    }
    else
    {
        LOG_ERR("[%s] rdkc_get_device_basic_info failed", MODULE_NAME);
        return ret;
    }
#elif defined(XHB1) || defined(XHC3)
    if(1 == getMfrDeviceInfo(mfrSERIALIZED_TYPE_SERIALNUMBER, dev_info->serial_num))
    {
       LOG_ERR("[%s] Error reteiving device serial number.", MODULE_NAME);
    }
    if(1 == getMfrDeviceInfo(mfrSERIALIZED_TYPE_MODELNAME, dev_info->model))
    {
       LOG_ERR("[%s] Error reteiving device model", MODULE_NAME);
    }

    if(1 == getMfrDeviceInfo(mfrSERIALIZED_TYPE_DEVICEMAC, dev_info->mac))
    {
      LOG_ERR("[%s] Error reteiving device MAC.", MODULE_NAME);
    }
#endif
#if defined(ENABLE_XCAM_SUPPORT) || defined(XHB1) || defined(XHC3)
    int get_result = getSetFileContent(partnerIdFile, "get", dev_info->partnerId);
    if (get_result)
      LOG_ERR("[%s] Error reteiving partnerId", MODULE_NAME);
    else
    {
      dev_info->partnerId[strcspn(dev_info->partnerId, "\n")] = 0;
      LOG_DBG("[%s] PartnerID obtained is : %s", MODULE_NAME, dev_info->partnerId);
    }
    LOG_TRACE( "[%s] auth token update for xcam", MODULE_NAME);
    memset(dev_info->auth_token, 0, MAX_AUTH_TOKEN_LEN+1);
    readtoken = get_token_length(tokenlength);
    if(readtoken != 0) {
        LOG_DBG("getDeviceInfo - Error in token read ");
        return false;
    }
    readtoken = get_laf_token(dev_info->auth_token,tokenlength);
    if(readtoken != 0 ) {
        LOG_ERR("[%s] getDeviceInfo - Token read error", MODULE_NAME);
        return false;
    }
    LOG_DBG("[%s] getDeviceInfo - token is : %s", MODULE_NAME, dev_info->auth_token);
    ret = true;
#endif // ENABLE_XCAM_SUPPORT, XHC3 and XHB1 defined
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

    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if (laf_is_lnfssid(wificred->ssid) && (true == isLAFCurrConnectedssid) && (WIFI_CONNECTED == get_WifiRadioStatus()))
    {
        LOG_INFO("[%s] Already connected to LF ssid Ignoring the request", MODULE_NAME);
        return 0;
    }
    if(bStopLNFWhileDisconnected)
    {
        LOG_INFO("[%s] Already WPS flow has intiated so skipping the request the request", MODULE_NAME);
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
           LOG_INFO("[%s] Lnf response status Credentials changed : BOTH", MODULE_NAME);
          }
          else if(strcmp(wificred->ssid, savedWiFiConnList.ssidSession.ssid) != 0)
          {
           LOG_INFO("[%s] Lnf response status Credentials changed : SSID", MODULE_NAME);
          }
          else if(strcmp(wificred->passphrase, savedWiFiConnList.ssidSession.passphrase) != 0)
          {
           LOG_INFO("[%s] Lnf response status Credentials changed : KeyPassphrase", MODULE_NAME);
          }
        }
    }
#ifdef USE_RDK_WIFI_HAL
    if(gWifiLNFStatus != CONNECTED_PRIVATE)
    {
         setLNFState(LNF_IN_PROGRESS);
         if((bLNFConnect == true) && (RETURN_OK != wifi_disconnectEndpoint(1, wifiConnData.ssid)))
         {
            LOG_ERR("Failed to  Disconnect in wifi_disconnectEndpoint()");
         }
        retVal=connect_withSSID(ssidIndex, wificred->ssid, (SsidSecurity)wificred->security_mode, NULL, wificred->psk, wificred->passphrase, (int)(!bLNFConnect),wificred->identity,wificred->carootcert,wificred->clientcert,wificred->privatekey,bLNFConnect ? WIFI_CON_LNF : WIFI_CON_PRIVATE);
    }
        //    retVal=connect_withSSID(ssidIndex, wificred->ssid, NET_WIFI_SECURITY_NONE, NULL, NULL, wificred->psk);
//  }
#endif
    if(false == retVal)
    {
        LOG_ERR("[%s] connect with ssid %s failed ", MODULE_NAME, wificred->ssid);
        LOG_DBG("[%s] connect with ssid %s failed (passphrase %s)", MODULE_NAME, wificred->ssid, wificred->passphrase );
        return EPERM;
    }
    while(get_WifiRadioStatus() != WIFI_CONNECTED && retry <= confProp.wifiProps.lnfRetryCount) {
        retry++; //max wait for 180 seconds
        if(bStopLNFWhileDisconnected)
        {
            LOG_INFO( "[%s] Already WPS flow has intiated so skipping the request the request", MODULE_NAME);
            return EPERM;
        }
        sleep(1);
        LOG_TRACE("[%s] Device not connected to wifi. waiting to connect...", MODULE_NAME);
    }
    if(retry > confProp.wifiProps.lnfRetryCount) {
        LOG_ERR("[%s] Waited for %d seconds. Failed to connect. Abort ", MODULE_NAME, confProp.wifiProps.lnfRetryCount);
        return EPERM;
    }

    /* Bounce xre session only if switching from LF to private */
    if(! laf_is_lnfssid(wificred->ssid))
    {
        setLNFState(CONNECTED_PRIVATE);
        notify = true;
//        sleep(10); //waiting for default route before bouncing the xre connection
//        LOG_INFO( "[%s] Connecting private ssid. Bouncing xre connection.", MODULE_NAME);
//#ifdef ENABLE_IARM
//        if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
//        {
//            LOG_ERR("[%s] refresh xre session failed ", MODULE_NAME);
//        }
//#endif
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return 0;
}

/* Callback function to disconenct wifi */
int laf_wifi_disconnect(void)
{
    int retry = 0;
    bool retVal=false;
    int radioIndex = 1;

    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if ( false == isLAFCurrConnectedssid )
    {
        LOG_INFO("[%s] Connected to private ssid Ignoring the request ", MODULE_NAME);
        return 0;
    }
    if(bStopLNFWhileDisconnected)
    {
        LOG_INFO("[%s] Already WPS flow has intiated so skipping the request the request", MODULE_NAME);
        return EPERM;
    }
#ifdef USE_RDK_WIFI_HAL
    char *ap_ssid = savedWiFiConnList.ssidSession.ssid;
    retVal = wifi_disconnectEndpoint(radioIndex, ap_ssid);
#endif
    setLNFState(LNF_IN_PROGRESS);
    if(retVal == false) {
        LOG_ERR("[%s] Tried to disconnect before connect and it failed ", MODULE_NAME);
        return EPERM;
    }

    while(get_WifiRadioStatus() != WIFI_DISCONNECTED && retry <= 30) {
        if(bStopLNFWhileDisconnected)
        {
            LOG_INFO("[%s] Already WPS flow has intiated so skipping the request the request", MODULE_NAME);
            return EPERM;
        }
        LOG_TRACE("[%s] Device is connected to wifi. waiting to disconnect...", MODULE_NAME);
        retry++; //max wait for 180 seconds
        sleep(1);
    }
    if(retry > 30) {
        LOG_ERR("[%s] Waited for 30seconds. Failed to disconnect. Abort", MODULE_NAME);
        return EPERM;
    }

    LOG_TRACE("[%s] Exit", MODULE_NAME );
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
        LOG_TRACE("[%s] LNF Status changed to %d", MODULE_NAME, status );

    gWifiLNFStatus = status;
}

static void signalStartLAF()
{
    pthread_mutex_lock(&mutexLAF);
    startLAF = true;
    LOG_DBG("[%s] Signal to start LAF private SSID ", MODULE_NAME);
    pthread_cond_signal(&condLAF);
    pthread_mutex_unlock(&mutexLAF);
}

void *lafConnPrivThread(void* arg)
{
    int counter=0;
    bool retVal;
    int ssidIndex=1;
    LAF_REQUEST_TYPE reqType;
    bool lnfReturnStatus=false;

    LOG_TRACE("[%s] Enter",MODULE_NAME);

    while (true) {
        pthread_mutex_lock(&mutexLAF);
        // if already connected to private wifi, suppress any start LAF signal that was previously issued
        if (gWifiLNFStatus == CONNECTED_PRIVATE && gWifiAdopterStatus == WIFI_CONNECTED)
            startLAF = false;
        while (false == startLAF)
            pthread_cond_wait(&condLAF, &mutexLAF);
        startLAF = false;
        pthread_mutex_unlock(&mutexLAF);
        {
           LOG_DBG("[%s] Starting the LAF Connect private SSID",MODULE_NAME);
            if(gWifiLNFStatus != CONNECTED_LNF)
            {
                LOG_INFO("[%s] WifiLNFStatus = %d .Setting LNF state as in progress.",MODULE_NAME, gWifiLNFStatus );
                setLNFState(LNF_IN_PROGRESS);
            }
            if(bPrivConnectionLost)
            {
                bPrivConnectionLost=false;
                LOG_INFO("[%s] Wait for 10 sec before starting lost and found ", MODULE_NAME );
                sleep(10);
            }
            do
            {
#ifdef ENABLE_XCAM_SUPPORT
                if(rdkc_get_wifi_interface_status() == 3)
                {
                  LOG_INFO("Camera is connected to Y-CABLE. Aborting LNF ");
                  setLNFState(LNF_UNITIALIZED);
                  break;
                }
#endif
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
                    LOG_INFO("stopLNFWhileDisconnected pressed coming out of LNF");
                    setLNFState(LNF_UNITIALIZED);
                    break;
                }

                if((gWifiAdopterStatus == WIFI_CONNECTED) && (false == isLAFCurrConnectedssid))
                {
                    LOG_INFO("connection status %d is LAF current ssid %d ", gWifiAdopterStatus, isLAFCurrConnectedssid );
                    setLNFState(CONNECTED_PRIVATE);
                    break;
                }

                bIsStopLNFWhileDisconnected=false;
                pthread_mutex_lock(&mutexTriggerLAF);
                doLnFBackoff();
                if ((gWifiLNFStatus == CONNECTED_PRIVATE) && (gWifiAdopterStatus == WIFI_CONNECTED))
                {
                   LOG_INFO("[%s] Connected to Private SSID already. Aborting requested LNF.", MODULE_NAME);
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
                        LOG_INFO("stopLNFWhileDisconnected pressed coming out of LNF" );
                        setLNFState(LNF_UNITIALIZED);
                        break;
                    }
//                  counter++;
                    retVal=false;
                    retVal=lastConnectedSSID(&savedWiFiConnList);
                    if (retVal && savedWiFiConnList.ssidSession.ssid[0] != '\0')
                    {
                        LOG_INFO("[%s] Trying Manual Connect with ssid %s security %d since not able to connect through LNF",MODULE_NAME, savedWiFiConnList.ssidSession.ssid,savedWiFiConnList.ssidSession.security_mode);
                        retVal=connect_withSSID(ssidIndex, savedWiFiConnList.ssidSession.ssid,savedWiFiConnList.ssidSession.security_mode, NULL, savedWiFiConnList.ssidSession.passphrase, savedWiFiConnList.ssidSession.passphrase,true,NULL,NULL,NULL,NULL,WIFI_CON_PRIVATE);
                        if(false == retVal)
                        {
                            LOG_ERR("[%s] connect with ssid %s  failed ", MODULE_NAME, savedWiFiConnList.ssidSession.ssid );
                        }
                    }
                    else
                    {
                        LOG_ERR("[%s] Failed to get previous SSID, Manual connect failed. ", MODULE_NAME);
                    }
                    if (last_laf_status.backoff == 0.0f)
                    {
                        last_laf_status.backoff = (float)confProp.wifiProps.lnfRetryInSecs;
                        LOG_INFO( "[%s] Added default lnf back off time %fs ",MODULE_NAME,last_laf_status.backoff );
                    }
                }
                else
                {
                    if (LAF_REQUEST_CONNECT_TO_LFSSID == reqType)
                    {
                        setLNFState(CONNECTED_LNF);
                        LOG_TRACE("[%s] Connected to LNF",MODULE_NAME);
                    }
                    else
                    {
                        if (gWifiLNFStatus == CONNECTED_PRIVATE)
                        {
                            setLNFState(CONNECTED_PRIVATE);
                            LOG_INFO("pressed coming out of LNF since box is connected to private ");
                        }
                    }
                    break;
                }
                sleep(1);
            }while ((gWifiLNFStatus != CONNECTED_PRIVATE) && (bAutoSwitchToPrivateEnabled));
            bIsStopLNFWhileDisconnected=true;
        }
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
}
void *lafConnThread(void* arg)
{
    int ret = 0;
    bool lnfReturnStatus=false;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
                    LOG_INFO("[%s] Connected through non LAF path", MODULE_NAME);
                    bIsStopLNFWhileDisconnected=true;
                    bLnfActivationLoop=false;
                    return NULL;
                }
                break;
            }
            if (true == bStopLNFWhileDisconnected)
            {
                LOG_INFO("[%s] stopLNFWhileDisconnected pressed coming out of LNF", MODULE_NAME);
                bIsStopLNFWhileDisconnected=true;
                bLnfActivationLoop=false;
                setLNFState(LNF_UNITIALIZED);
                return NULL;
            }
#if defined(ENABLE_XCAM_SUPPORT) || defined(XHB1) || defined(XHC3)
            int readtoken=0;
            unsigned int tokenlength=0;
            readtoken = get_token_length(tokenlength);
            if(readtoken != 0) {
                LOG_ERR("[%s] LNF TOKEN ERROR", MODULE_NAME);
                setLNFState(CONNECTED_PRIVATE);
                break;
            }
#endif
            pthread_mutex_lock(&mutexTriggerLAF);
            doLnFBackoff();
            if (gWifiLNFStatus == CONNECTED_PRIVATE)
            {
                LOG_INFO("[%s] Connected to Private SSID already. Aborting requested LNF.", MODULE_NAME);
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
                    LOG_INFO("[%s] stopLNFWhileDisconnected pressed coming out of LNF ", MODULE_NAME);
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
                LOG_DBG("[%s] Connection to LAF success", MODULE_NAME);
                break;
            }
        }
        bIsStopLNFWhileDisconnected=false;
        sleep(1);
        if((gWifiLNFStatus != CONNECTED_PRIVATE) && (false == isLAFCurrConnectedssid) && (gWifiAdopterStatus == WIFI_CONNECTED))
        {
            LOG_INFO("[%s] isLAFCurrConnectedssid = %d gWifiAdopterStatus = %d", MODULE_NAME,isLAFCurrConnectedssid,gWifiAdopterStatus );
            gWifiLNFStatus=CONNECTED_PRIVATE;
        }
        if (true == bStopLNFWhileDisconnected)
        {
            LOG_INFO("[%s] stopLNFWhileDisconnected pressed coming out of LNF ", MODULE_NAME);
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
    LOG_TRACE( "[%s] Exit",MODULE_NAME);
    pthread_exit(NULL);
}

static void* startLAFIfNecessary(void* arg)
{
    lastConnectedSSID(&savedWiFiConnList);
    if (savedWiFiConnList.ssidSession.ssid[0] != '\0')
    {
        sleep(confProp.wifiProps.lnfStartInSecs);
        lastConnectedSSID(&savedWiFiConnList);
    }
    /* If Device is activated and already connected to Private or any network,
        but defineltly not LF SSID. - No need to trigger LNF*/
    /* Here, 'getDeviceActivationState == true'*/
    if((! laf_is_lnfssid(savedWiFiConnList.ssidSession.ssid)) &&  (WIFI_CONNECTED == get_WifiRadioStatus())) {
       LOG_DBG("[%s] Now connected to Private SSID %s", MODULE_NAME, savedWiFiConnList.ssidSession.ssid);
    }
    else {
#ifdef ENABLE_IARM
        if(IARM_BUS_SYS_MODE_WAREHOUSE != sysModeParam)
#endif
        {
            signalStartLAF();
            bPrivConnectionLost=true;
        }
    }
    pthread_exit(NULL);
}

void connectToLAF()
{
    LOG_DBG("[%s] Enter", MODULE_NAME );
    pthread_attr_t attr;
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
                LOG_INFO("[%s] Broadcast to monitor.", MODULE_NAME);
            }
            pthread_mutex_unlock(&mutexGo);
        } else {
            set_WiFiStatusCode(WIFI_DISCONNECTED);
        }
#elif defined(XHB1) || defined(XHC3)
        set_WiFiStatusCode(WIFI_CONNECTED);
        //signal to monitor wifi status
        pthread_mutex_lock(&mutexGo);
        if(0 == pthread_cond_signal(&condGo)) {
          LOG_INFO("[%s] Broadcast to monitor.", MODULE_NAME);
        }
        pthread_mutex_unlock(&mutexGo);
#endif
        pthread_t t;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&t, &attr, startLAFIfNecessary, NULL);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    memset(lafSsid, 0, SSID_SIZE);
    laf_get_lfssid(lafSsid);
    if(lafSsid)
    {
        LOG_TRACE("[%s] lfssid is %s", MODULE_NAME, lafSsid);
        return true;
    }
    else
    {
        LOG_ERR("[%s] lfssid is empty ", MODULE_NAME);
        return false;
    }
}
#if 0
bool isLAFCurrConnectedssid()
{
    bool retVal=false;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if(laf_is_lnfssid(savedWiFiConnList.ssidSession.ssid))
    {
        LOG_TRACE("[%s] LAF is the current connected ssid ", MODULE_NAME);
        retVal=true;
    }
    else
    {
        retVal=false;
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return retVal;

}
#endif
bool getDeviceActivationState()
{
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
    char scriptOutput[BUFFER_SIZE_SCRIPT_OUTPUT] = {'\0'};
    unsigned int count=0;
    std::string str;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    while(deviceID && !deviceID[0])
    {
        if (access(SCRIPT_FILE_TO_GET_DEVICE_ID, F_OK ) != -1)
        {
            if (!netSrvMgrUtiles::getCommandOutput(SCRIPT_FILE_TO_GET_DEVICE_ID, scriptOutput, sizeof (scriptOutput)))
            {
                LOG_ERR("[%s] Error in running device id script.", MODULE_NAME);
                return bDeviceActivated;
            }
            g_stpcpy(deviceID,scriptOutput);
        }
        else
        {
            if(! confProp.wifiProps.authServerURL)
            {
                LOG_ERR("[%s] Authserver URL is NULL", MODULE_NAME);
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
    LOG_INFO( "[%s] device id string is %s", MODULE_NAME,deviceID);
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
                  LOG_INFO("[%s] partner id is %s", MODULE_NAME, partnerID);
                }
            }
        }
      }
    if(tokens)
        g_strfreev(tokens);
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return bDeviceActivated;
#else
    return true;
#endif // ENABLE_XCAM_SUPPORT and XHB1 and XHC3
}
bool isWifiConnected()
{
    return (get_WiFiStatusCode() == WIFI_CONNECTED);
}

void lnfConnectPrivCredentials()
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if(bLnfActivationLoop)
    {
       LOG_DBG( "[%s] Still in LnF Activation Loop so discarding getting private credentials",MODULE_NAME);
        return;
    }
    signalStartLAF();
    LOG_TRACE("[%s] Exit", MODULE_NAME);
}
#endif // ENABLE_LOST_FOUND

#ifdef ENABLE_IARM
bool setHostifParam (char *name, HostIf_ParamType_t type, void *value)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    bool retValue=false;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    HOSTIF_MsgData_t param = {0};

    if(NULL == name || value == NULL) {
        LOG_ERR("[%s]  Null pointer caught for Name/Value", MODULE_NAME );
        return retValue;
    }

    /* Initialize hostIf Set structure */
    strncpy(param.paramName, name, strlen(name)+1);
    param.reqType = HOSTIF_SET;

    switch (type) {
    case hostIf_StringType:
    case hostIf_DateTimeType:
    {
        param.paramtype = hostIf_StringType;
        STRCPY_S(param.paramValue, sizeof(param.paramValue), (char *) value);
        break;
    }
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
        LOG_ERR("[%s] Error executing Set parameter call from tr69Client, error code",MODULE_NAME);
    }
    else
    {
        LOG_DBG("[%s] sent iarm message to set hostif parameter",MODULE_NAME);
        retValue=true;
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
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

    LOG_TRACE("[%s] Enter", MODULE_NAME);
    param.requestType=WIFI_GET_CREDENTIALS;
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        if (param.wifiCredentials.iSecurityMode == -1){
        LOG_INFO("[%s] IARM success in retrieving the stored wifi credentials having no security mode ",MODULE_NAME);
        retVal = connect_withSSID(ssidIndex, param.wifiCredentials.cSSID, NET_WIFI_SECURITY_WPA_PSK_AES, NULL, param.wifiCredentials.cPassword, param.wifiCredentials.cPassword, true, NULL, NULL, NULL, NULL, WIFI_CON_PRIVATE);
        }
        else{
        LOG_INFO("[%s] IARM success in retrieving the stored wifi credentials having the security mode %d",MODULE_NAME,param.wifiCredentials.iSecurityMode );
        retVal = connect_withSSID(ssidIndex, param.wifiCredentials.cSSID, (SsidSecurity)param.wifiCredentials.iSecurityMode, NULL, param.wifiCredentials.cPassword, param.wifiCredentials.cPassword, true, NULL, NULL, NULL, NULL, WIFI_CON_PRIVATE);
        }
    }
    else
    {
        LOG_INFO("[%s] IARM failure in retrieving the stored wifi credentials", MODULE_NAME);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    retVal=lastConnectedSSID(&savedWiFiConnList);
    if(retVal == false)
    {
        LOG_ERR("[%s] Last connected ssid fetch failure", MODULE_NAME);
        return retVal;
    }
    else
    {
        retVal=false;
        LOG_TRACE("[%s] fetched ssid details  %s ",MODULE_NAME,savedWiFiConnList.ssidSession.ssid);
    }
    param.requestType=WIFI_GET_CREDENTIALS;
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        LOG_TRACE("[%s] IARM success in retrieving the stored wifi credentials ",MODULE_NAME);
        if ((strcmp (param.wifiCredentials.cSSID, savedWiFiConnList.ssidSession.ssid) == 0) &&
                (strcmp (param.wifiCredentials.cPassword, savedWiFiConnList.ssidSession.passphrase) == 0) &&
                (param.wifiCredentials.iSecurityMode == savedWiFiConnList.ssidSession.security_mode || param.wifiCredentials.iSecurityMode == -1) )
        {
            LOG_INFO("[%s] Same ssid info not storing it stored ssid %s new ssid %s",
                    MODULE_NAME, param.wifiCredentials.cSSID, savedWiFiConnList.ssidSession.ssid);
            return true;
        }
        else
        {
            LOG_INFO("[%s] ssid info is different continue to store ssid %s new ssid %s",
                    MODULE_NAME, param.wifiCredentials.cSSID, savedWiFiConnList.ssidSession.ssid);
        }
    }
    memset(&param,0,sizeof(param));
    param.requestType=WIFI_SET_CREDENTIALS;
    STRCPY_S(param.wifiCredentials.cSSID, sizeof(param.wifiCredentials.cSSID), savedWiFiConnList.ssidSession.ssid);
    STRCPY_S(param.wifiCredentials.cPassword, sizeof(param.wifiCredentials.cPassword), savedWiFiConnList.ssidSession.passphrase);
    param.wifiCredentials.iSecurityMode = savedWiFiConnList.ssidSession.security_mode;
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        LOG_TRACE("[%s] IARM success in storing wifi credentials",MODULE_NAME);
        memset(&param,0,sizeof(param));
        param.requestType=WIFI_GET_CREDENTIALS;
        if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
        {
            LOG_TRACE("[%s] IARM success in retrieving the stored wifi credentials ",MODULE_NAME);
            if ((strcmp (param.wifiCredentials.cSSID, savedWiFiConnList.ssidSession.ssid) == 0) &&
                    (strcmp (param.wifiCredentials.cPassword, savedWiFiConnList.ssidSession.passphrase) == 0) &&
                    (param.wifiCredentials.iSecurityMode == savedWiFiConnList.ssidSession.security_mode || param.wifiCredentials.iSecurityMode == -1) )
            {
                retVal=true;
                LOG_INFO("[%s] Successfully stored the credentails and verified stored ssid %s current ssid %s and security_mode %d",MODULE_NAME,param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid,param.wifiCredentials.iSecurityMode);
            }
            else
            {
                LOG_ERR("[%s] failure in storing  wifi credentials",MODULE_NAME);
            }

        }
        else
        {
            LOG_ERR("[%s] IARM error in retrieving stored wifi credentials mfr error code %d ",MODULE_NAME,param.returnVal );
        }
    }
    else
    {
        LOG_ERR("[%s] IARM error in storing wifi credentials mfr error code ",MODULE_NAME ,param.returnVal);
    }
#endif // ENABLE_IARM
#endif // USE_RDK_WIFI_HAL
    LOG_TRACE("[%s] Exit", MODULE_NAME);
    return retVal;
}


bool eraseMfrWifiCredentials(void)
{
    bool retVal=false;
#ifdef USE_RDK_WIFI_HAL
    LOG_TRACE("[%s] Enter", MODULE_NAME);
#ifdef ENABLE_IARM
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_EraseAllData,0,0))
    {
        LOG_TRACE("[%s] IARM success in erasing wifi credentials ",MODULE_NAME);
        retVal=true;
    }
    else
    {
        LOG_ERR("[%s] failure in erasing  wifi credentials ",MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME);
#endif
#endif
    return retVal;
}


void getEndPointInfo(WiFi_EndPoint_Diag_Params *endPointInfo)
{
    int radioIndex = 0;
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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

    LOG_DBG("[%s] \n Profile : \"EndPoint.1.\": \n \
            [Enable : \"%d\"| Status : \"%s\" | SSIDReference : \"%s\" ]",
            MODULE_NAME, endPointInfo->enable, endPointInfo->status, endPointInfo->SSIDReference);

    LOG_DBG("[%s] \n Profile : \"EndPoint.1.Stats.\": \n \
            [SignalStrength : \"%d\"| Retransmissions : \"%d\" | LastDataUplinkRate : \"%d\" | LastDataDownlinkRate : \" %d\" ]",
            MODULE_NAME,endPointInfo->stats.signalStrength, endPointInfo->stats.retransmissions,
            endPointInfo->stats.lastDataDownlinkRate, endPointInfo->stats.lastDataUplinkRate);
#endif // USE_RDK_WIFI_HAL
    LOG_TRACE("[%s] Exit", MODULE_NAME );
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
       LOG_DBG("[%s] Successfully set Roaming param- [roamingEnable=%d,preassnBestThreshold=%d,preassnBestDelta=%d]", MODULE_NAME,param->roamingEnable,param->preassnBestThreshold,param->preassnBestDelta);
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
       LOG_DBG("[%s] Successfully set Roaming param- [roamingEnable=%d,preassnBestThreshold=%d,preassnBestDelta=%d]", MODULE_NAME,in_param.roamingEnable,in_param.preassnBestThreshold,in_param.preassnBestDelta);
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
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    wifi_radioTrafficStats_t *trafficStats =(wifi_radioTrafficStats_t *) malloc(sizeof(wifi_radioTrafficStats_t));
    if(trafficStats == NULL)
    {
        LOG_TRACE( "[%s]Malloc Memory allocation failure", MODULE_NAME);
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
        LOG_ERR("[%s] HAL wifi_getRadioTrafficStats FAILURE", MODULE_NAME);
    }
    free(trafficStats);

    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return true;
}

void logs_Period1_Params()
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
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
                LOG_INFO("%s:%lu ",  (char *)iter->data, endPointInfo.stats.lastDataDownlinkRate);
            }
            /* Device.WiFi.EndPoint.{i}.Stats.LastDataUplinkRate */
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "LastDataUplinkRate ")) {
                LOG_INFO("%s:%lu ",  (char *)iter->data, endPointInfo.stats.lastDataUplinkRate);
            }

            /* Device.WiFi.EndPoint.{i}.Stats.Retransmissions*/
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Retransmissions")) {
                LOG_INFO("%s:%lu ",  (char *)iter->data, endPointInfo.stats.retransmissions);
            }

            /*Device.WiFi.Endpoint.{i}.Stats.SignalStrength*/
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "SignalStrength")) {
                LOG_INFO("%s:%d ",  (char *)iter->data, endPointInfo.stats.signalStrength);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Channel")) {
                unsigned long output_ulong = 0;
                int radioIndex = 0;
                if (wifi_getRadioChannel(radioIndex, &output_ulong) == RETURN_OK) {
                    LOG_INFO("%s:%lu ", (char *)iter->data , output_ulong);
                }
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.FCSErrorCount")) {
                LOG_INFO("%s:%u ",  (char *)iter->data, params.fcsErrorCount);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.Noise")) {
                LOG_INFO("%s:%u ",  (char *)iter->data, params.noiseFloor);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "TransmitPower"))
            {
                INT output_INT = 0;
                int radioIndex = 0;
                if (wifi_getRadioTransmitPower( radioIndex,  &output_INT) == RETURN_OK) {
                    LOG_INFO("%s:%d ", (char *)iter->data , output_INT);
                }
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.ErrorsReceived")) {
                LOG_INFO( "%s:%u ",  (char *)iter->data, params.errorsReceived);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.ErrorsSent")) {
                LOG_INFO("%s:%u ",  (char *)iter->data, params.errorsSent);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.PacketsReceived")) {
                LOG_INFO( "%s:%lu ",  (char *)iter->data, params.packetsReceived);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.PacketsSent")) {
                LOG_INFO("%s:%lu ",  (char *)iter->data, params.packetsSent);
            }

            iter = g_list_next(iter);
        }

    }

    LOG_TRACE("Exit");
}


void logs_Period2_Params()
{
    static bool print_flag = true;
    time_t start_t;
    static time_t lastExec_t;
    double diff_t;

    LOG_ENTRY_EXIT;

    time(&start_t);
    LOG_TRACE("print_flag : %d", print_flag);
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
                LOG_TRACE("%s", (char *)iter->data);
                /* Device.WiFi.Endpoint.{i}.NumberOfEntries*/
                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "NumberOfEntries")) {
                    int noOfEndpoint = 1;
                    LOG_INFO( "%s:%d",  (char *)iter->data, noOfEndpoint);
                }

                if((g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.EndPoint.1.Profile.1.SSID"))
                        || (g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.SSID.1.SSID"))
                        || (g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.EndPoint.1.SSIDReference")))
                {
                    LOG_INFO("%s:%s",  (char *)iter->data, currSsidInfo.ssidSession.ssid);
                }

                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "BSSID")) {
                    LOG_INFO("%s:%s",  (char *)iter->data, bssid_string);
                }

                /* Device.WiFi.EndPoint.{i}.Profile.{i}.SSID*/
                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Name")) {
                    memset(output_string,0, BUFF_MAX);
                    if(wifi_getSSIDName(ssidIndex, output_string) == RETURN_OK) {
                        LOG_INFO("%s:%s",  (char *)iter->data, output_string);
                    }
                }

                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.SSID.1.MACAddress")) {
                    if(gWifiMacAddress[0] != '\0') {
                        LOG_INFO("%s:%s",  (char *)iter->data, gWifiMacAddress);
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

}


void readValue(FILE *pFile, char *pToken, char *data)
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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
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
        LOG_ERR("get lfat from auth service failed with error code %d", http_err_code);
        if(http_err_code == 0)
            http_err_code = 404;
        return http_err_code;
    }

    /* parse json response */
    LOG_DBG("Response from authservice to get lfat %s", retStr);
    response = cJSON_Parse(retStr);
    RDK_ASSERT_NOT_NULL(response);
    RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "version"));
    STRCPY_S(lfat->version, sizeof(lfat->version), cJSON_GetObjectItem(response, "version")->valuestring);
    RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "expires"));
    lfat->ttl = cJSON_GetObjectItem(response, "expires")->valueint;
    RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "LFAT"));
    lfat->len = strlen(cJSON_GetObjectItem(response, "LFAT")->valuestring);
    lfat->token = (char *) malloc(lfat->len+1);
    if(lfat->token == NULL)
        return ENOMEM;
    STRCPY_S(lfat->token, lfat->len+1, cJSON_GetObjectItem(response, "LFAT")->valuestring);
    lfat->token[lfat->len] = '\0';
    cJSON_Delete(response);
    return 0;
}
#else // ENABLE_XCAM_SUPPORT or XHB1 or XHC3
int laf_get_lfat(laf_lfat_t *lfat)
{
	LOG_ENTRY_EXIT;
	int ret=0;
	unsigned int tokenlength=0;
        FILE *fd = NULL;
        char version[MAX_VERSION_LEN];
        char ttl[MAX_VERSION_LEN];
        long value = 0;
        char *eptr;

	ret = get_token_length(tokenlength);
	if(ret != 0) {
		LOG_ERR("laf_get_lfat - Error in token read");
		return ret;
	}
	lfat->token = (char *) malloc(tokenlength+1);
	if(lfat->token == NULL)
		return ENOMEM;
	ret = get_laf_token(lfat->token,tokenlength);
	lfat->len = tokenlength;
	lfat->token[tokenlength] = '\0';
	LOG_DBG("laf_get_lfat - token size is - %d", lfat->len);
	LOG_DBG("laf_get_lfat - token is - %s", lfat->token);

        //Read lfat version and ttl from /mnt/ramdisk/env/.lnf_conf
        if (0 == access(LFAT_CONF_FILE, F_OK)) {
          memset(version,'\0', MAX_VERSION_LEN);
          memset(ttl, '\0', MAX_VERSION_LEN);
          fd = fopen(LFAT_CONF_FILE, "r");
          if (fd != NULL) {
            readValue(fd, LFAT_VERSION, version);
            if(version != NULL){
                STRCPY_S(lfat->version, sizeof(lfat->version), version);
            }else{
                STRCPY_S(lfat->version, sizeof(lfat->version), "1.1");
            }

            readValue(fd, LFAT_TTL, ttl);
            if(ttl != NULL){
              value = strtol(ttl, &eptr, 10);
              lfat->ttl = value;
            }else{
              lfat->ttl = 31536000;
            }
            LOG_INFO("lfat_version : %s, lfat_ttl : %s",version, ttl);
            LOG_INFO("lfat_version : %s, lfat_ttl : %ld",lfat->version, lfat->ttl);
            fclose(fd);
          }else{
            LOG_INFO("Unable to open lfat_conf file");
          }
        }
        else //If .lnf_conf file is not present
        {
          // Read from netsrvmgr.conf
          STRCPY_S(lfat->version, sizeof(lfat->version), confProp.wifiProps.lfatVersion);
          lfat->ttl = confProp.wifiProps.lfatTTL;
          LOG_INFO("lfat_version : %s, lfat_ttl : %ld",lfat->version, lfat->ttl);
        }
	return ret;
}
#endif // ENABLE_XCAM_SUPPORT

#if !defined (ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
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
        LOG_ERR("set lfat to auth service failed with error code %d message", http_err_code);
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
#else // ENABLE_XCAM_SUPPORT or XHB1 or XHC3
int laf_set_lfat(laf_lfat_t* const lfat)
{
  LOG_ENTRY_EXIT;
  char* tokenfilename = getlfatfilename();
  if(NULL != lfat->token){
     LOG_INFO("Updating lfat token after token expiry/recovery");
      int result = getSetFileContent(tokenfilename, "set", lfat->token);
      LOG_DBG("tokenfilename : %s lfat->token : %s", tokenfilename, lfat->token);
      if (result)
      {
        LOG_ERR("Error setting lfat token");
        return -1;
      }
  }else {
       LOG_ERR( "LFAT Token is Null");
       return -1;
  }
  return 0;
}
#endif // ENABLE_XCAM_SUPPORT or XHB1 or XHC3

bool addSwitchToPrivateResults(int lnfError,char *currTime)
{
  bool ret = false;
  guint lstLnfPvtResultsLength=0;
  WiFiLnfSwitchPrivateResults_t *WiFiLnfSwitchPrivateResultsData=NULL;
  LOG_TRACE("[%s] Enter", MODULE_NAME);
  WiFiLnfSwitchPrivateResultsData = g_new0(WiFiLnfSwitchPrivateResults_t,1);
  if(NULL == WiFiLnfSwitchPrivateResultsData) {
    LOG_ERR("[%s] \'Failed to allocate memory", MODULE_NAME);
  }
  else {
    WiFiLnfSwitchPrivateResultsData->lnfError = (unsigned char)lnfError;
    g_stpcpy((gchar*)WiFiLnfSwitchPrivateResultsData->currTime,currTime);
    lstLnfPvtResults=g_list_append(lstLnfPvtResults,WiFiLnfSwitchPrivateResultsData);
    lstLnfPvtResultsLength=g_list_length(lstLnfPvtResults);
    if ((lstLnfPvtResults != NULL)&&(lstLnfPvtResultsLength > 0)) {
         LOG_INFO("[%s] Time = %s lnf error = %d length of list = %d ", MODULE_NAME,WiFiLnfSwitchPrivateResultsData->currTime,WiFiLnfSwitchPrivateResultsData->lnfError,g_list_length(lstLnfPvtResults));
         ret = true;
    }
    else {
         LOG_ERR("[%s] Error in added Time and lnf error to list, error code %d", MODULE_NAME, ret );
    }
  }
  LOG_TRACE("[%s] Exit", MODULE_NAME );
  return ret;
}

bool convertSwitchToPrivateResultsToJson(char *buffer)
{
  cJSON *rootObj = NULL, *array_element = NULL, *array_obj = NULL;
  UINT output_array_size = 0;
  INT index;
  char *out = NULL;
  char privateResultsLength=0;
  WiFiLnfSwitchPrivateResults_t *WiFiLnfSwitchPrivateResultsData=NULL;
  LOG_TRACE("[%s] Enter", MODULE_NAME);
  rootObj = cJSON_CreateObject();
  if(NULL == rootObj) {
    LOG_ERR("[%s] Failed to create root json object.", MODULE_NAME);
    return false;
  }
  cJSON_AddItemToObject(rootObj, "results", array_obj=cJSON_CreateArray());
  GList *element = g_list_first(lstLnfPvtResults);
  privateResultsLength=g_list_length(lstLnfPvtResults);

  while ((element != NULL)&&(privateResultsLength > 0))
  {
    WiFiLnfSwitchPrivateResultsData = (WiFiLnfSwitchPrivateResults_t*)element->data;
    cJSON_AddItemToArray(array_obj,array_element=cJSON_CreateObject());
    cJSON_AddStringToObject(array_element, "timestamp",(const char*)WiFiLnfSwitchPrivateResultsData->currTime);
    cJSON_AddNumberToObject(array_element, "result", WiFiLnfSwitchPrivateResultsData->lnfError);
    LOG_TRACE("[%s] Time = %s lnf error = %d ", MODULE_NAME, WiFiLnfSwitchPrivateResultsData->currTime,WiFiLnfSwitchPrivateResultsData->lnfError );
    element = g_list_next(element);
    privateResultsLength--;
  }
  out = cJSON_PrintUnformatted(rootObj);

  if(out) {
    strncpy(buffer, out, strlen(out)+1);
    LOG_TRACE("[%s] Buffer = %s", MODULE_NAME,buffer );
  }
  if(rootObj) {
    cJSON_Delete(rootObj);
  }
  if(out) free(out);
  LOG_TRACE("[%s] Exit", MODULE_NAME );
  return true;
}
bool clearSwitchToPrivateResults()
{
    LOG_TRACE("[%s] Enter", MODULE_NAME);
    if((lstLnfPvtResults) && (g_list_length(lstLnfPvtResults) != 0))
    {
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
           g_list_free_full((GList *)g_steal_pointer(&lstLnfPvtResults),g_free);
#else
           g_list_free_full(lstLnfPvtResults,g_free);
#endif
    }
    else
    {
        LOG_INFO("[%s] switch to private results list is empty ", MODULE_NAME);
    }
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return true;
}
#endif // ENABLE_LOST_FOUND
#endif // USE_RDK_WIFI_HAL
bool shutdownWifi()
{
    bool result=true;
    bShutdownWifi= true;
    LOG_TRACE("[%s] Enter", MODULE_NAME);

    pthread_mutex_lock(&mutexGo);
    if(0 == pthread_cond_signal(&condGo)) {
        LOG_INFO("[%s] Signalling wifiStatusMonitorThread to exit cond wait", MODULE_NAME);
    }
    pthread_mutex_unlock(&mutexGo);

    LOG_INFO("[%s] calling pthread_join wifiStatusMonitorThread", MODULE_NAME);
    pthread_join (wifiStatusMonitorThread, NULL);

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
    pthread_mutex_lock(&mutexAPDetails);
    bsignalAPDetailsReady = true;
    if(0 == pthread_cond_signal(&condAPDetails)) {
        LOG_INFO("[%s] Signalling apDetailsCollectionThread to exit cond wait", MODULE_NAME);
    }
    pthread_mutex_unlock(&mutexAPDetails);

    LOG_INFO("[%s] calling pthread_cancel apDetailsCollectionThread", MODULE_NAME);
    if ((apDetailsCollectionThread) && (pthread_cancel(apDetailsCollectionThread) == -1 )) {
        LOG_ERR("[%s] apDetailsCollectionThread cancel failed!", MODULE_NAME);
        result = false;
    }
#endif

#ifdef ENABLE_LOST_FOUND
    if ((lafConnectThread) && (pthread_cancel(lafConnectThread) == -1 )) {
        LOG_ERR("[%s] lafConnectThread cancel failed!", MODULE_NAME);
        result=false;
    }
    if ((lafConnectToPrivateThread) && (pthread_cancel(lafConnectToPrivateThread) == -1 )) {
        LOG_ERR("[%s] lafConnectToPrivateThread failed!", MODULE_NAME);
        result=false;
    }
#endif
    LOG_INFO("[%s] Start WiFi Uninitialization", MODULE_NAME);
#ifdef USE_RDK_WIFI_HAL
    wifi_uninit();
#endif
    LOG_INFO("[%s]  WiFi Uninitialization done", MODULE_NAME);
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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
    condAPDetails = PTHREAD_COND_INITIALIZER;
    mutexAPDetails = PTHREAD_MUTEX_INITIALIZER;
#endif
    memset(&gSsidList,0,sizeof gSsidList);
    memset(&savedWiFiConnList,0,sizeof savedWiFiConnList);
    memset(&wifiConnData,0,sizeof wifiConnData);
    LOG_TRACE("[%s] Exit", MODULE_NAME );
    return result;
}


