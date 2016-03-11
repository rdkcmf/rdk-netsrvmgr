## Copyright (c) 2013 Comcast. All rights reserved.

SUBDIRS =
DIST_SUBDIRS =

AM_CXXFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)  -I$(top_srcdir)/src/main/include -I$(top_srcdir)/src/services/wifi/include $(GLIB_CFLAGS) $(G_THREAD_CFLAGS)

if RDKWIFIHAL
AM_CXXFLAGS += -DUSE_RDK_WIFI_HAL
endif
if LOSTFOUND
AM_CXXFLAGS += -DENABLE_LOST_FOUND
endif


AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
	$(G_THREAD_LIBS) -ldbus-1  -lIARMBus 

if RDKWIFIHAL
AM_LDFLAGS += -lwifihal
endif
if LOSTFOUND
AM_LDFLAGS += -llaf
endif

noinst_LTLIBRARIES = libwifiservices.la
libwifiservices_la_SOURCES =	src/wifiSrvMgr.cpp src/wifiHalUtiles.cpp
