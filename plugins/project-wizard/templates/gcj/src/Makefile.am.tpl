[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

## Java natively compiled using gcj

## Add classpaths and other flags here
## AM_GCJFLAGS=

bin_PROGRAMS = [+NameLower+]

[+NameCLower+]_SOURCES = \
	[+MainClass+].java

[+NameCLower+]_LDFLAGS = \
	--main=[+MainClass+]

[+NameCLower+]_LDADD = $(PACKAGE_LIBS)
