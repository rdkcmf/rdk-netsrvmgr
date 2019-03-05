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

/*****************************************************************************
 * STANDARD INCLUDE FILES
 *****************************************************************************/
/**
* @defgroup netsrvmgr
* @{
* @defgroup netsrvmgr
* @{
**/


/**
 * @file netSrvMgrUtiles.cpp
 *
 * @brief DeviceInfo X_RDKCENTRAL-COM_xBlueTooth API Implementation.
 *
 * This is the implementation of the netsrvmgr utility API.
 *
 * @par Document
 * TBD Relevant design or API documentation.
 *
 */


#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <algorithm>
#include "NetworkMgrMain.h"
#include "netsrvmgrUtiles.h"
#include <string.h>
#include <errno.h>
#ifdef ENABLE_NLMONITOR
#include "netlinkifc.h"
#else
#include <net/if.h>
#endif //ENABLE_NLMONITOR
#include <arpa/inet.h>
#define IN_IS_ADDR_LINKLOCAL(a)     (((a) & htonl(0xffff0000)) == htonl (0xa9fe0000))
#define INTERFACE_SIZE 10

#define TRIGGER_DHCP_LEASE_FILE "/lib/rdk/triggerDhcpLease.sh"
#define MAX_TIME_LENGTH 32
#define INTERFACE_STATUS_FILE_PATH_BUFFER 100
#define INTERFACE_STATUS_FILE_PATH "/sys/class/net/%s/operstate"
#define ETHERNET_UP_STATUS "UP"

static const char INTERFACE_PERSISTENCE_FILE[] = "/opt/persistent/interfacePersistent.dat";

EntryExitLogger::EntryExitLogger (const char* func, const char* file) :
        func (func), file (file)
{
    RDK_LOG (RDK_LOG_TRACE1, LOG_NMGR, "Entry: %s [%s]\n", func, file);
}

EntryExitLogger::~EntryExitLogger ()
{
    RDK_LOG (RDK_LOG_TRACE1, LOG_NMGR, "Exit: %s [%s]\n", func, file);
}

static bool loadKeyFile (const char* filename, GKeyFile* keyFile);
static bool writeKeyFile (const char* filename, GKeyFile* keyFile);

using namespace netSrvMgrUtiles;

/**
 * @fn netSrvMgrUtiles::getMacAddress_IfName(char *ifName_in, char *macAddress_out)
 * @brief This function gets the MAC address of the AspectRatio against the id specified, only if
 * the id passed is valid.
 *
 * @param[in] ifName_in Indicates the interface name which the mac address is required.
 * @param[out] macAddress_out Indicates the mac address of ifname_in interface name.
 *
 * @return Returns true if successfully gets the mac address of interface provided or else false.
 */
bool netSrvMgrUtiles::getMacAddress_IfName(char *ifName_in, char macAddress_out[MAC_ADDR_BUFF_LEN])
{
    struct ifreq ifr;
    int sd;
    unsigned char *mac = NULL;
    bool ret = false;

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);

    if(NULL == ifName_in)
    {
        RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d] Failed, due to NULL for interface name (\'ifName_in\') .\n", __FUNCTION__, __LINE__);
        return ret;
    }

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd > 0) {
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, ifName_in, IFNAMSIZ-1);
        if (0 == ioctl(sd, SIOCGIFHWADDR, &ifr)) {
            mac = (unsigned char *) ifr.ifr_hwaddr.sa_data;
            if(mac) {
                snprintf( macAddress_out, MAC_ADDR_BUFF_LEN, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                RDK_LOG(RDK_LOG_DEBUG,LOG_NMGR,"[%s:%d]The Mac address is \'%s\' (for provided interface %s).\n", __FUNCTION__, __LINE__, macAddress_out, ifName_in );
                ret = true;
            }
        }
        else {
            RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] Failed in ioctl() with  \'%s\'..\n", __FUNCTION__, __LINE__, strerror (errno) );
        }
        close(sd);
    }
    else {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] Failed to create socket() with \'%s\' return as \'%d\'.\n", __FUNCTION__, __LINE__, sd, strerror (errno) );
    }
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return ret;
}

/**
 * @fn netSrvMgrUtiles::get_IfName_devicePropsFile(void)
 * @brief This function gets the interface name based on the device/properties file.
 * the id passed is valid.
 *
 * @param[in] void
 *
 * @return Returns interface .
 */
