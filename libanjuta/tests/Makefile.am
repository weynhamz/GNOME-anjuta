noinst_PROGRAMS = anjuta-tabber-test \
		anjuta-token-test 

# Include paths
AM_CPPFLAGS = \
        $(WARN_CFLAGS) \
        $(DEPRECATED_FLAGS) \
        $(GDA_CFLAGS) \
        $(LIBANJUTA_CFLAGS)

anjuta_tabber_test_CFLAGS = $(LIBANJUTA_CFLAGS)
anjuta_tabber_test_LDADD = $(LIBANJUTA_LIBS) $(ANJUTA_LIBS)

anjuta_tabber_test_SOURCES = anjuta-tabber-test.c


anjuta_token_test_CFLAGS = -g -O0 -fprofile-arcs -ftest-coverage
anjuta_token_test_LDADD = $(ANJUTA_LIBS)

anjuta_token_test_SOURCES = anjuta-token-test.c \
			../anjuta-token.c \
			../anjuta-debug.c

CLEANFILES = anjuta_token_test-anjuta-token.gcno \
             anjuta_token_test-anjuta-token-test.gcno \
             anjuta_token_test-anjuta-debug.gcno

