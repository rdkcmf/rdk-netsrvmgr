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

#ifndef _WIFIMGR_IARMIF_H
#define _WIFIMGR_IARMIF_H

//#include "libIBus.h"
#ifdef ENABLE_IARM
#include "libIARM.h"
#endif

#define MODULE_NAME "WIFI_MODULE"
#define SUB_MODULE_NAME "LNF"
#define MAX_FILE_PATH_LEN       4096

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
#define BUFF_LENGTH_24  24
#define PASSPHRASE_BUFF BUFF_LENGTH_64
#define MAX_SSIDLIST_BUF 20000

#define LNF_NON_SECURE_SSID "A16746DF2466410CA2ED9FB2E32FE7D9"
#define LNF_SECURE_SSID "D375C1D9F8B041E2A1995B784064977B"

/*IARM Interface for wifiManager_2 */
#define IARM_BUS_WIFI_MGR_API_getAvailableSSIDs     "getAvailableSSIDs"      /*!< Retrives the array of strings representing ssids*/
#define IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName     "getAvailableSSIDsWithName"      /*!< Retrives the array of strings representing ssids info for a specifc ssid and band*/
#define IARM_BUS_WIFI_MGR_API_getAvailableSSIDsAsync "getAvailableSSIDsAsync"/*!< Retrives the array of strings representing ssids*/
#define IARM_BUS_WIFI_MGR_API_getAvailableSSIDsAsyncIncr "getAvailableSSIDsAsyncIncr"/*!< Retrives the array of strings representing ssids in incremental way*/
#define IARM_BUS_WIFI_MGR_API_stopProgressiveWifiScanning "stopProgressiveWifiScanning"  /*!< Stop any in-prpogress wifi progressive scanning thread*/
#define IARM_BUS_WIFI_MGR_API_getCurrentState       "getCurrentState"        /*!< Retrives the current state*/
#define IARM_BUS_WIFI_MGR_API_getConnectedSSID	    "getConnectedSSID"		 /*!< Returns the properties of the currently connected SSID */
#define IARM_BUS_WIFI_MGR_API_getPairedSSID         "getPairedSSID"          /*!< Returns the paired ssid as a string*/
#define IARM_BUS_WIFI_MGR_API_setEnabled            "setEnabled"             /*!< Enable the wifi adapter on the box*/
#define IARM_BUS_WIFI_MGR_API_connect               "connect"                /*!< Connect with given or saved ssid and passphrase */
#define IARM_BUS_WIFI_MGR_API_initiateWPSPairing    "initiateWPSPairing"     /*!< Initiates the connection via WPS*/
#define IARM_BUS_WIFI_MGR_API_saveSSID              "saveSSID"               /*!< Save the ssid and passphrase */
#define IARM_BUS_WIFI_MGR_API_clearSSID             "clearSSID"              /*!< Clear the given ssid*/
#define IARM_BUS_WIFI_MGR_API_getPairedSSID         "getPairedSSID"          /*!< Get the paired SSID */
#define IARM_BUS_WIFI_MGR_API_isPaired              "isPaired"               /*!< Retrieve the paired status*/
#define IARM_BUS_WIFI_MGR_API_getLNFState           "getLNFState"        /*!< Retrives the LNF state*/
#define IARM_BUS_WIFI_MGR_API_isStopLNFWhileDisconnected          "isStopLNFWhileDisconnected" 		/*!< Check if LNF is stopped */
#define IARM_BUS_WIFI_MGR_API_getConnectionType     "getConnectionType"        /*!< Retrives the current state*/
#define IARM_BUS_WIFI_MGR_API_getSwitchToPrivateResults        "getSwitchToPrivateResults" 		/*!< get all switch to private results*/
#define IARM_BUS_WIFI_MGR_API_isAutoSwitchToPrivateEnabled          "isAutoSwitchToPrivateEnabled" 		/*!< informs whether switch to private is enabled */
#define IARM_BUS_WIFI_MGR_API_getPairedSSIDInfo          "getPairedSSIDInfo" 		/*!< get last paired ssid info */


