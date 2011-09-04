[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])

AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([1.11])
AM_MAINTAINER_MODE

AC_PROG_CC

[+IF (=(get "HaveLangCPP") "1")+]
AC_PROG_CPP
AC_PROG_CXX
[+ENDIF+]

[+IF (=(get "HaveI18n") "1")+]
dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=[+NameHLower+]
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
[+ENDIF+]

[+IF (=(get "HaveSharedlib") "1")+]
LT_INIT
[+ENDIF+]

PKG_CHECK_MODULES(LIBANJUTA, [libanjuta-1.0])

[+IF (=(get "HavePackage") "1")+]
PKG_CHECK_MODULES([+NameCUpper+], [[+PackageModule1+] [+PackageModule2+]])
AC_SUBST([+NameCUpper+]_CFLAGS)
AC_SUBST([+NameCUpper+]_LIBS)
[+ENDIF+]

dnl Setup Plugin directories
dnl ------------------------
anjutalibdir=`pkg-config --variable=libdir libanjuta-1.0`
anjutadatadir=`pkg-config --variable=datadir libanjuta-1.0`
AC_SUBST(anjutalibdir)
AC_SUBST(anjutadatadir)
anjuta_plugin_dir='$(anjutalibdir)/anjuta'
anjuta_data_dir='$(anjutadatadir)/anjuta'
anjuta_ui_dir='$(anjutadatadir)/anjuta/ui'
anjuta_glade_dir='$(anjutadatadir)/anjuta/glade'
anjuta_image_dir='$(anjutadatadir)/pixmaps/anjuta'
AC_SUBST(anjuta_plugin_dir)
AC_SUBST(anjuta_data_dir)
AC_SUBST(anjuta_ui_dir)
AC_SUBST(anjuta_glade_dir)
AC_SUBST(anjuta_image_dir)

[+IF (=(get "HaveGtkDoc") "1")+]
GTK_DOC_CHECK([1.0])
[+ENDIF+]

AC_OUTPUT([
Makefile
src/Makefile
[+IF (=(get "HaveI18n") "1")+]po/Makefile.in[+ENDIF+]
])
