/*
 * WiFiNetworkMgr.h
 *
 *  Created on: Jul 17, 2015
 *      Author: rdey
 */

#ifndef _WIFINETWORKMGR_H_
#define _WIFINETWORKMGR_H_


#if 0
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

    static WiFiNetworkMgr* getInstance();
    static bool isReady();

    int Start();
    int Stop();
#if 0
    static IARM_Result_t getAvailableSSIDs(void *arg);
    static IARM_Result_t getCurrentState(void *arg);
    static IARM_Result_t setEnabled(void *arg);
    static IARM_Result_t connect(void *arg);
    static IARM_Result_t saveSSID(void* arg);
    static IARM_Result_t clearSSID(void* arg);
    static IARM_Result_t getPairedSSID(void *arg);
    static IARM_Result_t isPaired(void *arg);
#endif

private:

//    WiFiNetworkMgr() {};
//    WiFiNetworkMgr(WiFiNetworkMgr const&){};
    WiFiNetworkMgr(NetworkMedium::NetworkType _type);
    ~WiFiNetworkMgr();

    static bool m_isenabled;
    static WiFiNetworkMgr* instance;
    static bool instanceIsReady;
}
#endif

#endif /* _WIFINETWORKMGR_H_ */
