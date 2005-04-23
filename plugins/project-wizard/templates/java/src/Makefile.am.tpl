[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

## Compiled version

## Add classpaths and other flags here
## AM_GCJFLAGS=

bin_PROGRAMS = [+NameLower+]

[+NameCLower+]_SOURCES = \
	[+MainClass+].java

[+NameCLower+]_LDFLAGS = \
	--main=[+MainClass+]

[+NameCLower+]_LDADD = $(PACKAGE_LIBS)

[+IF (=(get "HaveJavaClasses") "1")+]
## Interpreted version

## Add classpaths and other flags here
## AM_JAVACFLAGS=

JAVAROOT = $(builddir)
[+NameCLower+]_classesdir = $(libdir)/[+NameLower+]
[+NameCLower+]_classes_JAVA = $([+NameCLower+]_SOURCES)
[+ENDIF+]
