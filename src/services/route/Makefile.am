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

AM_CXXFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)  -I$(top_srcdir)/src/main/include -I$(top_srcdir)/src/services/route/include \
	-I${PKG_CONFIG_SYSROOT_DIR}/$(includedir)/directfb  $(GLIB_CFLAGS) $(G_THREAD_CFLAGS) $(NLMONITOR_CFLAGS) $(NL_CFLAGS) -I$(RDK_FSROOT_PATH)/usr/local/include \
        $(YOCTO_FLAG)

AM_CFLAGS = -I${PKG_CONFIG_SYSROOT_DIR}/$(includedir)/directfb

AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
        $(G_THREAD_LIBS) -ldbus-1  -lIARMBus -lcjson $(NLMONITOR_LINK)

AM_CXXFLAGS += $(IARM_CFLAGS)
if IS_YOCTO_ENABLED
AM_LDFLAGS += -lsecure_wrapper
endif

noinst_LTLIBRARIES = librouteservices.la
librouteservices_la_SOURCES =    src/routeSrvMgr.cpp 

