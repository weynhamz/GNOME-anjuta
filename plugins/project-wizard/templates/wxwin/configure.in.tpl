[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(configure.in)
AM_INIT_AUTOMAKE([+NameLower+], [+Version+])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AM_OPTIONS_WXCONFIG
AM_PATH_WXCONFIG(2.3.2, wxWin=1)
        if test "$wxWin" != 1; then
        AC_MSG_ERROR([
                wxWindows must be installed on your system.

                Please check that wx-config is in path, the directory
                where wxWindows libraries are installed (returned by
                'wx-config --libs' or 'wx-config --static --libs' command)
                is in LD_LIBRARY_PATH or equivalent variable and
                wxWindows version is 2.3.2 or above.
                ])
        else
               dnl Quick hack until wx-config does it
               ac_save_LIBS=$LIBS
               ac_save_CXXFLAGS=$CXXFLAGS
               LIBS=$WX_LIBS
               CXXFLAGS=$WX_CXXFLAGS
               AC_LANG_SAVE
               AC_LANG_CPLUSPLUS
               AC_TRY_LINK([#include <wx/wx.h>],
               [wxString test=""],
               ,[WX_LIBS=$WX_LIBS_STATIC])
               AC_LANG_RESTORE
               LIBS=$ac_save_LIBS
               CXXFLAGS=$ac_save_CXXFLAGS
        fi

[+IF (=(get "HaveLangCPP") "1")+]
AC_PROG_CPP
AC_PROG_CXX
[+ENDIF+]

[+IF (=(get "HaveI18n") "1")+]
dnl Set gettext package name
GETTEXT_PACKAGE=[+NameLower+]
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

dnl Add the languages which your application supports here.
ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT
[+ENDIF+]

[+IF (=(get "HaveSharedlib") "1")+]
AM_PROG_LIBTOOL
[+ENDIF+]

[+IF (=(get "HavePackage") "1")+]
PKG_CHECK_MODULES(PACKAGE, [[+PackageModule1+] [+PackageModule2+] [+PackageModule3+] [+PackageModule4+] [+PackageModule5+]])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)
[+ENDIF+]

AC_OUTPUT([
Makefile
src/Makefile
[+IF (=(get "HaveI18n") "1")+]po/Makefile.in[+ENDIF+]
])
