##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
SUBDIRS =
DIST_SUBDIRS =

AM_CXXFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)  -I$(top_srcdir)/src/main/include -I$(top_srcdir)/src/services/wifi/include -I$(top_srcdir)/../../xupnp/generic/include $(GLIB_CFLAGS) $(G_THREAD_CFLAGS)

if RDKWIFIHAL
AM_CXXFLAGS += -DUSE_RDK_WIFI_HAL
endif
if LOSTFOUND
AM_CXXFLAGS += -DENABLE_LOST_FOUND -I$(RDK_SOURCE_PATH)/../opensource/include -I$(RDK_SOURCE_PATH)/../opensource/include/cjson
endif

if WIFI_CLIENT_ROAMING
AM_CXXFLAGS += -DWIFI_CLIENT_ROAMING
endif

AM_CXXFLAGS += $(IARM_CFLAGS)  $(NLMONITOR_CFLAGS) $(NL_CFLAGS)

AM_LDFLAGS = $(IARM_LFLAGS)  $(NLMONITOR_LINK)

if XCAM_SUPPORT
AM_CXXFLAGS += -DENABLE_XCAM_SUPPORT -I$(RDK_FSROOT_PATH)/usr/include -I$(RDK_SOURCE_PATH)/../utility/sysUtils/include/
AM_LDFLAGS += -lrdkc -lcgicomm -lcgilib -lsccomm -lmxml -lasound -L$(RDK_SOURCE_PATH)/../utility/sysUtils/src/ -lsysutils
endif

if XHB1
AM_CXXFLAGS += -DXHB1 -I$(RDK_PROJECT_ROOT_PATH)/mfrlibs/include -I$(RDK_SOURCE_PATH)/../utility/sysUtils/include/
AM_LDFLAGS += -lmfrApi -lcgicomm -lsccomm -lmxml -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/ramdisk/amba/prebuild/third-party/armv8-a/alsa-lib/usr/lib/ -lasound -L$(RDK_SOURCE_PATH)/../utility/sysUtils/src/ -lsysutils
endif

if XHC3
AM_CXXFLAGS += -DXHC3 -I$(RDK_PROJECT_ROOT_PATH)/mfrlibs/include -I$(RDK_SOURCE_PATH)/../utility/sysUtils/include/
AM_LDFLAGS += -lmfrApi -lcgicomm -lsccomm -lmxml -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/ramdisk/amba/prebuild/third-party/armv7-a-hf/alsa-lib/usr/lib/ -lasound -L$(RDK_SOURCE_PATH)/../utility/sysUtils/src/ -lsysutils
endif

if RTMESSAGE
AM_CXXFLAGS += -DENABLE_RTMESSAGE -std=c++11 -I$(RDK_SOURCE_PATH)/../opensource/src/rtmessage -I$(RDK_SOURCE_PATH)/../opensource/src/rtmessage/dataProvider
AM_LDFLAGS += -L$(RDK_SOURCE_PATH)/../opensource/lib/ -lrtMessage -ldataProvider
endif

if RDKWIFIHAL
AM_LDFLAGS += -lwifihal
endif
if LOSTFOUND
AM_LDFLAGS += -llaf
endif

if TELEMETRY_2_0
AM_LDFLAGS += -ltelemetry_msgsender
AM_CXXFLAGS += -DUSE_TELEMETRY_2_0
endif

libwifiservices_la_SOURCES =
if RDKWIFIHAL
noinst_LTLIBRARIES = libwifiservices.la
libwifiservices_la_SOURCES += src/wifiSrvMgr.cpp src/wifiHalUtiles.cpp
endif
