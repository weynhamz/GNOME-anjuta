
EXTRA_DIST = testsuite.at $(TESTSUITE_AT) $(srcdir)/package.m4 $(TESTSUITE) \
	atlocal.in

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	rm -f $@ $@.tmp
	{ \
	  echo '# Signature of the current package.'; \
	  echo 'm4_define([AT_PACKAGE_NAME],      [$(PACKAGE_NAME)])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [$(PACKAGE_TARNAME)])'; \
  	  echo 'm4_define([AT_PACKAGE_VERSION],   [$(PACKAGE_VERSION)])'; \
  	  echo 'm4_define([AT_PACKAGE_STRING],    [$(PACKAGE_STRING)])'; \
 	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])'; \
	} >$@.tmp
	mv $@.tmp $@


TESTSUITE_AT = \
	$(srcdir)/empty.at \
	$(srcdir)/anjuta.at \
	$(srcdir)/group.at \
	$(srcdir)/target.at \
	$(srcdir)/source.at \
	$(srcdir)/parser.at \
	$(srcdir)/acinit.at \
	$(srcdir)/include.at \
	$(srcdir)/variable.at \
	$(srcdir)/properties.at \
	$(srcdir)/module.at \
	$(srcdir)/package.at \
	$(srcdir)/target_module.at \
	$(srcdir)/data_target.at

TESTSUITE = $(srcdir)/testsuite

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(srcdir)/testsuite.at $(srcdir)/package.m4 $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) -o $@.tmp $@.at
	mv $@.tmp $@

clean-local: 
	test ! -f $(TESTSUITE) || $(SHELL) $(TESTSUITE) --clean

check-local: atconfig $(TESTSUITE)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)
