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

#ifndef _WIFIMGR_IARMIF_H
#define _WIFIMGR_IARMIF_H

//#include "libIBus.h"
#include "libIARM.h"

#define IARM_BUS_NM_SRV_MGR_NAME "NET_SRV_MGR"

#define BUFF_LENGTH_64  65
#define BUFF_LENGTH_256 257
#define BUFF_MAX    	1025
#define BUFF_MAC 	18
#define BUFF_MIN 	17
#define BUFF_LENGTH_32  33
#define BUFF_LENGTH_4  4
#define SSID_SIZE 	BUFF_LENGTH_32
#define BSSID_BUFF 	20
#define PASSPHRASE_BUFF BUFF_LENGTH_64
#define MAX_SSIDLIST_BUF 10000

#define IARM_BUS_WIFI_MGR_API_getAvailableSSIDs     "getAvailableSSIDs"      /*!< Retrives the array of strings representing ssids*/
#define IARM_BUS_WIFI_MGR_API_getCurrentState       "getCurrentState"        /*!< Retrives the current state*/
#define IARM_BUS_WIFI_MGR_API_getPairedSSID         "getPairedSSID"          /*!< Returns the paired ssid as a string*/
#define IARM_BUS_WIFI_MGR_API_setEnabled            "setEnabled"             /*!< Enable the wifi adapter on the box*/
#define IARM_BUS_WIFI_MGR_API_connect               "connect"                /*!< Connect with given or saved ssid and passphrase */
#define IARM_BUS_WIFI_MGR_API_initiateWPSPairing    "initiateWPSPairing"     /*!< Initiates the connection via WPS*/
#define IARM_BUS_WIFI_MGR_API_saveSSID              "saveSSID"               /*!< Save the ssid and passphrase */
#define IARM_BUS_WIFI_MGR_API_clearSSID             "clearSSID"              /*!< Clear the given ssid*/
#define IARM_BUS_WIFI_MGR_API_getPairedSSID         "getPairedSSID"          /*!< Get the paired SSID */
#define IARM_BUS_WIFI_MGR_API_isPaired              "isPaired"               /*!< Retrieve the paired status*/

/*Diagnostic Apis */
#define IARM_BUS_WIFI_MGR_API_getRadioProps         "getRadioProps"           /*!< Retrieve the get radio status properties*/
#define IARM_BUS_WIFI_MGR_API_getRadioStatsProps    "getRadioStatsProps"      /*!< Retrieve the get radio stats properties*/
#define IARM_BUS_WIFI_MGR_API_setRadioProps         "setRadioProps"           /*!< Set radio properties*/
#define IARM_BUS_WIFI_MGR_API_getSSIDProps          "getSSIDProps"            /*!< Retrieve the ssid properties*/

/*! Event states associated with WiFi connection  */
typedef enum _WiFiStatusCode_t {
    WIFI_UNINSTALLED,						/* !< The device was in an installed state, and was uninstalled	*/
    WIFI_DISABLED,							/* !< The device is installed (or was just installed) and has not yet been enabled*/
    WIFI_DISCONNECTED,						/* !< The device is not connected to a network */
    WIFI_PAIRING,							/* !< The device is not connected to a network, but not yet connecting to a network*/
    WIFI_CONNECTING,						/* !< The device is attempting to connect to a network */
    WIFI_CONNECTED,							/* !< The device is successfully connected to a network */
    WIFI_FAILED								/* !< The device has encountered an unrecoverable error with the wifi adapter */
} WiFiStatusCode_t;

/*! Error code: A recoverable, unexpected error occurred,
 * as defined by one of the following values */
typedef enum _WiFiErrorCode_t {
    WIFI_SSID_CHANGED,						/* !< The SSID of the network changed */
    WIFI_CONNECTION_LOST,					/* !< The connection to the network was lost */
    WIFI_CONNECTION_FAILED,					/* !< The connection failed for an unknown reason */
    WIFI_CONNECTION_INTERRUPTED,			/* !< The connection was interrupted */
    WIFI_INVALID_CREDENTIALS,				/* !< The connection failed due to invalid credentials */
    WIFI_NO_SSID,							/* !< The SSID does not exist */
    WIFI_UNKNOWN							/* !< Any other error */
} WiFiErrorCode_t;

/*! Supported values are NONE - 0, WPA - 1, WEP - 2*/
typedef enum _SsidSecurity {
    NONE = 0,
    WPA,
    WEP,
    WPA2,
    WPA_WPA2,
    WPA_Enterprise,
    WPA2_Enterprise,
    WPA_WPA2_Enterprise
} SsidSecurity;

typedef enum _eConnectionMethodType {
    WPS_PUSH_CONNECT,
    SSID_SECLECTION_CONNECT
} eConnMethodType;


typedef struct _ssidList
{
    char ssid[SSID_SIZE];
    char bssid[BSSID_BUFF];
    SsidSecurity security;
    int signalstrength;
    double frequency;
} ssidList;


typedef struct _wifiSsidData_t {
    char jdata[MAX_SSIDLIST_BUF];                    		/**< Buffer containing the serialized data.                      */
    size_t jdataLen;                      	/**< Length of the data buffer.                                  */
} wifiSsidData_t;


typedef struct _setWiFiAdapter
{
    bool enable;
} setWiFiAdapter;

