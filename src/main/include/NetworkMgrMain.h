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
#ifdef ENABLE_IARM
#include "irMgr.h"
#include "comcastIrKeyCodes.h"
#include "libIARM.h"
#endif

#define IARM_BUS_NM_SRV_MGR_NAME "NET_SRV_MGR"

#define LOG_NMGR "LOG.RDK.NETSRVMGR"
#define WIFI_CONFIG "WiFiMgr_Config"

#define MAX_TIMEOUT_ON_DISCONNECT       "MAX_TIMEOUT_ON_DISCONNECT"
#define STATS_POLL_INTERVAL          	"STATS_POLL_INTERVAL"
#define ENABLE_LOST_FOUND_RUN           "ENABLE_LOST_FOUND"
#define LAF_CONNECT_RETRY_INTERVAL      "LAF_CONNECT_RETRY_INTERVAL"
#define LAF_CONNECT_START_INTERVAL 	"LAF_CONNECT_START_INTERVAL"
#define AUTHSERVER_URL                  "AUTHSERVER_URL"
#define GET_AUTHTOKEN_URL               "GET_AUTHTOKEN_URL"
#define GET_LFAT_URL                    "GET_LFAT_URL"
#define SET_LFAT_URL                    "SET_LFAT_URL"
#define DISABLE_WPS_XRE			"disableWpsXRE"
#define LFAT_VERSION			"LFAT_VERSION"
#define LFAT_TTL			"LFAT_TTL"
#define LAF_CONNECTION_RETRY            "LAF_CONNECTION_RETRY"

#define WIFI_BCK_PATHNAME				"/opt/persistent/wifi"
#define WIFI_BCK_FILENAME				"/opt/persistent/wifi/wifiConnectionInfo.json"
#define WIFI_CONN_DETAILS				"wifi_conn_details"
#define SSID_STR 			"ssid"
#define PSK_STR 			"psk"
#define CONN_TYPE			"conn_type"
#define MAX_TIME_OUT_PERIOD     60
#define BUFFER_SIZE_128 128

/*Telemetery logging file configurations */
#define TELEMETRY_LOGGING_PARAM_FILE "/etc/netsrvmgr_Telemetry_LoggingParams.json"
#define T_PERIOD_1_INTERVAL  		"wifi_period1_time"
#define T_PERIOD_1_PARAMETER_LIST "wifi_parameter_list_period1"
#define T_PERIOD_2_INTERVAL  		"wifi_period2_time"
#define T_PERIOD_2_PARAMETER_LIST "wifi_parameter_list_period2"


typedef struct {
    int timePeriod;
    GList* paramlist;
} telemetryParams;


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
    bool bEnableLostFound;
    unsigned short lnfRetryInSecs;
    unsigned short lnfStartInSecs;
    char authServerURL[BUFFER_SIZE_128];
    bool disableWpsXRE;
    char getAuthTokenUrl[BUFFER_SIZE_128];
    char getLfatUrl[BUFFER_SIZE_128];
    char setLfatUrl[BUFFER_SIZE_128];
    char lfatVersion[BUFFER_SIZE_128];
    unsigned short lnfRetryCount;
    unsigned int lfatTTL;
} wifiMgrConfigProps;

typedef struct  _netMgrConfigProps
{
    wifiMgrConfigProps wifiProps;
} netMgrConfigProps;

#endif /* _NETWORKMGRMAIN_H_ */
