INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/intl \
	$(GNOME_INCLUDEDIR)\
	$(EXTRA_GNOME_CFLAGS)\
	$(EXTRA_ORBIT_CFLAGS)\
	-I$(srcdir)/../scintilla/include \
	-I$(srcdir)/../tagmanager/include \
	-I$(srcdir)/../widgets \
	-I$(srcdir)/../pixmaps \
	-I.. -I.

CFLAGS = -Wall -Wimplicit -Wreturn-type -Wunused -Wswitch -Wcomment\
		 -Wuninitialized -Wparentheses -Wpointer-arith\
		 -Wmissing-prototypes -g -O2

noinst_LIBRARIES = libanjutaprint.a
libanjutaprint_a_SOURCES = print.c  print-doc.c  print-doc.h  print.h  print-util.c  print-util.h

