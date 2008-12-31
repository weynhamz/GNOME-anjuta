#!/bin/sh

project_root=`mktemp -d /tmp/test-am-XXXXXX`

echo Creating project in $project_root

if ! pushd $project_root > /dev/null; then 
    echo Failed to cd to the project directory
fi

# Create mandatory automake files
echo -n Creating NEWS AUTHORS INSTALL README COPYING ChangeLog
touch NEWS AUTHORS INSTALL README COPYING ChangeLog

echo -n configure.in
cat > configure.in <<EOF
AC_PREREQ(2.52)
AC_INIT(test-am, 0.1)
AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(src/Makefile.am)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AC_OUTPUT([
Makefile
src/Makefile
])

EOF

echo -n Makefile.am
cat > Makefile.am <<EOF
SUBDIRS = src

EOF

echo

# Create at least one directory
echo -n Creating src/
mkdir src
cd src

echo -n src/test.c src/test.h src/foo.c
touch test.c test.h foo.c

echo -n src/Makefile.am
cat > Makefile.am <<EOF
INCLUDES = -DFOO

lib_LTLIBRARIES = libfoo.la

libfoo_la_SOURCES = test.c test.h

include_HEADERS = test.h

bin_PROGRAMS = foo

foo_SOURCES = \
	foo.c

foo_LDADD = libfoo.la

EOF

echo
if test "$1" = "-n" -o "$1" = "--no-test"; then
    exit 0;
fi

echo Now running test program...

popd > /dev/null
echo ./test $project_root
libtool --mode=execute ./test $project_root

if [ $? -eq 0 ]; then
    echo Removing test project
    rm -Rf $project_root
    echo DONE!
    echo
fi

