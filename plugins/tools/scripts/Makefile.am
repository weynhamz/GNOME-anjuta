## File created by the gnome-build tools


toolsdir = $(anjuta_data_dir)/tools
dist_tools_SCRIPTS = \
	prepare-changelog.pl \
	find-fixmes.pl \
	translation-status.pl
dist_tools_DATA = tools-2.xml

EXTRA_DIST = \
	tools-2.xml.in

# from the autoconf manual
edit = sed \
             -e 's|@ANJUTA_DATA_DIR[@]|$(anjuta_data_dir)|g'
     
tools-2.xml: Makefile
	$(AM_V_at)rm -f $@ $@.tmp
	$(AM_V_at)$(edit) '$(srcdir)/$@.in' >$@.tmp
	$(AM_V_at)chmod +x $@.tmp
	$(AM_V_at)chmod a-w $@.tmp
	$(AM_V_GEN)mv $@.tmp $@
     
tools-2.xml: tools-2.xml.in

CLEANFILES = tools-2.xml
-include $(top_srcdir)/git.mk
