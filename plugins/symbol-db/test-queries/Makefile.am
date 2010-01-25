noinst_PROGRAMS = \
	test-queries
	

AM_CPPFLAGS = \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(GDL_CFLAGS) \
	-DDEBUG

test_queries_SOURCES = \
	main.c
 

test_queries_LDFLAGS = \
	$(LIBANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS)

test_queries_LDADD = ../libanjuta-symbol-db.la

## File created by the gnome-build tools










-include $(top_srcdir)/git.mk
