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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
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
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
#include "connectivity.h"
#endif

#ifdef ENABLE_STUN_CLIENT
#include "StunClient.h"
#endif

#ifdef ENABLE_ROUTE_SUPPORT
#include "routeSrvMgr.h"
#endif

#ifdef USE_RDK_MOCA_HAL
#include "mocaSrvMgr.h"
#endif

#ifdef ENABLE_NLMONITOR
#include "netlinkifc.h"
#endif

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
#include "rfcapi.h"     // for RFC queries
#endif

#ifdef SAFEC_RDKV
#include "safec_lib.h"
#else
#define STRCPY_S(dest,size,source)                    \
        strcpy(dest, source);
#endif
char configProp_FilePath[100] = {'\0'};;
netMgrConfigProps confProp;

#ifdef ENABLE_STUN_CLIENT
#define STUN_DEFAULT_BIND_TIMEOUT 30 //seconds
#define STUN_DEFAULT_CACHE_TIMEOUT 0 //seconds (0 for disable)
stun::client stunClient;
#endif
#define NETSRVMGR_DHCP_SERVERIP_PATH "/tmp/netsrvmgr.dhcp.server.ip"

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
static bool getDNSip(const unsigned int family, char *primaryDNS, char *secondaryDNS);
static IARM_Result_t getSTBip_family(void *arg);
static IARM_Result_t isConnectedToInternet(void *arg);
static IARM_Result_t setConnectivityTestEndpoints(void *arg);
static IARM_Result_t isAvailable(void *arg);
#ifdef ENABLE_STUN_CLIENT
static IARM_Result_t getPublicIP(void* arg);
#endif
#endif // ifdef ENABLE_IARM

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
static bool isFeatureEnabled(const char* feature);
static bool validate_interface_can_be_disabled (const char* interface);
#ifdef ENABLE_NLMONITOR
static bool getDefaultInterface(std::string &interface, std::string &gateway);
#endif // ifdef ENABLE_NLMONITOR
static bool setDefaultInterface(const char* interface, bool persist);
static bool setInterfaceEnabled(const char* interface, bool enabled, bool persist);
static bool setInterfaceState(std::string interface_name, bool enabled);
static bool setIPSettings(IARM_BUS_NetSrvMgr_Iface_Settings_t *param);
static bool getIPSettings(IARM_BUS_NetSrvMgr_Iface_Settings_t *param);
#ifdef ENABLE_STUN_CLIENT
static bool getPublicIP(IARM_BUS_NetSrvMgr_Iface_StunRequest_t *param);
#endif
#endif // if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)

std::string gDhcpServerIP;
static bool getDhcpServerIP();

void NetworkMgr_SignalHandler (int sigNum)
{
    LOG_ERR("Received signal %d", sigNum);
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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
static bool breakpadDumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                        void* context,
                        bool succeeded)
{
    LOG_INFO("breakpadDumpCallback: Netsrvmgr crashed ---- Dump path: %s", descriptor.path());
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
const char* getenvOrDefault (const char* name, const char* defaultValue)
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
        LOG_ERR("Unexpected number of tokens. Arguments received = %s", str.c_str ());
        return false;
    }
    return true;
}

#ifdef ENABLE_IARM

static void eventInterfaceEnabledStatusChanged(const std::string& interface, bool enabled)
{
    LOG_INFO("interface=%s, enabled=%d", interface.c_str(), enabled);

    IARM_BUS_NetSrvMgr_Iface_EventInterfaceEnabledStatus_t e;
    snprintf(e.interface, sizeof(e.interface), "%s", interface.c_str());
    e.status = enabled;

    if (IARM_RESULT_SUCCESS != IARM_Bus_BroadcastEvent (IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_ENABLED_STATUS, &e, sizeof(e)))
    {
        LOG_ERR("IARM Bus Error!");
    }
}

static void eventInterfaceConnectionStatusChanged(const std::string& interface, bool connected)
{
    LOG_INFO("interface=%s, connected=%d", interface.c_str(), connected);

    IARM_BUS_NetSrvMgr_Iface_EventInterfaceConnectionStatus_t e;
    snprintf(e.interface, sizeof(e.interface), "%s", interface.c_str());
    e.status = connected;

    if (IARM_RESULT_SUCCESS != IARM_Bus_BroadcastEvent (IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_CONNECTION_STATUS, &e, sizeof(e)))
    {
        LOG_ERR("IARM Bus Error!");
    }
}

static void eventInterfaceIPAddressStatusChanged (const std::string& interface, const std::string& ip_address, bool is_ipv6, bool acquired)
{
    LOG_INFO("interface=%s, ip_address=%s, is_ipv6=%d, acquired=%d", interface.c_str(), ip_address.c_str(), is_ipv6, acquired);

    IARM_BUS_NetSrvMgr_Iface_EventInterfaceIPAddress_t e;
    snprintf(e.interface, sizeof(e.interface), "%s", interface.c_str());
    snprintf(e.ip_address, sizeof(e.ip_address), "%s", ip_address.c_str());
    e.is_ipv6 = is_ipv6;
    e.acquired = acquired;

    if (IARM_RESULT_SUCCESS != IARM_Bus_BroadcastEvent (IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_INTERFACE_IPADDRESS, &e, sizeof(e)))
    {
        LOG_ERR("IARM Bus Error!");
    }

    if (!is_ipv6)
    {
        if (getDhcpServerIP())
        {
            LOG_INFO("Updated the DHCP Server IP Address: [%s]", gDhcpServerIP.c_str());
        }
    }
}

