/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "NetworkMgrMain.h"
#include "wifiSrvMgr.h"
#ifdef ENABLE_ROUTE_SUPPORT
#include "routeSrvMgr.h"
#endif
#ifdef USE_RDK_MOCA_HAL
#include "mocaSrvMgr.h"
#endif
#include "netsrvmgrUtiles.h"
#include "netsrvmgrIarm.h"

char configProp_FilePath[100] = {'\0'};;
netMgrConfigProps confProp;

/*Telemetry Configuration Parameter List*/
#ifdef USE_RDK_WIFI_HAL
telemetryParams wifiParams_Tele_Period1;
telemetryParams wifiParams_Tele_Period2;
#endif // USE_RDK_WIFI_HAL
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
#ifdef ENABLE_IARM
static void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static IARM_Result_t getActiveInterface(void *arg);
static IARM_Result_t getNetworkInterfaces(void *arg);
static IARM_Result_t isInterfaceEnabled(void *arg);
static IARM_Result_t getInterfaceControlPersistence(void *arg);
static void setInterfaceEnabled(const char* interface, bool enabled);
static void setInterfaceControlPersistence(const char* interface, bool interfaceControlPersistence);
#endif // ENABLE_IARM

static bool validate_interface_can_be_disabled (const char* interface);
#ifdef USE_RDK_WIFI_HAL
static bool bWiFiEnabled = false; // assumes WiFi is disabled when netsrvmgr starts
static bool setWifiEnabled (bool newState);
#endif // USE_RDK_WIFI_HAL

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
#endif // USE_RDK_WIFI_HAL
    kill(getpid(), sigNum );
    exit(0);
}

int main(int argc, char *argv[])
{
    const char* debugConfigFile = NULL;
    const char* configFilePath = NULL;
    int itr=0;
#ifdef ENABLE_IARM
    IARM_Result_t err = IARM_RESULT_IPCCORE_FAIL;
#endif
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
                memset(&configProp_FilePath,0,sizeof(configProp_FilePath));
                strncpy(configProp_FilePath, configFilePath, sizeof(configProp_FilePath));
            }
            else
            {
                break;
            }
        }
        itr++;
    }
#ifdef ENABLE_IARM
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
#endif

#ifdef RDK_LOGGER_ENABLED
    if(rdk_logger_init(debugConfigFile) == 0) b_rdk_logger_enabled = 1;
    if(configFilePath) {
        memset(&configProp_FilePath,0,sizeof(configProp_FilePath));
        strncpy(configProp_FilePath, configFilePath, sizeof(configProp_FilePath));
    }
#ifdef ENABLE_IARM
    IARM_Bus_RegisterForLog(logCallback);
#endif

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
#ifdef ENABLE_IARM
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getActiveInterface, getActiveInterface);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getNetworkInterfaces, getNetworkInterfaces);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_isInterfaceEnabled, isInterfaceEnabled);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getInterfaceControlPersistence, getInterfaceControlPersistence);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED, _eventHandler);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE, _eventHandler);
#endif
    netSrvMgr_start();
    netSrvMgr_Loop();

}

bool validate_interface_can_be_disabled (const char* interface)
{
    char activeInterface[INTERFACE_SIZE];
    if (!netSrvMgrUtiles::getRouteInterface (activeInterface))
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] Cannot determine the active interface\n", __FUNCTION__);
        return false;
    }
    if (strcmp (interface, activeInterface) == 0)
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] Cannot disable the active interface '%s'\n", __FUNCTION__, activeInterface);
        return false;
    }
    return true;
}

void netSrvMgr_start()
{
    LOG_ENTRY_EXIT;

#ifdef ENABLE_ROUTE_SUPPORT
    RouteNetworkMgr::getInstance()->Start();
#endif

#ifdef USE_RDK_WIFI_HAL
    bool enable_wifi = true; // enable WiFi by default;
#ifndef ENABLE_XCAM_SUPPORT
    if (netSrvMgrUtiles::getSavedInterfaceConfig ("WIFI", enable_wifi) == false) // no saved enable/disable config
        enable_wifi = true;
    if (enable_wifi == false && validate_interface_can_be_disabled ("WIFI") == false)
        enable_wifi = true;
#endif // ifndef ENABLE_XCAM_SUPPORT
    setWifiEnabled (enable_wifi);
#endif // USE_RDK_WIFI_HAL

#ifdef USE_RDK_MOCA_HAL
    MocaNetworkMgr::getInstance()->Start();
#endif
}

