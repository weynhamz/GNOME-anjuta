INCLUDES = \
	$(ANJUTA_CFLAGS)\
	-I$(top_srcdir)\
	-I$(top_srcdir)/src \
	-I$(srcdir)/../scintilla/include \
	-I$(srcdir)/../tagmanager/include \
	-I$(srcdir)/../widgets \
	-I$(srcdir)/../pixmaps \
	-I.. -I.
#	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
#	-DGDK_PIXBUF_DEPRECATED \
#	-DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED

noinst_LIBRARIES = libanjutaprint.a
libanjutaprint_a_SOURCES = \
	print.c\
	print.h
