SUBDIRS = 16x16 22x22 24x24 32x32 48x48 scalable

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
	echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

anjuta_pixmapsdir = $(anjuta_image_dir)

dist_anjuta_pixmaps_DATA = \
	madeinanjuta.xcf \
	anjuta_logo.png \
	anjuta_splash.png \
	directory.png \
	directory-accept.png \
	password.png \
	start-with-import.png 

anjuta_glade_pixmapsdir = $(anjuta_glade_dir)
dist_anjuta_glade_pixmaps_DATA = \
	start-with-wizard.png \
	start-with-import.png \
	start-with-open.png


-include $(top_srcdir)/git.mk
