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

#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <ifaddrs.h>

#include "NetworkMgrMain.h"
#include "netsrvmgrUtiles.h"

#define TRIGGER_DHCP_LEASE_FILE "/lib/rdk/triggerDhcpLease.sh"
#define MAX_TIME_LENGTH 32
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



void netSrvMgrUtiles::triggerDhcpLease(void)
{
    static gint retType;
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
    retType=system(TRIGGER_DHCP_LEASE_FILE);
    if(retType == SYSTEM_COMMAND_ERROR)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Error has occured in shell command  \n", __FUNCTION__, __LINE__);
    }
    else if (WEXITSTATUS(retType) == SYSTEM_COMMAND_SHELL_NOT_FOUND)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] That shell command is not found  \n", __FUNCTION__, __LINE__);
    }
    else if (WEXITSTATUS(retType) == SYSTEM_COMMAND_SHELL_SUCESS)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] system call dhcp lease success %d  \n", __FUNCTION__, __LINE__,WEXITSTATUS(retType));
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] unknown error   %d  \n", __FUNCTION__, __LINE__,WEXITSTATUS(retType));
    }
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
}

bool netSrvMgrUtiles::getRouteInterface(char* devname)
{
    char dst[50],gw[50];
    char line[100] , *ptr , *ctr, *sptr;
    int ret;
    FILE *fp;
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
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

		readDevFile(devname);
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] active interface v4 %s  \n", __FUNCTION__, __LINE__,devname);
                return true;
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
		readDevFile(devname);
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] active interface v6 %s  \n", __FUNCTION__, __LINE__,devname);
                return true;
            }
        }
    }
    fclose(fp);
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
    return false;
}

bool netSrvMgrUtiles::readDevFile(char *deviceName)
{
    GError                  *error=NULL;
    gboolean                result = FALSE;
    gchar* devfilebuffer = NULL;
    const gchar* deviceFile = "//etc/device.properties";
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Enter\n", __FUNCTION__, __LINE__);
    if(!deviceName)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] device name is null  \n", __FUNCTION__, __LINE__);
	return result;
    }
    result = g_file_get_contents (deviceFile, &devfilebuffer, NULL, &error);
    if (result == FALSE)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] No contents in device properties  \n", __FUNCTION__, __LINE__);
    }
    else
    {
        /* reset result = FALSE to identify device properties from devicefile contents */
        result = FALSE;
        gchar **tokens = g_strsplit_set(devfilebuffer,",='\n'", -1);
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
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Network Type  %s  \n", __FUNCTION__, __LINE__,deviceName);
    }
    RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR,"[%s:%d]Exit\n", __FUNCTION__, __LINE__);
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
