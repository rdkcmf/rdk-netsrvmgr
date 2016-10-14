#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
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


AC_SUBST(RDKWIFIHAL)
AC_SUBST(LOSTFOUND)


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
AC_CONFIG_FILES([test/Makefile])

AC_OUTPUT
