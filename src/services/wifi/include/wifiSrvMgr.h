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


#ifndef _WIFINETWORKMGR_H_
#define _WIFINETWORKMGR_H_

#include "libIBus.h"
#include "libIARM.h"



class WiFiNetworkMgr/*: public NetworkMedium*/
{
public:

    static WiFiNetworkMgr* getInstance();
    static bool isReady();

    int Start();
    int Stop();
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
private:

    WiFiNetworkMgr();
    virtual ~WiFiNetworkMgr();

    static bool m_isenabled;
    static WiFiNetworkMgr* instance;
    static bool instanceIsReady;
};

#endif /* _WIFINETWORKMGR_H_ */