static void eventDefaultInterfaceChanged(const std::string& oldInterface, const std::string& newInterface)
{
    LOG_INFO("oldInterface=%s, newInterface=%s", oldInterface.c_str(), newInterface.c_str());

    IARM_BUS_NetSrvMgr_Iface_EventDefaultInterface_t e;
    snprintf(e.oldInterface, sizeof(e.oldInterface), "%s", oldInterface.c_str());
    snprintf(e.newInterface, sizeof(e.newInterface), "%s", newInterface.c_str());

    if (IARM_RESULT_SUCCESS != IARM_Bus_BroadcastEvent (IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_EVENT_DEFAULT_INTERFACE, &e, sizeof(e)))
    {
        LOG_ERR("IARM Bus Error!");
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
    LOG_INFO("Arguments received = %s", args.c_str());
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
    LOG_INFO("Arguments received = %s", args.c_str());
    std::vector<std::string> tokens;
    if (split(args, ' ', tokens, 5) == false)
        return;

    if (tokens[4] != "global")
        return; // we are only interested in global addresses

    // do not event any IPv4 addresses reserved for documentation
    if (!(tokens[1] == "ipv4" && netSrvMgrUtiles::isIPv4AddressScopeDocumentation(tokens[3])))
        eventInterfaceIPAddressStatusChanged(tokens[2], tokens[3], tokens[1] == "ipv6", tokens[0] == "add");

    if ((tokens[2].find(':') != std::string::npos) && (tokens[0] == "delete")) // virtual interface deleted
        detectDefaultInterfaceChange(); // needed as netlink events are not sent for ipv4 route deletes that occur implicitly when a virtual interface goes down
}

const int DEFAULT_PRIORITY = 1024;
const int MAX_DEFAULT_ROUTES_PER_INTERFACE = 20;

static void defaultRouteCallback(std::string args)
{
    // Message Format: family interface destinationip gatewayip preferred_src metric add/delete
    LOG_INFO(" Arguments received = %s", args.c_str());
    std::vector<std::string> tokens;
    if (split(args, ' ', tokens, 7) == false)
        return;

    detectDefaultInterfaceChange();
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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
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
#ifdef ENABLE_LOST_FOUND
        confProp.wifiProps.bEnableLostFound = false;
        confProp.wifiProps.lnfRetryInSecs = MAX_TIME_OUT_PERIOD;
        confProp.wifiProps.lnfStartInSecs = MAX_TIME_OUT_PERIOD;
        STRCPY_S(confProp.wifiProps.authServerURL, sizeof(confProp.wifiProps.authServerURL), "http://localhost:50050/authService/getDeviceId");
#endif
    }
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)  && !defined(XHV1)
    Read_Telemetery_Param_File();
#endif
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
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_isConnectedToInternet, isConnectedToInternet);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_setConnectivityTestEndpoints, setConnectivityTestEndpoints);
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_isAvailable, isAvailable);
#ifdef ENABLE_STUN_CLIENT
    IARM_Bus_RegisterCall(IARM_BUS_NETSRVMGR_API_getPublicIP, getPublicIP);
#endif
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

#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)

bool isFeatureEnabled(const char* feature)
{
    RFC_ParamData_t param = {0};
    if (WDMP_SUCCESS != getRFCParameter("netsrvmgr", feature, &param))
    {
        LOG_ERR("getRFCParameter for %s failed.", feature);
        return false;
    }
    LOG_INFO( "name = %s, type = %d, value = %s", param.name, param.type, param.value);
    return (strcmp(param.value, "true") == 0);
}

bool validate_interface_can_be_disabled (const char* interface)
{
    if (isFeatureEnabled("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Network.AllowDisableDefaultNetwork.Enable"))
        return true;

    char activeInterface[INTERFACE_SIZE];
    if (!netSrvMgrUtiles::getRouteInterfaceType (activeInterface))
    {
        LOG_ERR("Cannot determine the active interface");
        return false;
    }
    if (strcmp (interface, activeInterface) == 0)
    {
        LOG_ERR("Cannot disable the active interface '%s'", activeInterface);
        return false;
    }
    return true;
}

#ifdef USE_RDK_WIFI_HAL

// pni_controller.service determines, enables and configures the interface to make active (wifi/ethernet)
static void launch_pni_controller ()
{
    LOG_INFO( "systemctl restart pni_controller.service");
    system("systemctl restart pni_controller.service");
}

#endif // USE_RDK_WIFI_HAL

#endif // ifndef ENABLE_XCAM_SUPPORT and XHB1 and XHC3

