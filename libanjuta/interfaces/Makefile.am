INCLUDES = \
	$(WARN_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(DEPRECATED_FLAGS)

lib_LTLIBRARIES=libanjuta-interfaces.la

libanjuta_interfaces_la_LIADD = $(LIBANJUTA_LIBS)
libanjuta_interfaces_la_LIBADD =

libanjuta_interfaces_la_SOURCES= \
	ianjuta-editor.c \
	ianjuta-document-manager.c \
	ianjuta-message-view.c \
	ianjuta-message-manager.c \
	ianjuta-file.c

libanjuta_interfaces_includedir=$(includedir)/libanjuta-1.0/libanjuta/interfaces
libanjuta_interfaces_include_HEADERS = \
	ianjuta-editor.h \
	ianjuta-document-manager.h \
	ianjuta-message-view.h \
	ianjuta-message-manager.h \
	ianjuta-file.h
