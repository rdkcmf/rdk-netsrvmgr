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
#ifndef ENABLE_NLMONITOR
#include <net/if.h>
#endif
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

/**
 * @addtogroup NETSRVMGR_TYPES
 * @{
 */
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
/** @} */  //END OF GROUP NETSRVMGR_TYPES

/**
 * @addtogroup NETSRVMGR_APIS
 * @{
 */
#ifdef ENABLE_RTMESSAGE
void rtConnection_init();
void* rtMessage_Receive(void* arg);
void rtConnection_destroy();
void onMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
void onConnectWifi(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
#endif

#ifdef ENABLE_IARM
/**
 * @brief This function is used to get host interface parameters through IARM TR69 bus calls.
 *
 * @param[out] param    Host Interface message data.
 *
 * @return  Returns true if successfully gets the interface parameters, Otherwise false.
 */
bool gpvFromTR069hostif( HOSTIF_MsgData_t *param);
#endif
/**
 * @brief This function is used to get WIFI connectivity status.
 *
 * @return  Returns WIFI connectivity status(whether uninstalled/disabled/disconnected/pairing/connecting/connected/failed).
 */
WiFiStatusCode_t get_WifiRadioStatus();

/**
 * @brief This function is used to get WIFI connection type.
 *
 * @return  Returns WIFI connection type(whether WPS/mannual/LNF/private).
 */
WiFiConnectionTypeCode_t get_WifiConnectionType();

/**
 * @brief This function is used to get the ethernet connection ON/OFF status in order to proceed with IARM bus calls.
 *
 * @return  Returns true if connection status OK, Otherwise false.
 */
bool ethernet_on();

/**
 * @brief This function gets the host interface parameters through IARM TR69 calls and update BSS identifier list data.
 *
 * @return  Returns false in case of WIFI disabled or any other error, Otherwise true in success case.
 */
extern bool updateWiFiList();
extern ssidList gSsidList;
#ifdef ENABLE_IARM
extern IARM_Bus_Daemon_SysMode_t sysModeParam;
#endif

/**
 * @brief This function is used to cancel all the WIFI related threads and un initialize the WIFI module.
 *
 * @return  Returns true if successfully terminated all operations, Otherwise false.
 */
bool shutdownWifi();
#ifdef USE_RDK_WIFI_HAL

/**
 * @brief This function will check the current WIFI connection status and,
 * 1. If status not connected, then it connects with WIFI type WPS.
 * 2. If connected, then it Disconnect first, then again re-connect to same or different AP.
 *  This function waits for certain timeout period during the disconnection period to check the status.
 *
 * @return  Returns true if successfully WPS connection made, Otherwise false.
 */
bool connect_WpsPush();

/**
 * @brief This callback function received to handle WIFI connect action.
 *
 * @param[in] ssidIndex    Service Set Identifier.
 * @param[in] AP_SSID    Access Point SSID.
 * @param[in] connStatus    WIFI connection status.
 *
 */
INT wifi_connect_callback(INT , CHAR *ap, wifiStatusCode_t *err);

/**
 * @brief This callback function received to handle WIFI disconnect action.
 *
 * @param[in] ssidIndex    Service Set Identifier.
 * @param[in] AP_SSID    Access Point SSID.
 * @param[in] connStatus    WIFI connection status.
 *
 * @return  Returns ok, after handled the required WIFI connectivity action.
 */
INT wifi_disconnect_callback(INT , CHAR *ap, wifiStatusCode_t *err);

/**
 * @brief This function is used to connect WIFI from client with an AP using SSID selection.
 * It uses the input parameters to connect using a supported security method and encryption standard.
 * The security methods can make use of passphrase, public/private key pairs etc.
 *
 * @param[in] ssidIndex                    Service Set Identifier Index value.
 * @param[in] ap_SSID                      Access Point SSID.
 * @param[in] ap_security_mode             AP security mode.
 * @param[in] ap_security_WEPKey           Security pass code of WIFI device.
 * @param[in] ap_security_PreSharedKey     The pre shared key.
 * @param[in] ap_security_KeyPassphrase    The key passphrase.
 * @param[in] saveSSID                     Option to save profile in wpa supplicant.
 * @param[in] eapIdentity                  Extensible Authentication Protocol Identity.
 * @param[in] carootcert                   Network CA Root certificate.
 * @param[in] clientcert                   Client certificate.
 * @param[in] privatekey                   Private Key password.
 * @param[in] conType                      WIFI connection type(WPS/MANNUAL/LNF/PRIVATE).
 *
 * @return  Returns true if connection made successfully, Otherwise false in failure case.
 */
bool connect_withSSID(int, char *, SsidSecurity, char *, char *, char *,int,char *,char *,char *,char *,int conType = 0);

/**
 * @brief This function is used to Start Wi-Fi scan & get the result to parser. This is used to manage endpoint list .
 *
 * @param[in] buffer    Buffer to store Neighboring WIFI AP information(eg. ssid, security, signal strength, frequency,etc).
 *
 * @return  Returns false in case of any failure to scan, Otherwise returns true in success case.
 */
bool scan_Neighboring_WifiAP(char *);

/**
 * @brief This function is used to Start Wi-Fi scan with specified SSID details & get the result to parser.
 * This is used to manage endpoint list.
 *
 * @param[in] buffer     Buffer to store Specified SSID WIFI AP information(eg. ssid, security, signal strength, frequency,etc).
 * @param[in] SSID       SSID
 * @param[in] freq_in    Frequency
 *
 * @return  Returns false in case of any failure to scan, Otherwise returns true in success case.
 */
bool scan_SpecificSSID_WifiAP(char *buffer, const char* SSID, double freq_in);

/**
 * @brief This function is used to get previously connected SSID params(SSID, passphrase, bssid, security, etc.,).
 *
 * @param[out] ConnParams    Connection param structure to store SSID parameters.
 *
 * @return  Returns true if successfully gets all the details, Otherwise false.
 */
bool lastConnectedSSID(WiFiConnectionStatus *ConnParams);

/**
 * @brief This function creates a thread to monitor wifi status events.
 */
void monitor_WiFiStatus();

/**
 * @brief This function clears all previously configured SSID and password details and Disconnect to AP.
 *
 * @return  Returns true if successfully disconnect all SSID, Otherwise false.
 */
bool clearSSID_On_Disconnect_AP();
bool disconnectFromCurrentSSID();

/**
 * @brief This function gets WIFI hal version.
 */
bool getHALVersion();
#ifdef WIFI_CLIENT_ROAMING
/**
 * @brief This function is used to get WIFI roaming configurations details using SSID index.
 *
 * @param[out] param    Roaming Control structure to store all configurations.
 *
 * @return  Returns true if successfully gets all roaming configurations, Otherwise false.
 */
bool getRoamingConfigInfo(WiFi_RoamingCtrl_t *param);

/**
 * @brief This function is used to set WIFI roaming configurations(roamingenabled, threshold, delta value etc.,).
 *
 * @param[in] param    Roaming Control structure to store all configurations.
 *
 * @return  Returns true if successfully set the roaming configurations, Otherwise false.
 */
bool setRoamingConfigInfo(WiFi_RoamingCtrl_t *param);
#endif
#endif

#ifdef ENABLE_LOST_FOUND
/**
 * @brief This function returns the WIFI connection status(uninstalled/disabled/disconnected/pairing/connecting/connected/failed).
 */
bool isWifiConnected();

/**
 * @brief This function ensures the device is not connected to LF SSID, so that it trigger the thread to connect to
 * Lost and Found Wireless SSID.
 */
void connectToLAF();

/**
 * @brief This function trigger the thread to connect to private connection.
 */
void lafConnectToPrivate();

/**
 * @brief This function is used to get LAF Wireless SSID.
 *
 * @return  Returns true if successfully gets the LAF SSID Otherwise false.
 */
bool getLAFssid();
//bool isLAFCurrConnectedssid();

/**
 * @brief This function returns the WIFI LNF status(uninitialized/in progress/connected to LNF network/not connected to LNF network/
 * unable to connect to LNF network/ etc.,).
 */
WiFiLNFStatusCode_t get_WiFiLNFStatusCode();
void doLnFBackoff();

/**
 * @brief This function is used to loads application configuration and initializes client.
 * Establish WIFI connection depends on input LAF request type(LNF SSID/PRIVATE WIFI).
 *
 * @param[in] lafRequestType    Lost and Found Request Type(SSID,private WIFI).
 */
bool triggerLostFound(LAF_REQUEST_TYPE lafRequestType);

/**
 * @brief This function is used to get the mac address from interface name.
 *
 * @param[in] ifname    Interface Name.
 * @param[out] data     Buffer to return mac address.
 *
 * @return  Returns true if successfully returns the mac address, Otherwise false.
 */
bool getmacaddress(gchar* ifname,GString *data);

/**
 * @brief This function is used to get LAF device info details such as serial number, mac address, model name.
 *
 * @param[out] dev_info    Buffer to return device info.
 *
 * @return  Returns true if successfully gets all informations, Otherwise false.
 */
bool getDeviceInfo(laf_device_info_t *dev_info);

/**
 * @brief This function is used to store the results which is from switch to private connection with current time.
 *
 * @param[in] lnfError    Error code result.
 * @param[in] currTime    Current time.
 */
bool addSwitchToPrivateResults(int lnfError,char *currTime);

/**
 * @brief This function is used to convert the results list maintained which is from switch to private connection to JSON format.
 *
 * @param[out] buffer    Result Buffer in JSON format.
 */
bool convertSwitchToPrivateResultsToJson(char *buffer);

/**
 * @brief This function clears the switch to private results list.
 */
bool clearSwitchToPrivateResults();
#ifdef ENABLE_IARM

/**
 * @brief This function retrieves manufacturer specific data from the box.
 *
 * @param[in] mfrDataStr    Structure to store manufacturer data.
 * @param[in] mfrType       Manufacturer data request type(model name, serial number, device mac, etc).
 *
 * @return  Returns true if successfully gets the manufacturer data from IARM bus call, Otherwise false.
 */
bool getMfrData(GString* mfrDataStr,mfrSerializedType_t mfrType);
#endif

/**
 * @brief Callback function to connect to WIFI using LF SSID.
 *
 * @param[in] wificred    WIFI credentials(SSID, security mode, passphrase, private key, etc).
 *
 * @return  Returns 0 once successfully established the connection, Otherwise return appropriate error code.
 */
int laf_wifi_connect(laf_wifi_ssid_t* const wificred);

/**
 * @brief Callback function to disconnect to WIFI.
 *
 * @return  Returns 0 once successfully disconnected the connection.
 */
int laf_wifi_disconnect(void);
int laf_get_lfat(laf_lfat_t *lfat);
int laf_set_lfat(laf_lfat_t* const lfat);

/**
 * @brief Callback function to log messages from lost and found.
 *
 * @param[in] level       Log level.
 * @param[in] function    Function name.
 * @param[in] line        Line number.
 * @param[in] msg         Log message.
 */
void log_message(laf_loglevel_t level, char const* function, int line, char const* msg);
#ifdef ENABLE_IARM
static void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
#endif

/**
 * @brief This Function is used to get the device activation state.
 *
 * @return  Returns true if device is activated, false if not activated.
 */
bool getDeviceActivationState();

/**
 * @brief This function is used to start LAF private SSID connection.
 */
void lnfConnectPrivCredentials();
#endif

/**
 * @brief This function is used to check whether the device supports wireless communication or not.
 *
 * @return  Returns true if the device supports wireless connections,Otherwise false.
 */
bool isWiFiCapable();

/**
 * @brief This function is used to get current SSID and connection status from persistent memory.
 *
 * @param[out] currSsidConnInfo    SSID connection details structure.
 */
void get_CurrentSsidInfo(WiFiConnectionStatus *currSsidConnInfo);
#ifdef ENABLE_IARM

/**
 * @brief This function is used to sent iarm message to set hostif parameter.
 *
 * @param[in] name     Parameter name.
 * @param[in] type     Parameter type(string/date and time/boolean).
 * @param[in] value    Parameter value.
 *
 * @return  Returns true if successfully sent the iarm message, Otherwise false.
 */
bool setHostifParam (char *name, HostIf_ParamType_t type, void *value);
#endif

/**
 * @brief This function converts the input data to Boolean type.
 *
 * @param[in] ptr    Address to store the boolean value.
 * @param[in] val    Boolean value.
 */
void put_boolean(char *ptr, bool val);

/**
 * @brief This function converts the input data to integer type.
 *
 * @param[in] ptr    Address to store the integer value.
 * @param[in] val    Integer value.
 */
void put_int(char *ptr, int val);

/**
 * @brief This function retrieve and store the WIFI credentials(SSID, password).
 *
 * @return  Returns true if successfully stored the MFR WIFI credentials, Otherwise false.
 */
bool storeMfrWifiCredentials(void);

/**
 * @brief This function erase the stored MFR WIFI credentials.
 *
 * @return  Returns true if successfully erased the MFR WIFI credentials, Otherwise false.
 */
bool eraseMfrWifiCredentials(void);

#ifdef ENABLE_IARM
/**
 * @brief This function connects to the ssid details stored by MFR
 *
 * @return  Returns true if successfully connects to MFR WIFI credentials, Otherwise false.
 */
bool connectToMfrWifiCredentials(void);
#endif

/**
 * @brief This function is used to get detail radio traffic statistics information.
 *
 * @param[out] params    Structure that saves the traffic statistics.
 *
 * @return  Returns true if successfully gets all traffic statistics informations, Otherwise false.
 */
bool getRadioStats(WiFi_Radio_Stats_Diag_Params *params);

/**
 * @brief This function is used to get the connected SSID informations such as SSID, BSSID, signal strength etc,.
 *
 * @param[out] WiFiConnectedSSIDInfo_t    Structure that saves the connected SSID details.
 */
void getConnectedSSIDInfo(WiFiConnectedSSIDInfo_t *);

/**
 * @brief This function checks for WIFI connection enable status,
 * If connection enabled, then it provide WIFI statistics information such as enable status, SSID, signal strength,
 * data transmission rate etc.,
 *
 * @param[out] WiFi_EndPoint_Diag_Params    WIFI end point diagnostic parameter.
 */
void getEndPointInfo(WiFi_EndPoint_Diag_Params *);
bool cancelWPSPairingOperation();

/** @} */  //END OF GROUP NETSRVMGR_APIS
#endif /* WIFIHALUTILES_H_ */
