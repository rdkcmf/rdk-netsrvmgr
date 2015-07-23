## Copyright (c) 2013 Comcast. All rights reserved.

SUBDIRS =  services
DIST_SUBDIRS = services

AM_CXXFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) \
	-I$(top_srcdir)/src/main/include \
	-I$(top_srcdir)/src/services/include 


AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
	$(G_THREAD_LIBS) -ldbus-1 -ldirect \
	-lfusion ${DIRECTFB_LIBS} -lIARMBus 


AM_CFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)
AM_CPPFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)


bin_PROGRAMS = networkmgr
networkmgr_SOURCES = $(top_srcdir)/src/main/src/NetworkMgrMain.cpp $(top_srcdir)/src/main/src/NetworkMedium.cpp
networkmgr_LDADD = $(top_builddir)/src/services/wifi/libwifiservices.la

