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
#include "netsrvmgrUtiles.h"


bool mocaLogEnable=true;
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
        rmh=RMH_Initialize(eventCallback, NULL); /* TODO: Need to find the correct location to call RMH_Destroy(rmh); */
        instanceIsReady = true;
    }
    return instance;
}


int  MocaNetworkMgr::Start()
{
    bool retVal=false;
    LOG_INFO("Enter");
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG, _mocaEventHandler);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION, _mocaEventHandler);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogStatus, mocaTelemetryLogEnable);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogDuration, mocaTelemetryLogDuration);
    RMH_SetEventCallbacks(rmh, RMH_EVENT_LINK_STATUS_CHANGED | RMH_EVENT_MOCA_VERSION_CHANGED);
    startMocaTelemetry();
    return 0;
}

void *mocaTelemetryThrd(void* arg)
{
    LOG_ENTRY_EXIT;
    int ret = 0;

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
    LOG_ENTRY_EXIT;
    RMH_LinkStatus status;
    unsigned int ncNodeID;
    uint8_t mac[6];
    char macStr[32];
    RMH_MoCAVersion mocaVersion;
    unsigned int totalMoCANode;
    unsigned int count=0;
    unsigned int selfNodeId;
    unsigned int i;
    static char bFirst=1;
    RMH_NodeList_Uint32_t nodeList;
    GString *resString=g_string_new(NULL);

    if (RMH_Self_GetLinkStatus(rmh, &status) != RMH_SUCCESS) {
        LOG_ERR("Failed calling RMH_Self_GetLinkStatus!");
    }
    else if(status == RMH_LINK_STATUS_UP)
    {
        LOG_INFO("TELEMETRY_MOCA_STATUS:UP");
        if(bFirst)
        {
            LOG_INFO("TELEMETRY_MOCA_LINK_STATUS_CHANGED:%d",status);
            bFirst=0;
            if (RMH_Network_GetMoCAVersion(rmh,&mocaVersion) != RMH_SUCCESS)
            {
                LOG_ERR("Failed calling RMH_Network_GetMoCAVersion!");
            }
            else {
                LOG_INFO("TELEMETRY_MOCA_VERSION_CHANGED:%s", RMH_MoCAVersionToString(mocaVersion));
            }
        }
            if (RMH_Network_GetNCNodeId(rmh, &ncNodeID) != RMH_SUCCESS) {
                LOG_ERR("Failed calling RMH_Network_GetNCNodeId!");
            }
            else {
                LOG_INFO("TELEMETRY_MOCA_NC_NODEID:%d",ncNodeID);
            }
        if (RMH_Network_GetNCMac(rmh, &mac) != RMH_SUCCESS) {
            LOG_ERR("Failed calling RMH_Network_GetNCMacString!");
        }
        else {
            LOG_INFO("TELEMETRY_MOCA_NC_MAC:%s", RMH_MacToString(mac, macStr, sizeof(macStr)/sizeof(macStr[0])));
        }

        if (RMH_Network_GetNumNodes(rmh, &totalMoCANode) != RMH_SUCCESS) {
            LOG_ERR("Failed calling RMH_Network_GetNumNodes!");
        }
        else {
            LOG_INFO("TELEMETRY_MOCA_TOTAL_NODE:%d",totalMoCANode);
        }

        if (RMH_Network_GetNodeId(rmh, &selfNodeId) != RMH_SUCCESS) {
            LOG_ERR("Failed calling RMH_Network_GetNodeId!");
        } else if (RMH_Network_GetAssociatedIds(rmh, &nodeList) != RMH_SUCCESS) {
            LOG_ERR("Failed calling RMH_Network_GetAssociatedIds!");
        } else {
            g_string_assign(resString,"");
            for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
                if (nodeList.nodePresent[i]) {
                    uint32_t phyRate;
                    if (RMH_RemoteNode_GetRxUnicastPhyRate(rmh, i, &phyRate) != RMH_SUCCESS) {
                        LOG_ERR("Failed calling RemoteNode_GetRxUnicastPhyRate!");
                    }
                    else {
                        LOG_INFO("TELEMETRY_MOCA_PHYRXRATE_%d_%d:%d", selfNodeId, i, phyRate);
                        g_string_append_printf(resString,"%d,",phyRate);
                    }
                }
            }
            if (resString->len > 0) {
                g_string_truncate(resString, resString->len-1); /*Remove last comma */
                LOG_INFO("TELEMETRY_MOCA_PHY_RX_RATE:%s", resString->str);
                g_string_assign(resString,"");
            }

            for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
                if (nodeList.nodePresent[i]) {
                    uint32_t phyRate;
                    if (RMH_RemoteNode_GetTxUnicastPhyRate(rmh, i, &phyRate) != RMH_SUCCESS) {
                        LOG_ERR("Failed calling RMH_RemoteNode_GetTxUnicastPhyRate!");
                    }
                    else {
                        LOG_INFO("TELEMETRY_MOCA_PHYTXRATE_%d_%d:%d", selfNodeId, i, phyRate);
                        g_string_append_printf(resString,"%d,",phyRate);
                    }
                }
            }
            if (resString->len > 0) {
                g_string_truncate(resString, resString->len-1); /*Remove last comma */
                LOG_INFO("TELEMETRY_MOCA_PHY_TX_RATE:%s", resString->str);
                g_string_assign(resString,"");
            }

            for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
                if (nodeList.nodePresent[i]) {
                    float power;
                    if (RMH_RemoteNode_GetRxUnicastPower(rmh, i, &power) != RMH_SUCCESS) {
                        LOG_ERR("Failed calling RemoteNode_GetRxUnicastPower!");
                    }
                    else {
                        LOG_INFO("TELEMETRY_MOCA_RXPOWER_%d_%d:%2.02f", selfNodeId, i, power);
                        g_string_append_printf(resString,"%2.02f,",power);
                    }
                }
            }
            if (resString->len > 0) {
                g_string_truncate(resString, resString->len-1); /*Remove last comma */
                LOG_INFO("TELEMETRY_MOCA_RX_POWER:%s", resString->str);
                g_string_assign(resString,"");
            }

            g_string_assign(resString,"");
            for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
                if (nodeList.nodePresent[i]) {
                    uint32_t powerReduction;
                    if (RMH_RemoteNode_GetTxPowerReduction(rmh, i, &powerReduction) != RMH_SUCCESS) {
                        LOG_ERR("Failed calling RMH_RemoteNode_GetTxPowerReduction!");
                    }
                    else {
                        LOG_INFO("TELEMETRY_MOCA_TXPOWERREDUCTION_%d_%d:%d", selfNodeId, i, powerReduction);
                        g_string_append_printf(resString,"%d,",powerReduction);
                    }
                }
            }
            if (resString->len > 0) {
                g_string_truncate(resString, resString->len-1); /*Remove last comma */
                LOG_INFO("TELEMETRY_MOCA_TX_POWER_REDUCTION:%s", resString->str);
                g_string_assign(resString,"");
            }

        }
        if (selfNodeId == ncNodeID )
        {
            LOG_INFO("TELEMETRY_MOCA_IS_CURRENT_NODE_NC:1");
        }
        else
        {
            LOG_INFO("TELEMETRY_MOCA_IS_CURRENT_NODE_NC:0");
        }

    }
    else
    {
        LOG_INFO("TELEMETRY_MOCA_STATUS:DOWN");

    }
    g_string_free(resString,TRUE);
}

