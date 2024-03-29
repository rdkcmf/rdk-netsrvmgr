##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#AC_PREREQ([2.69])
AC_INIT([nm-generic], [1.0], [rajnish_dey2@cable.comcast.com])
AC_CONFIG_SRCDIR([nm])
AC_CONFIG_MACRO_DIR([cfg])
AC_CONFIG_AUX_DIR([cfg])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
AC_CONFIG_HEADERS([cfg/config.h])
AC_CONFIG_SRCDIR([src/main/src/netSrvMgrMain.cpp])

LT_INIT
AC_LANG([C++])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
[AM_DEFAULT_VERBOSITY=1
AC_SUBST(AM_DEFAULT_VERBOSITY)])


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.0])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h unistd.h])
#PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.0])
#PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 2.24.0])
RDKWIFIHAL=" "
RDKMOCAHAL=" "
NLMONITOR_CFLAGS=" "
NLMONITOR_LINK=" "
YOCTO_FLAG=" "
IS_YOCTO_ENABLED=" "
BREAKPAD_CFLAGS=" "
BREAKPAD_LFLAGS=" "

AC_ARG_ENABLE([rdk-moca-hal],
        AS_HELP_STRING([--enable-rdk-moca-hal],[This will enable/disable rdk moca hal.]),
        [
          case "${enableval}" in
           yes) RDKMOCAHAL=true ;;
           no) RDKMOCAHAL=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-rdk-moca-hal ]) ;;
           esac
           ],
         [echo " rdk-moca-hal is enable"])
AM_CONDITIONAL([RDKMOCAHAL], [test x$RDKMOCAHAL = xtrue])
AC_ARG_ENABLE([rdk-wifi-hal],
        AS_HELP_STRING([--enable-rdk-wifi-hal],[This will enable/disable rdk wifi hal.]),
        [
          case "${enableval}" in
           yes) RDKWIFIHAL=true ;;
           no) RDKWIFIHAL=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-rdk-wifi-hal ]) ;;
           esac
           ],
         [echo " rdk-wifi-hal is enable"])
AM_CONDITIONAL([RDKWIFIHAL], [test x$RDKWIFIHAL = xtrue])
LOSTFOUND=" "
AC_ARG_ENABLE([lost-found],
        AS_HELP_STRING([--enable-lost-found],[This will enable/disable lost and found.]),
        [
          case "${enableval}" in
           yes) LOSTFOUND=true ;;
           no) LOSTFOUND=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-lost-found ]) ;;
           esac
           ],
         [echo " lost-found is enable"])
AM_CONDITIONAL([LOSTFOUND], [test x$LOSTFOUND = xtrue])
ROUTE_SUPPORT=" "
AC_ARG_ENABLE([route-support],
        AS_HELP_STRING([--enable-route-support],[This will enable/disable lost and found.]),
        [
          case "${enableval}" in
           yes) ROUTE_SUPPORT=true ;;
           no) ROUTE_SUPPORT=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-route-support ]) ;;
           esac
           ],
         [echo "route-support is enable"])
AM_CONDITIONAL([ROUTE_SUPPORT], [test x$ROUTE_SUPPORT = xtrue])

IARM_CFLAGS=" "
IARM_LFLAGS=" "
AC_ARG_ENABLE([iarm],
              AS_HELP_STRING([--enable-iarm],[enable iarm (default is yes)]),
              [
                case "${enableval}" in
                 yes) IARM_CFLAGS="-DENABLE_IARM"
                      IARM_LFLAGS="-lIARMBus -ldbus-1 -lIARMBus " ;;
                 no)  IARM_CFLAGS=" "
                      IARM_LFLAGS=" " ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-iarm]) ;;
                esac
              ],
              [echo "iarm is enabled"])
#AM_CONDITIONAL([ENABLE_IARM], [test x$iarm = xtrue])

AC_ARG_ENABLE([xcam-support],
        AS_HELP_STRING([--enable-xcam-support],[This will enable/disable xcam device.]),
        [
          case "${enableval}" in
           yes) XCAM_SUPPORT=true ;;
           no) XCAM_SUPPORT=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-xcam-support ]) ;;
           esac
           ],
         [echo "xcam-support is disabled"])
AM_CONDITIONAL([XCAM_SUPPORT], [test x$XCAM_SUPPORT = xtrue])

RTMESSAGE=" "
AC_ARG_ENABLE([rtmessage],
        AS_HELP_STRING([--enable-rtmessage],[This will enable/disable rtmessage]),
        [
          case "${enableval}" in
           yes) RTMESSAGE=true ;;
           no) RTMESSAGE=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-rtmessage]) ;;
           esac
           ],
         [echo " rtmessage is enable"])
AM_CONDITIONAL([RTMESSAGE], [test x$RTMESSAGE = xtrue])

XHB1=" "
AC_ARG_ENABLE([xhb1],
        AS_HELP_STRING([--enable-xhb1],[This will be to enable/disable doorbell camera]),
        [
          case "${enableval}" in
           yes) XHB1=true ;;
           no) XHB1=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-xhb1]) ;;
           esac
           ],
         [echo " Doorbell camera is enabled"])
