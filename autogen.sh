#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="anjuta"

(test -f $srcdir/configure.in \
  && test -f $srcdir/README \
  && test -d $srcdir/src) || {
    echo -n "**Error**: Directory "\$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}
		
which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}

echo "Generating initial interface files"
sh -c "cd $srcdir/libanjuta/interfaces && \
perl anjuta-idl-compiler.pl libanjuta && \
touch iface-built.stamp"

USE_GNOME2_MACROS=1 NOCONFIGURE=1 . gnome-autogen.sh
conf_flags="--enable-maintainer-mode --enable-compile-warnings=yes --enable-gtk-doc --enable-debug"
echo $srcdir/configure $conf_flags "$@"
$srcdir/configure $conf_flags "$@"