void netSrvMgr_start()
{
    LOG_ENTRY_EXIT;

#ifdef ENABLE_NLMONITOR
    NetLinkIfc* netifc = NetLinkIfc::get_instance();
    netifc->addSubscriber(new FunctionSubscriber(NlType::link, linkCallback));
    netifc->addSubscriber(new FunctionSubscriber(NlType::address, addressCallback));
    netifc->addSubscriber(new FunctionSubscriber(NlType::dfltroute, defaultRouteCallback));
    netifc->initialize();
    netifc->run(false);
#endif

#ifdef ENABLE_ROUTE_SUPPORT
    RouteNetworkMgr::getInstance()->Start();
#endif

#ifdef USE_RDK_WIFI_HAL
    WiFiNetworkMgr::getInstance()->Init();
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
    launch_pni_controller();
#else
    WiFiNetworkMgr::getInstance()->setWifiEnabled (true); // enable WiFi by default for XCAMs
#endif // ifndef ENABLE_XCAM_SUPPORT and XHB1 and XHC3
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
#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)
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
    LOG_ENTRY_EXIT;
    bool status = false;
    GKeyFile *key_file = NULL;
    GError *error = NULL;
    gsize length = 0;
    gdouble double_value = 0;
    guint group = 0, key = 0;
    static char *ethIfName=NULL; 

    if(configProp_FilePath[0] == '\0')
    {
       LOG_ERR("Failed to read NETSRVMGR Configuration file");
        return false;
    }

    key_file = g_key_file_new();

    if(!key_file) {
        LOG_ERR("Failed to g_key_file_new()");
        return false;
    }

    if(!g_key_file_load_from_file(key_file, configProp_FilePath, G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        LOG_ERR("Failed with \"%s\"", error->message);
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
            LOG_DBG("Group %u/%u: \t%s", group, groups_id - 1, groups[group]);
            if(0 == strncasecmp(WIFI_CONFIG, groups[group], strlen(groups[group])))
            {
                keys = g_key_file_get_keys(key_file, groups[group], &num_keys, &error);
                for(key = 0; key < num_keys; key++)
                {
                    value = g_key_file_get_value(key_file,	groups[group],	keys[key],	&error);
                   LOG_DBG("[ \t\tkey %u/%u: \t%s => %s]", key, num_keys - 1, keys[key], value);
                    if(0 == strncasecmp(MAX_TIMEOUT_ON_DISCONNECT, keys[key], strlen(keys[key])))
                    {
                        confProp.wifiProps.max_timeout = atoi(value);
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
                        STRCPY_S(confProp.wifiProps.getAuthTokenUrl, sizeof(confProp.wifiProps.getAuthTokenUrl), value);
                    }
                    if(0 == strncasecmp(GET_LFAT_URL, keys[key], strlen(keys[key]) ) )
                    {
                        STRCPY_S(confProp.wifiProps.getLfatUrl, sizeof(confProp.wifiProps.getLfatUrl), value);
                    }
                    if(0 == strncasecmp(SET_LFAT_URL, keys[key], strlen(keys[key]) ) )
                    {
                        STRCPY_S(confProp.wifiProps.setLfatUrl, sizeof(confProp.wifiProps.setLfatUrl), value);
                    }
                    if(0 == strncasecmp(LFAT_VERSION, keys[key], strlen(keys[key]) ) )
                    {
                        STRCPY_S(confProp.wifiProps.lfatVersion, sizeof(confProp.wifiProps.lfatVersion), value);
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
                        STRCPY_S(confProp.wifiProps.authServerURL, sizeof(confProp.wifiProps.authServerURL), value);
                    }
                    if(0 == strncasecmp(DISABLE_WPS_XRE, keys[key], strlen(keys[key])))
                    {
                        confProp.wifiProps.disableWpsXRE = (atoi(value) == 0) ? false : true;
                    }
                    if(value) g_free(value);
                }
                if(keys) g_strfreev(keys);
            }
#ifdef ENABLE_STUN_CLIENT
            else if(0 == strncasecmp(STUN_CONFIG, groups[group], strlen(groups[group])))
            {
                keys = g_key_file_get_keys(key_file, groups[group], &num_keys, &error);
                for(key = 0; key < num_keys; key++)
                {
                    value = g_key_file_get_value(key_file,	groups[group],	keys[key],	&error);

                    LOG_DBG("[ \t\tkey %u/%u: \t%s => %s]", key, num_keys - 1, keys[key], value);

                    if(0 == strncasecmp(STUN_SERVER, keys[key], strlen(keys[key])))
                    {
                        STRCPY_S(confProp.stunProps.server, sizeof(confProp.stunProps.server), value);
                    }
                    else if(0 == strncasecmp(STUN_PORT, keys[key], strlen(keys[key])))
                    {
                        confProp.stunProps.port = atoi(value);
                    }
                    else if(0 == strncasecmp(STUN_INTERFACE, keys[key], strlen(keys[key])))
                    {
                        STRCPY_S(confProp.stunProps.interface, sizeof(confProp.stunProps.interface), value);
                    }
                    else if(0 == strncasecmp(STUN_IPV6, keys[key], strlen(keys[key])))
                    {
                        confProp.stunProps.ipv6 = (atoi(value) == 0) ? false : true;
                    }
                    else if(0 == strncasecmp(STUN_BIND_TIMEOUT, keys[key], strlen(keys[key])))
                    {
                        confProp.stunProps.bind_timeout = atoi(value);
                    }
                    else if(0 == strncasecmp(STUN_CACHE_TIMEOUT, keys[key], strlen(keys[key])))
                    {
                        confProp.stunProps.cache_timeout = atoi(value);
                    }
                    if(value) g_free(value);
                }
                if(keys) g_strfreev(keys);

                if(!confProp.stunProps.bind_timeout)
                    confProp.stunProps.bind_timeout = STUN_DEFAULT_BIND_TIMEOUT;

                if(!confProp.stunProps.cache_timeout)
                    confProp.stunProps.cache_timeout = STUN_DEFAULT_CACHE_TIMEOUT;

                LOG_WARN("stun config: server=%s port=%u iface=%s ipv6=%u bind_timeout=%u cache_timeout=%u",
                    confProp.stunProps.server,
                    confProp.stunProps.port,
                    confProp.stunProps.interface,
                    confProp.stunProps.ipv6,
                    confProp.stunProps.bind_timeout,
                    confProp.stunProps.cache_timeout);
            }
#endif
        }
        if(groups) g_strfreev(groups);
    }
    if(key_file) g_key_file_free(key_file);

    return true;
}

/*Read Telemetry Parameter configurations*/
void Read_Telemetery_Param_File()
{
    LOG_ENTRY_EXIT;
    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;

    gboolean fstatus =  g_file_get_contents ((const gchar *) TELEMETRY_LOGGING_PARAM_FILE, &contents, &length, &error);

    if(!fstatus) {
       LOG_ERR("Failed to read \"%s\" file using g_file_get_contents() due to %s(%d)",
                TELEMETRY_LOGGING_PARAM_FILE, error->message, error->code);
        return;
    }
    else {
        LOG_DBG("Successfully read \"%s\". The file Contents are \"%s\" with length (%d).", TELEMETRY_LOGGING_PARAM_FILE, contents, (int)length);
    }

    if(contents)  {
        parse_telemetry_logging_configuration(contents);
        g_free(contents);
    }
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
        LOG_INFO("Parameter Name : \"%s\"", (char *)iter->data);
        iter = g_list_next(iter);
    }
}