typedef struct _WiFiConnection
{
    char ssid[SSID_SIZE];
    char passphrase[PASSPHRASE_BUFF];
    char security_mode[BUFF_MIN];
    char security_WEPKey[PASSPHRASE_BUFF];
    char security_PSK[PASSPHRASE_BUFF];
} WiFiConnection;

typedef struct _WiFiConnectionStatus
{
    WiFiConnection ssidSession;
    bool isConnected;
    eConnMethodType conn_type;
} WiFiConnectionStatus;


/*! Get/Set Data associated with WiFi Service Manager */
typedef struct _IARM_Bus_WiFiSrvMgr_SsidList_Param_t {
    wifiSsidData_t curSsids;
    bool status;
} IARM_Bus_WiFiSrvMgr_SsidList_Param_t;

typedef struct _IARM_Bus_WiFiSrvMgr_Param_t {
    union {
        WiFiStatusCode_t wifiStatus;
        setWiFiAdapter setwifiadapter;
        WiFiConnection connect;
        WiFiConnection saveSSID;
        WiFiConnection clearSSID;
        struct getPairedSSID {
            char ssid[SSID_SIZE];
        } getPairedSSID;
        bool isPaired;
    } data;
    bool status;
} IARM_Bus_WiFiSrvMgr_Param_t;


/*! Event Data associated with WiFi Service Manager */
typedef struct _IARM_BUS_WiFiSrvMgr_EventData_t {
    union {
        struct _WIFI_STATECHANGE_DATA {
            WiFiStatusCode_t state;
        } wifiStateChange;
        struct _WIFI_ERROR {
            WiFiErrorCode_t code;
        } wifiError;
    } data;
} IARM_BUS_WiFiSrvMgr_EventData_t;

/*! Events published from WiFi Service Manager */
typedef enum _IARM_Bus_NMgr_WiFi_EventId_t {
    IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged = 1,
    IARM_BUS_WIFI_MGR_EVENT_onError,
    IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged,
    IARM_BUS_WIFI_MGR_EVENT_MAX,           		/*!< Maximum event id*/
} IARM_Bus_NMgr_WiFi_EventId_t;


typedef struct _WiFi_Radio_Diag_Params {
    bool enable;
    char status[BUFF_MIN];
    char alias[64];
    char name[20];
    unsigned int lastChange;
    char lowerLayers[64];
    bool upstream;
    unsigned int maxBitRate;
    char supportedFrequencyBands[8];
    char operatingFrequencyBand[24];
    char supportedStandards[20];
    char operatingStandards[24];
    char possibleChannels[24];
    char channelsInUse[24];
    unsigned int channel;
    bool autoChannelSupported;
    bool autoChannelEnable;
    unsigned int autoChannelRefreshPeriod;
    char operatingChannelBandwidth[24];
    char extensionChannel[24];
    char guardInterval[24];
    int mcs;
    char transmitPowerSupported[64];
    int transmitPower;
    bool ieee80211hSupported;
    bool ieeee80211hEnabled;
    char regulatoryDomain[BUFF_LENGTH_4];
} WiFi_Radio_DiagParams;


typedef struct _WiFi_SSID_Diag_Params {
    bool enable;
    char status[BUFF_MIN];
    char name[BUFF_LENGTH_32];
    char bssid[BUFF_MAC];
    char macaddr[BUFF_MAC];
    char ssid[BUFF_LENGTH_32];
} WiFi_SSID_Diag_Params;


typedef struct _WiFi_Radio_Stats_Diag_Params {
    unsigned long	bytesSent;
    unsigned long	bytesReceived;
    unsigned long	packetsSent;
    unsigned long	packetsReceived;
    unsigned int	errorsSent;
    unsigned int	errorsReceived;
    unsigned int	discardPacketsSent;
    unsigned int	discardPacketsReceived;
    unsigned int	plcErrorCount;
    unsigned int	fcsErrorCount;
    unsigned int	invalidMACCount;
    unsigned int	packetsOtherReceived;
    unsigned int 	noiseFloor;
} WiFi_Radio_Stats_Diag_Params;

typedef enum _IARM_Bus_WiFiSrvMgr_NumEntry_t {
    IARM_BUS_WIFI_MGR_SSID_Tbl_Props = 0,
    IARM_BUS_WIFI_MGR_SSIDEntry = 1,
    IARM_BUS_WIFI_MGR_RadioEntry = 2,
} IARM_Bus_WiFiSrvMgr_NumEntry_t;


typedef struct _IARM_BUS_WiFi_DiagsPropParam_t {
    union {
        unsigned int radioNumberOfEntries;
        struct _Radio_Data {
            short radioIndex;
            WiFi_Radio_DiagParams params;
        } radio;
        struct _Radio_Stats_Data {
            short radioIndex;
            WiFi_Radio_Stats_Diag_Params params;
        } radio_stats;
        unsigned int ssidNumberOfEntries;
        struct _Ssid_Data {
            short ssidIndex;
            WiFi_SSID_Diag_Params params;
        } ssid;
    } data;
    bool status;
    IARM_Bus_WiFiSrvMgr_NumEntry_t numEntry;
} IARM_BUS_WiFi_DiagsPropParam_t;




#endif
