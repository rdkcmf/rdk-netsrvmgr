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

#ifdef INCLUDE_BREAKPAD
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
#include <client/linux/handler/exception_handler.h>
#else
#include "breakpadwrap.h"
#endif
#endif

#include "NetworkMgrMain.h"
#include "wifiSrvMgr.h"
#include "netsrvmgrUtiles.h"
#include "netsrvmgrIarm.h"
#include <string>
#include <vector>
#include <sstream>

#ifdef ENABLE_ROUTE_SUPPORT
#include "routeSrvMgr.h"
#endif

#ifdef USE_RDK_MOCA_HAL
#include "mocaSrvMgr.h"
#endif

#ifdef ENABLE_NLMONITOR
#include "netlinkifc.h"
#endif

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
#include "rfcapi.h"     // for RFC queries
#endif

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
#ifdef ENABLE_NLMONITOR
static IARM_Result_t getInterfaceList(void *arg);
static IARM_Result_t getDefaultInterface(void *arg);
#endif // ifdef ENABLE_NLMONITOR
static IARM_Result_t setDefaultInterface(void *arg);
static IARM_Result_t isInterfaceEnabled(void *arg);
static IARM_Result_t setInterfaceEnabled(void *arg);
static IARM_Result_t getSTBip(void *arg);
static IARM_Result_t setIPSettings(void *arg);
static IARM_Result_t getIPSettings(void *arg);
static bool getDNSip(char *primaryDNS, char *secondaryDNS);
static IARM_Result_t getSTBip_family(void *arg);
#endif // ifdef ENABLE_IARM

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
static bool isFeatureEnabled(const char* feature);
static bool validate_interface_can_be_disabled (const char* interface);
#ifdef ENABLE_NLMONITOR
static bool getDefaultInterface(std::string &interface, std::string &gateway);
#endif // ifdef ENABLE_NLMONITOR
static bool setDefaultInterface(const char* interface, bool persist);
static bool isInterfaceEnabled(const char* interface, bool& enabled);
static bool setInterfaceEnabled(const char* interface, bool enabled, bool persist);
static bool setInterfaceState(std::string interface_name, bool enabled);
static bool setIPSettings(IARM_BUS_NetSrvMgr_Iface_Settings_t *param);
static bool getIPSettings(IARM_BUS_NetSrvMgr_Iface_Settings_t *param);
#endif // ifndef ENABLE_XCAM_SUPPORT and XHB1

#ifdef USE_RDK_WIFI_HAL
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

#ifdef INCLUDE_BREAKPAD
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
static bool breakpadDumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                        void* context,
                        bool succeeded)
{
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]breakpadDumpCallback: Netsrvmgr crashed ---- Dump path: %s\n", __FUNCTION__, __LINE__,descriptor.path());
    return true;
}
#endif
#endif

// TODO: move this into the utility file?
/**
 * Returns:
 * the specified environment variable's value if it is not NULL.
 * the specified default value otherwise.
 */
char* getenvOrDefault (const char* name, char* defaultValue)
{
    char* value = getenv (name);
    return value ? value : defaultValue;
}

static bool split (const std::string &str, char delimiter, std::vector<std::string> &tokens, int min_expected_tokens = 0)
{
    std::stringstream stream (str);
    std::string token;
    tokens.clear();
    while (getline (stream, token, delimiter))
        tokens.push_back (token);
    if (min_expected_tokens > 0 && tokens.size () < min_expected_tokens)
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]: Unexpected number of tokens. Arguments received = %s\n", __FUNCTION__, __LINE__, str.c_str ());
        return false;
    }
    return true;
}

#ifdef ENABLE_IARM

static void eventInterfaceEnabledStatusChanged(std::string& interface, bool enabled)
{
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: interface=%s, enabled=%d\n", __FUNCTION__, __LINE__, interface.c_str(), enabled);

    IARM_BUS_NetSrvMgr_Iface_EventInterfaceEnabledStatus_t e;
    snprintf(e.interface, sizeof(e.interface), "%s", interface.c_str());
    e.status = enabled;

    if (IARM_RESULT_SUCCESS != IARM_Bus_BroadcastEvent (IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_ENABLED_STATUS, &e, sizeof(e)))
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]: IARM Bus Error!\n", __FUNCTION__, __LINE__);
    }
}

static void eventInterfaceConnectionStatusChanged(std::string& interface, bool connected)
{
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: interface=%s, connected=%d\n", __FUNCTION__, __LINE__, interface.c_str(), connected);

    IARM_BUS_NetSrvMgr_Iface_EventInterfaceConnectionStatus_t e;
    snprintf(e.interface, sizeof(e.interface), "%s", interface.c_str());
    e.status = connected;

    if (IARM_RESULT_SUCCESS != IARM_Bus_BroadcastEvent (IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_CONNECTION_STATUS, &e, sizeof(e)))
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]: IARM Bus Error!\n", __FUNCTION__, __LINE__);
    }
}

