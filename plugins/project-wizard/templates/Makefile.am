
SUBDIRS = minimal terminal cpp gtk anjuta-plugin anjuta-plugin-vala \
	gtkmm wxwin xlib xlib-dock gcj java \
	python mkfile sdl licenses m4

wizard_filesdir = $(anjuta_data_dir)/project
wizard_files_DATA = \
	minimal.wiz \
	terminal.wiz \
	terminal-logo.png \
	cpp.wiz \
	cpp-logo.png \
	django.wiz \
	django-logo.png \
	gtk.wiz \
	gtk-logo.png \
	anjuta-plugin.wiz \
	anjuta-plugin-vala.wiz \
	anjuta-plugin-logo.png \
	gnome-logo.png \
	gtkmm.wiz \
	gtkmm-logo.png \
	wxwin.wiz \
	wxwin-logo.png \
	xlib.wiz \
	xlib-dock.wiz \
	xlib-logo.png \
	gcj.wiz \
	gcj-logo.png \
	java.wiz \
	java-logo.png \
	python.wiz \
	python-logo.png \
	mkfile.wiz \
	mkfile-logo.png \
	sdl.wiz \
	appwiz_sdl.png \
	gnome-applet.wiz


BUILT_SOURCES = translatable-strings.h extract-translatable-strings.pl
translatable-strings.h: $(wizard_files_DATA)
	perl $(srcdir)/extract-translatable-strings.pl $(wizard_files_DATA) > translatable-strings.h

EXTRA_DIST = $(wizard_files_DATA) $(BUILT_SOURCES)
