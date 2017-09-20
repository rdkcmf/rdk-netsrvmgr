## Copyright (c) 2013 Comcast. All rights reserved.

#AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS =  services
DIST_SUBDIRS = services

AM_CXXFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) \
	-I$(top_srcdir)/src/main/include \
	-I$(RDK_FSROOT_PATH)/usr/local/include \
	-I$(top_srcdir)/src/services/wifi/include \
	-I$(top_srcdir)/src/services/route/include \
	-I${PKG_CONFIG_SYSROOT_DIR}/$(includedir)/directfb \
	-I$(top_srcdir)/src/services/moca/include

if RDKWIFIHAL
AM_CXXFLAGS += -DUSE_RDK_WIFI_HAL
endif

if LOSTFOUND
AM_CXXFLAGS += -DENABLE_LOST_FOUND
endif

if XCAM_SUPPORT
AM_CXXFLAGS += -DENABLE_XCAM_SUPPORT -I$(RDK_FSROOT_PATH)/usr/include
sysconf_DATA = $(top_srcdir)/conf/netsrvmgr.conf
endif

if ROUTE_SUPPORT
AM_CXXFLAGS += -DENABLE_ROUTE_SUPPORT
endif

AM_LDFLAGS = $(GLIB_LDFLAGS) $(GLIB_LIBS) \
	$(G_THREAD_LIBS) -lrdkloggers $(LOG4C_LIBS) -lpthread -lcjson

AM_LDFLAGS += $(IARM_LFLAGS)

if RDKWIFIHAL
AM_LDFLAGS += -lwifihal
endif
if LOSTFOUND
AM_LDFLAGS += -llaf -lcurl
endif

if XCAM_SUPPORT
AM_LDFLAGS += -lrdkc -lcgicomm -lcgilib -lsccomm -lmxml -lasound
endif

AM_CFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)
AM_CPPFLAGS =  $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)

# the list of header files that belong to the library (to be installed later)
include_HEADERS =    $(top_srcdir)/src/services/wifi/include/wifiSrvMgrIarmIf.h \
		     $(top_srcdir)/src/main/include/netsrvmgrIarm.h

bin_PROGRAMS = netsrvmgr
netsrvmgr_SOURCES = $(top_srcdir)/src/main/src/netSrvMgrMain.cpp \
			$(top_srcdir)/src/main/src/NetworkMedium.cpp \
			$(top_srcdir)/src/main/src/netsrvmgrUtiles.cpp
if LOSTFOUND
netsrvmgr_SOURCES += $(top_srcdir)/src/main/src/netSrvCurl.cpp
endif
netsrvmgr_LDADD = 
if RDKWIFIHAL
netsrvmgr_LDADD += $(top_builddir)/src/services/wifi/libwifiservices.la
endif

if ROUTE_SUPPORT
netsrvmgr_LDADD +=  $(top_builddir)/src/services/route/librouteservices.la
endif

if RDKMOCAHAL
netsrvmgr_LDADD +=  $(top_builddir)/src/services/moca/libmocaservices.la
AM_LDFLAGS +=  -lrdkmocahal
AM_CXXFLAGS += -DUSE_RDK_MOCA_HAL
endif
