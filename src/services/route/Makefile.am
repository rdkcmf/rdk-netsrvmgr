## Copyright (c) 2013 Comcast. All rights reserved.

SUBDIRS =
DIST_SUBDIRS =

AM_CXXFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)  -I$(top_srcdir)/src/main/include -I$(top_srcdir)/src/services/route/include \
	-I${PKG_CONFIG_SYSROOT_DIR}/$(includedir)/directfb  $(GLIB_CFLAGS) $(G_THREAD_CFLAGS) $(NLMONITOR_CFLAGS) $(NL_CFLAGS)

AM_CFLAGS = -I${PKG_CONFIG_SYSROOT_DIR}/$(includedir)/directfb

AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
        $(G_THREAD_LIBS) -ldbus-1  -lIARMBus -lcjson $(NLMONITOR_LINK)


noinst_LTLIBRARIES = librouteservices.la
librouteservices_la_SOURCES =    src/routeSrvMgr.cpp 

