dnl
dnl Docklike Taskbar - A modern, minimalist taskbar for Xfce
dnl --------------------------------------------------------
dnl (c) 2019-2020		Nicolas Szabo <nszabo@vivaldi.net>
dnl (c) 2020-2021		David Keogh <davidtkeogh@gmail.com>
dnl

dnl ***************************
dnl *** Version information ***
dnl ***************************
XDT_VERSION_INIT([0.4.2], [git])

dnl minimum required versions
m4_define([glib_min_version], [2.58.0])
m4_define([gtk_min_version], [3.24.0])
m4_define([cairo_min_version], [1.16.0])
m4_define([xfce_build_version], [4.16.0])
m4_define([libxfce4windowing_min_version], [4.19.4])

m4_define([libx11_min_version], [1.6.7])

m4_define([gtk_layer_shell_min_version], [0.7.0])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2019-2023 The Xfce development team. All rights reserved.
  Written for Xfce by Nicolas Szabo <nszabo@vivaldi.net> and David Keogh <davidtkeogh@gmail.com>.])
AC_INIT([xfce4-docklike-plugin], [xdt_version], [https://gitlab.xfce.org/panel-plugins/xfce4-docklike-plugin])
AC_PREREQ([2.69])
AC_CANONICAL_TARGET()
AC_REVISION([xdt_version_build])
AC_CONFIG_MACRO_DIRS([m4])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.8 no-dist-gzip dist-bzip2 tar-ustar foreign])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl **************************
dnl *** Initialize libtool ***
dnl **************************
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CXX()
AX_CXX_COMPILE_STDCXX_11()
AC_PROG_INSTALL()
AC_PROG_LN_S()

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
XDT_CHECK_PACKAGE([GIO_UNIX], [gio-unix-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [gtk_min_version])
XDT_CHECK_PACKAGE([CAIRO], [cairo], [cairo_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [xfce_build_version])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [xfce_build_version])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [xfce_build_version])
XDT_CHECK_PACKAGE([LIBXFCE4WINDOWING], [libxfce4windowing-0], [libxfce4windowing_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4WINDOWINGUI], [libxfce4windowingui-0], [libxfce4windowing_min_version])

XDT_CHECK_OPTIONAL_FEATURE([X11],
                           [x11],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_X11], [gdk-x11-3.0], [gtk_min_version])
                             XDT_FEATURE_DEPENDENCY([LIBX11], [x11], [libx11_min_version])
                             XDT_FEATURE_DEPENDENCY([LIBXFCE4WINDOWING_X11], [libxfce4windowing-x11-0], [libxfce4windowing_min_version])
                           ],
                           [the X11 windowing system])
XDT_CHECK_OPTIONAL_FEATURE([WAYLAND],
                           [wayland],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_WAYLAND], [gdk-wayland-3.0], [gtk_min_version])
                             XDT_FEATURE_DEPENDENCY([GTK_LAYER_SHELL], [gtk-layer-shell-0], [gtk_layer_shell_min_version])
                           ],
                           [the Wayland windowing system])
if test x"$ENABLE_X11" != x"yes" -a x"$ENABLE_WAYLAND" != x"yes"; then
  AC_MSG_ERROR([Either both X11 and Wayland support was disabled, or required dependencies are missing. One of the two must be enabled.])
fi

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl ************************
dnl *** Makefile outputs ***
dnl ************************
AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
])
AC_OUTPUT
