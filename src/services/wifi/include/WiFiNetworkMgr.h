/*
 * WiFiNetworkMgr.h
 *
 *  Created on: Jul 17, 2015
 *      Author: rdey
 */

#ifndef GENERIC_SRC_INCLUDE_WIFINETWORKMGR_H_
#define GENERIC_SRC_INCLUDE_WIFINETWORKMGR_H_


class WiFiNetworkMgr: public NetworkMedium
{
public:
    enum ConnectionState {
        UNINSTALLED=0,
        DISABLED,
        DISCONNECTED,
        PAIRING,
        CONNECTING,
        CONNECTED,
        FAILED
    };
    enum ErrorCode {
        SSID_CHANGED=0,
        CONNECTION_LOST,
        CONNECTION_FAILED,
        CONNECTION_INTERRUPTED,
        INVALID_CREDENTIALS,
        NO_SSID,
        UNKNOWN
    };

    static WiFiNetworkManager* getInstance();
    static bool isReady();

//    int Start();
//    int Stop();

    static IARM_Result_t getAvailableSSIDs(void *arg);
    static IARM_Result_t getCurrentState(void *arg);
    static IARM_Result_t setEnabled(void *arg);
    static IARM_Result_t connect(void *arg);
    static IARM_Result_t saveSSID(void* arg);
    static IARM_Result_t clearSSID(void* arg);
    static IARM_Result_t getPairedSSID(void *arg);
    static IARM_Result_t isPaired(void *arg);


private:

    WiFiNetworkMgr(NetworkMedium::NetworkType _type);
    ~WiFiNetworkMgr();

    static bool m_isenabled;
    static WiFiNetworkMgr* instance;
    static bool instanceIsReady;
}


#endif /* GENERIC_SRC_INCLUDE_WIFINETWORKMGR_H_ */