static void eventInterfaceIPAddressStatusChanged (std::string& interface, std::string ip_address, bool is_ipv6, bool acquired)
{
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: interface=%s, ip_address=%s, is_ipv6=%d, acquired=%d\n",
            __FUNCTION__, __LINE__, interface.c_str(), ip_address.c_str(), is_ipv6, acquired);

    IARM_BUS_NetSrvMgr_Iface_EventInterfaceIPAddress_t e;
    snprintf(e.interface, sizeof(e.interface), "%s", interface.c_str());
    snprintf(e.ip_address, sizeof(e.ip_address), "%s", ip_address.c_str());
    e.is_ipv6 = is_ipv6;
    e.acquired = acquired;

    if (IARM_RESULT_SUCCESS != IARM_Bus_BroadcastEvent (IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_IPADDRESS, &e, sizeof(e)))
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]: IARM Bus Error!\n", __FUNCTION__, __LINE__);
    }
}

static void eventDefaultInterfaceChanged(std::string& oldInterface, std::string& newInterface)
{
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: oldInterface=%s, newInterface=%s\n", __FUNCTION__, __LINE__, oldInterface.c_str(), newInterface.c_str());

    IARM_BUS_NetSrvMgr_Iface_EventDefaultInterface_t e;
    snprintf(e.oldInterface, sizeof(e.oldInterface), "%s", oldInterface.c_str());
    snprintf(e.newInterface, sizeof(e.newInterface), "%s", newInterface.c_str());

    if (IARM_RESULT_SUCCESS != IARM_Bus_BroadcastEvent (IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_DEFAULT_INTERFACE, &e, sizeof(e)))
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]: IARM Bus Error!\n", __FUNCTION__, __LINE__);
    }
}

#endif // ifdef ENABLE_IARM

#ifdef ENABLE_NLMONITOR

static void detectDefaultInterfaceChange()
{
    static std::string default_interface ("");
    std::string old_default_interface = default_interface;
    std::string gateway;
    getDefaultInterface (default_interface, gateway);
    if (old_default_interface != default_interface)
        eventDefaultInterfaceChanged (old_default_interface, default_interface);
}

static void linkCallback(std::string args)
{
    // Message Format: interface up/down/add/delete
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: Arguments received = %s\n", __FUNCTION__, __LINE__, args.c_str());
    std::vector<std::string> tokens;
    if (split(args, ' ', tokens, 2) == false)
        return;

    if (tokens[1] == "up")
        eventInterfaceEnabledStatusChanged(tokens[0], true);
    else if (tokens[1] == "down")
    {
        eventInterfaceEnabledStatusChanged(tokens[0], false);
        detectDefaultInterfaceChange(); // needed as netlink events are not sent for ipv4 route deletes that occur implicitly when a physical interface goes down
    }
    else if (tokens[1] == "add")
        eventInterfaceConnectionStatusChanged(tokens[0], true);
    else if (tokens[1] == "delete")
        eventInterfaceConnectionStatusChanged(tokens[0], false);
}

static void addressCallback(std::string args)
{
    // Message Format: add/delete ipv4/ipv6 interface address global/local
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: Arguments received = %s\n", __FUNCTION__, __LINE__, args.c_str());
    std::vector<std::string> tokens;
    if (split(args, ' ', tokens, 5) == false)
        return;

    if (tokens[4] != "global")
        return; // we are only interested in global addresses

    eventInterfaceIPAddressStatusChanged(tokens[2], tokens[3], tokens[1] == "ipv6", tokens[0] == "add");

    if ((tokens[2].find(':') != std::string::npos) && (tokens[0] == "delete")) // virtual interface deleted
        detectDefaultInterfaceChange(); // needed as netlink events are not sent for ipv4 route deletes that occur implicitly when a virtual interface goes down
}

const int DEFAULT_PRIORITY = 1024;
const int MAX_DEFAULT_ROUTES_PER_INTERFACE = 20;

static void defaultRouteCallback(std::string args)
{
    // Message Format: family interface destinationip gatewayip preferred_src metric add/delete
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: Arguments received = %s\n", __FUNCTION__, __LINE__, args.c_str());
    std::vector<std::string> tokens;
    if (split(args, ' ', tokens, 7) == false)
        return;

    detectDefaultInterfaceChange();

    if (tokens[6] != "add")
        return; // rest of the function is only relevant to "default route add" messages

    unsigned int family = (unsigned int)stoul(tokens[0]);
    std::string &interface = tokens[1];
    std::string &gatewayip = tokens[3];
    std::string &metric = tokens[5];

    std::string wifi_interface (getenvOrDefault("WIFI_INTERFACE", ""));
    std::string moca_interface (getenvOrDefault("MOCA_INTERFACE", ""));
    std::string ethernet_interface (getenvOrDefault("ETHERNET_INTERFACE", ""));

    if ((interface != wifi_interface) && (interface != moca_interface) && (interface != ethernet_interface))
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: Unrecognized interface %s\n", __FUNCTION__, __LINE__, interface.c_str());
        return;
    }
    if (!std::all_of(metric.begin(), metric.end(), ::isdigit))
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: Priority is not a Numerical value. Received Priority =%s\n", __FUNCTION__, __LINE__, metric.c_str());
        return;
    }

    unsigned int ui_metric = (unsigned int)stoul(metric);

    if (ui_metric != DEFAULT_PRIORITY)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: Priority is not a default value. Received Priority =%u\n", __FUNCTION__, __LINE__, ui_metric);
        return;
    }
    if (NetLinkIfc::get_instance()->userdefinedrouteexists(interface, gatewayip, family))
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: User defined routes are present for Gateway : %s\n", __FUNCTION__, __LINE__, gatewayip.c_str());
        return;
    }
    if ( ( ( ( interface == moca_interface ) || ( interface == ethernet_interface ) ) && ( access("/tmp/ani_wifi", F_OK) == 0 ) )
        || ( ( interface == wifi_interface ) && ( access("/tmp/ani_wifi", F_OK) != 0 ) ) )
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: Not Changing Priority for default route via %s\n", __FUNCTION__, __LINE__, interface.c_str());
        return;
    }

    // prioritize (assign lower metric to) the default route via the interface that should be made the active network interface (ani)
    int priorityvalue = DEFAULT_PRIORITY / 2;
    int prioritystop = priorityvalue + MAX_DEFAULT_ROUTES_PER_INTERFACE;
    for (;priorityvalue< prioritystop;++priorityvalue)
    {
        if (!NetLinkIfc::get_instance()->routeexists(interface, gatewayip, family, priorityvalue))
            break;
    }
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]: Changing Priority for default route via %s, with gateway %s from %u to %d\n",
            __FUNCTION__, __LINE__, interface.c_str(), gatewayip.c_str(), ui_metric, priorityvalue);
    NetLinkIfc::get_instance()->changedefaultroutepriority(interface, gatewayip, family, ui_metric, priorityvalue);
}

