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

#ifndef WIFIHALUTILES_H_
#define WIFIHALUTILES_H_

#include "wifiSrvMgr.h"
#include "NetworkMgrMain.h"
#include "wifiSrvMgrIarmIf.h"

#ifdef ENABLE_IARM
#include "hostIf_tr69ReqHandler.h"
#endif

#ifdef ENABLE_XCAM_SUPPORT
extern "C" {
#include "rdkc_api.h"
}
#endif

#ifdef ENABLE_LOST_FOUND
#ifdef ENABLE_IARM
#include "authserviceIARM.h"
#endif
#endif

#ifdef USE_RDK_WIFI_HAL
extern "C" {
#include "wifi_client_hal.h"
}
#endif
#ifdef ENABLE_LOST_FOUND
#include "netSrvCurl.h"
extern "C" {
#include "lost_and_found.h"
}
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
//}

#define WIFI_DEFAULT_INTERFACE "wlan0"

#endif

#ifdef ENABLE_IARM
#include "mfrMgr.h"
#include "sysMgr.h"
#include "libIBusDaemon.h"
#endif

#ifdef ENABLE_RTMESSAGE
/* RtMessage */
#include "rtLog.h"
#include "rtConnection.h"
#include "rtMessage.h"
#define SOCKET_ADDRESS "tcp://127.0.0.1:10001"
#endif

#define ACTION_ON_CONNECT 	1
#define ACTION_ON_DISCONNECT 	0
#define RETRY_TIME_INTERVAL     3
#define DEVICE_ACTIVATED 	1
#define DEFAULT_TR69_INSTANCE 	0
#define XRE_REFRESH_SESSION         "Device.X_COMCAST-COM_Xcalibur.Client.XRE.xreRefreshXreSession"
#define XRE_REFRESH_SESSION_WITH_RR "Device.X_COMCAST-COM_Xcalibur.Client.XRE.xreRefreshXreSessionWithRR"

#define WIFI_ADAPTER_STATUS_PARAM 	"Device.WiFi.Radio.1.Status"
#define WIFI_ADAPTER_ENABLE_PARAM	"Device.WiFi.Radio.1.Enable"
#define WIFI_SSID_ENABLE_PARAM		"Device.WiFi.SSID.1.Enable"
#define WIFI_SSID_STATUS_PARAM		"Device.WiFi.SSID.1.Status"
#define WIFI_SSID_NAME_PARAM		"Device.WiFi.SSID.1.SSID"
#define	WIFI_SSID_BSSID_PARAM 		"Device.WiFi.SSID.1.BSSID"
#define WIFI_SSID_MACADDR_PARAM		"Device.WiFi.SSID.1.MACAddress"

#define SERIALNUMBER_SIZE 15
#define MODELNAME_SIZE  10
#define MANUFACTURER_SIZE   40
#define WIFIMAC_SIZE    20
#define DEVICEID_SIZE 512
#define PARTNERID_SIZE 128
#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

#ifdef ENABLE_RTMESSAGE
void rtConnection_init();
static void* rtMessage_Receive(void * arg);
void rtConnection_destroy();
static void onMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
#endif

#ifdef ENABLE_IARM
bool gpvFromTR069hostif( HOSTIF_MsgData_t *param);
#endif
WiFiStatusCode_t get_WifiRadioStatus();
WiFiConnectionTypeCode_t get_WifiConnectionType();
bool ethernet_on();

extern bool updateWiFiList();
extern ssidList gSsidList;
#ifdef ENABLE_IARM
extern IARM_Bus_Daemon_SysMode_t sysModeParam;
#endif

bool shutdownWifi();
#ifdef USE_RDK_WIFI_HAL
bool connect_WpsPush();
INT wifi_connect_callback(INT , CHAR *ap, wifiStatusCode_t *err);
INT wifi_disconnect_callback(INT , CHAR *ap, wifiStatusCode_t *err);
bool connect_withSSID(int, char *, SsidSecurity, char *, char *, char *,int,char *,char *,char *,char *,int conType = 0);
bool scan_Neighboring_WifiAP(char *);
bool lastConnectedSSID(WiFiConnectionStatus *ConnParams);
void monitor_WiFiStatus();
bool clearSSID_On_Disconnect_AP();
#ifdef WIFI_CLIENT_ROAMING
bool getRoamingConfigInfo(WiFi_RoamingCtrl_t *param);
bool setRoamingConfigInfo(WiFi_RoamingCtrl_t *param);
#endif
#endif

#ifdef ENABLE_LOST_FOUND
bool isWifiConnected();
void connectToLAF();
void lafConnectToPrivate();
bool getLAFssid();
//bool isLAFCurrConnectedssid();
WiFiLNFStatusCode_t get_WiFiLNFStatusCode();
void doLnFBackoff();
bool triggerLostFound(LAF_REQUEST_TYPE lafRequestType);
bool getmacaddress(gchar* ifname,GString *data);
bool getDeviceInfo(laf_device_info_t *dev_info);
bool getHALVersion();
bool addSwitchToPrivateResults(int lnfError,char *currTime);
bool convertSwitchToPrivateResultsToJson(char *buffer);
bool clearSwitchToPrivateResults();
#ifdef ENABLE_IARM
bool getMfrData(GString* mfrDataStr,mfrSerializedType_t mfrType);
#endif
int laf_wifi_connect(laf_wifi_ssid_t* const wificred);
int laf_wifi_disconnect(void);
int laf_get_lfat(laf_lfat_t *lfat);
int laf_set_lfat(laf_lfat_t* const lfat);
void log_message(laf_loglevel_t level, char const* function, int line, char const* msg);
#ifdef ENABLE_IARM
static void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
#endif
bool getDeviceActivationState();
void lnfConnectPrivCredentials();
#endif
bool isWiFiCapable();
void get_CurrentSsidInfo(WiFiConnectionStatus *currSsidConnInfo);
#ifdef ENABLE_IARM
bool setHostifParam (char *name, HostIf_ParamType_t type, void *value);
#endif
void put_boolean(char *ptr, bool val);
void put_int(char *ptr, int val);
bool storeMfrWifiCredentials(void);
bool eraseMfrWifiCredentials(void);
bool getRadioStats(WiFi_Radio_Stats_Diag_Params *params);
void getConnectedSSIDInfo(WiFiConnectedSSIDInfo_t *);
void getEndPointInfo(WiFi_EndPoint_Diag_Params *);
#endif /* WIFIHALUTILES_H_ */