static void _mocaEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    LOG_ENTRY_EXIT;
    if (strcmp(owner, IARM_BUS_NM_SRV_MGR_NAME)  == 0) {
        switch (eventId) {
        case IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG:
        {
            bool *param = (bool *)data;
            mocaLogEnable=*param;
            LOG_INFO("got event IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG %d", mocaLogEnable);
        }
        break;
        case IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION:
        {
            unsigned int *param = (unsigned int *)data;
            mocaLogDuration=*param;
            LOG_INFO("got event IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION %d", mocaLogDuration);
        }
        break;
        default:
            break;
        }
    }
}

IARM_Result_t MocaNetworkMgr::mocaTelemetryLogEnable(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool *param = (bool *)arg;
    *param=mocaLogEnable;
    LOG_INFO("get event to get status of telemetry log enable %d", mocaLogEnable);
    return ret;
}

IARM_Result_t MocaNetworkMgr::mocaTelemetryLogDuration(void *arg)
{
    LOG_ENTRY_EXIT;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    unsigned int *param = (unsigned int *)arg;
    *param=mocaLogDuration;
    LOG_INFO("get venent to get the moca log duration set %d", mocaLogDuration);
    return ret;
}

static void eventCallback(const enum RMH_Event event, const struct RMH_EventData *eventData, void* userContext) {
    switch(event) {
    case RMH_EVENT_LINK_STATUS_CHANGED:
        LOG_INFO("MoCA Link status changed to %d", eventData->RMH_EVENT_LINK_STATUS_CHANGED.status );
        LOG_INFO("TELEMETRY_MOCA_LINK_STATUS_CHANGED:%d", eventData->RMH_EVENT_LINK_STATUS_CHANGED.status);
        break;
    case RMH_EVENT_MOCA_VERSION_CHANGED:
        LOG_INFO("MoCA version changed to %s", RMH_MoCAVersionToString(eventData->RMH_EVENT_MOCA_VERSION_CHANGED.version));
        LOG_INFO("TELEMETRY_MOCA_VERSION_CHANGED:%s", RMH_MoCAVersionToString(eventData->RMH_EVENT_MOCA_VERSION_CHANGED.version));
        break;
    default:
        LOG_ERR("MoCA Event Not Supported");
        break;
    }
}