void netSrvMgr_Loop()
{
    time_t curr = 0;
    while(1)
    {
        time(&curr);
#ifndef ENABLE_XCAM_SUPPORT
        printf("I-ARM NET-SRV-MGR: HeartBeat at %s\r\n", ctime(&curr));
#endif
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
    static char *ethIfName=NULL;

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
        ethIfName=getenv("ETHERNET_INTERFACE");

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
                    if((0 == strncasecmp(ENABLE_LOST_FOUND_RUN, keys[key], strlen(keys[key]))) && (!netSrvMgrUtiles::checkInterfaceActive(ethIfName)))
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
                    if(0 == strncasecmp(GET_AUTHTOKEN_URL, keys[key], strlen(keys[key]) ) )
                    {
                        strcpy(confProp.wifiProps.getAuthTokenUrl,value);
                    }
                    if(0 == strncasecmp(GET_LFAT_URL, keys[key], strlen(keys[key]) ) )
                    {
                        strcpy(confProp.wifiProps.getLfatUrl,value);
                    }
                    if(0 == strncasecmp(SET_LFAT_URL, keys[key], strlen(keys[key]) ) )
                    {
                        strcpy(confProp.wifiProps.setLfatUrl,value);
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

#endif // USE_RDK_WIFI_HAL

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

#ifdef ENABLE_IARM

IARM_Result_t getActiveInterface(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    char devName[INTERFACE_SIZE];
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
    if(netSrvMgrUtiles::getRouteInterface(devName))
        strcpy(param->activeIface,devName);
    else
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] No Route Found.\n", __FUNCTION__, __LINE__);
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Current Active Interface : [%s].\n", __FUNCTION__, __LINE__,param->activeIface);
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_NETWORK_MANAGER_ACTIVE_INTERFACE:%s\n",param->activeIface);
    return ret;
}

IARM_Result_t getNetworkInterfaces(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    char devName[INTERFACE_LIST];
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
    char count = netSrvMgrUtiles::getAllNetworkInterface(devName);
    if(count)
        g_stpcpy(param->allNetworkInterfaces,devName);
    else
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] list of Network Interface is empty.\n", __FUNCTION__, __LINE__);
    param->interfaceCount=count;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] list of Network Interface : [%s].\n", __FUNCTION__, __LINE__,param->allNetworkInterfaces);
    return ret;
}

void _eventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    LOG_ENTRY_EXIT;

    if (strcmp(owner, IARM_BUS_NM_SRV_MGR_NAME)  == 0) {
        IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t  *)data;
        switch (eventId) {
        case IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED:
        {
            setInterfaceEnabled (param->setInterface, param->isInterfaceEnabled);
            break;
        }
        case IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE:
        {
            setInterfaceControlPersistence (param->setInterface, param->isInterfaceEnabled);
            break;
        }
        default:
            break;
        }
    }
}

IARM_Result_t isInterfaceEnabled(void *arg)
{
    LOG_ENTRY_EXIT;

    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
    param->isInterfaceEnabled = false;
    if (false)
        ;
#ifdef USE_RDK_WIFI_HAL
    else if (strcmp (param->setInterface, "WIFI") == 0)
        param->isInterfaceEnabled = bWiFiEnabled;
#endif // USE_RDK_WIFI_HAL
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] interface [%s] not supported\n", __FUNCTION__, param->setInterface);
    }
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface = [%s] enabled = [%s]\n",
            __FUNCTION__, param->setInterface, param->isInterfaceEnabled ? "true" : "false");

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t getInterfaceControlPersistence(void *arg)
{
    LOG_ENTRY_EXIT;

    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
    bool persistedValue;
    param->isInterfaceEnabled = netSrvMgrUtiles::getSavedInterfaceConfig (param->setInterface, persistedValue);
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface = [%s] interfaceControlPersistence = [%s]\n",
            __FUNCTION__, param->setInterface, param->isInterfaceEnabled ? "true" : "false");

    return IARM_RESULT_SUCCESS;
}