AM_CONDITIONAL([XHB1], [test x$XHB1 = xtrue])

XHC3=" "
AC_ARG_ENABLE([xhc3],
        AS_HELP_STRING([--enable-xhc3],[This will be to enable/disable xhc3]),
        [
          case "${enableval}" in
           yes) XHC3=true ;;
           no) XHC3=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-xhc3]) ;;
           esac
           ],
         [echo " XHC3 is enabled"])
AM_CONDITIONAL([XHC3], [test x$XHC3 = xtrue])

DEBUG=" "
AC_ARG_ENABLE([debug],
        AS_HELP_STRING([--enable-debug],[This will be to enable/disable debug symbols]),
        [
          case "${enableval}" in
           yes) DEBUG=true ;;
           no) DEBUG=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
           esac
           ],
         [echo " Debug symbols are enabled"])
AM_CONDITIONAL([DEBUG], [test x$DEBUG = xtrue])

AC_ARG_ENABLE([nlmonitor],
        AS_HELP_STRING([--enable-nlmonitor],[This will enable/disable netlink monitor support.]),
        [
          case "${enableval}" in
           yes) NLMONITOR_CFLAGS="-std=c++1y -DENABLE_NLMONITOR"
                PKG_CHECK_MODULES([NL], [libnl-3.0 >= 3.2.22])
                NLMONITOR_LINK="-lnlmonitor";;
           no) NLMONITOR_CFLAGS=" "
               NLMONITOR_LINK=" ";;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-nlmonitor ]) ;;
           esac
           ],
         [echo "Network Monitor Support is enabled"])

# Enable wifi client Roaming
WIFI_CLIENT_ROAMING=" "
AC_ARG_ENABLE([client_roaming],
        AS_HELP_STRING([--enable-client-roaming],[This will enable/disable wifi client roaming.]),
        [
          case "${enableval}" in
           yes) WIFI_CLIENT_ROAMING=true 
                echo " wifi-client-roaming is enabled";;
           no) WIFI_CLIENT_ROAMING=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-client-roaming ]) ;;
           esac
           ],
         [echo " wifi-client-roaming is enabled"])
AM_CONDITIONAL([WIFI_CLIENT_ROAMING], [test x$WIFI_CLIENT_ROAMING = xtrue])
AC_ARG_ENABLE([yocto],
        AS_HELP_STRING([--enable-yocto],[enable yocto (default is no)]),
        [
          case "${enableval}" in
           yes) IS_YOCTO_ENABLED=true
                YOCTO_FLAG=" -DYOCTO_BUILD ";;
           no)  IS_YOCTO_ENABLED=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-yocto]) ;;
           esac
           ],
        [echo "yocto is disabled"])
AM_CONDITIONAL([IS_YOCTO_ENABLED], [test x$IS_YOCTO_ENABLED = xtrue])
AC_ARG_ENABLE([breakpad],
		AS_HELP_STRING([--enable-breakpad],[enable breakpad support (default is no)]),
		[
			case "${enableval}" in
        	 yes) BREAKPAD_CFLAGS="-DINCLUDE_BREAKPAD"
                    BREAKPAD_LFLAGS="-lbreakpad_client";;
        	 no) AC_MSG_ERROR([breakpad is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-breakpad]) ;;
        	 esac
        	 ],
    	    [echo "breakpad is disabled"])
AC_ARG_ENABLE([telemetry_2_0],
        AS_HELP_STRING([--enable-telemetry_2_0],[This will enable/disable telemetry 2.0]),
        [
          case "${enableval}" in
           yes) TELEMETRY_2_0=true ;;
           no) TELEMETRY_2_0=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-telemetry_2_0 ]) ;;
           esac
           ],
         [echo " telemetry 2.0 is enabled"])
AM_CONDITIONAL([TELEMETRY_2_0], [test x$TELEMETRY_2_0 = xtrue])
AC_SUBST(BREAKPAD_CFLAGS)
AC_SUBST(BREAKPAD_LFLAGS)
AC_SUBST(YOCTO_FLAG)
AC_SUBST(RDKWIFIHAL)
AC_SUBST(RDKMOCAHAL)
AC_SUBST(LOSTFOUND)
AC_SUBST(XCAM_SUPPORT)
AC_SUBST(NLMONITOR_CFLAGS)
AC_SUBST(NLMONITOR_LINK)
AC_SUBST(ROUTE_SUPPORT)
AC_SUBST(IARM_CFLAGS)
AC_SUBST(IARM_LFLAGS)
AC_SUBST(WIFI_CLIENT_ROAMING)

AM_CONDITIONAL([CAMERA_DEVICE], [test "x$XCAM_SUPPORT" = "xtrue" || test "x$XHB1" == "xtrue" || test "x$XHC3" == "xtrue"])

#PKG_CHECK_MODULES([DBUS], [dbus-1])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/services/Makefile])
AC_CONFIG_FILES([src/services/wifi/Makefile])
AC_CONFIG_FILES([src/services/route/Makefile])
AC_CONFIG_FILES([src/services/moca/Makefile])
AC_CONFIG_FILES([test/Makefile])

AC_OUTPUT
