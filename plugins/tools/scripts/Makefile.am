## File created by the gnome-build tools


toolsdir = $(anjuta_data_dir)/tools
tools_SCRIPTS = \
	prepare-changelog.pl \
	find-fixmes.pl \
	translation-status.pl
tools_DATA = tools-2.xml

EXTRA_DIST = \
	$(tools_SCRIPTS) \
	$(tools_DATA) \
	tools-2.xml.in

# from the autoconf manual
edit = sed \
             -e 's|@ANJUTA_DATA_DIR[@]|$(anjuta_data_dir)|g'
     
tools-2.xml: Makefile
	rm -f $@ $@.tmp
	$(edit) '$(srcdir)/$@.in' >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@
     
tools-2.xml: tools-2.xml.in