void setInterfaceEnabled (const char* interface, bool enabled)
{
    LOG_ENTRY_EXIT;

    bool (*fnPtrSetInterfaceEnabled)(bool) = NULL;
    if (false)
        ;
#ifdef USE_RDK_WIFI_HAL
    else if (strcmp (interface, "WIFI") == 0)
        fnPtrSetInterfaceEnabled = setWifiEnabled;
#endif // USE_RDK_WIFI_HAL
//    else if (strcmp (interface, "ETHERNET") == 0)
//        fnPtrSetInterfaceEnabled = setEthernetEnabled;
//    else if (strcmp (interface, "MOCA") == 0)
//        fnPtrSetInterfaceEnabled = setMocaEnabled;
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface [%s] not supported\n", __FUNCTION__, interface);
        return;
    }

    if ((enabled || validate_interface_can_be_disabled (interface)) &&
            (fnPtrSetInterfaceEnabled != NULL && fnPtrSetInterfaceEnabled (enabled)))
    {
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] success. interface = [%s] enabled = [%s]\n",
                __FUNCTION__, interface, enabled ? "true" : "false");

        // HomeNetworking - Servicemanager spec for "setInterfaceControlPersistence"
        // "When true, ... Furthermore, any time an existing interface is enabled or disabled,
        // the persisted configuration must also be updated."
        bool value;
        if (netSrvMgrUtiles::getSavedInterfaceConfig (interface, value)) // if interface config persistence is active
            netSrvMgrUtiles::saveInterfaceConfig (interface, enabled); // persist new interface enable/disable config
    }
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface = [%s] enabled = [%s]\n",
                __FUNCTION__, interface, enabled ? "true" : "false");
    }
}

void setInterfaceControlPersistence (const char* interface, bool interfaceControlPersistence)
{
    LOG_ENTRY_EXIT;

    bool valueToPersist;
    if (false)
        ;
#ifdef USE_RDK_WIFI_HAL
    else if (strcmp (interface, "WIFI") == 0)
        valueToPersist = bWiFiEnabled;
#endif // USE_RDK_WIFI_HAL
//    else if (strcmp (interface, "ETHERNET") == 0)
//        valueToPersist = bEthernetEnabled;
//    else if (strcmp (interface, "MOCA") == 0)
//        valueToPersist = bMocaEnabled;
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface [%s] not supported\n", __FUNCTION__, interface);
        return;
    }

    bool result = false;
    if (interfaceControlPersistence)
        result = netSrvMgrUtiles::saveInterfaceConfig (interface, valueToPersist); // persist interface enable/disable config
    else
        result = netSrvMgrUtiles::removeSavedInterfaceConfig (interface); // remove persisted interface enable/disable config

    if (result == true)
    {
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] success. interface = [%s] interfaceControlPersistence = [%s]\n",
                __FUNCTION__, interface, interfaceControlPersistence ? "true" : "false");
    }
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface = [%s] interfaceControlPersistence = [%s]\n",
                __FUNCTION__, interface, interfaceControlPersistence ? "true" : "false");
    }
}

#endif // ENABLE_IARM

#ifdef USE_RDK_WIFI_HAL
// TODO: move this into WifiSrvMgr.cpp as WiFiNetworkMgr::setWifiEnabled(bool newState) ?
bool setWifiEnabled (bool newState)
{
    LOG_ENTRY_EXIT;

    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] WiFi state: current [%d] requested [%d]\n", __FUNCTION__, bWiFiEnabled, newState);

    if (!bWiFiEnabled == !newState)
    {
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] Already in requested state. Nothing to do.\n", __FUNCTION__);
        return true;
    }

    bWiFiEnabled = newState; // change WiFi state
    if (bWiFiEnabled)
    {
        WiFiNetworkMgr::getInstance()->Start();
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_NETWORK_MANAGER_ENABLE_WIFI.\n");
    }
    else
    {
        WiFiNetworkMgr::getInstance()->Stop();
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_NETWORK_MANAGER_DISABLE_WIFI.\n");
    }
    return true;
}
#endif // USE_RDK_WIFI_HAL
