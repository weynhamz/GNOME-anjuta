[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

bin_PROGRAMS = [+NameLower+]

[+NameCLower+]_SOURCES = \
	[+NameLower+].java

[+NameCLower+]_LDFLAGS = \
	--main=[+MainClass+]

[+NameCLower+]_LDADD = $(PACKAGE_LIBS)