#endif // ifdef ENABLE_NLMONITOR

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


#ifdef INCLUDE_BREAKPAD
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
    std::string minidump_path;
    RFC_ParamData_t secValue = {0};
    WDMP_STATUS status = getRFCParameter("SecureCoreFile", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SecDump.Enable", &secValue);
    if (( WDMP_SUCCESS == status ) && ( 0 == strncmp(secValue.value, "false", 5)))
    {
        minidump_path = "/opt/minidumps";
    }
    else
    {
        minidump_path = "/opt/secure/minidumps";
    }
    google_breakpad::MinidumpDescriptor descriptor(minidump_path.c_str());
    google_breakpad::ExceptionHandler eh(descriptor, NULL, breakpadDumpCallback, NULL, true, -1);
#else
    sleep(1);
    BreakPadWrapExceptionHandler eh;
    eh = newBreakPadWrapExceptionHandler();
#endif
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
#ifdef ENABLE_NLMONITOR
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getInterfaceList, getInterfaceList);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getDefaultInterface, getDefaultInterface);
#endif // ifdef ENABLE_NLMONITOR
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_setDefaultInterface, setDefaultInterface);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_isInterfaceEnabled, isInterfaceEnabled);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_setInterfaceEnabled, setInterfaceEnabled);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getSTBip, getSTBip);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_setIPSettings, setIPSettings);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getIPSettings, getIPSettings);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getSTBip_family, getSTBip_family);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_WIFI_INTERFACE_STATE, _eventHandler);
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

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)

bool isFeatureEnabled(const char* feature)
{
    RFC_ParamData_t param = {0};
    if (WDMP_SUCCESS != getRFCParameter("netsrvmgr", feature, &param))
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] getRFCParameter for %s failed.\n", __FUNCTION__, feature);
        return false;
    }
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] name = %s, type = %d, value = %s\n", __FUNCTION__, param.name, param.type, param.value);
    return (strcmp(param.value, "true") == 0);
}

bool validate_interface_can_be_disabled (const char* interface)
{
    if (isFeatureEnabled("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Network.AllowDisableDefaultNetwork.Enable"))
        return true;

    char activeInterface[INTERFACE_SIZE];
    if (!netSrvMgrUtiles::getRouteInterfaceType (activeInterface))
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

#ifdef USE_RDK_WIFI_HAL

// pni_controller.service determines, enables and configures the interface to make active (wifi/ethernet)
static void launch_pni_controller ()
{
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] systemctl restart pni_controller.service\n", __FUNCTION__);
    system("systemctl restart pni_controller.service");
}

#endif // USE_RDK_WIFI_HAL

#endif // ifndef ENABLE_XCAM_SUPPORT and XHB1

void netSrvMgr_start()
{
    LOG_ENTRY_EXIT;

#ifdef ENABLE_NLMONITOR
    NetLinkIfc* netifc = NetLinkIfc::get_instance();
    netifc->initialize();
    netifc->addSubscriber(new FunctionSubscriber(NlType::link, linkCallback));
    netifc->addSubscriber(new FunctionSubscriber(NlType::address, addressCallback));
    netifc->addSubscriber(new FunctionSubscriber(NlType::dfltroute, defaultRouteCallback));
    netifc->run(false);
#endif

#ifdef ENABLE_ROUTE_SUPPORT
    RouteNetworkMgr::getInstance()->Start();
#endif

#ifdef USE_RDK_WIFI_HAL
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
    launch_pni_controller();
#else
    setWifiEnabled (true); // enable WiFi by default for XCAMs
#endif // ifndef ENABLE_XCAM_SUPPORT and XHB1
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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)
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
                    if(0 == strncasecmp(LFAT_VERSION, keys[key], strlen(keys[key]) ) )
                    {
                        strcpy(confProp.wifiProps.lfatVersion,value);
                    }
                    if(0 == strncasecmp(LFAT_TTL, keys[key], strlen(keys[key]) ) )
                    {
                        confProp.wifiProps.lfatTTL = atoi(value);
                    }
                    if(0 == strncasecmp(LAF_CONNECTION_RETRY, keys[key], strlen(keys[key]) ) )
                    {
                        confProp.wifiProps.lnfRetryCount = atoi(value);
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
    char devName[INTERFACE_SIZE]={0};
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
    param->activeIface[0]='\0';
    if(netSrvMgrUtiles::getRouteInterfaceType(devName))
        strcpy(param->activeIface,devName);
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] No Route Found.\n", __FUNCTION__, __LINE__);
        if(netSrvMgrUtiles::currentActiveInterface(devName))
        {
            if(!netSrvMgrUtiles::readDevFile(devName))
            {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Get Active interface type failed for %s  \n", __FUNCTION__, __LINE__,devName);
            }
            else
                strcpy(param->activeIface,devName);
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] No Active Interface.\n", __FUNCTION__, __LINE__);
        }
    }
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Current Active Interface : [%s].\n", __FUNCTION__, __LINE__,param->activeIface);
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_NETWORK_MANAGER_ACTIVE_INTERFACE:%s\n",param->activeIface);
    return ret;
}

