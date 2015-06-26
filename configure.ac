#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([nm-generic], [1.0], [cmcsa])
AC_CONFIG_SRCDIR([nm])

AC_CONFIG_MACRO_DIR([cfg])
AC_CONFIG_AUX_DIR([cfg])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
AC_CONFIG_HEADERS([cfg/config.h])

LT_INIT
AC_LANG([C++])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
[AM_DEFAULT_VERBOSITY=1
AC_SUBST(AM_DEFAULT_VERBOSITY)])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdlib.h unistd.h])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.0])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 2.24.0])
PKG_CHECK_MODULES([DIRECT], [direct])
PKG_CHECK_MODULES([FUSION], [fusion])
PKG_CHECK_MODULES([DBUS], [dbus-1])

AC_CHECK_LIB(gthread-2.0, g_thread_init)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
