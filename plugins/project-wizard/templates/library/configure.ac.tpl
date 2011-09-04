[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])
AM_MAINTAINER_MODE

AM_SILENT_RULES([yes])

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

[+CASE (get "LibraryType") +]
[+ == "Static" +]AC_DISABLE_SHARED
[+ == "Shared" +]AC_DISABLE_STATIC
[+ESAC+]LT_INIT

[+IF (=(get "HavePackage") "1")+]
PKG_CHECK_MODULES([+NameCUpper+], [[+PackageModule1+] [+PackageModule2+]])
[+ENDIF+]

[+IF (=(get "HaveGtkDoc") "1")+]
GTK_DOC_CHECK([1.0])
[+ENDIF+]

AC_OUTPUT([
Makefile
src/lib[+NameHLower+]-[+Version+].pc
src/Makefile
[+IF (=(get "HaveI18n") "1")+]po/Makefile.in[+ENDIF+]])