bool parse_telemetry_logging_configuration(gchar *string)
{
    LOG_ENTRY_EXIT;

    if(NULL == string)
    {
        LOG_ERR("Failed due to NULL request buffer");
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

    return true;
}

bool update_telemetryParams_list(gchar *input_buffer, telemetryParams *telemery_params, gchar *time_period_string, gchar *param_name_string)
{
    cJSON *request_msg = NULL;

    LOG_ENTRY_EXIT;

    request_msg = cJSON_Parse(input_buffer);

    if (!request_msg)
    {
        LOG_ERR("Failed to parse json buffer with \"%s\"", cJSON_GetErrorPtr());
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
                LOG_ERR("Failed in cJSON_GetArrayItem()");
            }
            else {
                telemery_params->paramlist = g_list_prepend(telemery_params->paramlist, g_strdup(param_item->valuestring));
            }
        }
        if(telemery_params->paramlist)
            telemery_params->paramlist = g_list_reverse(telemery_params->paramlist);

        cJSON_Delete(request_msg);
    }

    return true;
}

void teleParamList_free (gpointer val)
{
    LOG_ENTRY_EXIT;
    if(val) {
        LOG_DBG("\"%s\"", (gchar *)val);
        g_free (val);
    }
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
    {
        STRCPY_S(param->activeIface, sizeof(param->activeIface), devName);
    }
    else
    {
        LOG_ERR("No Route Found");
        if(netSrvMgrUtiles::currentActiveInterface(devName))
        {
            if(!netSrvMgrUtiles::readDevFile(devName))
            {
                LOG_ERR("Get Active interface type failed for %s", devName);
            }
            else
            {
                STRCPY_S(param->activeIface, sizeof(param->activeIface), devName);
            }	
        }
        else
        {
            LOG_ERR(" No Active Interface.");
        }
    }
    LOG_INFO("Current Active Interface : [%s].", param->activeIface);
    LOG_INFO("TELEMETRY_NETWORK_MANAGER_ACTIVE_INTERFACE:%s", param->activeIface);
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
       LOG_ERR("list of Network Interface is empty");
    param->interfaceCount=count;
    LOG_INFO("list of Network Interface : [%s]", param->allNetworkInterfaces);
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
        WiFiNetworkMgr::getInstance()->setWifiEnabled (param->isInterfaceEnabled);
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
    if (list->size > (sizeof(list->interfaces)/sizeof(list->interfaces[0])))
    {
        LOG_INFO(" Size of list is larger than allocated size. Size returned = %d", list->size);
        return IARM_RESULT_IPCCORE_FAIL;
    }
    for (int i = 0; i < list->size; i++)
    {
        LOG_INFO("Before Copying name=%s, mac=%s, flags=0x%08x",
                interfaces[i].m_if_name.c_str(), interfaces[i].m_if_macaddr.c_str(), interfaces[i].m_if_flags);
        snprintf(list->interfaces[i].name, sizeof(list->interfaces[i].name), "%s", interfaces[i].m_if_name.c_str());
        snprintf(list->interfaces[i].mac, sizeof(list->interfaces[i].mac), "%s", interfaces[i].m_if_macaddr.c_str());
        list->interfaces[i].flags = interfaces[i].m_if_flags;
        LOG_INFO("name=%s, mac=%s, flags=0x%08x",
                list->interfaces[i].name, list->interfaces[i].mac, list->interfaces[i].flags);
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
    LOG_ENTRY_EXIT;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    char stbip[MAX_IP_ADDRESS_LEN];
    bool isIpv6=false;
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;

    if(netSrvMgrUtiles::getSTBip(stbip,&isIpv6))
    {
        STRCPY_S(param->activeIfaceIpaddr, sizeof(param->activeIfaceIpaddr), stbip);
        LOG_INFO("stb ipaddress : [%s]", stbip);
    }
    else
        LOG_ERR("stb ipaddress not found");
    return ret;
}

IARM_Result_t getSTBip_family(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    char stbip[MAX_IP_ADDRESS_LEN];
    IARM_BUS_NetSrvMgr_Iface_EventData_t *param = (IARM_BUS_NetSrvMgr_Iface_EventData_t *)arg;


	if (netSrvMgrUtiles::getSTBip_family(stbip,param->ipfamily))
	{
        STRCPY_S(param->activeIfaceIpaddr, sizeof(param->activeIfaceIpaddr), stbip);
        LOG_INFO("stb ipaddress : [%s]", stbip);
	}
    else
        LOG_ERR("stb ipaddress not found");
    return ret;
}

IARM_Result_t isConnectedToInternet(void *arg)
{
    LOG_ENTRY_EXIT;
    long timeout_ms = 2000; // using timeout = 2s ( < default IARM call timeout of 5s)
    LOG_INFO("test_connectivity: : BEGIN timeout = %ldms", timeout_ms);
    bool connectivity = (test_connectivity(timeout_ms) > 0);
    LOG_INFO("test_connectivity: : %s", connectivity ? "PASS" : "FAIL");
    *((bool*) arg) = connectivity;
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t setConnectivityTestEndpoints(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_Iface_TestEndpoints_t *param = (IARM_BUS_NetSrvMgr_Iface_TestEndpoints_t *)arg;
    std::vector<std::string> endpoints(param->endpoints, param->endpoints + param->size);
    return set_connectivity_test_endpoints(endpoints) ? IARM_RESULT_SUCCESS : IARM_RESULT_IPCCORE_FAIL;
}

IARM_Result_t isAvailable(void *arg)
{
    LOG_ENTRY_EXIT;
    LOG_INFO("[%s] IARM_BUS_NETSRVMGR_API_isAvailable is called", MODULE_NAME);
    return IARM_RESULT_SUCCESS;
}
#ifdef ENABLE_STUN_CLIENT
IARM_Result_t getPublicIP(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_BUS_NetSrvMgr_Iface_StunRequest_t *param = (IARM_BUS_NetSrvMgr_Iface_StunRequest_t *)arg;
    return getPublicIP (param) ? IARM_RESULT_SUCCESS : IARM_RESULT_IPCCORE_FAIL;
}
#endif
#endif // ENABLE_IARM


#if !defined(ENABLE_XCAM_SUPPORT) && !defined(XHB1) && !defined(XHC3)

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
        LOG_INFO("default route interface = %s gateway = %s", interface.c_str(), gateway.c_str());
        return true;
    }
    else
    {
        LOG_INFO("no default route");
        interface.clear();
        gateway.clear();
        return false;
    }
}
#endif // ifdef ENABLE_NLMONITOR

