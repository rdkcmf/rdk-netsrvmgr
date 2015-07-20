
#include "NetworkMgrMain.h"
#include "WiFiNetworkMgr.h"

char networkMgr_ConfigProp_FilePath[100] = {'\0'};

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
    printf("[%s:%s]\n NetworkMgr signal handler with sigNum : %d \r\n",__FILE__, __FUNCTION__, sigNum);
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
#endif

    WiFiNetworkMgr* WiFiNetwork = createNetworkMedium(NetworkMedium::WIFI);

    WiFiNetwork->Start();

    while(true)
    {
        sleep(300);
    }
}

NetworkMedium* createNetworkMedium(NetworkMedium::NetworkType _type)
{
    NetworkMedium* mMedium=NULL;

    if(_type==NetworkMedium::WIFI)
    {
        mMedium = new WiFiNetworkMgr(_type);
    }
}
