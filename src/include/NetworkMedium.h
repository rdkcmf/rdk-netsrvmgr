#ifndef NETWORKMEDIUM_H
#define NETWORKMEDIUM_H

/* Base class abstracting different network mediums such as Wifi, Bluetooth etc.,*/
class NetworkMedium
{
public:
    enum NetworkType
    {
        WIFI=0,
        BLUETOOTH,
        MOCA
    };

    virtual int Start();
    virtual int Stop();
    NetworkType getType() {return m_type;}
protected:
    NetworkMedium(NetworkMedium::NetworkType _type);
    virtual ~NetworkMedium();
    NetworkType m_type;
private:
    
}

class WiFiNetworkMedium: public NetworkMedium
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
    static IARM_Result_t getAvailableSSIDs(void *arg);
    static IARM_Result_t getCurrentState(void *arg);
    static IARM_Result_t setEnabled(void *arg);
    static IARM_Result_t connect(void *arg);
    static IARM_Result_t saveSSID(void* arg);
    static IARM_Result_t clearSSID(void* arg);
    static IARM_Result_t getPairedSSID(void *arg);
    static IARM_Result_t isPaired(void *arg);

    int Start();
    int Stop();

private:

    WiFiNetworkMedium(NetworkMedium::NetworkType _type);
    ~WiFiNetworkMedium();
    static bool m_isenabled;
}
#endif

