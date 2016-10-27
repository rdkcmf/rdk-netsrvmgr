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
#include "routeSrvMgr.h"

char configProp_FilePath[100] = {'\0'};;
netMgrConfigProps confProp;

/*Telemetry Configuration Parameter List*/
#ifdef USE_RDK_WIFI_HAL
telemetryParams wifiParams_Tele_Period1;
telemetryParams wifiParams_Tele_Period2;
#endif
static bool parse_telemetry_logging_configuration( char *);
static void teleParamList_free (gpointer val);

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
static bool read_ConfigProps();
void Read_Telemetery_Param_File();
static bool update_telemetryParams_list(gchar *, telemetryParams *, gchar *, gchar *);
void NetworkMgr_SignalHandler (int sigNum)
{
    RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "%s(): Received signal %d \n",__FUNCTION__, sigNum);
    signal(sigNum, SIG_DFL );
#ifdef USE_RDK_WIFI_HAL
    /* Telemetry Parameter list*/
    if (wifiParams_Tele_Period1.paramlist != NULL)    {
        g_list_free_full(wifiParams_Tele_Period1.paramlist, (GDestroyNotify)&teleParamList_free);
    }

    if (wifiParams_Tele_Period2.paramlist != NULL)
    {
        g_list_free_full(wifiParams_Tele_Period2.paramlist, (GDestroyNotify)&teleParamList_free);
    }
#endif
    kill(getpid(), sigNum );
    exit(0);
}

int main(int argc, char *argv[])
{
    const char* debugConfigFile = NULL;
    const char* configFilePath = NULL;
    int itr=0;
    IARM_Result_t err = IARM_RESULT_IPCCORE_FAIL;

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
                memcpy(configProp_FilePath, configFilePath, strlen(configFilePath));
            }
            else
            {
                break;
            }
        }
        itr++;
    }
    err = IARM_Bus_Init(IARM_BUS_NM_SRV_MGR_NAME);
    if(IARM_RESULT_SUCCESS != err)
    {
        //LOG("Error initializing IARM.. error code : %d\n",err);
        return err;
    }
    err = IARM_Bus_Connect();
    if(IARM_RESULT_SUCCESS != err)
    {
        //LOG("Error connecting to IARM.. error code : %d\n",err);
        return err;
    }

#ifdef RDK_LOGGER_ENABLED
    if(rdk_logger_init(debugConfigFile) == 0) b_rdk_logger_enabled = 1;
    if(configFilePath) {
        memcpy(configProp_FilePath, configFilePath, strlen(configFilePath))
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
    if(false == read_ConfigProps()) {
        confProp.wifiProps.max_timeout = MAX_TIME_OUT_PERIOD;
        confProp.wifiProps.statsParam_PollInterval = MAX_TIME_OUT_PERIOD;
#ifdef ENABLE_LOST_FOUND
        confProp.wifiProps.bEnableLostFound = false;
        confProp.wifiProps.lnfRetryInSecs = MAX_TIME_OUT_PERIOD;
        confProp.wifiProps.lnfStartInSecs = MAX_TIME_OUT_PERIOD;
        strcpy(confProp.wifiProps.authServerURL,"http://localhost:50050/authService/getDeviceId");
#endif
    }
    Read_Telemetery_Param_File();
    netSrvMgr_start();
    netSrvMgr_Loop();

}

void netSrvMgr_start()
{
    RouteNetworkMgr::getInstance()->Start();
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


static bool read_ConfigProps()
{
    bool status = false;
    GKeyFile *key_file = NULL;
    GError *error = NULL;
    gsize length = 0;
    gdouble double_value = 0;
    guint group = 0, key = 0;

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Entering... \n",__FUNCTION__);

    if(configProp_FilePath[0] == '\0')
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] Failed to read NETSRVMGR Configuration file \n", __FUNCTION__, __LINE__);
        return false;
    }

    key_file = g_key_file_new();

    if(!key_file) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] Failed to g_key_file_new() \n", __FUNCTION__, __LINE__);
        return false;
    }

    if(!g_key_file_load_from_file(key_file, configProp_FilePath, G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] Failed with \"%s\"", __FUNCTION__, __LINE__, error->message);
        return false;
    }
    else
    {
        gsize groups_id, num_keys;
        gchar **groups = NULL, **keys = NULL, *value = NULL;

        groups = g_key_file_get_groups(key_file, &groups_id);

        for(group = 0; group < groups_id; group++)
        {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d]Group %u/%u: \t%s\n", __FUNCTION__, __LINE__, group, groups_id - 1, groups[group]);
            if(0 == strncasecmp(WIFI_CONFIG, groups[group], strlen(groups[group])))
            {
                keys = g_key_file_get_keys(key_file, groups[group], &num_keys, &error);
                for(key = 0; key < num_keys; key++)
                {
                    value = g_key_file_get_value(key_file,	groups[group],	keys[key],	&error);
                    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[ \t\tkey %u/%u: \t%s => %s]\n", key, num_keys - 1, keys[key], value);
                    if(0 == strncasecmp(MAX_TIMEOUT_ON_DISCONNECT, keys[key], strlen(keys[key])))
                    {
                        confProp.wifiProps.max_timeout = atoi(value);
                    }
                    if(0 == strncasecmp(STATS_POLL_INTERVAL, keys[key], strlen(keys[key])))
                    {
                        confProp.wifiProps.statsParam_PollInterval = atoi(value);
                    }
#ifdef ENABLE_LOST_FOUND
                    if(0 == strncasecmp(ENABLE_LOST_FOUND_RUN, keys[key], strlen(keys[key])))
                    {
                        confProp.wifiProps.bEnableLostFound = (atoi(value) == 0) ? false : true;
                    }
                    if(0 == strncasecmp(LAF_CONNECT_RETRY_INTERVAL, keys[key], strlen(keys[key])))
                    {
                        confProp.wifiProps.lnfRetryInSecs = atoi(value);
                    }
                    if(0 == strncasecmp(LAF_CONNECT_START_INTERVAL, keys[key], strlen(keys[key])))
                    {
                        confProp.wifiProps.lnfStartInSecs = atoi(value);
                    }
#endif
                    if(0 == strncasecmp(AUTHSERVER_URL, keys[key], strlen(keys[key])))
                    {
                        strcpy(confProp.wifiProps.authServerURL,value);
                    }
                    if(0 == strncasecmp(DISABLE_WPS_XRE, keys[key], strlen(keys[key])))
                    {
                        confProp.wifiProps.disableWpsXRE = (atoi(value) == 0) ? false : true;
                    }
                    if(value) g_free(value);
                }
                if(keys) g_strfreev(keys);
            }
        }
        if(groups) g_strfreev(groups);
    }
    if(key_file) g_key_file_free(key_file);

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Exiting... \n",__FUNCTION__);
    return true;
}