IARM_Result_t getNetworkInterfaces(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    char devName[INTERFACE_LIST]={0};
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
    param->allNetworkInterfaces[0]='\0';
    char count = netSrvMgrUtiles::getAllNetworkInterface(devName);
    if(count)
        g_stpcpy(param->allNetworkInterfaces,devName);
    else
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] list of Network Interface is empty.\n", __FUNCTION__, __LINE__);
    param->interfaceCount=count;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] list of Network Interface : [%s].\n", __FUNCTION__, __LINE__,param->allNetworkInterfaces);
    return ret;
}

void _eventHandler (const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    LOG_ENTRY_EXIT;

    if (strcmp (owner, IARM_BUS_NM_SRV_MGR_NAME) != 0)
        return;

    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *) data;

    switch (eventId)
    {
#ifdef USE_RDK_WIFI_HAL
    case IARM_BUS_NETWORK_MANAGER_EVENT_WIFI_INTERFACE_STATE:
    {
        setWifiEnabled (param->isInterfaceEnabled);
        break;
    }
#endif // USE_RDK_WIFI_HAL
    default:
        break;
    }
}

#ifdef ENABLE_NLMONITOR

IARM_Result_t getInterfaceList(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_InterfaceList_t *list = (IARM_BUS_NetSrvMgr_InterfaceList_t*) arg;
    std::vector<iface_info> interfaces;
    NetLinkIfc::get_instance()->getInterfaces(interfaces);
    list->size = interfaces.size();
    for (int i = 0; i < list->size; i++)
    {
        snprintf(list->interfaces[i].name, sizeof(list->interfaces[i].name), "%s", interfaces[i].m_if_name.c_str());
        snprintf(list->interfaces[i].mac, sizeof(list->interfaces[i].mac), "%s", interfaces[i].m_if_macaddr.c_str());
        list->interfaces[i].flags = interfaces[i].m_if_flags;
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] name=%s, mac=%s, flags=0x%08x\n",
                __FUNCTION__, __LINE__, list->interfaces[i].name, list->interfaces[i].mac, list->interfaces[i].flags);
    }
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t getDefaultInterface(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_DefaultRoute_t *default_route = (IARM_BUS_NetSrvMgr_DefaultRoute_t*) arg;
    std::string interface;
    std::string gateway;
    if (getDefaultInterface(interface, gateway))
    {
        snprintf(default_route->interface, sizeof(default_route->interface), "%s", interface.c_str());
        snprintf(default_route->gateway, sizeof(default_route->gateway), "%s", gateway.c_str());
    }
    else
    {
        memset(default_route, 0, sizeof(IARM_BUS_NetSrvMgr_DefaultRoute_t)); // default_route->interface[0] = '\0';
    }
    return IARM_RESULT_SUCCESS;
}

#endif // ifdef ENABLE_NLMONITOR

IARM_Result_t setDefaultInterface(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t*) arg;
    return setDefaultInterface (param->setInterface, param->persist) ? IARM_RESULT_SUCCESS : IARM_RESULT_IPCCORE_FAIL;
}

IARM_Result_t isInterfaceEnabled(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
    return isInterfaceEnabled(param->setInterface, param->isInterfaceEnabled) ? IARM_RESULT_SUCCESS : IARM_RESULT_IPCCORE_FAIL;
}

// TODO - Fix HomeNetworkingService references to
// - IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_ENABLED
// - IARM_BUS_NETWORK_MANAGER_EVENT_SET_INTERFACE_CONTROL_PERSISTENCE
IARM_Result_t setInterfaceEnabled(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
    return setInterfaceEnabled (param->setInterface, param->isInterfaceEnabled, param->persist) ? IARM_RESULT_SUCCESS : IARM_RESULT_IPCCORE_FAIL;
}

IARM_Result_t setIPSettings(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_Iface_Settings_t *param = (IARM_BUS_NetSrvMgr_Iface_Settings_t *)arg;
    return setIPSettings (param) ? IARM_RESULT_SUCCESS : IARM_RESULT_IPCCORE_FAIL;
}

IARM_Result_t getIPSettings(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_Iface_Settings_t *param = (IARM_BUS_NetSrvMgr_Iface_Settings_t *)arg;
    return getIPSettings (param) ? IARM_RESULT_SUCCESS : IARM_RESULT_IPCCORE_FAIL;
}