char* netSrvMgrUtiles::get_IfName_devicePropsFile(void)
{
    static char *ifName = NULL;

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);

    const char *Wifi_Enable_file = "/tmp/wifi-on";
    bool isWifiEnabled = (!access (Wifi_Enable_file, F_OK))?true:false;

    ifName = isWifiEnabled ? getenv("WIFI_INTERFACE") : getenv("MOCA_INTERFACE");

    RDK_LOG(RDK_LOG_DEBUG,LOG_NMGR,"[%s:%d]get_Eth_If_Name(): ethIf=%s\n", __FUNCTION__, __LINE__, ifName);

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return ifName;
}


/* End of doxygen group */
/**
 * @}
 */
/* End of file netSrvMgrUtiles.cpp */

/** @} */



void netSrvMgrUtiles::triggerDhcpLease(Dhcp_Lease_Operation op)
{
    static char* RENEW = "renew";
    static char* RELEASE_AND_RENEW = "release_and_renew";

    char* operation = NULL;
    if (netSrvMgrUtiles::DHCP_LEASE_RENEW == op)
        operation = RENEW;
    else if (netSrvMgrUtiles::DHCP_LEASE_RELEASE_AND_RENEW == op)
        operation = RELEASE_AND_RENEW;
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] Unsupported DHCP lease operation [%d]\n", __FUNCTION__, op);
        return;
    }

    char command[200];
    sprintf (command, "%s %s 2>&1", TRIGGER_DHCP_LEASE_FILE, operation);

    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] Executing command [%s]\n", __FUNCTION__, command);

    char scriptLogOutputLine[256];

    FILE *fp = popen (command, "r");
    if (fp == NULL)
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] Failed to execute command [%s]\n", __FUNCTION__, command);
        return;
    }

    // log script output at INFO level
    while (fgets (scriptLogOutputLine, sizeof (scriptLogOutputLine), fp) != NULL)
    {
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] %s", __FUNCTION__, scriptLogOutputLine);
    }

    int pclose_status = pclose (fp);
    int status = WEXITSTATUS (pclose_status);

    RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] Exit code [%d] from command [%s]\n", __FUNCTION__, status, command);
}

bool netSrvMgrUtiles::getRouteInterface(char* devname)
{
    LOG_ENTRY_EXIT;

    bool route_found = false;
    char dst[50],gw[50];
    char line[100] , *ptr , *ctr, *sptr;
    int ret;
    FILE *fp;
    fp = fopen("/proc/net/ipv6_route","r");
    if(fp!=NULL) {
        while((ret=fscanf(fp,"%s %*x %*s %*x %s %*x %*x %*x %*x %s",
                          dst,gw,
                          devname))!=EOF) {
            /*return value must be parameter's number*/
            if(ret!=3) {
                continue;
            }
            if((strcmp(dst,gw) == 0) && (strcmp(devname,"sit0") != 0) && (strcmp(devname,"lo") != 0) )
            {
//                readDevFile(devname);
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] active interface v6 %s  \n", __FUNCTION__, __LINE__,devname);
                route_found = true;
                goto close_file;
            }
        }
    }
    fclose(fp);

    fp = fopen("/proc/net/route" , "r");
    while(fgets(line , 100 , fp))
    {
        ptr = strtok_r(line , " \t", &sptr);
        ctr = strtok_r(NULL , " \t", &sptr);

        if(ptr!=NULL && ctr!=NULL)
        {
            if(strcmp(ctr , "00000000") == 0)
            {
                strcpy(devname,ptr);
//                readDevFile(devname);
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] active interface v4 %s  \n", __FUNCTION__, __LINE__,devname);
                route_found = true;
                goto close_file;
            }
        }
    }

close_file:
    fclose(fp);

    if (!route_found)
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] No Route Found\n", __FUNCTION__, __LINE__);

    return route_found;
}

bool netSrvMgrUtiles::getRouteInterfaceType(char* devname)
{
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
    if(!getRouteInterface(devname))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Get Route interface failure  \n", __FUNCTION__, __LINE__);
        return false;
    }
    if(!readDevFile(devname))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Get Route interface type failure  \n", __FUNCTION__, __LINE__);
        return false;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Route interface type %s  \n", __FUNCTION__, __LINE__,devname);
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return true;
}

