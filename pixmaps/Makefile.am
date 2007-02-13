## Process this file with automake to produce Makefile.in

icondir = $(datadir)/icons/hicolor/48x48/apps
icon_DATA = anjuta.png
svgicondir = $(datadir)/icons/hicolor/scalable/apps
svgicon_DATA = anjuta.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
	echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

anjuta_pixmapsdir = $(anjuta_image_dir)

anjuta_pixmaps_DATA = \
	anjuta.png \
	anjuta_logo1.png \
	anjuta_logo2.png \
	anjuta_splash.png \
	anjuta_splash_mini.png \
	applogo.png \
	appwizard.png \
	appwiz_bonobo2.png \
	appwiz_bonobo.png \
	appwiz_gnome2.png \
	appwiz_gnomemm2.png \
	appwiz_gnomemm.png \
	appwiz_gnome.png \
	appwiz_gtk2.png \
	appwiz_gtkmm2.png \
	appwiz_gtkmm.png \
	appwiz_gtk.png \
	appwiz_libglade2.png \
	appwiz_libglade.png \
	appwiz_opengl.png \
	appwiz_sdl.png \
	appwiz_terminal.png \
	appwiz_wxwin.png \
	appwiz_xlib.png \
	close_project.png \
	directory.png \
	directory-accept.png \
	password.png \
	preferences-general.png

anjuta_glade_pixmapsdir = $(anjuta_glade_dir)
anjuta_glade_pixmaps_DATA = \
	start-with-wizard.png \
	start-with-import.png \
	start-with-open.png

EXTRA_DIST = $(anjuta_pixmaps_DATA) \
						$(anjuta_glade_pixmaps_DATA) \
						$(icon_DATA) \
						$(svgicon_DATA)

