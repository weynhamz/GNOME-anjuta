anjuta_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@

INCLUDES = \
	-I$(top_srcdir)/intl \
	$(ANJUTA_CFLAGS)\
	-DHAVE_CONFIG_H\
	-DPIXMAPS_DIR=\"$(anjuta_pixmapsdir)\"
	-g -O2

noinst_LIBRARIES = libwidgets.a

libwidgets_a_SOURCES = \
	gnomedirlist.c \
	gnomedirlist.h \
	gnomefilelist.c \
	gnomefilelist.h \
	scroll-menu.c \
	scroll-menu.h \
	e-shell-about-box.c \
	e-shell-about-box.h