/*Diagnostic Apis */
#define IARM_BUS_WIFI_MGR_API_getRadioProps         "getRadioProps"           /*!< Retrieve the get radio status properties*/
#define IARM_BUS_WIFI_MGR_API_getRadioStatsProps    "getRadioStatsProps"      /*!< Retrieve the get radio stats properties*/
#define IARM_BUS_WIFI_MGR_API_setRadioProps         "setRadioProps"           /*!< Set radio properties*/
#define IARM_BUS_WIFI_MGR_API_getSSIDProps          "getSSIDProps"            /*!< Retrieve the ssid properties*/
#define IARM_BUS_WIFI_MGR_API_getEndPointProps      "getEndPointProps"        /*!< Retrieve the Endpoint properties*/

#ifdef WIFI_CLIENT_ROAMING
#define IARM_BUS_WIFI_MGR_API_getRoamingCtrls       "getRoamingCtrls"        /* !< Retrieve the Roaming Controls */
#define IARM_BUS_WIFI_MGR_API_setRoamingCtrls       "setRoamingCtrls"        /* !< set the Roaming Controls */
#endif
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

/*! LNF states  */
typedef enum _WiFiLNFStatusCode_t {
    LNF_UNITIALIZED,  // Network manager hasn't started the LNF process
    LNF_IN_PROGRESS, // Network manager has started LNF, and waiting for operation to complete
    CONNECTED_LNF, // Connected to the LNF network
    CONNECTED_PRIVATE, // Connected to a network that is not LNF
    DISCONNECTED_NO_LNF_GATEWAY_DETECTED, // unable to connect to LNF network
    DISCONNECTED_GET_LFAT_FAILED, // client wasn't able to acquire an LFAT
    DISCONNECTED_CANT_CONNECT_TO_PRIVATE // client could obtain LFAT, but couldn't connect to private network                                                       /* !< The device has encountered an unrecoverable error with the wifi adapter */
} WiFiLNFStatusCode_t;

typedef enum _WiFiConnectionTypeCode_t {
    WIFI_CON_UNKNOWN,
    WIFI_CON_WPS,
    WIFI_CON_MANUAL,
    WIFI_CON_LNF,
    WIFI_CON_PRIVATE,
    WIFI_CON_MAX,
} WiFiConnectionTypeCode_t;

/*! Error code: A recoverable, unexpected error occurred,
 * as defined by one of the following values */
typedef enum _WiFiErrorCode_t {
    WIFI_SSID_CHANGED,              /* !< The SSID of the network changed */
    WIFI_CONNECTION_LOST,           /* !< The connection to the network was lost */
    WIFI_CONNECTION_FAILED,         /* !< The connection failed for an unknown reason */
    WIFI_CONNECTION_INTERRUPTED,    /* !< The connection was interrupted */
    WIFI_INVALID_CREDENTIALS,       /* !< The connection failed due to invalid credentials */
    WIFI_NO_SSID,                   /* !< The SSID does not exist */
    WIFI_UNKNOWN,                   /* !< Any other error */
    WIFI_AUTH_FAILED                /* !< The connection failed due to auth failure */
} WiFiErrorCode_t;

