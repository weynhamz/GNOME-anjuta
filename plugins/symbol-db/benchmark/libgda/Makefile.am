noinst_PROGRAMS = \
	benchmark-libgda


AM_CPPFLAGS =  $(LIBANJUTA_CFLAGS) \
	$(GDA_CFLAGS) \
	-DDEBUG

benchmark_libgda_SOURCES = \
	libgda.c


benchmark_libgda_LDFLAGS = \
	$(LIBANJUTA_LIBS) \
	$(ANJUTA_LIBS) \
	$(GDA_LIBS)

benchmark_libgda_LDADD = ../../libanjuta-symbol-db.la


## File created by the gnome-build tools










-include $(top_srcdir)/git.mk
