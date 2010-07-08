noinst_PROGRAMS = \
	benchmark-sqlite


AM_CPPFLAGS =  $(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
	-DDEBUG

benchmark_sqlite_SOURCES = \
	sqlite.c

benchmark_sqlite_LDFLAGS = \
	$(LIBANJUTA_LIBS) \
	$(ANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS) \
	`pkg-config --libs sqlite3`
	
benchmark_libgda_LDADD = ../../libanjuta-symbol-db.la


## File created by the gnome-build tools










-include $(top_srcdir)/git.mk