IARM_Result_t getSTBip(void *arg)
{
        IARM_Result_t ret = IARM_RESULT_SUCCESS;
        char stbip[MAX_IP_ADDRESS_LEN];
        bool isIpv6=false;
        IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
        if(netSrvMgrUtiles::getSTBip(stbip,&isIpv6))
        {
           strcpy(param->activeIfaceIpaddr,stbip);
           RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] stb ipaddress : [%s].\n", __FUNCTION__, __LINE__,stbip);
        }
        else
           RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] stb ipaddress not found.\n", __FUNCTION__, __LINE__);
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
        return ret;
}

IARM_Result_t getSTBip_family(void *arg)
{
        IARM_Result_t ret = IARM_RESULT_SUCCESS;
        char stbip[MAX_IP_ADDRESS_LEN];
        IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

	if (netSrvMgrUtiles::getSTBip_family(stbip,param->ipfamily))
	{
           strcpy(param->activeIfaceIpaddr,stbip);
           RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] stb ipaddress : [%s].\n", __FUNCTION__, __LINE__,stbip);
	}
        else
           RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] stb ipaddress not found.\n", __FUNCTION__, __LINE__);
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
        return ret;
}
#endif // ENABLE_IARM


#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1)

/*
 * touches /tmp/<filename>
 * touches /opt/persistent/<filename> also if persist = true
 */
static void mark(const char* filename, bool persist)
{
    char fqn[64];
    snprintf (fqn, sizeof(fqn), "/tmp/%s", filename);
    FILE *fp = fopen(fqn, "w"); // TODO: not expecting errors but handle failure
    fclose(fp);
    if (persist)
    {
        snprintf (fqn, sizeof(fqn), "/opt/persistent/%s", filename);
        FILE *fp = fopen(fqn, "w"); // TODO: not expecting errors but handle failure
        fclose(fp);
    }
}

/*
 * removes /tmp/<filename>
 * removes /opt/persistent/<filename> also if persist = true
 */
static void unmark(const char* filename, bool persist)
{
    char fqn[64];
    snprintf (fqn, sizeof(fqn), "/tmp/%s", filename);
    remove (fqn);
    if (persist)
    {
        snprintf (fqn, sizeof(fqn), "/opt/persistent/%s", filename);
        remove (fqn);
    }
}

#ifdef ENABLE_NLMONITOR
bool getDefaultInterface(std::string &interface, std::string &gateway)
{
    LOG_ENTRY_EXIT;

    if (NetLinkIfc::get_instance()->getDefaultRoute(true, interface, gateway) || // ipv6 default route exists
        NetLinkIfc::get_instance()->getDefaultRoute(false, interface, gateway))  // ipv4 default route exists
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] default route interface = %s gateway = %s\n",
                __FUNCTION__, __LINE__, interface.c_str(), gateway.c_str());
        return true;
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] no default route\n", __FUNCTION__, __LINE__);
        interface.clear();
        gateway.clear();
        return false;
    }
}
#endif // ifdef ENABLE_NLMONITOR

bool setDefaultInterface (const char* interface, bool persist)
{
    LOG_ENTRY_EXIT;
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface = [%s], persist = [%d]\n", __FUNCTION__, interface, persist);

#ifdef USE_RDK_WIFI_HAL
    if (strcmp (interface, "ETHERNET") == 0)
    {
        unmark("pni_wifi", persist); // remove marker file (if present) that says "WIFI is the preferred network interface"
        launch_pni_controller();
        return true;
    }
    else if (strcmp (interface, "WIFI") == 0)
    {
        const char* RFC_PNI_ENABLE = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.PreferredNetworkInterface.Enable";
        if (isFeatureEnabled(RFC_PNI_ENABLE) == false)
        {
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface [%s] can be set as default interface only if RFC %s = true.\n",
                    __FUNCTION__, interface, RFC_PNI_ENABLE);
            return false;
        }
        bool enabled;
        if (isInterfaceEnabled(interface, enabled) && !enabled) // even if this check is not done here, pni_controller will not use wifi if "/tmp/wifi_disallowed" exists
        {
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface [%s] is disabled.\n", __FUNCTION__, interface);
            return false;
        }
        mark("pni_wifi", persist); // touch marker file that says "WIFI is the preferred network interface"
        launch_pni_controller();
        return true;
    }
#else
    if (strcmp (interface, "ETHERNET") == 0)
    {
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] nothing to do. interface [%s] is always the default interface.\n", __FUNCTION__, interface);
        return true;
    }
#endif // #ifdef USE_RDK_WIFI_HAL
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface [%s] is invalid.\n", __FUNCTION__, interface);
        return false;
    }
}

bool isInterfaceEnabled(const char* interface, bool& enabled)
{
    LOG_ENTRY_EXIT;
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface = [%s]\n", __FUNCTION__, interface);

    std::string interface_name;
    if (strcmp (interface, "ETHERNET") == 0)
    {
        interface_name = getenvOrDefault("ETHERNET_INTERFACE", "");
    }
#ifdef USE_RDK_WIFI_HAL
    else if (strcmp (interface, "WIFI") == 0)
    {
        interface_name = getenvOrDefault("WIFI_INTERFACE", "");
    }
#endif // USE_RDK_WIFI_HAL
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] interface [%s] is invalid\n", __FUNCTION__, interface);
        return false;
    }
