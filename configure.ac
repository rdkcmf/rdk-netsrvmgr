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
        AS_HELP_STRING([--enable-route-support],[This will enable/disable route support.]),
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
                      IARM_LFLAGS="-lIARMBus -ldirect -lfusion -ldbus-1 -lIARMBus " ;;
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

AC_ARG_ENABLE([nlmonitor],
        AS_HELP_STRING([--enable-nlmonitor],[This will enable/disable netlink monitor support.]),
        [
          case "${enableval}" in
           yes) NLMONITOR_CFLAGS="-std=c++14 -DENABLE_NLMONITOR"
                PKG_CHECK_MODULES([NL], [libnl-3.0 >= 3.2.22])
                NLMONITOR_LINK="-lnlmonitor";;
           no) NLMONITOR_CFLAGS=" "
               NLMONITOR_LINK=" ";;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-nlmonitor ]) ;;
           esac
           ],
         [echo "Network Monitor Support is enabled"])


AC_SUBST(RDKWIFIHAL)
AC_SUBST(RDKMOCAHAL)
AC_SUBST(LOSTFOUND)
AC_SUBST(ROUTE_SUPPORT)
AC_SUBST(IARM_CFLAGS)
AC_SUBST(IARM_LFLAGS)
AC_SUBST(XCAM_SUPPORT)
AC_SUBST(NLMONITOR_CFLAGS)
AC_SUBST(NLMONITOR_LINK)


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
