
SUBDIRS = minimal terminal cpp gtk anjuta-plugin \
	gtkmm wxwin xlib xlib-dock gcj java
	python mkfile sdl library directory licenses m4 js_minimal

template_in_files = \
	minimal.wiz.in \
	terminal.wiz.in \
	cpp.wiz.in \
	django.wiz.in \
	gtk.wiz.in \
	anjuta-plugin.wiz.in \
	gtkmm.wiz.in \
	wxwin.wiz.in \
	xlib.wiz.in \
	xlib-dock.wiz.in \
	gcj.wiz.in \
	java.wiz.in \
	js.wiz.in \
	python.wiz.in \
	mkfile.wiz.in \
	sdl.wiz.in \
	library.wiz.in

wizard_filesdir = $(anjuta_data_dir)/project
wizard_files_DATA = \
	terminal-logo.png \
	cpp-logo.png \
	django-logo.png \
	gtk-logo.png \
	anjuta-plugin-logo.png \
	gnome-logo.png \
	gtkmm-logo.png \
	wxwin-logo.png \
	xlib-logo.png \
	gcj-logo.png \
	java-logo.png \
	python-logo.png \
	mkfile-logo.png \
	appwiz_sdl.png \
	$(template_in_files:.wiz.in=.wiz)

# Translate wiz file
%.wiz: %.wiz.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -x -u -c $(top_builddir)/po/.intltool-merge-cache

EXTRA_DIST = $(wizard_files_DATA) $(template_in_files)

-include $(top_srcdir)/git.mk