#ifdef ENABLE_NLMONITOR
    std::vector<iface_info> interfaces;
    NetLinkIfc::get_instance()->getInterfaces(interfaces);
    for (const auto& i : interfaces)
    {
        if (i.m_if_name == interface_name)
        {
            enabled = ((i.m_if_flags & IFF_UP) != 0);
            RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface = [%s] enabled = [%d]\n", __FUNCTION__, interface, enabled);
            return true;
        }
    }
#endif // ifdef ENABLE_NLMONITOR
    RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] interface = [%s] enabled = [NA]\n", __FUNCTION__, interface);
    return false;
}

bool setInterfaceEnabled (const char* interface, bool enabled, bool persist)
{
    LOG_ENTRY_EXIT;
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface = [%s] enable = [%d] persist = [%d]\n", __FUNCTION__, interface, enabled, persist);

    if (strcmp (interface, "ETHERNET") == 0)
    {
        if (enabled)
        {
            RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] nothing to do. interface [%s] is always enabled.\n", __FUNCTION__, interface);
            return true;
        }
        else
        {
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface [%s] cannot be disabled.\n", __FUNCTION__, interface);
            return false;
        }
    }
#ifdef USE_RDK_WIFI_HAL
    else if (strcmp (interface, "WIFI") == 0)
    {
        if (enabled)
        {
            unmark("wifi_disallowed", persist); // remove marker file (if present) that says "WIFI is disallowed"
            return setInterfaceState (getenvOrDefault("WIFI_INTERFACE", ""), true);
        }
        else if (validate_interface_can_be_disabled(interface))
        {
            mark("wifi_disallowed", persist); // touch marker file that says "WIFI is disallowed"
            setInterfaceState (getenvOrDefault("WIFI_INTERFACE", ""), false);
            setDefaultInterface("ETHERNET", persist);
            return true;
        }
        else
        {
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface [%s] cannot be disabled.\n", __FUNCTION__, interface);
            return false;
        }
    }
#endif // USE_RDK_WIFI_HAL
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface [%s] is invalid.\n", __FUNCTION__, interface);
        return false;
    }
}

bool getDNSip (char *primaryDNS, char *secondaryDNS)
{
    char nameserver[31]={0};
    int ip[4]={0};
    int fvalue = 0;
    int foundDNS = 0;
    FILE *fptr= NULL;
    const int dnsParamvalue = 5;

    if ((fptr = fopen("/etc/resolv.dnsmasq", "r")) != NULL)
    {
        while (!feof(fptr))
        {
            fvalue = fscanf(fptr, "%s %d.%d.%d.%d", nameserver,&ip[0],&ip[1],&ip[2],&ip[3]);

            if (dnsParamvalue == fvalue)
            {
                if (foundDNS == 0)
                {
                    sprintf(primaryDNS,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
                    foundDNS = 1;
                }
                else
                {
                    sprintf(secondaryDNS,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
                    break;
                }
            }
        }
        if (foundDNS == 0)
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "DNS ip is not configured [%s:%d]  \n", __FUNCTION__, __LINE__);
        }
        fclose(fptr);
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "DNS file is not present [%s:%d]  \n", __FUNCTION__, __LINE__);
        return false;
    }

    return true;
}

bool clearIpConfig(char *interface)
{
    char file[512];
    char cmd[512];
    char line[512];
    char dns[16];
    char gw[16];
    FILE *fp = NULL;
    int  ipLen = 0;
    bool ret = false;

    memset(gw, 0, sizeof(gw));
    memset(file, 0, sizeof(file));
    snprintf(file, sizeof(file), "/opt/persistent/ip.%s.0",interface);
    fp = fopen(file, "r");
    if(fp != NULL)
    {
        while(NULL != fgets(line, sizeof(line), fp))
        {
            memset(dns, 0, sizeof(dns));
            if(strstr(line,"primarydns="))
            {
                //exclude trailing newline
                ipLen = strlen(line)-(strlen("primarydns="))-1;
                strncpy(dns, line+strlen("primarydns="), ipLen);
            }
            if(strstr(line,"secondarydns="))
            {
                ipLen = strlen(line)-(strlen("secondarydns="))-1;
                strncpy(dns, line+strlen("secondarydns="), ipLen);
            }
            if(strstr(line,"gateway="))
            {
                ipLen = strlen(line)-(strlen("gateway="))-1;
                strncpy(gw, line+strlen("gateway="), ipLen);
            }

            //delete dns entries from resolv.dnsmasq
            if(strlen(dns))
            {
                memset(cmd, 0, sizeof(cmd));
                snprintf(cmd, sizeof(cmd), "sed -i '/%s/d' /tmp/resolv.dnsmasq.udhcpc", dns);
                RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__,cmd);
                system(cmd);
            }
        }
        fclose(fp);

        //delete gw entry
        if(strlen(gw))
        {
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "/sbin/route del default gw %s", gw);
            RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__,cmd);
            system(cmd);
        }

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "rm -f %s", file);
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__,cmd);
        system(cmd);
        ret = true;
    }
    return ret;
}

