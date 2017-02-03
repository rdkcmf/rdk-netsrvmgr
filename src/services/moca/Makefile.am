## Copyright (c) 2013 Comcast. All rights reserved.
AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS =
DIST_SUBDIRS = 

AM_CXXFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) -I$(top_srcdir)/src/main/include \
		-I$(top_srcdir)/src/services/moca/include $(GLIB_CFLAGS) $(G_THREAD_CFLAGS) \
		-I$(RDK_FSROOT_PATH)/usr/local/include

AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
        $(G_THREAD_LIBS) -lrdkmocahal

noinst_LTLIBRARIES = libmocaservices.la
libmocaservices_la_SOURCES = src/mocaSrvMgr.cpp 

