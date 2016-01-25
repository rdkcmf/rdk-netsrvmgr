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

#ifndef _NETWORKMGRMAIN_H_
#define _NETWORKMGRMAIN_H_


#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib.h>


extern "C" {
#include "cJSON.h"
#ifdef ENABLE_SD_NOTIFY
#include <systemd/sd-daemon.h>
#endif
}

#include "rdk_debug.h"
#include "NetworkMedium.h"
#include "irMgr.h"
#include "comcastIrKeyCodes.h"

#define LOG_NMGR "LOG.RDK.NETSRVMGR"
#define WIFI_CONFIG "WiFiMgr_Config"

#define MAX_TIMEOUT_ON_DISCONNECT       "MAX_TIMEOUT_ON_DISCONNECT"
#define STATS_POLL_INTERVAL          	"STATS_POLL_INTERVAL"

#define WIFI_BCK_PATHNAME				"/opt/persistent/wifi"
#define WIFI_BCK_FILENAME				"/opt/persistent/wifi/wifiConnectionInfo.json"
#define WIFI_CONN_DETAILS				"wifi_conn_details"
#define SSID_STR 			"ssid"
#define PSK_STR 			"psk"
#define CONN_TYPE			"conn_type"
#define MAX_TIME_OUT_PERIOD     60

typedef enum _WiFiResult
{
    WiFiResult_ok= 0,
    WiFiResult_notFound,
    WiFiResult_inUse,
    WiFiResult_readError,
    WiFiResult_writeError,
    WiFiResult_invalidParameter,
    WiFiResult_fail
} WiFiResult;

typedef struct  _wifiMgrConfigProps
{
    unsigned short max_timeout;
    unsigned short statsParam_PollInterval;

} wifiMgrConfigProps;

typedef struct  _netMgrConfigProps
{
    wifiMgrConfigProps wifiProps;
} netMgrConfigProps;

#endif /* _NETWORKMGRMAIN_H_ */
