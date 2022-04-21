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
#include "netsrvmgrIarm.h"
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
#ifdef SAFEC_RDKV
#include "safec_lib.h"
#else
#define STRCPY_S(dest,size,source)                    \
        strcpy(dest, source);
#endif

EntryExitLogger::EntryExitLogger (const char* func, const char* file) :
        func (func), file (file)
{
    RDK_LOG (RDK_LOG_TRACE1, LOG_NMGR, "Entry: %s [%s]\n", func, file);
}

EntryExitLogger::~EntryExitLogger ()
{
    RDK_LOG (RDK_LOG_TRACE1, LOG_NMGR, "Exit: %s [%s]\n", func, file);
}

#ifdef USE_TELEMETRY_2_0

void telemetry_init(char* name)
{
    t2_init(name);
}

void telemetry_event_s(char* marker, char* value)
{
    T2ERROR t2error = t2_event_s(marker, value);
    if (t2error != T2ERROR_SUCCESS)
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "t2_event_s(\"%s\", \"%s\") returned error code %d \n", marker, value, t2error);
}

void telemetry_event_d(char* marker, int value)
{
    T2ERROR t2error = t2_event_d(marker, value);
    if (t2error != T2ERROR_SUCCESS)
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "t2_event_d(\"%s\", %d) returned error code %d \n", marker, value, t2error);
}

#endif // #ifdef USE_TELEMETRY_2_0

static bool loadKeyFile (const char* filename, GKeyFile* keyFile);
static bool writeKeyFile (const char* filename, GKeyFile* keyFile);

using namespace netSrvMgrUtiles;

/**
 * @fn netSrvMgrUtiles::getMacAddress_IfName(const char *ifName_in, char *macAddress_out)
 * @brief This function gets the MAC address of the AspectRatio against the id specified, only if
 * the id passed is valid.
 *
 * @param[in] ifName_in Indicates the interface name which the mac address is required.
 * @param[out] macAddress_out Indicates the mac address of ifname_in interface name.
 *
 * @return Returns true if successfully gets the mac address of interface provided or else false.
 */
bool netSrvMgrUtiles::getMacAddress_IfName(const char *ifName_in, char macAddress_out[MAC_ADDR_BUFF_LEN])
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

/* End of doxygen group */
/**
 * @}
 */
/* End of file netSrvMgrUtiles.cpp */

/** @} */



/**
 * @fn netSrvMgrUtiles::triggerDhcpLease(const char* operation, const char* interface)
 * @brief Triggers a "renew" or "release and renew" of DHCP lease on specified interface.
 * If DHCP lease renew is requested, the specified interface may be NULL in which case
 * a DHCP lease renew will be triggered for all interfaces running the DHCPv4 client.
 * If DHCP lease release and renew is requested, the specified interface must not be NULL.
 *
 * @param[in] operation DHCP lease operation to perform ("renew" or "release and renew")
 * @param[in] interface interface (wlan0, eth0, etc.) to perform DHCP lease operation on
 */
void netSrvMgrUtiles::triggerDhcpLease(const char* operation, const char* interface)
{
    char command[200];
    snprintf (command, sizeof(command), "%s %s %s 2>&1",
            TRIGGER_DHCP_LEASE_FILE, operation, interface ? interface : "");

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

void netSrvMgrUtiles::triggerDhcpRenew(const char* interface)
{
    netSrvMgrUtiles::triggerDhcpLease("renew", interface);
}

void netSrvMgrUtiles::triggerDhcpReleaseAndRenew(const char* interface)
{
    netSrvMgrUtiles::triggerDhcpLease("release_and_renew", interface);
}

static bool getIPv6RouteInterface (char *devname)
{
    bool route_found = false;
    FILE *fp = fopen ("/proc/net/ipv6_route", "r");
    if (fp != NULL)
    {
        char dst[50], gw[50];
        int ret;
        while ((ret = fscanf (fp, "%s %*x %*s %*x %s %*x %*x %*x %*x %s", dst, gw, devname)) != EOF)
        {
            /*return value must be parameter's number*/
            if (ret != 3)
            {
                continue;
            }
            if ((strcmp (dst, gw) == 0) && (strcmp (devname, "sit0") != 0) && (strcmp (devname, "lo") != 0))
            {
//                readDevFile(devname);
                RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%d] active interface v6 %s  \n", __FUNCTION__, __LINE__, devname);
                route_found = true;
                break;
            }
        }
        fclose (fp);
    }
    return route_found;
}

