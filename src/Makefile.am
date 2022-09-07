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
AM_CXXFLAGS += -DENABLE_XCAM_SUPPORT -I$(RDK_FSROOT_PATH)/usr/include -I$(RDK_SOURCE_PATH)/../opensource/include/cjson -I$(RDK_PROJECT_ROOT_PATH)/breakpadwrap/ -I$(RDK_SOURCE_PATH)/../utility/sysUtils/include/
sysconf_DATA = $(top_srcdir)/conf/netsrvmgr.conf
endif

if RTMESSAGE
AM_CXXFLAGS += -DENABLE_RTMESSAGE -I$(RDK_SOURCE_PATH)/../opensource/src/rtmessage
endif

if ROUTE_SUPPORT
AM_CXXFLAGS += -DENABLE_ROUTE_SUPPORT
endif

AM_CXXFLAGS += $(IARM_CFLAGS)

AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
	$(G_THREAD_LIBS) -lrdkloggers $(LOG4C_LIBS) -lpthread -lcjson -lsqlite3

AM_LDFLAGS += $(IARM_LFLAGS) $(NLMONITOR_LINK) $(BREAKPAD_LFLAGS)

if RDKWIFIHAL
AM_LDFLAGS += -lwifihal
endif
if LOSTFOUND
AM_LDFLAGS += -llaf -lcurl
endif

if XCAM_SUPPORT
AM_LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/breakpadwrap/ -lbreakpadwrap -L$(RDK_SOURCE_PATH)/../utility/sysUtils/src/ -lsysutils
AM_LDFLAGS += -lrdkc -lcgicomm -lcgilib -lsccomm -lmxml -lasound
endif

if XHB1
AM_CXXFLAGS += -DXHB1 -I$(RDK_SOURCE_PATH)/../utility/sysUtils/include/
AM_LDFLAGS += -lmfrApi -lcgicomm -lsccomm -lmxml -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/ramdisk/amba/prebuild/third-party/armv8-a/alsa-lib/usr/lib/ -lasound -L$(RDK_SOURCE_PATH)/../utility/sysUtils/src/ -lsysutils
endif

if XHC3
AM_CXXFLAGS += -DXHC3 -I$(RDK_SOURCE_PATH)/../utility/sysUtils/include/
AM_LDFLAGS += -lmfrApi -lcgicomm -lsccomm -lmxml -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/ramdisk/amba/prebuild/third-party/armv7-a-hf/alsa-lib/usr/lib/ -lasound -L$(RDK_SOURCE_PATH)/../utility/sysUtils/src/ -lsysutils  -Wl,-rpath-link,$(RDK_FSROOT_PATH)/usr/lib/
endif

if RTMESSAGE
AM_LDFLAGS += -L$(RDK_SOURCE_PATH)/../opensource/lib/ -lrtMessage
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

netsrvmgr_LDADD =

if !CAMERA_DEVICE
netsrvmgr_SOURCES += $(top_srcdir)/src/main/src/StunClient.cpp
AM_CXXFLAGS += -DENABLE_STUN_CLIENT

lib_LTLIBRARIES = libconnectivitytest.la
libconnectivitytest_la_SOURCES = $(top_srcdir)/src/main/src/connectivity.cpp
libconnectivitytest_la_LIBADD = -lrdkloggers -lcurl

bin_PROGRAMS += test_connectivity
test_connectivity_SOURCES = $(top_srcdir)/src/main/src/test_connectivity.cpp
test_connectivity_LDADD = libconnectivitytest.la

netsrvmgr_LDADD += libconnectivitytest.la
endif

if LOSTFOUND
netsrvmgr_SOURCES += $(top_srcdir)/src/main/src/netSrvCurl.cpp
endif

if RDKWIFIHAL
netsrvmgr_LDADD += $(top_builddir)/src/services/wifi/libwifiservices.la
endif

if ROUTE_SUPPORT
netsrvmgr_LDADD +=  $(top_builddir)/src/services/route/librouteservices.la
endif

if RDKMOCAHAL
netsrvmgr_LDADD +=  $(top_builddir)/src/services/moca/libmocaservices.la
AM_LDFLAGS +=  -lrdkmocahal
AM_CXXFLAGS += -DUSE_RDK_MOCA_HAL
endif

if TELEMETRY_2_0
AM_LDFLAGS += -ltelemetry_msgsender
AM_CXXFLAGS += -DUSE_TELEMETRY_2_0
endif

if DEBUG
AM_CFLAGS += -g
AM_CXXFLAGS += -g
endif
