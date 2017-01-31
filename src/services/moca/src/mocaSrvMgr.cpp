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

#include <iostream>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <glib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include "mocaSrvMgr.h"
#include "netsrvmgrIarm.h"
#include "NetworkMgrMain.h"
#include "NetworkMedium.h"


bool mocaLogEnable=false;
unsigned int mocaLogDuration=3600;


MocaNetworkMgr* MocaNetworkMgr::instance = NULL;
bool MocaNetworkMgr::instanceIsReady = false;
static RMH_Handle rmh = NULL;


MocaNetworkMgr::MocaNetworkMgr() {}
MocaNetworkMgr::~MocaNetworkMgr() { }

MocaNetworkMgr* MocaNetworkMgr::getInstance()
{
    if (instance == NULL)
    {
        instance = new MocaNetworkMgr();
        rmh=RMH_Initialize(); /* TODO: Need to find the correct location to call RMH_Destroy(rmh); */
        instanceIsReady = true;
    }
    return instance;
}


int  MocaNetworkMgr::Start()
{
    bool retVal=false;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG, _mocaEventHandler);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION, _mocaEventHandler);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogStatus, mocaTelemetryLogEnable);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogDuration, mocaTelemetryLogDuration);
    RMH_Callback_RegisterEvent(rmh, eventCallback, NULL, LINK_STATUS_CHANGED | MOCA_VERSION_CHANGED);
    startMocaTelemetry();
}

void *mocaTelemetryThrd(void* arg)
{
    int ret = 0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );

    MocaNetworkMgr::printMocaTelemetry();
    while (true) {
        if(mocaLogEnable)
        {
            MocaNetworkMgr::printMocaTelemetry();
            sleep(mocaLogDuration);
        }
        else
        {
            sleep(300);
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}

void startMocaTelemetry()
{
    pthread_t mocaTelemetryThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&mocaTelemetryThread, &attr, &mocaTelemetryThrd, NULL);
}


void MocaNetworkMgr::printMocaTelemetry()
{
    RMH_LinkStatus status;
    unsigned int ncNodeID;
    char mac[32];
    RMH_MoCAVersion mocaVersion;
    unsigned int totalMoCANode;
    unsigned int count=0;
    GString *phyRate=g_string_new(NULL);
    GString *mocaPower=g_string_new(NULL);
    RMH_NetworkStatus txNetworkStatus, rxNetworkStatus;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );
    if (RMH_Network_GetLinkStatus(rmh, &status) != RMH_SUCCESS) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]Failed calling RMH_Network_GetLinkStatus!\n",__FUNCTION__, __LINE__ );
    }
    else if(status == RMH_INTERFACE_UP)
    {
	RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_MOCA_LINK_STATUS_CHANGED:%d \n",status);
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_STATUS:UP\n");
        if (RMH_Self_GetHighestSupportedMoCAVersion(rmh,&mocaVersion) != RMH_SUCCESS)
	{
	   RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]Failed calling RMH_Self_GetHighestSupportedMoCAVersion!\n",__FUNCTION__, __LINE__ );
	}
	else {
	   RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_MOCA_VERSION_CHANGED:%s \n",RMH_MoCAVersionToString(mocaVersion));
	}
        if (RMH_Network_GetNCNodeId(rmh, &ncNodeID) != RMH_SUCCESS) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]Failed calling RMH_Network_GetNCNodeId!\n",__FUNCTION__, __LINE__ );
        }
        else {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_NC_NODEID:%d\n",ncNodeID);
        }

        if (RMH_Network_GetNCMacString(rmh, mac, sizeof(mac)) != RMH_SUCCESS) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]Failed calling RMH_Network_GetNCMacString!\n",__FUNCTION__, __LINE__ );
        }
        else {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_NC_MAC:%s\n",mac);
        }

        if (RMH_Network_GetNumNodes(rmh, &totalMoCANode) != RMH_SUCCESS) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]Failed calling RMH_Network_GetNumNodes!\n",__FUNCTION__, __LINE__ );
        }
        else {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_TOTAL_NODE:%d\n",totalMoCANode);
        }

        if (RMH_Network_GetStatusRx(rmh, &rxNetworkStatus) != RMH_SUCCESS) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]Failed calling RMH_Network_GetStatusRx!\n",__FUNCTION__, __LINE__ );
        }
        else if (RMH_Network_GetStatusTx(rmh, &txNetworkStatus) != RMH_SUCCESS) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d]Failed calling RMH_Network_GetStatusTx!\n",__FUNCTION__, __LINE__ );
        }
        else {
            while(count < rxNetworkStatus.remoteNodesPresent)
            {
                RMH_RemoteNodeStatus *status=&rxNetworkStatus.remoteNodes[count];
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_PHYRXRATE_%d_%d:%d\n", rxNetworkStatus.localNodeId, status->nodeId, status->phyRate);
                g_string_append_printf(phyRate,"%d",status->phyRate);
                count ++;
                if(count < rxNetworkStatus.remoteNodesPresent)
                {
                    g_string_append_c(phyRate,',');
                }
                else
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_PHY_RX_RATE:%s\n",phyRate->str);
                }
            }
            count=0;
            g_string_assign(phyRate,"");
            while(count < txNetworkStatus.remoteNodesPresent)
            {
                RMH_RemoteNodeStatus *status=&txNetworkStatus.remoteNodes[count];
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_PHYTXRATE_%d_%d:%d\n", txNetworkStatus.localNodeId, status->nodeId, status->phyRate);
                g_string_append_printf(phyRate,"%d",status->phyRate);
                count ++;
                if(count < txNetworkStatus.remoteNodesPresent)
                {
                    g_string_append_c(phyRate,',');
                }
                else
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_PHY_TX_RATE:%s\n",phyRate->str);
                }
            }
            count=0;
            while(count < rxNetworkStatus.remoteNodesPresent)
            {
                RMH_RemoteNodeStatus *status=&rxNetworkStatus.remoteNodes[count];
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_RXPOWER_%d_%d:%d\n", rxNetworkStatus.localNodeId, status->nodeId, status->power);
                g_string_append_printf(mocaPower,"%d",status->power);
                count ++;
                if(count < rxNetworkStatus.remoteNodesPresent)
                {
                    g_string_append_c(mocaPower,',');
                }
                else
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_RX_POWER:%s\n",mocaPower->str);
                }
            }
            count=0;
            g_string_assign(mocaPower,"");
            while(count < txNetworkStatus.remoteNodesPresent)
            {
                RMH_RemoteNodeStatus *status=&txNetworkStatus.remoteNodes[count];
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_TXPOWERREDUCTION_%d_%d:%d\n", txNetworkStatus.localNodeId, status->nodeId, status->backoff);
                g_string_append_printf(mocaPower,"%d",status->backoff);
                count ++;
                if(count < txNetworkStatus.remoteNodesPresent)
                {
                    g_string_append_c(mocaPower,',');
                }
                else
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_TX_POWER_REDUCTION:%s\n",mocaPower->str);
                }
            }
        }
        if (rxNetworkStatus.localNodeId == ncNodeID )
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_IS_CURRENT_NODE_NC:1\n");
        }
        else
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_IS_CURRENT_NODE_NC:0\n");
        }

    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_MOCA_STATUS:DOWN \n");

    }
    g_string_free(phyRate,TRUE);
    g_string_free(mocaPower,TRUE);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}

