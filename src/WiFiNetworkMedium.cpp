using namespace std;
#include "NM_WiFiManager.h"
#include "NetworkMedium.h"

#include "hostIf_tr69ReqHandler.h"

WiFiNetworkMedium::WiFiNetworkMedium(NetworkMedium::NetworkType _type)
{
}

WiFiNetworkMedium::~WiFiNetworkMedium()
{

}

int  WiFiNetworkMedium::Start()
{

    IARM_Result_t err = IARM_RESULT_IPCCORE_FAIL;
    err = IARM_Bus_Init(IARM_BUS_NM_MGR_NAME);

    if(IARM_RESULT_SUCCESS != err)
    {
        LOG("Error initializing IARM.. error code : %d\n",err);
        return err;
    }


    IARM_Bus_Connect();

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
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MGR_API_isPaired,isPaired);

}

int  WiFiNetworkMedium::Stop()
{
}

IARM_Result_t WiFiNetworkMedium::getAvailableSSIDs(void *arg)
{
    IARM_Result_t retCode = IARM_RESULT_IPCCORE_FAIL;
    NM_getAvailableSSID_Args *param = (NM_getAvailableSSID_Args *)arg;

    /* Query from hostif */

    unsigned int num_ssids = 0;
    HOSTIF_MsgData_t param = {0};
    strncpy(param.paramName, "Device.WiFi.SSIDNumberOfEntries", strlen("Device.WiFi.SSIDNumberOfEntries")+1)
    param.reqType = HOSTIF_GET;
    param.paramtype = hostIf_UnsignedIntType;


    retCode = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME,
                            IARM_BUS_TR69HOSTIFMGR_API_GetParams,
                            (void *)&param,
                            sizeof(HOSTIF_MsgData_t));

    num_ssids = (unsigned int) param.paramvalue; 
    
    return retCode;
}

IARM_Result_t WiFiNetworkMedium::getCurrentState(void *arg)
{
    IARM_Result_t retCode = IARM_RESULT_IPCCORE_FAIL;
    NM_getCurrentState *param = (NM_getCurrentState *)arg;
    return retCode;
}

IARM_Result_t WiFiNetworkMedium::connect(void *arg)
{
    IARM_Result_t retCode = IARM_RESULT_IPCCORE_FAIL;
    NM_connect_Args *param = (NM_connect_Args *)arg;
    return retCode;
}

IARM_Result_t WiFiNetworkMedium::saveSSID(void* arg)
{
    IARM_Result_t retCode = IARM_RESULT_IPCCORE_FAIL;
    NM_saveSSID_Args *param = (NM_saveSSID_Args *)arg;
    return retCode;
}

IARM_Result_t WiFiNetworkMedium::clearSSID(void* arg)
{
    IARM_Result_t retCode = IARM_RESULT_IPCCORE_FAIL;
    NM_clearSSID_Args *param = (NM_clearSSID_Args *)arg;
    return retCode;
}

IARM_Result_t WiFiNetworkMedium::getPairedSSID(void *arg)
{
    IARM_Result_t retCode = IARM_RESULT_IPCCORE_FAIL;
    NM_getPairedSSID_Args *param = (NM_getPairedSSID_Args *)arg;
    return retCode;
}

IARM_Result_t WiFiNetworkMedium::isPaired(void *arg)
{
    IARM_Result_t retCode = IARM_RESULT_IPCCORE_FAIL;
    NM_isPaired_Args *param = (NM_isPaired_Args *)arg;
    return retCode;
}

