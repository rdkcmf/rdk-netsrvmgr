//using namespace std;
#include "NetworkMgrMain.h"
#include "WiFiNetworkMgr.h"
#include "NetworkMedium.h"

#if 0 
//WiFiNetworkMgr* WiFiNetworkMgr::instance = NULL;


//bool WiFiNetworkMgr::instanceIsReady = false;


//WiFiNetworkMgr::WiFiNetworkMgr()
//{
//}
WiFiNetworkMgr::WiFiNetworkMgr(NetworkMedium::NetworkType _type)
{
}

WiFiNetworkMgr::~WiFiNetworkMgr()
{

}

WiFiNetworkMgr* WiFiNetworkMgr::getInstance()
{
    if (instance == NULL)
    {
        instance = new WiFiNetworkMgr();
        instanceIsReady = true;
    }
    return instance;
}


int  WiFiNetworkMgr::Start()
{
/*
    IARM_Result_t err = IARM_RESULT_IPCCORE_FAIL;
    err = IARM_Bus_Init(IARM_BUS_NM_MGR_NAME);

    if(IARM_RESULT_SUCCESS != err)
    {
        LOG("Error initializing IARM.. error code : %d\n",err);
        return err;
    }

    err = IARM_Bus_Connect();

    if(IARM_RESULT_SUCCESS != err)
    {
        LOG("Error connecting to IARM.. error code : %d\n",err);
        return err;
    }

    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MGR_API_getAvailableSSIDs,getAvailableSSIDs);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MGR_API_getCurrentState,getCurrentState);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MGR_API_getPairedSSID,getPairedSSID);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MGR_API_connect,connect);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MGR_API_saveSSID,saveSSID);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MGR_API_clearSSID,clearSSID);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MGR_API_isPaired,isPaired);*/

}

int  WiFiNetworkMgr::Stop()
{

}

#if 0
IARM_Result_t WiFiNetworkMgr::getAvailableSSIDs(void *arg)
{
 /*   IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;

    NM_getAvailableSSID_Args *param = (NM_getAvailableSSID_Args *)arg;

     Query from hostif
    unsigned int num_ssids = 0;
    HOSTIF_MsgData_t param = {0};
    strncpy(param.paramName, "Device.WiFi.SSIDNumberOfEntries", strlen("Device.WiFi.SSIDNumberOfEntries")+1)
    param.reqType = HOSTIF_GET;
    param.paramtype = hostIf_UnsignedIntType;
*/
//    getParamInfoFromHostIf();

    /*
     * retCode = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME,
                            IARM_BUS_TR69HOSTIFMGR_API_GetParams,
                            (void *)&param,
                            sizeof(HOSTIF_MsgData_t));
     */
//    num_ssids = (unsigned int) param.paramvalue;

    return ret;
}

IARM_Result_t WiFiNetworkMgr::getCurrentState(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    NM_getCurrentState *param = (NM_getCurrentState *)arg;
    return ret;
}

IARM_Result_t WiFiNetworkMgr::connect(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    NM_connect_Args *param = (NM_connect_Args *)arg;
    return ret;
}

IARM_Result_t WiFiNetworkMgr::saveSSID(void* arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    NM_saveSSID_Args *param = (NM_saveSSID_Args *)arg;
    return ret;
}

IARM_Result_t WiFiNetworkMgr::clearSSID(void* arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    NM_clearSSID_Args *param = (NM_clearSSID_Args *)arg;
    return ret;
}

IARM_Result_t WiFiNetworkMgr::getPairedSSID(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    NM_getPairedSSID_Args *param = (NM_getPairedSSID_Args *)arg;
    return ret;
}

IARM_Result_t WiFiNetworkMgr::isPaired(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    NM_isPaired_Args *param = (NM_isPaired_Args *)arg;
    return ret;
}
#endif

//---------------------------------------------------------
// generic Api for get HostIf parameters through IARM_TR69Bus
// --------------------------------------------------------
int getParamInfoFromHostIf ()
{
//    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    printf("[%s:%s:%d] Entering..\n", __FILE__, __FUNCTION__, __LINE__);
#if 0
    HOSTIF_MsgData_t param = {0};

    /* Initialize hostIf get structure */
    strncpy(param.paramName, name, strlen(name)+1);
    param.reqType = HOSTIF_GET;

    /* IARM get Call*/
    ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME, IARM_BUS_TR69HOSTIFMGR_API_GetParams, (void *)&param, sizeof(HOSTIF_MsgData_t));

    if(ret != IARM_RESULT_SUCCESS)
    {
        printf("[%s:%s:%d] Failed in IARM_Bus_Call(), with return value: %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
        return ERR_REQUEST_DENIED;
    }
    else
    {
        printf("[%s:%s:%d] The value of param: %s paramLen : %d\n", __FILE__, __FUNCTION__, __LINE__, name, param.paramLen);
        )
    }
#endif
    printf("[%s:%s:%d] Exiting..\n", __FILE__, __FUNCTION__, __LINE__);
    return WiFiResult_ok;
}
#endif
