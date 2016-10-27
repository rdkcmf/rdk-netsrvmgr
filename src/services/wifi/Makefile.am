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

AM_CXXFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)  -I$(top_srcdir)/src/main/include -I$(top_srcdir)/src/services/wifi/include $(GLIB_CFLAGS) $(G_THREAD_CFLAGS)

if RDKWIFIHAL
AM_CXXFLAGS += -DUSE_RDK_WIFI_HAL
endif
if LOSTFOUND
AM_CXXFLAGS += -DENABLE_LOST_FOUND -I$(RDK_SOURCE_PATH)/../opensource/include -I$(RDK_SOURCE_PATH)/../opensource/include/cjson
endif

if WIFI_CLIENT_ROAMING
AM_CXXFLAGS += -DWIFI_CLIENT_ROAMING
endif

AM_CXXFLAGS += $(IARM_CFLAGS)

AM_LDFLAGS = $(IARM_LFLAGS)

if XCAM_SUPPORT
AM_CXXFLAGS += -DENABLE_XCAM_SUPPORT -I$(RDK_FSROOT_PATH)/usr/include
if RTMESSAGE
AM_CXXFLAGS += -DENABLE_RTMESSAGE -std=c++11 -I$(RDK_SOURCE_PATH)/../opensource/src/rtmessage -I$(RDK_SOURCE_PATH)/../opensource/src/rtmessage/dataProvider
AM_LDFLAGS += -L$(RDK_SOURCE_PATH)/../opensource/lib/ -lrtMessage -ldataProvider
endif
AM_LDFLAGS += -lrdkc -lcgicomm -lcgilib -lsccomm -lmxml -lasound
endif

if RDKWIFIHAL
AM_LDFLAGS += -lwifihal
endif
if LOSTFOUND
AM_LDFLAGS += -llaf
endif

libwifiservices_la_SOURCES =
if RDKWIFIHAL
noinst_LTLIBRARIES = libwifiservices.la
libwifiservices_la_SOURCES += src/wifiSrvMgr.cpp src/wifiHalUtiles.cpp
endif

if XCAM_SUPPORT
if RTMESSAGE
libwifiservices_la_SOURCES += src/wifi_provider.cpp
install-data-local:
	mkdir -p $(RDK_FSROOT_PATH)/etc/model
	cp -fpR data/* $(RDK_FSROOT_PATH)/etc/model/
endif
endif

