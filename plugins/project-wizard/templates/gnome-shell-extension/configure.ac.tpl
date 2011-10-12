[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])

AC_CONFIG_HEADERS([config.h])

AC_PREFIX_DEFAULT(~/.local/share/gnome-shell/extensions)

AM_INIT_AUTOMAKE([1.11 foreign])

AM_SILENT_RULES([yes])

LT_INIT

AC_OUTPUT([
Makefile
src/Makefile
])
