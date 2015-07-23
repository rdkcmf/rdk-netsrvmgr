#ifndef _NETWORKMGRMAIN_H_
#define _NETWORKMGRMAIN_H_

#include "libIARM.h"
#include "iarmUtil.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
}
#endif

#include "NetworkMedium.h"

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
