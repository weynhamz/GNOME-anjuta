[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

[+IF (=(get "HaveLangCPP") "1")+]
AC_PROG_CPP
AC_PROG_CXX
[+ENDIF+]

[+IF (=(get "HaveI18n") "1")+]
dnl ***************************************************************************
dnl Internatinalization
dnl ***************************************************************************
GETTEXT_PACKAGE=[+NameHLower+]
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AC_DEFINE_DIR(GNOMELOCALEDIR, "${datadir}/locale", [locale directory])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])
[+ENDIF+]

PKG_CHECK_MODULES(GNOME_APPLETS, libpanelapplet-2.0) 
[+IF (=(get "HavePackage") "1")+]
PKG_CHECK_MODULES([+NameCUpper+], [[+PackageModule1+] [+PackageModule2+] [+PackageModule3+] [+PackageModule4+] [+PackageModule5+]])
[+ENDIF+]

AS_AC_EXPAND(LIBEXECDIR, $libexecdir)
AC_SUBST(LIBEXECDIR)

AC_OUTPUT([
Makefile
src/Makefile
src/[+Name+].server.in
[+IF (=(get "HaveI18n") "1")+]po/Makefile.in[+ENDIF+]
])
