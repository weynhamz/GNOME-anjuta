anjuta_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@

INCLUDES = \
	-I$(top_srcdir)/intl\
	-I$(srcdir)\
	$(ANJUTA_CFLAGS)\
	-DHAVE_CONFIG_H\
	-DPIXMAPS_DIR=\"$(anjuta_pixmapsdir)\"

noinst_LIBRARIES = libwidgets.a

libwidgets_a_SOURCES = \
	gnomefilelist.h \
	gnomefilelist.c \
	cell-renderer-captioned-image.c \
	cell-renderer-captioned-image.h \
	e-splash.h \
	e-splash.c

