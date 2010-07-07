noinst_PROGRAMS = \
	benchmark-libgda


AM_CPPFLAGS =  $(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
	-DDEBUG

benchmark_libgda_SOURCES = \
	libgda.c


benchmark_libgda_LDFLAGS = \
	$(LIBANJUTA_LIBS) \
	$(ANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS)

benchmark_libgda_LDADD = ../../libanjuta-symbol-db.la


## File created by the gnome-build tools










-include $(top_srcdir)/git.mk