static void _mocaEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );
    if (strcmp(owner, IARM_BUS_NM_SRV_MGR_NAME)  == 0) {
        switch (eventId) {
        case IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG:
        {
            bool *param = (bool *)data;
            mocaLogEnable=*param;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] got event IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG %d \n",__FUNCTION__, __LINE__,mocaLogEnable);
        }
        break;
        case IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION:
        {
            unsigned int *param = (unsigned int *)data;
            mocaLogDuration=*param;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] got event IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION %d \n",__FUNCTION__, __LINE__,mocaLogDuration);
        }
        break;
        default:
            break;
        }
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}

IARM_Result_t MocaNetworkMgr::mocaTelemetryLogEnable(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );
    bool *param = (bool *)arg;
    *param=mocaLogEnable;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] get event to get status of telemetry log enable %d \n",__FUNCTION__, __LINE__,mocaLogEnable);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t MocaNetworkMgr::mocaTelemetryLogDuration(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    unsigned int *param = (unsigned int *)arg;
    *param=mocaLogDuration;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] get venent to get the moca log duration set %d \n",__FUNCTION__, __LINE__,mocaLogDuration);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}

static void eventCallback(enum RMH_Event event, struct RMH_EventData *eventData, void* userContext){
    switch(event) {
    case LINK_STATUS_CHANGED:
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] MoCA Link status changed to %d \n",__FUNCTION__, __LINE__,eventData->LINK_STATUS_CHANGED.status );
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_MOCA_LINK_STATUS_CHANGED:%d \n",eventData->LINK_STATUS_CHANGED.status);
        break;
    case MOCA_VERSION_CHANGED:
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] MoCA version changed to %s \n",__FUNCTION__, __LINE__,RMH_MoCAVersionToString(eventData->MOCA_VERSION_CHANGED.version));
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_MOCA_VERSION_CHANGED:%s \n",RMH_MoCAVersionToString(eventData->MOCA_VERSION_CHANGED.version));
        break;
    default:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] MoCA Event Not Supported \n",__FUNCTION__, __LINE__ );
        break;
    }
}
