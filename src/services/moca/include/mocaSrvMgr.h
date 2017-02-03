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


#include "libIARM.h"
#include "libIBus.h"
class MocaNetworkMgr
{
public:
    static MocaNetworkMgr* getInstance();
    int Start();
    static void printMocaTelemetry();
    static IARM_Result_t mocaTelemetryLogEnable(void *arg);
    static IARM_Result_t mocaTelemetryLogDuration(void *arg);
private:
    MocaNetworkMgr();
    virtual ~MocaNetworkMgr();
    static bool instanceIsReady;
    static MocaNetworkMgr* instance;
};

void startMocaTelemetry();
void *mocaTelemetryThread(void* arg);
static void _mocaEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