/*Read Telemetry Parameter configurations*/
void Read_Telemetery_Param_File()
{
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Entering... \n",__FUNCTION__);

    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;

    gboolean fstatus =  g_file_get_contents ((const gchar *) TELEMETRY_LOGGING_PARAM_FILE, &contents, &length, &error);

    if(!fstatus) {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR , "[%s()] Failed to read \"%s\" file using g_file_get_contents() due to %s(%d) \n",
                __FUNCTION__, TELEMETRY_LOGGING_PARAM_FILE, error->message, error->code);
        return;
    }
    else {
        RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR,"[%s]Successfully read \"%s\". The file Contents are \"%s\" with length (%d).\n ",\
                __FUNCTION__, TELEMETRY_LOGGING_PARAM_FILE,  contents, (int)length);
    }

    if(contents)  {
        parse_telemetry_logging_configuration(contents);
        g_free(contents);
    }

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Exiting... \n",__FUNCTION__);
}

/*Print parameters in list*/
static void printf_list_info(GList *list)
{
    if(!list) {
        printf("Since list is NULL, Failed to print info.");
        return;
    }

    GList *iter = g_list_first(list);
    while(iter)
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR ,"[%s] Parameter Name :  \"%s\"\n", __FUNCTION__, (char *)iter->data);
        iter = g_list_next(iter);
    }
}


bool parse_telemetry_logging_configuration(gchar *string)
{
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Entering... \n",__FUNCTION__);

    if(NULL == string)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR ,"[%s:%d] Failed due to NULL request buffer.\n", __FUNCTION__, __LINE__);
        return false;
    }

#ifdef USE_RDK_WIFI_HAL
    wifiParams_Tele_Period1.timePeriod = 0;
    wifiParams_Tele_Period1.paramlist = NULL;

    update_telemetryParams_list(string, &wifiParams_Tele_Period1, (gchar *)T_PERIOD_1_INTERVAL, (gchar *)T_PERIOD_1_PARAMETER_LIST);

    wifiParams_Tele_Period2.timePeriod = 0;
    wifiParams_Tele_Period2.paramlist = NULL;

    update_telemetryParams_list(string, &wifiParams_Tele_Period2, (gchar *)T_PERIOD_2_INTERVAL, (gchar *)T_PERIOD_2_PARAMETER_LIST);

#endif

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Exiting... \n",__FUNCTION__);
    return true;
}

bool update_telemetryParams_list(gchar *input_buffer, telemetryParams *telemery_params, gchar *time_period_string, gchar *param_name_string)
{
    cJSON *request_msg = NULL;

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Entering... \n",__FUNCTION__);

    request_msg = cJSON_Parse(input_buffer);

    if (!request_msg)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s] Failed to parse json buffer with \"%s\"\n",cJSON_GetErrorPtr());
        return false;
    }
    else
    {
        int item, arrSize = 0;
        cJSON* param_item = NULL, *param_list_obj = NULL;

        telemery_params->timePeriod =  cJSON_GetObjectItem(request_msg, time_period_string)->valueint;
        param_list_obj = cJSON_GetObjectItem(request_msg, param_name_string);

        arrSize = cJSON_GetArraySize(param_list_obj);

        for ( item = 0; item < arrSize; item++) {
            param_item = cJSON_GetArrayItem(param_list_obj, item);
            if(!param_item) {
                RDK_LOG(RDK_LOG_ERROR, LOG_NMGR ,"[%s]Failed in cJSON_GetArrayItem() \n", __FUNCTION__);
            }
            else {
                telemery_params->paramlist = g_list_prepend(telemery_params->paramlist, g_strdup(param_item->valuestring));
            }
        }
        if(telemery_params->paramlist)
            telemery_params->paramlist = g_list_reverse(telemery_params->paramlist);

        cJSON_Delete(request_msg);
    }

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Exiting... \n",__FUNCTION__);
    return true;
}

void teleParamList_free (gpointer val)
{
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Exiting... \n",__FUNCTION__);
    if(val) {
        RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR,"[%s] :\"%s\"\n", __FUNCTION__, (gchar *)val);
        g_free (val);
    }
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR , "[%s()] Exiting... \n",__FUNCTION__);
}

