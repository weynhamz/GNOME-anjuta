[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$([+NameCUpper+]_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

libexec_PROGRAMS = [+NameHLower+]

[+NameCLower+]_SOURCES = \
	main.c

[+NameCLower+]_LDFLAGS = \
	-Wl,--export-dynamic

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

serverdir       = $(libdir)/bonobo/servers
server_in_files = [+Name+].server.in
server_DATA     = $(server_in_files:.server.in=.server)

CLEANFILES = $(server_in_files) $(server_DATA)

EXTRA_DIST = \
	[+Name+].server.in.in

[+IF (=(get "HaveI18n") "1")+]
%.server: %.server.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po)
	LC_ALL=C $(INTLTOOL_MERGE) -s -u -c $(top_buildir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@
[+ELSE+]
%.server: %.server.in
	cp $< $@
[+ENDIF+]

# Display a message at the end of install
install-data-hook:
	@echo "***"
	@echo "*** If you have installed your applet in a non standard directory. You have to"
	@echo "*** add it in the bonobo server list, so it can find your applet."
	@echo "***"
	@echo "*** Run 'bonobo-activation-sysconf --add-directory=$(serverdir)'"
	@echo "*** Log out from your current GNOME session"
	@echo "*** Run 'bonobo-slay'"
	@echo "*** Check that the bonobo server is not running"
	@echo "*** Log in again, your applet should be available"
	@echo "***"
