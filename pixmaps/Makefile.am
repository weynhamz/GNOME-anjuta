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
	anjuta_logo.png \
	anjuta_splash.png \
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
