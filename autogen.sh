#!/bin/sh
# Run this to generate all the initial makefiles, etc.

clear
echo ""
echo "This is Anjuta CVS HEAD you are trying to build and it is"
echo "currently not buildable. If you are looking for latest anjuta that"
echo "builds in CVS, please checkout the ANJUTA_1_0_0 branch with the"
echo "command 'cvs co -r ANJUTA_1_0_0 anjuta'."
echo ""
echo "Thank you"
echo "Anjuta development team"
echo ""
exit 0

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

USE_GNOME2_MACROS=1 . gnome-autogen.sh