/*! Supported values are NONE - 0, WPA - 1, WEP - 2*/
typedef enum _SsidSecurity
{
    NET_WIFI_SECURITY_NONE = 0,
    NET_WIFI_SECURITY_WEP_64,
    NET_WIFI_SECURITY_WEP_128,
    NET_WIFI_SECURITY_WPA_PSK_TKIP,
    NET_WIFI_SECURITY_WPA_PSK_AES,
    NET_WIFI_SECURITY_WPA2_PSK_TKIP,
    NET_WIFI_SECURITY_WPA2_PSK_AES,
    NET_WIFI_SECURITY_WPA_ENTERPRISE_TKIP,
    NET_WIFI_SECURITY_WPA_ENTERPRISE_AES,
    NET_WIFI_SECURITY_WPA2_ENTERPRISE_TKIP,
    NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES,
    NET_WIFI_SECURITY_NOT_SUPPORTED = 15,
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

typedef struct _wifiLnfPrivateResults {
    char jdata[MAX_SSIDLIST_BUF];                    		/**< Buffer containing the serialized data.                      */
    size_t jdataLen;                      	/**< Length of the data buffer.                                  */
} wifiLnfPrivateResults_t;

typedef struct _setWiFiAdapter
{
    bool enable;
} setWiFiAdapter;

typedef struct _WiFiConnection
{
    char ssid[SSID_SIZE];
    char bssid[BSSID_BUFF];
    char security[BUFF_LENGTH_64];
    char passphrase[PASSPHRASE_BUFF];
    SsidSecurity security_mode;
    char security_WEPKey[PASSPHRASE_BUFF];
    char security_PSK[PASSPHRASE_BUFF];
    char eapIdentity[BUFF_LENGTH_256];
    char carootcert[MAX_FILE_PATH_LEN];
    char clientcert[MAX_FILE_PATH_LEN];
    char privatekey[MAX_FILE_PATH_LEN];
} WiFiConnection;

typedef struct _WiFiConnectionStatus
{
    WiFiConnection ssidSession;
    bool isConnected;
    eConnMethodType conn_type;
} WiFiConnectionStatus;

typedef struct _WiFiConnectedSSIDInfo
{
    char ssid[SSID_SIZE];		/* !< The name of connected SSID. */
    char bssid[BSSID_BUFF];   	        /* !< The the Basic Service Set ID (mac address). */
    char band[BUFF_MIN];                /* !< The frequency band at which the client is conneted to. */
    float rate;				/* !< The Physical data rate in Mbps */
    float noise;			/* !< The average noise strength in dBm. */
    float signalStrength;		/* !< The RSSI value in dBm. */
} WiFiConnectedSSIDInfo_t;

typedef struct _WiFiPairedSSIDInfo
{
    char ssid[SSID_SIZE];		/* !< The name of connected SSID. */
    char bssid[BSSID_BUFF];		/* !< The the Basic Service Set ID (mac address). */
    char security[BUFF_LENGTH_64];	/* !< security of AP */
} WiFiPairedSSIDInfo_t;

typedef struct _WiFiLnfSwitchPrivateResults
{
    unsigned char currTime[BUFF_LENGTH_32];
    unsigned char lnfError;

} WiFiLnfSwitchPrivateResults_t;

/*! Get/Set Data associated with WiFi Service Manager */
typedef struct _IARM_Bus_WiFiSrvMgr_SsidList_Param_t {
    wifiSsidData_t curSsids;
    bool status;
} IARM_Bus_WiFiSrvMgr_SsidList_Param_t;

typedef struct _IARM_Bus_WiFiSrvMgr_SwitchPrivateResults_Param {
    wifiLnfPrivateResults_t switchPvtResults;
    bool status;
} IARM_Bus_WiFiSrvMgr_SwitchPrivateResults_Param_t;

typedef struct _IARM_Bus_WiFiSrvMgr_SpecificSsidList_Param_t {
        wifiSsidData_t curSsids;
            bool status;
            char SSID[SSID_SIZE+1];
            double frequency;
} IARM_Bus_WiFiSrvMgr_SpecificSsidList_Param_t;


typedef struct _IARM_Bus_WiFiSrvMgr_Param_t {
    union {
        WiFiLNFStatusCode_t wifiLNFStatus;
        WiFiStatusCode_t wifiStatus;
        setWiFiAdapter setwifiadapter;
        WiFiConnection connect;
        WiFiConnection saveSSID;
        WiFiConnection clearSSID;
        WiFiConnectedSSIDInfo_t getConnectedSSID;
        WiFiPairedSSIDInfo_t getPairedSSIDInfo;
        WiFiConnectionTypeCode_t connectionType;
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
        struct _WIFI_SSID_LIST {
            char ssid_list[MAX_SSIDLIST_BUF];
            bool more_data;
        } wifiSSIDList;
    } data;
} IARM_BUS_WiFiSrvMgr_EventData_t;

/*! Events published from WiFi Service Manager */
typedef enum _IARM_Bus_NMgr_WiFi_EventId_t {
    IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged = 1,
    IARM_BUS_WIFI_MGR_EVENT_onError,
    IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged,
    IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDs,
    IARM_BUS_WIFI_MGR_EVENT_onAvailableSSIDsIncr,
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
    char supportedFrequencyBands[24];
    char operatingFrequencyBand[24];
    char supportedStandards[20];
    char operatingStandards[24];
    char possibleChannels[BUFF_LENGTH_256];
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
    char bssid[BSSID_BUFF];
    char macaddr[BUFF_MAC];
    char ssid[SSID_SIZE];
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

typedef struct _WiFi_EndPoint_Stats_Diag_Params {
    unsigned long	lastDataDownlinkRate;
    unsigned long	lastDataUplinkRate;
    int				signalStrength;
    unsigned long	retransmissions;
} WiFi_EndPoint_Stats_Diag_Params;

typedef struct _WiFi_EndPoint_Security_Params {
    char	modesSupported[BUFF_LENGTH_256];
} WiFi_EndPoint_Security_Params;

typedef struct _WiFi_EndPoint_Diag_Params {
    bool	enable;
    char	status[BUFF_LENGTH_64];
    char	alias[BUFF_LENGTH_64];
    char	ProfileReference[BUFF_LENGTH_256];
    char	SSIDReference[BUFF_LENGTH_256];
    unsigned int ProfileNumberOfEntries;
    WiFi_EndPoint_Stats_Diag_Params stats;
    WiFi_EndPoint_Security_Params 	security;
} WiFi_EndPoint_Diag_Params;

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
        unsigned int endPointNumberOfEntries;
        WiFi_EndPoint_Diag_Params endPointInfo;
    } data;
    bool status;
    IARM_Bus_WiFiSrvMgr_NumEntry_t numEntry;
} IARM_BUS_WiFi_DiagsPropParam_t;

