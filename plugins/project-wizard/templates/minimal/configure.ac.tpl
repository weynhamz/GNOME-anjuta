[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 foreign])
AM_MAINTAINER_MODE

AM_SILENT_RULES([yes])

AC_PROG_CC

[+IF (=(get "HaveLangCPP") "1")+]
AC_PROG_CPP
AC_PROG_CXX
[+ENDIF+]

[+IF (=(get "HaveSharedlib") "1")+]
LT_INIT
[+ENDIF+]

[+IF (=(get "HavePackage") "1")+]
PKG_CHECK_MODULES([+NameCUpper+], [[+PackageModule1+] [+PackageModule2+]])
[+ENDIF+]

AC_OUTPUT([
Makefile
])
