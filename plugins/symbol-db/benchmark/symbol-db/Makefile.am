noinst_PROGRAMS = \
	benchmark


AM_CPPFLAGS =  $(LIBANJUTA_CFLAGS) \
	$(GDA_CFLAGS) \
	-DDEBUG

benchmark_SOURCES = \
	benchmark.c


benchmark_LDFLAGS = \
	$(LIBANJUTA_LIBS) \
	$(ANJUTA_LIBS) \
	$(GDA_LIBS)

benchmark_LDADD = ../../libanjuta-symbol-db.la


## File created by the gnome-build tools










-include $(top_srcdir)/git.mk
