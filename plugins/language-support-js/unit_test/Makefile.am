include Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS        += utest

utest_SOURCES = test.c ../plugin.h ../code-completion.c ../code-completion.h ../util.c ../util.h \
../y.tab.c ../y.tab.h ../lex.yy.c ../lex.yy.h ../jsparse.c ../jsparse.h ../ijs-symbol.h ../ijs-symbol.c ../gir-symbol.c ../gir-symbol.h ../gi-symbol.c ../gi-symbol.h ../simple-symbol.c ../simple-symbol.h ../local-symbol.c ../local-symbol.h  ../node-symbol.c ../node-symbol.h ../import-symbol.c ../import-symbol.h ../dir-symbol.c ../dir-symbol.h ../std-symbol.c ../std-symbol.h ../database-symbol.c ../database-symbol.h ../db-anjuta-symbol.c ../db-anjuta-symbol.h
utest_CPPFLAGS = \
	-DGIR_PATH=\"$(girdir)\" \
	-DGJS_PATH=\"$(gjsdir)\" \
	$(LIBANJUTA_CFLAGS) 

utest_LDADD = \
	$(LIBANJUTA_LIBS) 

-include $(top_srcdir)/git.mk
