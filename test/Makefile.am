## Copyright (c) 2013 Comcast. All rights reserved.
SUBDIRS =  

AM_CXXFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) -I$(top_srcdir)/src/main/include -I$(top_srcdir)/src/services/wifi/include

AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) $(G_THREAD_LIBS) -ldbus-1 -lIARMBus -lpthread 

if LOSTFOUND
AM_CXXFLAGS += -DENABLE_LOST_FOUND
endif


bin_PROGRAMS = wifiSrvMgrTestClient
wifiSrvMgrTestClient_SOURCES = wifiSrvMgrTestClient.cpp