static bool getIPv4RouteInterface (char *devname)
{
    bool route_found = false;
    FILE *fp = fopen ("/proc/net/route", "r");
    if (fp != NULL)
    {
        char line[100], *ptr, *ctr, *sptr;
        while (fgets (line, sizeof (line), fp))
        {
            ptr = strtok_r (line, " \t", &sptr);
            ctr = strtok_r (NULL, " \t", &sptr);
            if (ptr != NULL && ctr != NULL && strcmp (ctr, "00000000") == 0)
            {
                STRCPY_S(devname, INTERFACE_SIZE, ptr);
//                readDevFile(devname);
                RDK_LOG (RDK_LOG_INFO, LOG_NMGR, "[%s:%d] active interface v4 %s  \n", __FUNCTION__, __LINE__, devname);
                route_found = true;
                break;
            }
        }
        fclose (fp);
    }
    return route_found;
}

bool netSrvMgrUtiles::getRouteInterface(char* devname)
{
    LOG_ENTRY_EXIT;
    if (getIPv6RouteInterface (devname) || getIPv4RouteInterface (devname))
        return true;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] No Route Found\n", __FUNCTION__, __LINE__);
    return false;
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
                    STRCPY_S(deviceName, INTERFACE_SIZE, "ETHERNET");
                    result=TRUE;
                    break;
                }
            }
            else if (g_strrstr(g_strstrip(tokens[loopvar]), "MOCA_INTERFACE") && ((g_strcmp0(g_strstrip(tokens[loopvar+1]),deviceName)) == 0))
            {
                if ((loopvar+1) < toklength )
                {
                    STRCPY_S(deviceName, INTERFACE_SIZE, "MOCA");
                    result=TRUE;
                    break;
                }
            }
            else if (g_strrstr(g_strstrip(tokens[loopvar]), "WIFI_INTERFACE") && ((g_strcmp0(g_strstrip(tokens[loopvar+1]),deviceName)) == 0))
            {
                if ((loopvar+1) < toklength )
                {
                    STRCPY_S(deviceName, INTERFACE_SIZE, "WIFI");
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
    STRCPY_S(devAllInterface, INTERFACE_LIST, device->str);
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
    LOG_ENTRY_EXIT;
    FILE * file;
    char buffer[INTERFACE_STATUS_FILE_PATH_BUFFER]={0};
    bool ret=false;
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

bool netSrvMgrUtiles::getInterfaceConfig(const char *ifName, const unsigned int family, char *interfaceIp, char *netMask)
{
    LOG_ENTRY_EXIT;
#ifdef ENABLE_NLMONITOR
    char macAddress[MAC_ADDR_BUFF_LEN] = {'\0'};
    std::vector<std::string> ipAddr;

    /*to get IP address based on interface and family input parameter */
    NetLinkIfc::get_instance()->getIpaddr(ifName,family,ipAddr);
    if(ipAddr.empty())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] No ipaddress on interface %s\n", __FUNCTION__, __LINE__,ifName);
        return false;
    }
    else
    {
       /*to get MAC address based on interface*/
       netSrvMgrUtiles::getMacAddress_IfName(ifName,macAddress);
       std::string macAddrStr(macAddress);

       for (auto const& s : ipAddr)
       {
          if(netSrvMgrUtiles::check_global_v6_ula_address(s))
          {
             RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping ULA V6 IP. \n", __FUNCTION__, __LINE__);
             continue;
          }
          if(((ipAddr.size()) > 1) && (netSrvMgrUtiles::check_global_v6_based_macaddress(s,macAddrStr)))
          {
             RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping ip addr %s since it is based on mac %s \n", __FUNCTION__, __LINE__,s.c_str(),macAddrStr.c_str());
             continue;
          }
          std::string tempStr = s.substr(0, s.find("/", 0)); //remove /64 in ip  2601:a40:300:164:aa11:fcff:fefd:1e8d/64
          if (chk_ipaddr_linklocal(tempStr.c_str(),family))
          {
             RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping link local ip \n", __FUNCTION__, __LINE__);
             continue;
          }
          if (family == AF_INET && isIPv4AddressScopeDocumentation(tempStr))
          {
             RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping reserved ip %s\n", __FUNCTION__, __LINE__, tempStr.c_str());
             continue;
          }
          STRCPY_S(interfaceIp, INET6_ADDRSTRLEN, tempStr.c_str());
       }
    }
    /* To get Net Mask */
    netSrvMgrUtiles::getNetMask_IfName(ifName,family,netMask);
#else
    RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] ENABLE_NLMONITOR not set \n", __FUNCTION__, __LINE__);
