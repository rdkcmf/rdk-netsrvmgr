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

#include "hostIf_tr69ReqHandler.h"
#ifdef ENABLE_LOST_FOUND
#include "authserviceIARM.h"
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
#include "mfrMgr.h"
#include "sysMgr.h"
#include "libIBusDaemon.h"

#define ACTION_ON_CONNECT 	1
#define ACTION_ON_DISCONNECT 	0
#define RETRY_TIME_INTERVAL     5
#define DEVICE_ACTIVATED 	1
#define DEFAULT_TR69_INSTANCE 	0
#define XRE_REFRESH_SESSION         "Device.X_COMCAST-COM_Xcalibur.Client.XRE.xreRefreshXreSession"

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

bool gpvFromTR069hostif( HOSTIF_MsgData_t *param);
WiFiStatusCode_t get_WifiRadioStatus();
bool ethernet_on();

extern bool updateWiFiList();
extern ssidList gSsidList;
extern IARM_Bus_Daemon_SysMode_t sysModeParam;

#ifdef USE_RDK_WIFI_HAL
bool connect_WpsPush();
INT wifi_connect_callback(INT , CHAR *ap, wifiStatusCode_t *err);
INT wifi_disconnect_callback(INT , CHAR *ap, wifiStatusCode_t *err);
bool connect_withSSID(int, char *, SsidSecurity, char *, char *, char *,int);
bool scan_Neighboring_WifiAP(char *);
bool lastConnectedSSID();
void monitor_WiFiStatus();
bool clearSSID_On_Disconnect_AP();
#endif

#ifdef ENABLE_LOST_FOUND
static char gLAFssid[SSID_SIZE];
bool isWifiConnected();
void connectToLAF();
void lafConnectToPrivate();
bool getLAFssid();
//bool isLAFCurrConnectedssid();
WiFiLNFStatusCode_t get_WiFiLNFStatusCode();
bool triggerLostFound(LAF_REQUEST_TYPE lafRequestType);
bool getmacaddress(gchar* ifname,GString *data);
bool getMfrData(GString* mfrDataStr,mfrSerializedType_t mfrType);
int laf_wifi_connect(laf_wifi_ssid_t* const wificred);
int laf_wifi_disconnect(void);
void log_message(laf_loglevel_t level, char const* function, int line, char const* msg);
static void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
bool getDeviceActivationState();
void lnfConnectPrivCredentials();
#endif
bool isWiFiCapable();
void get_CurrentSsidInfo(WiFiConnectionStatus *currSsidConnInfo);
bool setHostifParam (char *name, HostIf_ParamType_t type, void *value);
void put_boolean(char *ptr, bool val);
void getConnectedSSIDInfo(WiFiConnectedSSIDInfo_t *);
bool storeMfrWifiCredentials(void);
bool eraseMfrWifiCredentials(void);
#endif /* WIFIHALUTILES_H_ */