bool saveIpConfig(bool isWifi, IARM_BUS_NetSrvMgr_Iface_Settings_t *param)
{
    char file[512];
    char line[512];
    FILE *fp = NULL;
    bool ret = false;

    memset(file, 0, sizeof(file));
    snprintf(file, sizeof(file), "/opt/persistent/ip.%s.0",(isWifi) ? "wifi" : "eth0");
    fp = fopen(file, "w");
    if(fp != NULL)
    {
        snprintf(line, 512, "ipaddress=%s\n",param->ipaddress);
        fwrite(line, 1, strlen(line), fp);
        snprintf(line, 512, "netmask=%s\n",param->netmask);
        fwrite(line, 1, strlen(line), fp);
        snprintf(line, 512, "gateway=%s\n",param->gateway);
        fwrite(line, 1, strlen(line), fp);
        snprintf(line, 512, "primarydns=%s\n",param->primarydns);
        fwrite(line, 1, strlen(line), fp);
        snprintf(line, 512, "secondarydns=%s\n",param->secondarydns);
        fwrite(line, 1, strlen(line), fp);
        fclose(fp);
        ret = true;
    }
    return ret;
}

//vaidates if the new config is different from the current one
bool configSameasCurrentConfig(const char* file, IARM_BUS_NetSrvMgr_Iface_Settings_t *param)
{
    FILE *fp = NULL;
    char line[512];
    bool ret = false;
    int params = 0;

    fp = fopen(file,"r");
    if(NULL != fp)
    {
        while(NULL != fgets(line, sizeof(line), fp))
        {
            if(strstr(line,"ipaddress="))
            {
                if(strncmp(param->ipaddress, line+strlen("ipaddress="), strlen(param->ipaddress)) == 0)
                {
                    params++;
                }
                continue;
            }
            if(strstr(line,"netmask="))
            {
                if(strncmp(param->netmask, line+strlen("netmask="), strlen(param->netmask)) == 0)
                {
                    params++;
                }
                continue;
            }
            if(strstr(line,"gateway="))
            {
                if(strncmp(param->gateway, line+strlen("gateway="), strlen(param->gateway)) == 0)
                {
                    params++;
                }
                continue;
            }
            if(strstr(line,"primarydns="))
            {
                if(strncmp(param->primarydns, line+strlen("primarydns="), strlen(param->primarydns)) == 0)
                {
                    params++;
                }
                continue;
            }
            if(strstr(line,"secondarydns="))
            {
                if(strncmp(param->secondarydns, line+strlen("secondarydns="), strlen(param->secondarydns)) == 0)
                {
                    params++;
                }
                continue;
            }
        }
        fclose(fp);
    }
    if(params == 5)
    {
        ret = true;
    }
    return ret;
}

bool setIPSettings(IARM_BUS_NetSrvMgr_Iface_Settings_t *param)
{
    bool wifi = false;
    bool ethernet = false;
    bool isWifiAuto = false;
    bool isEthernetAuto = false;
    char cmd[1024];

    const char* RFC_MANUALIP_ENABLE = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Network.ManualIPSettings.Enable";
    if (isFeatureEnabled(RFC_MANUALIP_ENABLE) == false)
    {
        param->isSupported = false;
        return false;
    }

    if (0 != strcasecmp(param->ipversion, "ipv4"))
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] unsupported ipversion [%s].\n", __FUNCTION__, param->ipversion);
        param->isSupported = false;
        return false;
    }

    if (0 == strcasecmp(param->interface, "WIFI"))
    {
        wifi = true;
    }
    else if (0 == strcasecmp(param->interface, "ETHERNET"))
    {
        ethernet = true;
    }
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] unsupported interface [%s].\n", __FUNCTION__, param->interface);
        return false;
    }

    //get the current autoconfig
    isWifiAuto = (access( "/opt/persistent/ip.wifi.0", F_OK ) != -1 ) ? false : true;
    isEthernetAuto = (access( "/opt/persistent/ip.eth0.0", F_OK ) != -1 ) ? false : true;

    if(param->autoconfig)
    {
        //Alreaady in autoconfig
        if((wifi && isWifiAuto) || (ethernet && isEthernetAuto))
        {
            RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] Already in autoconfig mode. \n", __FUNCTION__);
            return true;
        }

        //clear the existing config
        if(wifi && !isWifiAuto)
        {
            if(!clearIpConfig("wifi"))
            {
                RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed to clear the existing config\n", __FUNCTION__);
                return false;
            }
        } else if(ethernet && !isEthernetAuto)
        {
            if(!clearIpConfig("eth0"))
            {
                RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed to clear the existing config\n", __FUNCTION__);
                return false;
            }
        }

        //enable dhcp
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "systemctl restart virtual-%s-iface.service", (wifi) ? "wifi" : "moca");
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__,cmd);
        system(cmd);
    }
    else
    {
        //get the interface details
        char *interface = ((wifi) ? getenv("WIFI_INTERFACE") : getenv("ETHERNET_INTERFACE"));
        if(interface == NULL)
        {
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] Failed to get the interface\n", __FUNCTION__);
            return false;
        }
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface:[%s]\n", __FUNCTION__,interface);

        //validate the parameters
        if(!((checkvalidip((char *)param->ipaddress)) && (checkvalidip((char *)param->netmask)) &&
             (checkvalidip((char *)param->gateway)) && (checkvalidip((char *)param->primarydns))))
        {
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] Invalid parameters.\n", __FUNCTION__);
            return false;
        }
        if(strlen(param->secondarydns))
        {
            if(!checkvalidip((char *)param->secondarydns))
            {
                RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] Invalid parameters.\n", __FUNCTION__);
                return false;
            }
        }

        //check if the new config is different from the existing config
        if(wifi && !isWifiAuto)
        {
            if(configSameasCurrentConfig("/opt/persistent/ip.wifi.0", param))
            {
                RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] provided config is same as the current config\n", __FUNCTION__);
                return true;
            }

            if(!clearIpConfig("wifi"))
            {
                RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed to clear the existing config\n", __FUNCTION__);
                return false;
            }
        } else if(ethernet && !isEthernetAuto)
        {
            if(configSameasCurrentConfig("/opt/persistent/ip.eth0.0", param))
            {
                RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] provided config is same as the current config\n", __FUNCTION__);
                return true;
            }

            if(!clearIpConfig("eth0"))
            {
                RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed to clear the existing config\n", __FUNCTION__);
                return false;
            }
        }

        //save the new config and update dns configs
        if(!saveIpConfig(wifi, param))
        {
            RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed to save the new config. enabling dhcp\n", __FUNCTION__);

            //enable dhcp
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "systemctl restart virtual-%s-iface.service", (wifi) ? "wifi" : "moca");
            RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__,cmd);
            system(cmd);
            return false;
        }

        if(isWifiAuto || isEthernetAuto)
        {
            //stop dhcp
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "systemctl stop virtual-%s-iface.service", (wifi) ? "wifi" : "moca");
            RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__,cmd);
            system(cmd);
        }

        //configure manual ip
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s:0 %s netmask %s up", interface, param->ipaddress, param->netmask);
#ifdef NO_VIRTUAL_INTERFACES
        snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s %s netmask %s up", interface, param->ipaddress, param->netmask);
