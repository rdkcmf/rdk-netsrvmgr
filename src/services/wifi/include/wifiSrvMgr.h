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


#ifndef _WIFINETWORKMGR_H_
#define _WIFINETWORKMGR_H_

#ifdef ENABLE_IARM
#include "libIBus.h"
#include "libIARM.h"
#endif

class WiFiNetworkMgr/*: public NetworkMedium*/
{
public:

    static WiFiNetworkMgr* getInstance();
    static bool isReady();

    int Init();
    int Start();
    int Stop();
#ifdef ENABLE_IARM
    static IARM_Result_t getPairedSSIDInfo(void *arg);
    static IARM_Result_t getAvailableSSIDs(void *arg);
    static IARM_Result_t getAvailableSSIDsWithName(void *arg);
    static IARM_Result_t getAvailableSSIDsAsync(void *arg);
    static IARM_Result_t getAvailableSSIDsAsyncIncr(void *arg);
    static IARM_Result_t stopProgressiveWifiScanning(void *arg);
    static IARM_Result_t getCurrentState(void *arg);
    static IARM_Result_t setEnabled(void *arg);
    static IARM_Result_t connect(void *arg);
    static IARM_Result_t initiateWPSPairing(void *arg);
    static IARM_Result_t saveSSID(void* arg);
    static IARM_Result_t clearSSID(void* arg);
    static IARM_Result_t disconnectSSID(void* arg);
    static IARM_Result_t getPairedSSID(void *arg);
    static IARM_Result_t isPaired(void *arg);
    static IARM_Result_t getConnectedSSID(void *arg);
    static IARM_Result_t cancelWPSPairing(void *arg);

    static IARM_Result_t getRadioProps(void *arg);
    static IARM_Result_t setRadioProps(void *arg);
    static IARM_Result_t getRadioStatsProps(void *arg);
    static IARM_Result_t getSSIDProps(void *arg);
    static IARM_Result_t sysModeChange(void *arg);
    static IARM_Result_t getEndPointProps(void *args);
    static IARM_Result_t isStopLNFWhileDisconnected(void *arg);
    static IARM_Result_t getSwitchToPrivateResults(void *arg);
    static IARM_Result_t isAutoSwitchToPrivateEnabled(void *arg);

#ifdef WIFI_CLIENT_ROAMING
    static IARM_Result_t setRoamingCtrls(void *arg);
    static IARM_Result_t getRoamingCtrls(void *arg);
#endif


    static IARM_Result_t getCurrentConnectionType(void *arg);
#ifdef ENABLE_LOST_FOUND
    static IARM_Result_t getLNFState(void *arg);
#endif
#endif
private:

    WiFiNetworkMgr();
    virtual ~WiFiNetworkMgr();

#ifndef ENABLE_XCAM_SUPPORT
/**
 * @addtogroup NETSRVMGR_APIS
 * @{
 */

/**
 * @brief This function 1. Extract "SSID", "BSSID", "Password", "Security" from specified netapp_db_file.
 * 2. Write into specified wpa_supplicant_conf_file in following format:
 *        network={
 *            ssid="<SSID>"
 *            scan_ssid=1
 *            bssid=<BSSID>
 *            psk="<Password"
 *            key_mgmt=<Security>
 *            auth_alg=OPEN
 *            }
 *    example:
 *        network={
 *            ssid="tukken-axb3-5GHz"
 *            scan_ssid=1
 *            bssid=5C:B0:66:00:4D:10
 *            psk="Comcast2015"
 *            key_mgmt=Wpa2PskAes
 *            auth_alg=OPEN
 *            }
 *
 * @param[in] wpa_supplicant_conf_file    WPA supplicant configuration file to be created.
 * @param[in] netapp_db_file              NetAPP DB file to query configuration information.
 *
 * @return  Returns 0 if successfully created wpa_supplicant_conf_file, Otherwise 1.
 */
    int create_wpa_supplicant_conf_from_netapp_db (const char* wpa_supplicant_conf_file, const char* netapp_db_file);
/** @} */  //END OF GROUP NETSRVMGR_APIS
#endif // ENABLE_XCAM_SUPPORT

    static bool m_isenabled;
    static WiFiNetworkMgr* instance;
    static bool instanceIsReady;
};

#endif /* _WIFINETWORKMGR_H_ */
