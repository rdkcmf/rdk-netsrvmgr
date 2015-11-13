## Copyright (c) 2013 Comcast. All rights reserved.

SUBDIRS =
DIST_SUBDIRS =

AM_CXXFLAGS = -I$(top_srcdir)/src/main/include -I$(top_srcdir)/src/services/wifi/include $(GLIB_CFLAGS) $(G_THREAD_CFLAGS)

if RDKWIFIHAL
AM_CXXFLAGS += -DUSE_RDK_WIFI_HAL
endif


AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
	$(G_THREAD_LIBS) -ldbus-1  -lIARMBus 

noinst_LTLIBRARIES = libwifiservices.la
libwifiservices_la_SOURCES =	src/wifiSrvMgr.cpp src/wifiHalUtiles.cpp
