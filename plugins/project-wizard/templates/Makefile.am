
SUBDIRS = minimal terminal cpp gtk gtkapplication anjuta-plugin \
	gtkmm wxwin xlib xlib-dock gcj java pygtk \
	python mkfile sdl library directory m4 js_minimal \
	vala-gtk gnome-shell-extension

template_in_files = \
	minimal.wiz.in \
	terminal.wiz.in \
	cpp.wiz.in \
	django.wiz.in \
	gtk.wiz.in \
	gtkapplication.wiz.in \
	anjuta-plugin.wiz.in \
	gtkmm.wiz.in \
	wxwin.wiz.in \
	xlib.wiz.in \
	xlib-dock.wiz.in \
	gcj.wiz.in \
	java.wiz.in \
	js.wiz.in \
	python.wiz.in \
	pygtk.wiz.in \
	mkfile.wiz.in \
	sdl.wiz.in \
	library.wiz.in \
	vala-gtk.wiz.in \
	gnome-shell-extension.wiz.in \
	licenses.tpl.in

wizard_filesdir = $(anjuta_data_dir)/templates
wizard_files_DATA = \
	terminal-logo.png \
	cpp-logo.png \
	django-logo.png \
	gtk-logo.png \
	anjuta-plugin-logo.png \
	gnome-logo.png \
	wxwin-logo.png \
	xlib-logo.png \
	gcj-logo.png \
	java-logo.png \
	python-logo.png \
	pygtk-logo.png \
	mkfile-logo.png \
	appwiz_sdl.png \
	gnome.png \
	gnome.svg \
	$(template_in_files:.in=)

# Translate wiz file
%.wiz: %.wiz.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -x -u -c $(top_builddir)/po/.intltool-merge-cache

%.tpl: %.tpl.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po)
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@_ -x -u -c $(top_builddir)/po/.intltool-merge-cache
	echo "[+ autogen5 template +]" | cat - $@_ > $@;
	rm $@_
	

EXTRA_DIST = $(wizard_files_DATA) $(template_in_files)

-include $(top_srcdir)/git.mk
