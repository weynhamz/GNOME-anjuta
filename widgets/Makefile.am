anjuta_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@

INCLUDES = \
	-I$(top_srcdir)/intl\
	-I$(top_srcdir)\
	-I$(srcdir)\
	$(ANJUTA_CFLAGS)\
	-DHAVE_CONFIG_H\
	-DPIXMAPS_DIR=\"$(anjuta_pixmapsdir)\"
#	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
#	-DGDK_PIXBUF_DEPRECATED \
#	-DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED

noinst_LIBRARIES = libwidgets.a

libwidgets_a_SOURCES = \
	gnomefilelist.h \
	gnomefilelist.c \
	cell-renderer-captioned-image.c \
	cell-renderer-captioned-image.h \
	e-splash.h \
	e-splash.c