bool netSrvMgrUtiles::readDevFile(char *deviceName)
{
    LOG_ENTRY_EXIT;

    GError *error = NULL;
    gboolean result = FALSE;
    gchar* devfilebuffer = NULL;
    const gchar* deviceFile = "//etc/device.properties";
    if(!deviceName)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] device name is null  \n", __FUNCTION__, __LINE__);
    }
    else if (g_file_get_contents (deviceFile, &devfilebuffer, NULL, &error) == false)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] No contents in device properties  \n", __FUNCTION__, __LINE__);
        if (!error) g_error_free (error);
    }
    else
    {
        gchar **tokens = g_strsplit_set(devfilebuffer,",='\n'", -1);
        g_free (devfilebuffer);
        guint toklength = g_strv_length(tokens);
        guint loopvar=0;
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Interface Name  %s  \n", __FUNCTION__, __LINE__,deviceName);
        for (loopvar=0; loopvar<toklength; loopvar++)
        {
            if (g_strrstr(g_strstrip(tokens[loopvar]), "ETHERNET_INTERFACE") && ((g_strcmp0(g_strstrip(tokens[loopvar+1]),deviceName)) == 0))
            {
                if ((loopvar+1) < toklength )
                {
                    strcpy(deviceName,"ETHERNET");
                    result=TRUE;
                    break;
                }
            }
            else if (g_strrstr(g_strstrip(tokens[loopvar]), "MOCA_INTERFACE") && ((g_strcmp0(g_strstrip(tokens[loopvar+1]),deviceName)) == 0))
            {
                if ((loopvar+1) < toklength )
                {
                    strcpy(deviceName,"MOCA");
                    result=TRUE;
                    break;
                }
            }
            else if (g_strrstr(g_strstrip(tokens[loopvar]), "WIFI_INTERFACE") && ((g_strcmp0(g_strstrip(tokens[loopvar+1]),deviceName)) == 0))
            {
                if ((loopvar+1) < toklength )
                {
                    strcpy(deviceName,"WIFI");
                    result=TRUE;
                    break;
                }
            }
        }
        g_strfreev (tokens);
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Network Type  %s  \n", __FUNCTION__, __LINE__,deviceName);
    }
    return result;
}

char netSrvMgrUtiles::getAllNetworkInterface(char* devAllInterface)
{
    struct ifaddrs *ifAddrStruct,*tmpAddrPtr;
    bool firstInterface=false;
    char count=0;
    char tempInterface[10];
    getifaddrs(&ifAddrStruct);
    tmpAddrPtr = ifAddrStruct;
    GString *device = g_string_new(NULL);
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);

    while (tmpAddrPtr)
    {
        if (tmpAddrPtr->ifa_addr && (strcmp(tmpAddrPtr->ifa_name,"sit0") != 0) && (strcmp(tmpAddrPtr->ifa_name,"lo") != 0))
	{
	    g_stpcpy(tempInterface,tmpAddrPtr->ifa_name);
               RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] 1 interface [%s].  \n", __FUNCTION__, __LINE__,tempInterface);
	    if((readDevFile(tempInterface)) && (!g_strrstr(device->str,tempInterface)))
	    {
	       if(firstInterface)
	       {
                 g_string_append_printf(device,"%s",",");
               }
	       else
	       {
	         firstInterface=true;
               }
               g_string_append_printf(device,"%s",tempInterface);
               RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] interface [%s] interface List [%s]  \n", __FUNCTION__, __LINE__,tempInterface,device->str);
	       count++;
	    }
	}
        tmpAddrPtr = tmpAddrPtr->ifa_next;
    }
    freeifaddrs(ifAddrStruct);
    strcpy(devAllInterface,device->str);
    g_string_free(device,TRUE);
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return count;
}

bool netSrvMgrUtiles::getCurrentTime(char* currTime, const char *timeFormat)
{
  time_t cTime;
  struct tm *tmp;
  RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
  time(&cTime);
  tmp=localtime(&cTime);
  if(tmp == NULL)
  {
    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Error getting local time  \n", __FUNCTION__, __LINE__);
    return false;
  }
  if(strftime(currTime,MAX_TIME_LENGTH,timeFormat,tmp) == 0)
  {
    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] strftime failed in getting time format format %s  \n", __FUNCTION__, __LINE__,timeFormat);
    return false;
  }
  RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
  return true;
}

