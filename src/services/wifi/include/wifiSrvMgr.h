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

    int Start();
    int Stop();
#ifdef ENABLE_IARM
    static IARM_Result_t getPairedSSIDInfo(void *arg);
    static IARM_Result_t getAvailableSSIDs(void *arg);
    static IARM_Result_t getCurrentState(void *arg);
    static IARM_Result_t setEnabled(void *arg);
    static IARM_Result_t connect(void *arg);
    static IARM_Result_t initiateWPSPairing(void *arg);
    static IARM_Result_t saveSSID(void* arg);
    static IARM_Result_t clearSSID(void* arg);
    static IARM_Result_t getPairedSSID(void *arg);
    static IARM_Result_t isPaired(void *arg);
    static IARM_Result_t getConnectedSSID(void *arg);

    static IARM_Result_t getRadioProps(void *arg);
    static IARM_Result_t setRadioProps(void *arg);
    static IARM_Result_t getRadioStatsProps(void *arg);
    static IARM_Result_t getSSIDProps(void *arg);
    static IARM_Result_t sysModeChange(void *arg);
    static IARM_Result_t getEndPointProps(void *args);
    static IARM_Result_t isStopLNFWhileDisconnected(void *arg);
    static IARM_Result_t getSwitchToPrivateResults(void *arg);
    static IARM_Result_t isAutoSwitchToPrivateEnabled(void *arg);


    static IARM_Result_t getCurrentConnectionType(void *arg);
#ifdef ENABLE_LOST_FOUND
    static IARM_Result_t getLNFState(void *arg);
#endif
#endif
private:

    WiFiNetworkMgr();
    virtual ~WiFiNetworkMgr();

#ifndef ENABLE_XCAM_SUPPORT
    int create_wpa_supplicant_conf_from_netapp_db (const char* wpa_supplicant_conf_file, const char* netapp_db_file);
#endif // ENABLE_XCAM_SUPPORT

    static bool m_isenabled;
    static WiFiNetworkMgr* instance;
    static bool instanceIsReady;
};

#endif /* _WIFINETWORKMGR_H_ */
