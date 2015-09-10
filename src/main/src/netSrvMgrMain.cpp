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

#include "NetworkMgrMain.h"
#include "wifiSrvMgr.h"

char networkMgr_ConfigProp_FilePath[100] = {'\0'};

static void netSrvMgr_start();
static void netSrvMgr_Loop();

#ifdef RDK_LOGGER_ENABLED
int b_rdk_logger_enabled = 0;

void logCallback(const char *buff)
{
    DEBUG_LOG("%s",buff);
}
#endif

static void NetworkMgr_SignalHandler (int sigNum);

void NetworkMgr_SignalHandler (int sigNum)
{
    RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "%s(): Received signal %d \n",__FUNCTION__, sigNum);
    signal(sigNum, SIG_DFL );
    kill(getpid(), sigNum );
    exit(0);
}

int main(int argc, char *argv[])
{
    const char* debugConfigFile = NULL;
    const char* configFilePath = NULL;
    int itr=0;

    /* Signal handler */
    signal (SIGHUP, NetworkMgr_SignalHandler);
    signal (SIGINT, NetworkMgr_SignalHandler);
    signal (SIGQUIT, NetworkMgr_SignalHandler);
    signal (SIGTERM, NetworkMgr_SignalHandler);

    signal (SIGPIPE, SIG_IGN);


    while (itr < argc)
    {
        if(strcmp(argv[itr],"--debugconfig")==0)
        {
            itr++;
            if (itr < argc)
            {
                debugConfigFile = argv[itr];
            }
            else
            {
                break;
            }
        }

        if(strcmp(argv[itr],"--configFilePath")==0)
        {
            itr++;
            if (itr < argc)
            {
                configFilePath = argv[itr];
                memcpy(networkMgr_ConfigProp_FilePath, configFilePath, strlen(configFilePath));
            }
            else
            {
                break;
            }
        }
        itr++;
    }

#ifdef RDK_LOGGER_ENABLED
    if(rdk_logger_init(debugConfigFile) == 0) b_rdk_logger_enabled = 1;
    if(configFilePath) {
        memcpy(strMgr_ConfigProp_FilePath, configFilePath, strlen(configFilePath))
    }
    IARM_Bus_RegisterForLog(logCallback);
#else
    rdk_logger_init(debugConfigFile);
#endif

#ifdef ENABLE_SD_NOTIFY
    sd_notifyf(0, "READY=1\n"
              "STATUS=netsrvmgr is Successfully Initialized\n"
              "MAINPID=%lu",
              (unsigned long) getpid());
#endif

    netSrvMgr_start();
    netSrvMgr_Loop();

}

void netSrvMgr_start()
{
    WiFiNetworkMgr::getInstance()->Start();
}

void netSrvMgr_Loop()
{
    time_t curr = 0;
    while(1)
    {
        time(&curr);
        printf("I-ARM NET-SRV-MGR: HeartBeat at %s\r\n", ctime(&curr));
        sleep(60);
    }
}



#if 0
NetworkMedium* createNetworkMedium(NetworkMedium::NetworkType _type)
{
    NetworkMedium* mMedium=NULL;

    if(_type==NetworkMedium::WIFI)
    {
        mMedium = new WiFiNetworkMgr(_type);
    }
}
#endif