bool netSrvMgrUtiles::checkInterfaceActive(char *interfaceName)
{
    FILE * file;
    char buffer[INTERFACE_STATUS_FILE_PATH_BUFFER]={0};
    bool ret=false;
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
    if(!interfaceName)
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] Device doesnt support Ethernet !!!! \n", __FUNCTION__, __LINE__);
        return ret;
    }
    sprintf(buffer,INTERFACE_STATUS_FILE_PATH,interfaceName);
    file = fopen(buffer, "r");
    if (file)
    {
        fscanf(file,"%s",buffer);
        if(NULL != strcasestr(buffer,ETHERNET_UP_STATUS))
        {
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] TELEMETRY_NETWORK_MANAGER_ETHERNET_MODE \n", __FUNCTION__, __LINE__);
            ret=true;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] TELEMETRY_NETWORK_MANAGER_WIFI_MODE \n", __FUNCTION__, __LINE__);
        }
        fclose(file);
    }
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return ret;
}

// returns:
// true if file with given 'filename' could be loaded into 'keyFile'
// false otherwise
bool loadKeyFile (const char* filename, GKeyFile* keyFile)
{
    LOG_ENTRY_EXIT;

    bool ret = true;
    GError* err = NULL;
    g_key_file_load_from_file (keyFile, filename, G_KEY_FILE_NONE, &err);
    if (err != NULL)
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] error loading '%s' (error domain=%d code=%d message=%s)\n",
                __FUNCTION__, filename, err->domain, err->code, err->message);
        g_error_free (err);
        ret = false;
    }
    return ret;
}

// returns:
// true if 'keyFile' could be written into file with given 'filename'
// false otherwise
bool writeKeyFile (const char* filename, GKeyFile* keyFile)
{
    LOG_ENTRY_EXIT;

    bool ret = true;
    gchar *contents = g_key_file_to_data (keyFile, NULL, NULL);
    GError* err = NULL;
    g_file_set_contents (filename, contents, -1, &err);
    if (err != NULL)
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] error writing '%s' (error domain=%d code=%d message=%s)\n",
                __FUNCTION__, filename, err->domain, err->code, err->message);
        g_error_free (err);
        ret = false;
    }
    g_free (contents);
    return ret;
}

// returns:
// false if no saved enable/disable config exists for given interface (interfaceControlPersistence = false)
// true otherwise (interfaceControlPersistence = true) and also returns the saved enable/disable config by reference
bool netSrvMgrUtiles::getSavedInterfaceConfig(const char *interface, bool& enable)
{
    LOG_ENTRY_EXIT;

    bool saved_config_exists = false; // default
    GKeyFile *keyFile = g_key_file_new ();
    if (loadKeyFile (INTERFACE_PERSISTENCE_FILE, keyFile) && g_key_file_has_key (keyFile, "Interface", interface, NULL))
    {
        saved_config_exists = true;
        enable = g_key_file_get_boolean (keyFile, "Interface", interface, NULL);
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface = [%s] enable = [%s]\n", __FUNCTION__, interface, enable ? "true" : "false");
    }
    else
    {
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] interface = [%s] no saved config\n", __FUNCTION__, interface);
    }
    g_key_file_free (keyFile);
    return saved_config_exists;
}

// returns:
// true if given interface config could be persisted
// false otherwise
bool netSrvMgrUtiles::saveInterfaceConfig(const char *interface, bool enable)
{
    LOG_ENTRY_EXIT;

    bool ret = false;
    GKeyFile *keyFile = g_key_file_new ();
    loadKeyFile (INTERFACE_PERSISTENCE_FILE, keyFile);
    // if loadKeyFile is
    // successful:   updated persistence file will contain given interface/key and value; other keys are unaffected
    // unsuccessful: updated persistence file will contain only given interface/key and value
    g_key_file_set_boolean (keyFile, "Interface", interface, enable);
    if (writeKeyFile (INTERFACE_PERSISTENCE_FILE, keyFile))
    {
        ret = true;
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] success. interface = [%s] enable = [%s]\n",
                __FUNCTION__, interface, enable ? "true" : "false");
    }
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface = [%s] enable = [%s]\n",
                __FUNCTION__, interface, enable ? "true" : "false");
    }
    g_key_file_free (keyFile);
    return ret;
}