#endif
    return true;
}

bool netSrvMgrUtiles::getSTBip(char *stbip,bool *isIpv6)
{

    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);

    bool ret=false;
    *isIpv6 = false;

    if (getSTBip_family(stbip,"ipv6"))
    {
      ret = true;
      *isIpv6 = true;
    }
    else if (getSTBip_family(stbip,"ipv4"))
    {
      ret = true;
    }
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return ret;
}

bool netSrvMgrUtiles::getSTBip_family(char *stbip,char *family)
{
   RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
   bool ret=false;
#ifdef ENABLE_NLMONITOR
   std::vector<std::string> ipAddr;
   char interface[INTERFACE_SIZE]={0};
   char macAddress[MAC_ADDR_BUFF_LEN] = {'\0'};
   std::string s_family(family);
   int i_family = 0;

   //1. Filter for family.
   if (s_family == "ipv4")
   {
      i_family = AF_INET;
   }
   else if (s_family == "ipv6")
   {
      i_family = AF_INET6;
   }
   else
   {
      RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] Family [%s] Not recognized.\n", __FUNCTION__, __LINE__,s_family.c_str());
      return ret;
   }

   //2. Extract interface.
   if(!netSrvMgrUtiles::getRouteInterface(interface))
   {
      if(!netSrvMgrUtiles::currentActiveInterface(interface))
      {
         RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] No routable  interface found.\n", __FUNCTION__, __LINE__);
	 return ret;
      }
   }

   std::string ifcStr(interface);
   NetLinkIfc::get_instance()->getIpaddr(ifcStr,i_family,ipAddr);
   if(ipAddr.empty())
   {
      RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] No ipaddress on interface %s, for Family  %s\n", __FUNCTION__, __LINE__,ifcStr.c_str(),s_family.c_str());
      return ret;
   }

   netSrvMgrUtiles::getMacAddress_IfName(interface,macAddress);
   std::string macAddrStr(macAddress);

   for (auto const& s : ipAddr)
   {
      if(netSrvMgrUtiles::check_global_v6_ula_address(s))
      {
         RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping ULA V6 IP. \n", __FUNCTION__, __LINE__);
         continue;
      }
      if(((ipAddr.size()) > 1) && (netSrvMgrUtiles::check_global_v6_based_macaddress(s,macAddrStr)))
      {
         RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping ip addr %s since it is based on mac %s \n", __FUNCTION__, __LINE__,s.c_str(),macAddrStr.c_str());
         continue;
      }
      std::string tempStr = s.substr(0, s.find("/", 0)); //remove /64 in ip  2601:a40:300:164:aa11:fcff:fefd:1e8d/64
      if (chk_ipaddr_linklocal(tempStr.c_str(),i_family))
      {
         RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping link local ip \n", __FUNCTION__, __LINE__);
	 continue;
      }
      if (i_family == AF_INET && isIPv4AddressScopeDocumentation(tempStr))
      {
         RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] skipping reserved ip %s\n", __FUNCTION__, __LINE__, tempStr.c_str());
         continue;
      }
      STRCPY_S(stbip, MAX_IP_ADDRESS_LEN, tempStr.c_str());
      ret=true;
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
    LOG_ENTRY_EXIT;
    struct sockaddr_in6 sa6;
    struct sockaddr_in sa;
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
}

bool netSrvMgrUtiles::currentActiveInterface(char *currentInterface)
{
    LOG_ENTRY_EXIT;
    std::string ifcStr;
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
    STRCPY_S(currentInterface, INTERFACE_SIZE, ifcStr.c_str());
    return true;
}


