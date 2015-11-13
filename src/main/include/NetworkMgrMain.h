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

#ifndef _NETWORKMGRMAIN_H_
#define _NETWORKMGRMAIN_H_


#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef ENABLE_SD_NOTIFY
#include <systemd/sd-daemon.h>
#endif
#ifdef __cplusplus
}
#endif
#include "rdk_debug.h"
#include "NetworkMedium.h"
#include "irMgr.h"
#include "comcastIrKeyCodes.h"

#define LOG_NMGR "LOG.RDK.NETSRVMGR"

typedef enum _WiFiResult
{
    WiFiResult_ok= 0,
    WiFiResult_notFound,
    WiFiResult_inUse,
    WiFiResult_readError,
    WiFiResult_writeError,
    WiFiResult_invalidParameter,
    WiFiResult_fail
} WiFiResult;

#endif /* _NETWORKMGRMAIN_H_ */