// returns:
// true if persisted config for given interface could be removed / does not already exist
// false otherwise
bool netSrvMgrUtiles::removeSavedInterfaceConfig(const char *interface)
{
    LOG_ENTRY_EXIT;

    bool ret = false;
    GKeyFile *keyFile = g_key_file_new ();
    // remove only provided interface/key leaving other keys unaffected

    if (loadKeyFile (INTERFACE_PERSISTENCE_FILE, keyFile) &&
            (g_key_file_has_key (keyFile, "Interface", interface, NULL) == false || // key does not exist; already removed = success
                (g_key_file_remove_key (keyFile, "Interface", interface, NULL) &&
                        writeKeyFile (INTERFACE_PERSISTENCE_FILE, keyFile))))
    {
        ret = true;
        RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s] success. interface = [%s]\n", __FUNCTION__, interface);
    }
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s] failed. interface = [%s]\n", __FUNCTION__, interface);
    }
    g_key_file_free (keyFile);
    return ret;
}

bool netSrvMgrUtiles::getSTBip(char *stbip,bool *isIpv6)
{

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
    bool ret=false;
#ifdef ENABLE_NLMONITOR
    std::vector<std::string> ipAddr;
    char interface[INTERFACE_SIZE]={0};
    char macAddress[MAC_ADDR_BUFF_LEN] = {'\0'};
    if(netSrvMgrUtiles::getRouteInterface(interface))
    {
        std::string ifcStr(interface);
        NetLinkIfc::get_instance()->getIpaddr(ifcStr,AF_INET6,ipAddr);
        if(ipAddr.empty())
        {
            RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] ipv6 address not there for interface %s Trying ipv4 \n", __FUNCTION__, __LINE__,ifcStr.c_str());
            NetLinkIfc::get_instance()->getIpaddr(ifcStr,AF_INET,ipAddr);
            if(ipAddr.empty())
                RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] No ipaddress on interface %s \n", __FUNCTION__, __LINE__,ifcStr.c_str());
            else
            {
                for (auto const& s : ipAddr)
                {
                    if((ipAddr.size()) > 1)
                    {
                        std::string tempStr = s.substr(0, s.find("/", 0));
                        if(chk_ipaddr_linklocal(&tempStr[0],AF_INET))
                        {
                            RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping link local ip \n", __FUNCTION__, __LINE__);
                        }
                        else
                        {
                            *isIpv6=false;
                            strcpy(stbip,tempStr.c_str());
                            ret=true;
                        }
                        RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d] ipv4 address %s on interface %s \n", __FUNCTION__, __LINE__,s.c_str(),ifcStr.c_str());
                    }
                    else
                    {
                        *isIpv6=false;
                        std::string tempStr = s.substr(0, s.find("/", 0));
                        strcpy(stbip,tempStr.c_str());
                        ret=true;
                    }
                }
            }
        }
        else
        {
            netSrvMgrUtiles::getMacAddress_IfName(interface,macAddress);
            std::string macAddrStr(macAddress);
            for (auto const& s : ipAddr)
            {
                if(((ipAddr.size()) > 1) && (netSrvMgrUtiles::check_global_v6_based_macaddress(s,macAddrStr)))
                {
                    RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping ip addr %s since it is based on mac %s \n", __FUNCTION__, __LINE__,s.c_str(),macAddrStr.c_str());
                }
                else
                {
                    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d] ipv6 address %s on interface %s \n", __FUNCTION__, __LINE__,s.c_str(),ifcStr.c_str());
                    *isIpv6=true;
                    std::string tempStr = s.substr(0, s.find("/", 0)); //remove /64 in ip  2601:a40:300:164:aa11:fcff:fefd:1e8d/64
                    strcpy(stbip,tempStr.c_str());
                    ret=true;
                }
            }
        }
    }
    else
    {
        if(netSrvMgrUtiles::currentActiveInterface(interface))
        {
            std::string ifcStr(interface);
            NetLinkIfc::get_instance()->getIpaddr(ifcStr,AF_INET,ipAddr);
            if(ipAddr.empty())
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] No ipaddress on interface %s \n", __FUNCTION__, __LINE__,ifcStr.c_str());
            }
            else
            {
                for (auto const& s : ipAddr)
                {
                    std::string tempStr = s.substr(0, s.find("/", 0));
                    if(chk_ipaddr_linklocal(tempStr.c_str(),AF_INET))
                    {
                        *isIpv6=false;
                        strcpy(stbip,tempStr.c_str());
                        ret=true;
                    }
                }
            }

        }

    }
#else
    RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] ENABLE_NLMONITOR not set \n", __FUNCTION__, __LINE__);
#endif
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return ret;
}

//Check  IP string(IPv4 and IPv6) is link local address
// Check for 169.254 since it will be come as Global scope.

