[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])

SDL_REQUIRED=[+SDL_Version+]

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PATH_XTRA

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
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])
[+ENDIF+]

AM_PROG_LIBTOOL

dnl ***************************************************************************
dnl Check for SDL
dnl ***************************************************************************
AM_PATH_SDL($SDL_REQUIRED,
            :,
	    AC_MSG_ERROR([SDL version $SDL_REQUIRED not found]))
CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
CFLAGS="$CFLAGS $SDL_CFLAGS"
CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

[+IF (=(get "HaveSDL_image") "1")+]
dnl ***************************************************************************
dnl Check for SDL_image
dnl ***************************************************************************
AC_CHECK_LIB(SDL_image, IMG_Load,
    [ LIBS="$LIBS -lSDL_image" ],
    AC_MSG_ERROR([SDL_image not found]))
[+ENDIF+]

[+IF (=(get "HaveSDL_gfx") "1")+]
dnl ***************************************************************************
dnl Check for SDL_gfx
dnl ***************************************************************************
AC_CHECK_LIB(SDL_gfx, SDL_initFramerate,
    [ LIBS="$LIBS -lSDL_gfx" ],
    AC_MSG_ERROR([SDL_gfx not found]))
[+ENDIF+]

[+IF (=(get "HaveSDL_ttf") "1")+]
dnl ***************************************************************************
dnl Check for SDL_ttf
dnl ***************************************************************************
AC_CHECK_LIB(SDL_ttf, TTF_OpenFont,
    [ LIBS="$LIBS -lSDL_ttf" ],
    AC_MSG_ERROR([SDL_ttf not found]))
[+ENDIF+]

[+IF (=(get "HaveSDL_mixer") "1")+]
dnl ***************************************************************************
dnl Check for SDL_mixer
dnl ***************************************************************************
AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio,
    [ LIBS="$LIBS -lSDL_mixer" ],
    AC_MSG_ERROR([SDL_mixer not found]))
[+ENDIF+]

[+IF (=(get "HaveSDL_net") "1")+]
dnl ***************************************************************************
dnl Check for SDL_net
dnl ***************************************************************************
AC_CHECK_LIB(SDL_net, SDLNet_Init,
    [ LIBS="$LIBS -lSDL_net" ],
    AC_MSG_ERROR([SDL_net not found]))
[+ENDIF+]

AC_OUTPUT([
Makefile
src/Makefile
[+IF (=(get "HaveI18n") "1")+]po/Makefile.in[+ENDIF+]
])