bool setDefaultInterface (const char* interface, bool persist)
{
    LOG_ENTRY_EXIT;
    LOG_INFO("interface = [%s], persist = [%d]", interface, persist);

#ifdef USE_RDK_WIFI_HAL
    if (strcmp (interface, "ETHERNET") == 0)
    {
        unmark("pni_wifi", persist); // remove marker file (if present) that says "WIFI is the preferred network interface"
        launch_pni_controller();
        return true;
    }
    else if (strcmp (interface, "WIFI") == 0)
    {
#ifndef DISABLE_PNI
        const char* RFC_PNI_ENABLE = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.PreferredNetworkInterface.Enable";
        if (isFeatureEnabled(RFC_PNI_ENABLE) == false)
        {
            LOG_ERR("[%s] failed. RFC %s is not set.", RFC_PNI_ENABLE);
            return false;
        }
        bool enabled;
        if (isInterfaceEnabled(interface, enabled) && !enabled) // even if this check is not done here, pni_controller will not use wifi if "/tmp/wifi_disallowed" exists
        {
            LOG_ERR("failed. interface [%s] is disabled", interface);
            return false;
        }
        mark("pni_wifi", persist); // touch marker file that says "WIFI is the preferred network interface"
        launch_pni_controller();
        return true;
#else
        LOG_ERR("failed. Build configuration DISABLE_PNI is set");
        return false;
#endif // DISABLE_PNI
    }
#else
    if (strcmp (interface, "ETHERNET") == 0)
    {
        LOG_INFO("nothing to do. interface [%s] is always the default interface.", interface);
        return true;
    }
#endif // #ifdef USE_RDK_WIFI_HAL
    else
    {
        LOG_ERR("failed. interface [%s] is invalid", interface);
        return false;
    }
}

bool isInterfaceEnabled(const char* interface, bool& enabled)
{
    LOG_ENTRY_EXIT;
    LOG_INFO("interface = [%s]", interface);

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
        LOG_ERR("interface [%s] is invalid", interface);
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
            LOG_INFO("interface = [%s] enabled = [%d]", interface, enabled);
            return true;
        }
    }
#endif // ifdef ENABLE_NLMONITOR
    LOG_ERR("interface = [%s] enabled = [NA]", interface);
    return false;
}

bool setInterfaceEnabled (const char* interface, bool enabled, bool persist)
{
    LOG_ENTRY_EXIT;
    LOG_INFO("interface = [%s] enable = [%d] persist = [%d]", interface, enabled, persist);

#ifndef USE_RDK_WIFI_HAL
    if (strcmp (interface, "ETHERNET") == 0)
    {
        if (enabled)
        {
            LOG_INFO("nothing to do. interface [%s] is always enabled", interface);
            return true;
        }
        else
        {
            LOG_ERR("failed. interface [%s] cannot be disabled", interface);
            return false;
        }
    }
#else
    if (strcmp (interface, "ETHERNET") == 0)
    {
        if (enabled)
        {
            unmark("ethernet_disallowed", persist); // remove marker file (if present) that says "ETHERNET is disallowed"
            setInterfaceState (getenvOrDefault("ETHERNET_INTERFACE", ""), true);
            launch_pni_controller();
            return true;
        }
        else // if (validate_interface_can_be_disabled(interface)) not needed for ETHERNET as per clarification in ticket
        {
            mark("ethernet_disallowed", persist); // touch marker file that says "ETHERNET is disallowed"
            setInterfaceState (getenvOrDefault("ETHERNET_INTERFACE", ""), false);
            launch_pni_controller();
            return true;
        }
    }
    else if (strcmp (interface, "WIFI") == 0)
    {
        if (enabled)
        {
            unmark("wifi_disallowed", persist); // remove marker file (if present) that says "WIFI is disallowed"
            setInterfaceState (getenvOrDefault("WIFI_INTERFACE", ""), true);
            WiFiNetworkMgr::getInstance()->setWifiEnabled(true);
            launch_pni_controller();
            return true;
        }
        else if (validate_interface_can_be_disabled(interface))
        {
            mark("wifi_disallowed", persist); // touch marker file that says "WIFI is disallowed"
            setInterfaceState (getenvOrDefault("WIFI_INTERFACE", ""), false);
            WiFiNetworkMgr::getInstance()->setWifiEnabled(false);
            launch_pni_controller();
            return true;
        }
        else
        {
            LOG_ERR("failed. interface [%s] cannot be disabled.", interface);
            return false;
        }
    }
#endif // ifndef USE_RDK_WIFI_HAL
    else
    {
        LOG_ERR("failed. interface [%s] is invalid.", interface);
        return false;
    }
}

bool getDNSip (const unsigned int family, char *primaryDNS, char *secondaryDNS)
{
    LOG_ENTRY_EXIT;
    char* dns[2] = {primaryDNS, secondaryDNS};
    std::string line, keyword, value;
    unsigned char s[sizeof(struct in6_addr)];
    std::ifstream f("/etc/resolv.dnsmasq");
    if (!f.is_open())
    {
        LOG_ERR("DNS file is not present");
        return false;
    }
    for (int i = 0; i < 2 && std::getline (f, line); )
    {
        std::istringstream ss(line);
        ss >> keyword;
        if (keyword == "nameserver")
        {
            ss >> std::ws >> value;
            if (1 == inet_pton(family, value.c_str(), s))
            {
                    STRCPY_S(dns[i++], INET6_ADDRSTRLEN, value.c_str());
            }
        }
    }
    f.close();
    return true;
}