bool netSrvMgrUtiles::chk_ipaddr_linklocal(const char *stbip,unsigned int family)
{
    struct sockaddr_in6 sa6;
    struct sockaddr_in sa;
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
    switch(family)
    {
        case AF_INET:
            inet_pton(AF_INET, stbip, &(sa.sin_addr));
            return IN_IS_ADDR_LINKLOCAL(sa.sin_addr.s_addr);
        case AF_INET6:
            inet_pton(AF_INET6, stbip, &(sa6.sin6_addr));
            return IN6_IS_ADDR_LINKLOCAL(&sa6.sin6_addr);
            break;
        default:
            RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] interface family not supported %d \n", __FUNCTION__, __LINE__,family);
            return false;
    }
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
}

bool netSrvMgrUtiles::currentActiveInterface(char *currentInterface)
{
    std::string ifcStr;
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
    if (getenv("WIFI_INTERFACE") != NULL)
    {
        if (getenv("MOCA_INTERFACE") != NULL)
        {
            ifcStr=getenv("MOCA_INTERFACE");
            if(!checkInterfaceActive((char *)ifcStr.c_str()))
            {
                ifcStr=getenv("WIFI_INTERFACE");
            }
        }
    }
    else if (getenv("MOCA_INTERFACE") != NULL)
    {
        ifcStr=getenv("MOCA_INTERFACE");
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] current interface not there \n", __FUNCTION__, __LINE__);
        return false;
    }
    strcpy(currentInterface,ifcStr.c_str());
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return true;
}


//IPV6 Address based on mac XXXX:XXXX:XXXX:XXXX:ABAA:AAff:feAA:AAAA        XXXX:XXXX:XXXX:XXXX  is the prefix  mac address is AA:AA:AA:AA:AA:AA    B is A XOR 2   Inserted  FF:FE
//global ipv6 based on mac address

bool netSrvMgrUtiles::check_global_v6_based_macaddress(std::string ipv6Addr,std::string macAddr)
{
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
    if((ipv6Addr.empty()) && (macAddr.empty()))
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] ipv6 addr or mac addr is empty \n", __FUNCTION__, __LINE__);
        return false;
    }
    else if (macAddr.size() < 12)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] mac addr is less than 12 \n", __FUNCTION__, __LINE__);
        return false;
    }
    std::string tmpMacAddr(macAddr);//A8:11:FC:FD:1E:8D
    std::transform(tmpMacAddr.begin(), tmpMacAddr.end(), tmpMacAddr.begin(), ::tolower); //a8:11:fc:fd:1e:8d
    tmpMacAddr.erase(std::remove(tmpMacAddr.begin(), tmpMacAddr.end(), ':'), tmpMacAddr.end()); //a811fcfd1e8d
    std::string strTmpAdd2Mac="fffe";
    tmpMacAddr.insert(6,strTmpAdd2Mac);//a811fcfffefd1e8d
    tmpMacAddr.replace(0,2,"");//11fcfffefd1e8d
    std::string tmpIpv6Str(ipv6Addr);//2601:a40:300:164:aa11:fcff:fefd:1e8d
    std::transform(tmpIpv6Str.begin(), tmpIpv6Str.end(), tmpIpv6Str.begin(), ::tolower);//2601:a40:300:164:aa11:fcff:fefd:1e8d
    tmpIpv6Str.erase(std::remove(tmpIpv6Str.begin(), tmpIpv6Str.end(), ':'), tmpIpv6Str.end());//2601a40300164aa11fcfffefd1e8d
    if(tmpIpv6Str.find(tmpMacAddr) != std::string::npos)
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] mac %s based global v6 address %s \n", __FUNCTION__, __LINE__,macAddr.c_str(),ipv6Addr.c_str());
        return true;
    }
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return false;
}

bool netSrvMgrUtiles::getScriptOutput(char *scriptPath,char *scriptOutput)
{
    FILE *file = NULL;
    bool ret=false;
    if(!(file = popen(scriptPath, "r")))
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] script failed %s \n", __FUNCTION__, __LINE__,scriptPath);
        return ret;
    }
    if(fgets(scriptOutput,BUFFER_SIZE_SCRIPT_OUTPUT, file)!=NULL)
    {
        ret=true;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] Failed in getting output from script %s \n", __FUNCTION__, __LINE__,scriptPath);
    }
    pclose(file);
    return ret;
}
