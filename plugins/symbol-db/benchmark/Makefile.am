noinst_PROGRAMS = \
	benchmark
	

AM_CPPFLAGS =  $(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
	-DDEBUG

benchmark_SOURCES = \
	benchmark.c
 

benchmark_LDFLAGS = \
	$(LIBANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS)

benchmark_LDADD = ../libanjuta-symbol-db.la

## File created by the gnome-build tools










-include $(top_srcdir)/git.mk