//IPV6 Address based on mac XXXX:XXXX:XXXX:XXXX:ABAA:AAff:feAA:AAAA        XXXX:XXXX:XXXX:XXXX  is the prefix  mac address is AA:AA:AA:AA:AA:AA    B is A XOR 2   Inserted  FF:FE
//global ipv6 based on mac address

bool netSrvMgrUtiles::check_global_v6_based_macaddress(std::string ipv6Addr,std::string macAddr)
{
    LOG_ENTRY_EXIT;
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
    return false;
}

//Check if v6 address is ULA; If the IPv6 address begins with fd or fc, it is a ULA
bool netSrvMgrUtiles::check_global_v6_ula_address(std::string ipv6Addr)
{
    LOG_ENTRY_EXIT;
    if(ipv6Addr.empty())
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,"[%s:%d] ipv6 addr is empty \n", __FUNCTION__, __LINE__);
        return false;
    }

    std::string tmpIpv6Str(ipv6Addr);
    if ((tmpIpv6Str.rfind("fd", 0) != std::string::npos) || (tmpIpv6Str.rfind("fc", 0) != std::string::npos))
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] ULA v6 address %s \n", __FUNCTION__, __LINE__,ipv6Addr.c_str());
        return true;
    }
    return false;
}

bool netSrvMgrUtiles::getCommandOutput(const char *command, char *output_buffer, size_t output_buffer_size)
{
    bool ret = false;
    FILE *file = popen(command, "r");
    if (!file)
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] command failed %s \n", __FUNCTION__, __LINE__, command);
        return ret;
    }
    if (fgets(output_buffer, output_buffer_size, file) != NULL)
    {
        ret = true;
    }
    else
    {
        RDK_LOG (RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed in getting output from command %s \n", __FUNCTION__, __LINE__, command);
    }
    pclose(file);
    return ret;
}
bool netSrvMgrUtiles::getNetMask_IfName(const char *ifName_in,const unsigned int family, char *netMask_out)
{
    LOG_ENTRY_EXIT;
    struct ifreq ifr;
    struct sockaddr_in *ipaddr;
    bool ret = false;
    int sock;
    char address[INET6_ADDRSTRLEN]={0};

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock > 0)
    {
        ifr.ifr_addr.sa_family = family;

        strncpy(ifr.ifr_name, ifName_in, IFNAMSIZ-1);

        /* If the device uses virtual interface, then get the subnet mask of the virtual interface */
        if (!system("/lib/rdk/pni_controller.sh uses_virtual_interface"))
        {
            strcat(ifr.ifr_name, ":0");
        }

        /* To get Net Mask */
        if (ioctl(sock, SIOCGIFNETMASK, &ifr) != -1)
        {
            ipaddr = (struct sockaddr_in *)&ifr.ifr_netmask;
            if (inet_ntop(family, &ipaddr->sin_addr, address, sizeof(address)) != NULL)
            {
                STRCPY_S(netMask_out, INET6_ADDRSTRLEN, address);
                RDK_LOG(RDK_LOG_INFO, LOG_NMGR,"[%s:%d] Netmask %s \n", __FUNCTION__, __LINE__,netMask_out);
                ret = true;
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] Failed in ioctl() with  \'%s\'..\n", __FUNCTION__, __LINE__, strerror (errno) );
        }
        close(sock);
     }
     else
     {
         RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%d] Failed to create socket() with \'%s\' return as \'%d\'.\n", __FUNCTION__, __LINE__, sock, strerror (errno) );
     }
    return ret;
}

bool netSrvMgrUtiles::isIPv4AddressScopeDocumentation(const std::string& ipv4_address)
{
    struct sockaddr_in sa;
    inet_pton(AF_INET, ipv4_address.c_str(), &(sa.sin_addr));

    // IPv4 unicast address blocks reserved for documentation are all /24 networks (RFC 5737)
    // so extract the network part of the address by bitwise-ANDing with 255.255.255.0
    in_addr_t network_address = sa.sin_addr.s_addr & htonl(0xffffff00);

    return (network_address == htonl (0xc0000200) || // 192.0.2.0
            network_address == htonl (0xc6336400) || // 198.51.100.0
            network_address == htonl (0xcb007100));  // 203.0.113.0
}