#endif
        system(cmd);

        //configure gateway ip
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "/sbin/route add default gw %s", param->gateway);
        system(cmd);

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "echo \"nameserver %s\" >> /tmp/resolv.dnsmasq.udhcpc", param->primarydns);
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__,cmd);
        system(cmd);

        if(strlen(param->secondarydns))
        {
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "echo \"nameserver %s\" >> /tmp/resolv.dnsmasq.udhcpc", param->secondarydns);
            RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__,cmd);
            system(cmd);
        }
    }
    return true;
}

bool getIPSettings(IARM_BUS_NetSrvMgr_Iface_Settings_t *param)
{

    std::string interface;
    std::string gateway;
    char primaryDNSaddr[INET_ADDRSTRLEN]={0};
    char secondaryDNSaddr[INET_ADDRSTRLEN]={0};

    const char* RFC_MANUALIP_ENABLE = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Network.ManualIPSettings.Enable";
    if (isFeatureEnabled(RFC_MANUALIP_ENABLE) == false)
    {
        return false;
    }

    if (0 == strcasecmp(param->interface, "WIFI"))
    {
        param->autoconfig = (access( "/opt/persistent/ip.wifi.0", F_OK ) != -1 ) ? false : true;
    }
    else if (0 == strcasecmp(param->interface, "ETHERNET"))
    {
        param->autoconfig = (access( "/opt/persistent/ip.eth0.0", F_OK ) != -1 ) ? false : true;
    }
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] unsupported interface [%s].\n", __FUNCTION__, param->interface);
        return false;
    }

    if(!netSrvMgrUtiles::getInterfaceConfig(param->interface, param->ipaddress, param->netmask))
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed to fetch the details of [%s].\n", __FUNCTION__, param->interface);
        return false;
    }
    strcpy(param->ipversion, "IPv4");

    /* To get Default Gateway */
    if (NetLinkIfc::get_instance()->getDefaultRoute(false, interface, gateway))
    {
        snprintf(param->gateway, sizeof(param->gateway), "%s", gateway.c_str());
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "Default Gateway [%s] \n", param->gateway);
    }
    else
    {
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "Default Gateway not present [%s:%d] \n", __FUNCTION__, __LINE__);
    }

    /* To get Primary and Secondary DNS */
    if (getDNSip(primaryDNSaddr, secondaryDNSaddr))
    {
        strcpy(param->primarydns, primaryDNSaddr);
        strcpy(param->secondarydns, secondaryDNSaddr);
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] Primary DNS %s \n", __FUNCTION__, __LINE__, param->primarydns);
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] Secondary DNS %s \n", __FUNCTION__, __LINE__, param->secondarydns);
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR," No DNS ip is confiured [%s:%d]  \n", __FUNCTION__, __LINE__);
    }
    return true;
}

bool setInterfaceState (std::string interface_name, bool enabled)
{
    if (interface_name.empty())
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] no interface name\n", __FUNCTION__);
        return false;
    }
    char command[64];
    snprintf (command, sizeof(command), "ip link set dev %s %s", interface_name.c_str(), enabled ? "up" : "down");
    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s\n", __FUNCTION__, command);
    system (command);
    return true;
}

#endif // ifndef ENABLE_XCAM_SUPPORT and XHB1

#ifdef USE_RDK_WIFI_HAL
// TODO: move this into WifiSrvMgr.cpp as WiFiNetworkMgr::setWifiEnabled(bool newState) ?
bool setWifiEnabled (bool newState)
{
    static bool bWiFiEnabled = false; // TODO: assumes WiFi is disabled when netsrvmgr starts. correct assumption?

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
