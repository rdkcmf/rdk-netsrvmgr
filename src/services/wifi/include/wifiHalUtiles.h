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

#ifdef USE_RDK_WIFI_HAL
extern "C" {
#include "wifi_client_hal.h"
}
#endif

#define ACTION_ON_CONNECT 	1
#define ACTION_ON_DISCONNECT 	0
#define MAX_TIME_OUT_PERIOD     60
#define RETRY_TIME_INTERVAL     5

#define WIFI_ADAPTER_STATUS_PARAM 	"Device.WiFi.Radio.1.Status"
#define WIFI_ADAPTER_ENABLE_PARAM	"Device.WiFi.Radio.1.Enable"
#define WIFI_SSID_ENABLE_PARAM		"Device.WiFi.SSID.1.Enable"
#define WIFI_SSID_STATUS_PARAM		"Device.WiFi.SSID.1.Status"
#define WIFI_SSID_NAME_PARAM		"Device.WiFi.SSID.1.SSID"
#define	WIFI_SSID_BSSID_PARAM 		"Device.WiFi.SSID.1.BSSID"
#define WIFI_SSID_MACADDR_PARAM		"Device.WiFi.SSID.1.MACAddress"

bool gpvFromTR069hostif( HOSTIF_MsgData_t *param);
WiFiStatusCode_t get_WifiRadioStatus();

extern bool updateWiFiList();
extern ssidList gSsidList;


#ifdef USE_RDK_WIFI_HAL
bool connect_WpsPush();
INT wifi_connect_callback(INT , CHAR *ap, wifiStatusCode_t *err);
INT wifi_disconnect_callback(INT , CHAR *ap, wifiStatusCode_t *err);
bool connect_withSSID(int, char *, char *, char *, char *, char *);
bool scan_Neighboring_WifiAP(char *);
bool write_WiFiConnStatusInfo_To_File(WiFiConnectionStatus *);
bool read_WiFiConnStatusInfo_From_File(WiFiConnectionStatus *);
void monitor_WiFiStatus();
#endif

bool isWiFiCapable();
void get_CurrentSsidInfo(WiFiConnectionStatus *currSsidConnInfo);
#endif /* WIFIHALUTILES_H_ */
