INCLUDES= \
	$(WARN_CFLAGS) \
	$(PCRE_CFLAGS)\
	$(DEPRECATED_FLAGS)\
	$(LIBANJUTA_CFLAGS)

lib_LTLIBRARIES = libanjuta-message-view.la

libanjuta_message_view_la_LIADD = $(LIBANJUTA_LIBS)
libanjuta_message_view_la_LIBADD = \
	$(top_builddir)/libanjuta/libanjuta.la

libanjuta_message_view_la_SOURCES = \
	plugin.c\
	plugin.h\
	message-view.c\
	message-view.h
