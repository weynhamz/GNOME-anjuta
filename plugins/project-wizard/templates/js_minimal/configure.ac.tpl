[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([+NameHLower+], [+Version+])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_PROG_CC

PKG_CHECK_EXISTS([mozilla-js], [JS_PACKAGE=mozilla-js],
                 [PKG_CHECK_EXISTS([xulrunner-js], [JS_PACKAGE=xulrunner-js], [JS_PACKAGE=firefox-js])])
PKG_CHECK_MODULES(GJS,gtk+-2.0 gjs-gi-1.0 >= 0.6 $JS_PACKAGE)

## some flavors of Firefox .pc only set sdkdir, not libdir
FIREFOX_JS_SDKDIR=`$PKG_CONFIG --variable=sdkdir $JS_PACKAGE`
FIREFOX_JS_LIBDIR=`$PKG_CONFIG --variable=libdir $JS_PACKAGE`

## Ubuntu does not set libdir in mozilla-js.pc
if test x"$FIREFOX_JS_LIBDIR" = x ; then
   ## Ubuntu returns xulrunner-devel as the sdkdir, but for the
   ## libdir we want the runtime location on the target system,
   ## so can't use -devel.
   ## The library is in the non-devel directory also.
   ## Don't ask me why it's in two places.
   FIREFOX_JS_LIBDIR=`echo "$FIREFOX_JS_SDKDIR" | sed -e 's/-devel//g'`

   if ! test -d "$FIREFOX_JS_LIBDIR" ; then
      FIREFOX_JS_LIBDIR=
   fi
fi


GJS_JS_DIR=`$PKG_CONFIG --variable=jsdir gjs-1.0`
GJS_JS_NATIVE_DIR=`$PKG_CONFIG --variable=jsnativedir gjs-1.0`
AC_SUBST(GJS_JS_DIR)
AC_SUBST(FIREFOX_JS_LIBDIR)
AC_SUBST(GJS_JS_NATIVE_DIR)

AM_PATH_GLIB_2_0()
G_IR_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
AC_SUBST(G_IR_SCANNER)
G_IR_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`
AC_SUBST(G_IR_COMPILER)
G_IR_GENERATE=`$PKG_CONFIG --variable=g_ir_generate gobject-introspection-1.0`
AC_SUBST(G_IR_GENERATE)
GIRDIR=`$PKG_CONFIG --variable=girdir gobject-introspection-1.0`
AC_SUBST(GIRDIR)
TYPELIBDIR="$($PKG_CONFIG --variable=typelibdir gobject-introspection-1.0)"
AC_SUBST(TYPELIBDIR)

AM_PROG_LIBTOOL

AC_OUTPUT([
Makefile
src/Makefile
])