typedef enum _NetworkManager_WiFi_EventId_t {
        IARM_BUS_NETWORK_MANAGER_EVENT_SWITCH_TO_PRIVATE = 5,
        IARM_BUS_NETWORK_MANAGER_EVENT_STOP_LNF_WHILE_DISCONNECTED,
        IARM_BUS_NETWORK_MANAGER_EVENT_AUTO_SWITCH_TO_PRIVATE_ENABLED,
        IARM_BUS_NETWORK_MANAGER_EVENT_MAX,
} IARM_Bus_NetworkManager_WiFi_EventId_t;
typedef struct _IARM_BUS_NetworkManager_EventData_t {
        int value;
} IARM_BUS_NetworkManager_EventData_t;

#ifdef WIFI_CLIENT_ROAMING
typedef enum _WiFi_Roaming_Status_t {
    ROAM_PARAM_SUCCESS = 0,
    ROAM_PARAM_FAILURE = -1,
    ROAM_PARAM_DISABLED = -2
} WiFi_Roaming_Status_t;

typedef struct _WiFi_RoamingCtrl_t {
        WiFi_Roaming_Status_t status;
        bool roamingEnable;
        bool roaming80211kvrEnable;
        bool selfSteerOverride;
        int preassnBestThreshold;
        int preassnBestDelta;
        int postAssnLevelDeltaConnected;
        int postAssnLevelDeltaDisconnected;
        int postAssnSelfSteerThreshold;
        int postAssnSelfSteerTimeframe;
        int postAssnBackOffTime;
        //int postAssnSelfSteerBeaconsMissedTime;
        int postAssnAPcontrolThresholdLevel;
        int postAssnAPcontrolTimeframe;
    
} WiFi_RoamingCtrl_t;
#endif
#endif