static bool getDhcpServerIP()
{
    std::string line;;
    std::ifstream f(NETSRVMGR_DHCP_SERVERIP_PATH);
    if (!f.is_open())
    {
        LOG_INFO("DHCP SERVER IP file is not present");
        return false;
    }

    std::getline (f, line);
    struct in_addr ipv4address;
    if (inet_pton(AF_INET, line.c_str(), &ipv4address) <= 0)
    {
        LOG_ERR("invalid ipaddress [%s]", line.c_str());
        f.close();
        return false;
    }

    gDhcpServerIP = line;
    f.close();

    return true;
}

// returns true if IP settings are the same, false otherwise
bool ip_settings_compare(const IARM_BUS_NetSrvMgr_Iface_Settings_t& ip_settings1, const IARM_BUS_NetSrvMgr_Iface_Settings_t& ip_settings2)
{
    LOG_ENTRY_EXIT;
    return  0 == strcmp(ip_settings1.ipaddress, ip_settings2.ipaddress) &&
            0 == strcmp(ip_settings1.netmask, ip_settings2.netmask) &&
            0 == strcmp(ip_settings1.gateway, ip_settings2.gateway) &&
            0 == strcmp(ip_settings1.primarydns, ip_settings2.primarydns) &&
            0 == strcmp(ip_settings1.secondarydns, ip_settings2.secondarydns);
}

bool ip_settings_file_read(const char* ip_settings_filename, IARM_BUS_NetSrvMgr_Iface_Settings_t& ip_settings)
{
    LOG_ENTRY_EXIT;
    FILE *fp = fopen(ip_settings_filename, "r");
    if (fp == NULL)
        return false;
    char name_value_entry[512];
    while (NULL != fgets(name_value_entry, sizeof(name_value_entry), fp))
    {
        char* key = strtok (name_value_entry, "=\n");
        char* value = strtok (NULL, "=\n");
        if (0 == strcmp (key, "ipaddress"))
            snprintf(ip_settings.ipaddress, sizeof(ip_settings.ipaddress), "%s", value);
        else if (0 == strcmp (key, "netmask"))
            snprintf(ip_settings.netmask, sizeof(ip_settings.netmask), "%s", value);
        else if (0 == strcmp (key, "gateway"))
            snprintf(ip_settings.gateway, sizeof(ip_settings.gateway), "%s", value);
        else if (0 == strcmp (key, "primarydns"))
            snprintf(ip_settings.primarydns, sizeof(ip_settings.primarydns), "%s", value);
        else if (0 == strcmp (key, "secondarydns"))
            snprintf(ip_settings.secondarydns, sizeof(ip_settings.secondarydns), "%s", value);
    }
    fclose(fp);
    return true;
}

bool ip_settings_file_write(const char* ip_settings_filename, const IARM_BUS_NetSrvMgr_Iface_Settings_t& ip_settings)
{
    LOG_ENTRY_EXIT;
    FILE *fp = fopen(ip_settings_filename, "w");
    if (fp == NULL)
        return false;
    fprintf(fp, "ipaddress=%s\nnetmask=%s\ngateway=%s\nprimarydns=%s\nsecondarydns=%s\n",
            ip_settings.ipaddress, ip_settings.netmask, ip_settings.gateway, ip_settings.primarydns, ip_settings.secondarydns);
    fclose(fp);
    return true;
}

// triggers reconfigure of given interface if it is the active interface, otherwise does nothing.
void ipv4_reconfigure_interface (const char* interface)
{
    LOG_ENTRY_EXIT;
    std::string default_interface;
    std::string default_gateway;
    if ( getDefaultInterface(default_interface, default_gateway) && (0 != strcmp(interface, default_interface.c_str())) )
        return;
    char command[128];
    snprintf (command, sizeof(command), "/lib/rdk/pni_controller.sh ipv4_reconfigure_interface %s", interface);
    LOG_INFO("Executing command [%s]", command);
    int status = system(command);
    LOG_INFO("Exit code [%d] from command [%s]", status, command);
}

bool valid_ipv4_netmask (uint32_t mask)
{
    LOG_ENTRY_EXIT;
    if (mask == 0)        // = 00000000 00000000 00000000 00000000 (invalid)
        return false;
    // example mask          = 11111111 11111111 11111111 00000000 (255.255.255.0  valid IPv4 netmask = 1-bit string followed by 0-bit string, totaling 32 bits)
    uint32_t x = ~mask;   // = 00000000 00000000 00000000 11111111 (bitwise NOT of valid IPv4 netmask = 0-bit string followed by 1-bit string, totaling 32 bits)
    uint32_t y = x + 1;   // = 00000000 00000000 00000001 00000000 (add 1)
    return (x & y) == 0;  // = 00000000 00000000 00000000 00000000 (bitwise AND of last 2 sequences = 0 for a valid IPv4 netmask)
}

