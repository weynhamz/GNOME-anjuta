[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])
AM_MAINTAINER_MODE

AM_SILENT_RULES([yes])

AC_PROG_CC

[+IF (=(get "HaveSharedlib") "1")+]
LT_INIT
[+ENDIF+]

dnl Check for vala
AM_PROG_VALAC([0.10.0])

[+IF (=(get "HavePackage") "1")+]
PKG_CHECK_MODULES([+NameCUpper+], [[+PackageModule1+] [+PackageModule2+]])
[+ENDIF+]

AC_OUTPUT([
Makefile
src/Makefile
[+IF (=(get "HaveI18n") "1")+]po/Makefile.in[+ENDIF+]
])
