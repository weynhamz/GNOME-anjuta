[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 foreign])
AM_MAINTAINER_MODE

AM_SILENT_RULES([yes])

LT_INIT

AC_OUTPUT([
Makefile
src/Makefile
])
