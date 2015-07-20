## Copyright (c) 2013 Comcast. All rights reserved.

SUBDIRS =   src include 
DIST_SUBDIRS =  src include

AM_CXXFLAGS = -I$(top_srcdir)/src/main/include -I$(top_srcdir)/src/services/wifi/include $(GLIB_CFLAGS) $(G_THREAD_CFLAGS)


noinst_LTLIBRARIES = libwifiservices.la
libwifiservices_la_SOURCES =	src/WiFiNetworkMgr.cpp
