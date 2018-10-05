## Copyright (c) 2013 Comcast. All rights reserved.
SUBDIRS =  

AM_CXXFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) -I$(top_srcdir)/src/main/include \
		-I$(top_srcdir)/src/services/wifi/include \
		-I$(RDK_FSROOT_PATH)/usr/local/include \
                -I$(RDK_FSROOT_PATH)/usr/local/include/cjson

AM_LDFLAGS = -lpthread $(IARM_LFLAGS)

if LOSTFOUND
AM_CXXFLAGS += -DENABLE_LOST_FOUND
endif

AM_CXXFLAGS += $(IARM_CFLAGS)


bin_PROGRAMS = wifiSrvMgrTestClient
wifiSrvMgrTestClient_SOURCES = wifiSrvMgrTestClient.cpp

