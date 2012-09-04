#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

echo "Generating initial interface files"
sh -c "cd $srcdir/libanjuta/interfaces && \
perl anjuta-idl-compiler.pl libanjuta && \
touch iface-built.stamp"

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
 cd "$srcdir" &&
 gtkdocize &&
 autopoint --force &&
 AUTOPOINT='intltoolize --automake --copy' autoreconf --force --install
) || exit
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