bool setIPSettings(IARM_BUS_NetSrvMgr_Iface_Settings_t *param)
{
    LOG_ENTRY_EXIT;
    LOG_INFO("interface [%s], ipversion [%s], autoconfig [%d], ipaddress [%s], netmask [%s], gateway [%s], primarydns [%s], secondarydns [%s]",
            param->interface, param->ipversion, param->autoconfig, param->ipaddress, param->netmask, param->gateway, param->primarydns, param->secondarydns);

    std:string secondarydns = param->secondarydns;
    const char* RFC_MANUALIP_ENABLE = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Network.ManualIPSettings.Enable";

    bool ethernet = true;
    if (0 == strcasecmp(param->interface, "WIFI"))
    {
        ethernet = false;
    }
    else if (0 != strcasecmp(param->interface, "ETHERNET"))
    {
        LOG_ERR("unsupported interface [%s]", param->interface);
        return false;
    }

    const char *interface = ethernet ? getenv("ETHERNET_INTERFACE") : getenv("WIFI_INTERFACE");
    if (interface == NULL)
    {
        LOG_ERR("failed to identify interface");
        return false;
    }

    const char* ip_settings_file = ethernet ? "/opt/persistent/ip.eth0.0" : "/opt/persistent/ip.wifi.0";
    bool autoconfig = ( access(ip_settings_file, F_OK) != 0 );

    LOG_INFO("interface: [%s], autoconfig: current [%d] requested [%d]", interface, autoconfig, param->autoconfig);

    if (param->autoconfig)
    {
        param->isSupported = true;
        if (autoconfig)
        {
            LOG_INFO("already in autoconfig mode.");
            return true;
        }
        else if (0 != remove(ip_settings_file))
        {
            LOG_ERR("remove(%s) returned errno %d (%s)", ip_settings_file, errno, strerror(errno));
            return false;
        }
        ipv4_reconfigure_interface (interface);
        return true;
    }
    else
    {
        if (isFeatureEnabled(RFC_MANUALIP_ENABLE) == false)
        {
            LOG_ERR("RFC for manual IP settings is not enabled");
            param->isSupported = false;
            return false;
        }

        if (0 != strcasecmp(param->ipversion, "ipv4"))
        {
            LOG_ERR("unsupported ipversion [%s]", param->ipversion);
            param->isSupported = false;
            return false;
        }

        struct in_addr ipv4address;
        if (!inet_pton(AF_INET, param->ipaddress, &ipv4address))
        {
            LOG_ERR("invalid ipaddress [%s]", param->ipaddress);
            return false;
        }
        if (!inet_pton(AF_INET, param->netmask, &ipv4address) || !valid_ipv4_netmask(ntohl(ipv4address.s_addr)))
        {
            LOG_ERR("invalid netmask [%s]", param->netmask);
            return false;
        }
        if (!inet_pton(AF_INET, param->gateway, &ipv4address))
        {
            LOG_ERR("invalid gateway [%s]", param->gateway);
            return false;
        }
        if (!inet_pton(AF_INET, param->primarydns, &ipv4address))
        {
            LOG_ERR("invalid primarydns [%s]", param->primarydns);
            return false;
        }
        if (!secondarydns.empty())
        {
            if (!inet_pton(AF_INET, param->secondarydns, &ipv4address))
            {
                LOG_ERR("invalid secondarydns [%s]", param->secondarydns);
                return false;
            }
        }
        IARM_BUS_NetSrvMgr_Iface_Settings_t current_ip_settings = {};
        if (!autoconfig && ip_settings_file_read(ip_settings_file, current_ip_settings) && ip_settings_compare(current_ip_settings, *param))
        {
            LOG_INFO("provided config is same as the current config");
            return true;
        }
        if (!ip_settings_file_write(ip_settings_file, *param))
        {
            LOG_ERR("failed to save the new config. enabling dhcp");
            remove(ip_settings_file);
            ipv4_reconfigure_interface (interface);
            return false;
        }
        ipv4_reconfigure_interface (interface);
        return true;
    }
}

bool getIPSettings(IARM_BUS_NetSrvMgr_Iface_Settings_t *param)
{
    LOG_ENTRY_EXIT;
    bool is_ipv6=true;
    char interface[16] = {0};
    std::string inter;
    std::string gateway;
    char primaryDNSaddr[INET6_ADDRSTRLEN]={0};
    char secondaryDNSaddr[INET6_ADDRSTRLEN]={0};

    const char* RFC_MANUALIP_ENABLE = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Network.ManualIPSettings.Enable";
    if (isFeatureEnabled(RFC_MANUALIP_ENABLE) == false)
    {
        LOG_INFO("Manual IP settings RFC is disabled");
    }
    param->errCode = NETWORK_IPADDRESS_ACQUIRED;
    if (0 == strcasecmp (param->interface, "WIFI") )
    {
        char* c = getenv("WIFI_INTERFACE");
        snprintf (interface, sizeof(interface), "%s", c ? c : "");
        struct stat stat_buf;
        param->autoconfig = (access( "/opt/persistent/ip.wifi.0", F_OK ) != 0 ) ? true :(stat("/opt/persistent/ip.wifi.0", &stat_buf) == 0 ? stat_buf.st_size == 0 :false);
    }
    else if (0 == strcasecmp(param->interface, "ETHERNET"))
    {
        char* c = getenv("ETHERNET_INTERFACE");
        snprintf (interface, sizeof(interface), "%s", c ? c : "");
        struct stat stat_buf;
        param->autoconfig = (access( "/opt/persistent/ip.eth0.0", F_OK ) != 0 ) ? true :(stat("/opt/persistent/ip.eth0.0", &stat_buf) == 0 ? stat_buf.st_size == 0 :false);
    }
    //If interface param is not provided, use currently active interface
    else if (!netSrvMgrUtiles::currentActiveInterface(interface))
    {
        LOG_ERR("No routable  interface found.");
        param->errCode = NETWORK_NO_ROUTE_INTERFACE;
        return false;
    }
    else if (0 == strcasecmp(interface, getenv("ETHERNET_INTERFACE")))
    {
        STRCPY_S(param->interface, sizeof(param->interface), "ETHERNET");
        struct stat stat_buf;
        param->autoconfig = (access( "/opt/persistent/ip.eth0.0", F_OK ) != 0 ) ? true :(stat("/opt/persistent/ip.eth0.0", &stat_buf) == 0 ? stat_buf.st_size == 0 :false);

    }
    else if (0 == strcasecmp(interface, getenv("WIFI_INTERFACE")))
    {
        STRCPY_S(param->interface, sizeof(param->interface), "WIFI");
        struct stat stat_buf;
        param->autoconfig = (access( "/opt/persistent/ip.wifi.0", F_OK ) != 0 ) ? true :(stat("/opt/persistent/ip.wifi.0", &stat_buf) == 0 ? stat_buf.st_size == 0 :false);
    }
    LOG_INFO("interface [%s]", interface);

    /*To get ipaddress based on interface and family input paramter*/
    if (strcasecmp (param->ipversion, "IPV6") == 0)
    {
        if(!netSrvMgrUtiles::getInterfaceConfig(interface, AF_INET6, param->ipaddress, param->netmask))
        {
            LOG_ERR("stb ipv6 ipaddress not found");
            param->errCode = NETWORK_IPADDRESS_NOTFOUND;
            return true;
        }
    }
    else if (strcasecmp (param->ipversion, "IPV4") == 0)
    {
        is_ipv6=false;
        if(!netSrvMgrUtiles::getInterfaceConfig(interface, AF_INET, param->ipaddress, param->netmask))
        {
            LOG_ERR("stb ipv4 ipaddress not found.");
            param->errCode = NETWORK_IPADDRESS_NOTFOUND;
            return true;
        }
    }
    else if (netSrvMgrUtiles::getInterfaceConfig(interface, AF_INET6, param->ipaddress, param->netmask))
    {
        STRCPY_S(param->ipversion, sizeof(param->ipversion), "IPV6");
    }
    else if (netSrvMgrUtiles::getInterfaceConfig(interface, AF_INET, param->ipaddress, param->netmask))
    {
        STRCPY_S(param->ipversion, sizeof(param->ipversion), "IPV4");
        is_ipv6=false;
    }
    else
    {
        LOG_ERR("stb ipaddress not found");
        param->errCode = NETWORK_IPADDRESS_NOTFOUND;
        return true;
    }

    /* ipv4/ ipv6 default route depends  on is_ipv6 flag */
    if(NetLinkIfc::get_instance()->getDefaultRoute(is_ipv6, inter, gateway))
    {
        snprintf(param->gateway, sizeof(param->gateway), "%s", gateway.c_str());
        LOG_INFO("Default Gateway [%s]", param->gateway);
    }
    else
    {
        LOG_INFO("Default Gateway not present ");
        param->errCode = NETWORK_NO_DEFAULT_ROUTE;
    }

    /* To DHCP Server IP */
    if ((param->autoconfig) && (!is_ipv6))
    {
        snprintf(param->dhcp_server, sizeof(param->dhcp_server), "%s", gDhcpServerIP.c_str());
    }
    else
    {
        snprintf(param->dhcp_server, sizeof(param->dhcp_server), "");
    }
    LOG_INFO("DHCP Server IP Address [%s]", param->dhcp_server);

    /* To get Primary and Secondary DNS */
    if (getDNSip(is_ipv6 ? AF_INET6 : AF_INET,primaryDNSaddr, secondaryDNSaddr))
    {
        STRCPY_S(param->primarydns, sizeof(param->primarydns), primaryDNSaddr);
        STRCPY_S(param->secondarydns, sizeof(param->secondarydns), secondaryDNSaddr);
        LOG_INFO("Primary DNS %s", param->primarydns);
        LOG_INFO("Secondary DNS %s", param->secondarydns);
    }
    else
    {
        LOG_INFO("No DNS ip is confiured");
        param->errCode = NETWORK_DNS_NOT_CONFIGURED; //DNS file is not present
    }
    return true;
}

