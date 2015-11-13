## Copyright (c) 2013 Comcast. All rights reserved.

SUBDIRS =  services
DIST_SUBDIRS = services

AM_CXXFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) \
	-I$(top_srcdir)/src/main/include \
	-I$(top_srcdir)/src/services/wifi/include

if RDKWIFIHAL
AM_CXXFLAGS += -DUSE_RDK_WIFI_HAL
endif

AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
	$(G_THREAD_LIBS) -lrdkloggers $(LOG4C_LIBS) ${DIRECTFB_LIBS} \
	-lIARMBus -ldirect -lfusion -ldbus-1 -lIARMBus -lpthread


AM_CFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)
AM_CPPFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)

# the list of header files that belong to the library (to be installed later)
include_HEADERS =    $(top_srcdir)/src/services/wifi/include/wifiSrvMgrIarmIf.h

bin_PROGRAMS = netsrvmgr
netsrvmgr_SOURCES = $(top_srcdir)/src/main/src/netSrvMgrMain.cpp $(top_srcdir)/src/main/src/NetworkMedium.cpp
netsrvmgr_LDADD = $(top_builddir)/src/services/wifi/libwifiservices.la

