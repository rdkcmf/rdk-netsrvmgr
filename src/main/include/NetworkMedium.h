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
    NetworkType getType() {
        return m_type;
    }
protected:
    NetworkMedium(NetworkMedium::NetworkType _type);
    virtual ~NetworkMedium();
    NetworkType m_type;


}

#endif

