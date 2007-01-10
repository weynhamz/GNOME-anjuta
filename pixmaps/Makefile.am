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
	blank.xpm \
	breakpoint.xpm \
	build_all.xpm \
	build.xpm \
	calltip.xpm \
	close_file_small.xpm \
	close_project.png \
	compile.xpm \
	configure.xpm \
	continue.xpm \
	debug_stop.xpm \
	debug.xpm \
	dock.xpm \
	directory.png \
	directory-accept.png \
	password.png \
	preferences-general.png

#	frame.xpm \
#	handle_hide.xpm \
#	handle_undock.xpm \
#	help.xpm \
#	inspect.xpm \
#	interrupt.xpm \
#	madeinanjuta.xcf\
#	messages.xpm \
#	mini_build.xpm \
#	mini_cvs.xpm \
#	mini_debug.xpm \
#	mini_dock.xpm \
#	mini_find.xpm \
#	mini_locals.xpm \
#	mini-modules.xpm \
#	mini_term.xpm \
#	new_folder.xpm \
#	open_project.png \
#	pointer.xpm \
#	preferences-build.png\
#	preferences-encodings.png\
#	preferences-indentation.png\
#	preferences-cvs.png\
#	preferences-print.png\
#	project.xpm \
#	registers.xpm \
#	run_to_cursor.xpm \
#	save_project.png \
#	stack.xpm \
#	step_in.xpm \
#	step_out.xpm \
#	step_over.xpm \
#	sv_unknown.xpm \
#	sv_class.xpm \
#	sv_function.xpm \
#	sv_macro.xpm \
#	sv_private_fun.xpm \
#	sv_private_var.xpm \
#	sv_protected_fun.xpm \
#	sv_protected_var.xpm \
#	sv_public_fun.xpm \
#	sv_public_var.xpm \
#	sv_static_fun.xpm \
#	sv_static_var.xpm \
#	sv_struct.xpm \
#	sv_unknown.xpm \
#	sv_variable.xpm \
#	syntax.xpm \
#	watch.xpm \
#	wizard.xpm

anjuta_glade_pixmapsdir = $(anjuta_glade_dir)
anjuta_glade_pixmaps_DATA = \
	start-with-wizard.png \
	start-with-import.png \
	start-with-open.png

EXTRA_DIST = $(anjuta_pixmaps_DATA) \
						$(anjuta_glade_pixmaps_DATA) \
						$(icon_DATA) \
						$(svgicon_DATA)

