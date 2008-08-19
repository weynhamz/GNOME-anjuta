[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

## Give a shell command here that will set CLASSPATH environment variable.
CLASSPATH_ENV = 

## Add classpaths and other flags here
AM_JAVACFLAGS =

## Directory where .class files will be created.
JAVAROOT = $(top_builddir)/src

## Directory where .class files will be installed
[+NameCLower+]dir = $(libdir)/[+NameHLower+]

[+NameCLower+]_JAVA = \
	[+MainClass+].java
