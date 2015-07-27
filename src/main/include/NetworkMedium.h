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
    /*
        NetworkType getType() {
            return m_type;
        }
    */
protected:
//    NetworkMedium(NetworkMedium::NetworkType _type);
    virtual ~NetworkMedium();
//    NetworkType m_type;


};

#endif

