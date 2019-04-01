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
#AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS =  services
DIST_SUBDIRS = services

AM_CXXFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(NLMONITOR_CFLAGS) $(NL_CFLAGS) \
	-I$(top_srcdir)/src/main/include \
	-I$(RDK_FSROOT_PATH)/usr/local/include \
	-I$(top_srcdir)/src/services/wifi/include \
	-I$(top_srcdir)/src/services/route/include \
	-I${PKG_CONFIG_SYSROOT_DIR}/$(includedir)/directfb \
	-I$(top_srcdir)/src/services/moca/include

if RDKWIFIHAL
AM_CXXFLAGS += -DUSE_RDK_WIFI_HAL
endif

if LOSTFOUND
AM_CXXFLAGS += -DENABLE_LOST_FOUND
endif

if XCAM_SUPPORT
AM_CXXFLAGS += -DENABLE_XCAM_SUPPORT -I$(RDK_FSROOT_PATH)/usr/include -I$(RDK_SOURCE_PATH)/../opensource/include/cjson
if RTMESSAGE
AM_CXXFLAGS += -DENABLE_RTMESSAGE -I$(RDK_SOURCE_PATH)/../opensource/src/rtmessage -I$(RDK_SOURCE_PATH)/../opensource/src/rtmessage/dataProvider
endif
sysconf_DATA = $(top_srcdir)/conf/netsrvmgr.conf
endif


if ROUTE_SUPPORT
AM_CXXFLAGS += -DENABLE_ROUTE_SUPPORT
endif

AM_CXXFLAGS += $(IARM_CFLAGS)

AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
	$(G_THREAD_LIBS) -lrdkloggers $(LOG4C_LIBS) -lpthread -lcjson

AM_LDFLAGS += $(IARM_LFLAGS) $(NLMONITOR_LINK) $(BREAKPAD_LFLAGS)

if RDKWIFIHAL
AM_LDFLAGS += -lwifihal
endif
if LOSTFOUND
AM_LDFLAGS += -llaf -lcurl
endif

if XCAM_SUPPORT
AM_LDFLAGS += -lrdkc -lcgicomm -lcgilib -lsccomm -lmxml -lasound
if RTMESSAGE
AM_LDFLAGS += -L$(RDK_SOURCE_PATH)/../opensource/lib/ -lrtMessage -ldataProvider
endif
endif

AM_CFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(BREAKPAD_CFLAGS)
AM_CPPFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(BREAKPAD_CFLAGS)

# the list of header files that belong to the library (to be installed later)
include_HEADERS =    $(top_srcdir)/src/services/wifi/include/wifiSrvMgrIarmIf.h \
		     $(top_srcdir)/src/main/include/netsrvmgrIarm.h

bin_PROGRAMS = netsrvmgr
netsrvmgr_SOURCES = $(top_srcdir)/src/main/src/netSrvMgrMain.cpp \
			$(top_srcdir)/src/main/src/NetworkMedium.cpp \
			$(top_srcdir)/src/main/src/netsrvmgrUtiles.cpp
if LOSTFOUND
netsrvmgr_SOURCES += $(top_srcdir)/src/main/src/netSrvCurl.cpp
endif
netsrvmgr_LDADD = 
if RDKWIFIHAL
netsrvmgr_LDADD += $(top_builddir)/src/services/wifi/libwifiservices.la
endif

if XCAM_SUPPORT
if RTMESSAGE
install-data-local:
	mkdir -p $(RDK_FSROOT_PATH)/etc/model
	cp -fpR $(top_builddir)/src/services/wifi/data/* $(RDK_FSROOT_PATH)/etc/model/
endif
endif

if ROUTE_SUPPORT
netsrvmgr_LDADD +=  $(top_builddir)/src/services/route/librouteservices.la
endif

if RDKMOCAHAL
netsrvmgr_LDADD +=  $(top_builddir)/src/services/moca/libmocaservices.la
AM_LDFLAGS +=  -lrdkmocahal
AM_CXXFLAGS += -DUSE_RDK_MOCA_HAL
endif