bool setInterfaceState (std::string interface_name, bool enabled)
{
    if (interface_name.empty())
    {
        LOG_ERR("no interface name");
        return false;
    }
    char command[64];
    snprintf (command, sizeof(command), "ip link set dev %s %s", interface_name.c_str(), enabled ? "up" : "down");
    LOG_INFO("%s", command);
    system (command);
    return true;
}

#ifdef ENABLE_STUN_CLIENT
bool getPublicIP (IARM_BUS_NetSrvMgr_Iface_StunRequest_t* param)
{
    std::string     host    (param->server[0]        ? param->server             : confProp.stunProps.server);
    uint16_t        port    (param->port             ? param->port               : confProp.stunProps.port);
    stun::protocol  proto   (param->ipv6             ? stun::protocol::af_inet6  : stun::protocol::af_inet);
    uint16_t        bindtm  (param->bind_timeout     ? param->bind_timeout       : confProp.stunProps.bind_timeout);
    uint16_t        cachetm (param->cache_timeout    ? param->cache_timeout      : confProp.stunProps.cache_timeout);
    std:string interface = param->interface;
    char activeInterface[INTERFACE_SIZE];

    long timeout_ms = 2000; // using timeout = 2s ( < default IARM call timeout of 5s)
    LOG_DBG("BEGIN timeout = %ldms", timeout_ms);
    bool connectivity = (test_connectivity(timeout_ms) > 0);
    if (!connectivity)
    {
        LOG_ERR("testConnectivity failed: internet not available");
        return false;
    }

    if (0 == strcasecmp(param->interface, "WIFI"))
    {
        interface = getenvOrDefault("WIFI_INTERFACE", "");
    }
    else if (0 == strcasecmp(param->interface, "ETHERNET"))
    {
        interface = getenvOrDefault("ETHERNET_INTERFACE", "");
    }
    else if (0 == strcasecmp(param->interface, "MOCA"))
    {
        interface = getenvOrDefault("MOCA_INTERFACE", "");
    }
    else if (interface.empty())
    {
        if(!netSrvMgrUtiles::getRouteInterface(activeInterface))
        {
            if(!netSrvMgrUtiles::currentActiveInterface(activeInterface))
            {
                LOG_ERR("No routable  interface found");
                return false;
            }
        }
        interface = activeInterface;
    }
    else
    {
        LOG_ERR("failed to identify interface");
        return false;
    }

    if (!param->ipv6)
        interface += ":0";
    LOG_INFO("interface value: %s", interface.c_str());
    std::string     iface   (param->interface[0]     ? interface          : confProp.stunProps.interface);

    stun::bind_result result;
    if(stunClient.bind(host, port, iface, proto, bindtm, cachetm, result))
    {
        strncpy(param->public_ip, result.public_ip.c_str(), MAX_IP_ADDRESS_LEN);
        return true;
    }
    else
    {
        return false;
    }
}
#endif

#endif // ifndef ENABLE_XCAM_SUPPORT and XHB1 and XHC3