//---------------------------------------------------------
// generic Api for get HostIf parameters through IARM_TR69Bus
// --------------------------------------------------------
int get_ParamValues_tr69hostIf (const char *name, ParameterType type, ParameterValue *value)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
//  g_printf("[%s:%s:%d] Entering..\n", __FILE__, __FUNCTION__, __LINE__);

    HOSTIF_MsgData_t param = {0};

    if(NULL == name)    {
        DEBUG_OUTPUT (dbglog(SVR_ERROR, DBG_ACCESS,"Failed due to Parameter name is NULL\n");)
        return ERR_INVALID_PARAMETER_NAME;
    }

    if(name)
    {
        /* Initialize hostIf get structure */
        strncpy(param.paramName, name, strlen(name)+1);
        param.reqType = HOSTIF_GET;
        //printf("[%s:%s:%d] Param: %s DataType: %d\n",  __FILE__, __FUNCTION__, __LINE__, name, type);
        switch (type) {
        case StringType:
        case DefStringType:
        case DateTimeType:
        case hexBinaryType:
        case DefHexBinaryType:
            param.paramtype = hostIf_StringType;
            break;
        case IntegerType:
        case UnsignedIntType:
        case DefUnsignedIntType:
        case UnsignedLongType:
        case DefUnsignedLongType:
            param.paramtype = hostIf_IntegerType;
        case BooleanType:
            param.paramtype = hostIf_BooleanType;

        default:
            break;
        }

        /* IARM get Call*/
        ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME,
                            IARM_BUS_TR69HOSTIFMGR_API_GetParams,
                            (void *)&param,
                            sizeof(HOSTIF_MsgData_t));

        if(ret != IARM_RESULT_SUCCESS) {
            DEBUG_OUTPUT( dbglog(SVR_ERROR, DBG_ACCESS,"[%s:%s:%d] Failed in IARM_Bus_Call(), with return value: %d\n", __FILE__, __FUNCTION__, __LINE__, ret); )
            return ERR_REQUEST_DENIED;
        }
        else
        {
            DEBUG_OUTPUT(
                dbglog(SVR_DEBUG, DBG_ACCESS,"[%s:%s:%d] The value of param: %s paramLen : %d\n", __FILE__, __FUNCTION__, __LINE__, name, param.paramLen); )
            //printf("[%s:%s:%d] The param \'%s\' value : %s paramLen : %d\n", __FILE__, __FUNCTION__, __LINE__, name, (char *)param.paramValue, param.paramLen);
        }

        int str_len = 0;

        switch(type)
        {
        case StringType:
        case DefStringType:
            /* get the value as string */
            str_len = strlen(param.paramValue);
            if(str_len)
            {
                value->out_cval = (char *)malloc(str_len+1);
                if (value != NULL)
                {
                    strncpy(value->out_cval, (char*)param.paramValue, str_len+1);
                }
                else
                {
                    DEBUG_OUTPUT (dbglog(SVR_ERROR, DBG_ACCESS,"%s: Failed to allocate memory for ParamValue\n", __FUNCTION__);)
                }
            }
            else {
                DEBUG_OUTPUT (dbglog(SVR_ERROR, DBG_ACCESS,"%s(): Failed: No value returned for Param: %s\n", __FUNCTION__, name);)
                value->out_cval = NULL;
            }
            break;
        case DateTimeType:
            if(strlen(param.paramValue))
            {
                s2dateTime(param.paramValue,&(value->in_timet));
            } else
                s2dateTime(UNKNOWN_TIME, &(value->in_timet));
            break;
        case IntegerType:
        case UnsignedIntType:
        case DefUnsignedIntType:
        case UnsignedLongType:
        case DefUnsignedLongType:
            value->out_int = get_int(param.paramValue);
            //  printf("[%s:%s:%d] ParamPath: [%s]; PramaValue: [%d] \n", __FILE__, __FUNCTION__, __LINE__, name,value->out_int, param.paramLen, str_len);
            break;
        case BooleanType:
            value->out_int = get_boolean(param.paramValue);
            //  printf("[%s:%s:%d] ParamPath: [%s]; PramaValue: [%d] \n", __FILE__, __FUNCTION__, __LINE__, name,value->out_int, param.paramLen, str_len);
            break;
        case hexBinaryType:
        case DefHexBinaryType:
            str_len = strlen(param.paramValue);
            if(str_len == 0)
            {
                value->out_hexBin = (char*)malloc(1);
                *(value->out_hexBin) = '\0';
            }
            else
            {
                value->in_hexBin = (char*)malloc(str_len+1);
                strncpy(value->out_hexBin, param.paramValue, str_len);
                *(value->out_hexBin + str_len) = '\0';
            }
            break;
        default:
            break;
        }
    }
    return OK;

}
