## Process this file with automake to produce Makefile.in

anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA =  anjuta.ui

INCLUDES = \
	$(ANJUTA_CFLAGS)\
	$(PCRE_CFLAGS)\
	$(FREEBSD_BUILD)\
	-I$(top_srcdir)\
	-I$(srcdir)/../scintilla/include \
	-I$(srcdir)/../tagmanager/include \
	-I$(srcdir)/../widgets \
	-I$(srcdir)/../pixmaps \
	-I$(srcdir)/../printing \
	-I.. -I. -DPACKAGE_BIN_DIR=\"$(bindir)\"
#	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
#	-DGDK_PIXBUF_DEPRECATED \
#	-DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED

include $(top_srcdir)/scintilla/lexers.make

bin_PROGRAMS = anjuta

anjuta_SOURCES = \
	about.c\
	about.h\
	anjuta-app.c\
	anjuta-app.h\
	preferences.c\
	preferences.h\
	main.cpp
#	an_file_view.c\
#	an_file_view.h\
#	an_symbol_view.c\
#	an_symbol_view.h\
#	anjuta-tools.c\
#	anjuta-tools.h\
#	anjuta-plugins.c\
#	anjuta-plugins.h\
#	aneditor.cxx\
#	aneditor.h\
#	anjuta.h\
#	anjuta_cbs.c\
#	anjuta_dirs.c\
#	anjuta_dirs.h\
#	anjuta-encodings.c\
#	anjuta-encodings.h\
#	anjuta_info.c\
#	anjuta_info.h\
#	anjuta-marshalers.c\
#	anjuta-marshalers.h\
#	appwiz_page1.c\
#	appwiz_page2.c\
#	appwiz_page3.c\
#	appwizard.c\
#	appwizard.h\
#	appwizard_cbs.c\
#	appwizard_cbs.h\
#	appwizard_gui.c\
#	attach_process.c\
#	attach_process.h\
#	breakpoints.c\
#	breakpoints.h\
#	build_file.c\
#	build_file.h\
#	build_project.c\
#	build_project.h\
#	clean_project.c\
#	clean_project.h\
#	commands.c\
#	commands.h\
#	compile.c\
#	compile.h\
#	compiler_options.c\
#	compiler_options.h\
#	configurer.c\
#	configurer.h\
#	controls.c\
#	controls.h\
#	cvs.c\
#	cvs.h\
#	cvs_gui.c\
#	cvs_gui.h\
#	cvs_cbs.c\
#	cvs_cbs.h\
#	debug_tree.c\
#	debug_tree.h\
#	debugger.c\
#	debugger.h\
#	debug_tree.h\
#	dnd.c\
#	dnd.h\
#	executer.c\
#	executer.h\
#	fileselection.c\
#	fileselection.h\
#	getline.c\
#	global.h\
#	gnome_project.c\
#	gnome_project.h\
#	goto_line.c\
#	goto_line.h\
#	help.c\
#	help.h\
#	launcher.c\
#	launcher.h\
#	lexer.h\
#	main.c\
#	main_menubar.c\
#	main_menubar.h\
#	main_menubar_def.h\
#	mainmenu_callbacks.c\
#	mainmenu_callbacks.h\
#	memory.c\
#	memory.h\
#	message-manager.cc\
#	message-manager.h\
#	message-manager-private.cc\
#	message-manager-private.h\
#	message-manager-dock.c\
#	message-manager-dock.h\
#	pixmaps.h\
#	preferences.c\
#	preferences.h\
#	project_config.c\
#	project_config.h\
#	project_dbase.c\
#	project_dbase.h\
#	project_type.c\
#	project_type.h\
#	project_dbase_gui.c\
#	project_import.h\
#	project_import.c\
#	project_import_gui.c\
#	project_import_cbs.h\
#	project_import_cbs.c\
#	properties.cxx\
#	properties.h\
#	registers.c\
#	registers.h\
#	registers_cbs.c\
#	registers_cbs.h\
#	registers_gui.c\
#	sharedlib.c\
#	sharedlib.h\
#	sharedlib_cbs.c\
#	sharedlib_cbs.h\
#	sharedlib_gui.c\
#	signals.c\
#	signals.h\
#	signals_cbs.c\
#	signals_cbs.h\
#	signals_gui.c\
#	src_paths.c\
#	src_paths.h\
#	stack_trace.c\
#	stack_trace.h\
#	support.c\
#	support.h\
#	text_editor.c\
#	text_editor.h\
#	text_editor_cbs.c\
#	text_editor_cbs.h\
#	text_editor_gui.c\
#	text_editor_gui.h\
#	text_editor_menu.c\
#	text_editor_menu.h\
#	toolbar.c\
#	toolbar.h\
#	toolbar_callbacks.c\
#	toolbar_callbacks.h\
#	utilities.c\
#	utilities.h\
#	watch.c\
#	watch.h\
#	watch_cbs.c\
#	watch_cbs.h\
#	watch_gui.c\
#	watch_gui.h\
#	wizard_gui.h\
#	wizard_gui.c\
#	appwiz_page4.c\
#	source.c\
#	anjuta.c\
#	anjuta_gui.c\
#	source.h\
#	glade_iface.c\
#	glade_iface.h\
#	compatibility_0.c\
#	compatibility_0.h\
#	session.c\
#	session.h\
#	start-with.c\
#	start-with.h\
#	style-editor.c\
#	style-editor.h\
#	widget-registry.h\
#	widget-registry.c

anjuta_LDADD =  \
	$(ANJUTA_LIBS) -lutil\
	$(PCRE_LIBS)\
	$(top_builddir)/tagmanager/libctags.a\
	$(top_builddir)/widgets/libwidgets.a\
	$(top_builddir)/printing/libanjutaprint.a\
	$(top_builddir)/libegg/treeviewutils/libegg-treeview.a\
	$(top_builddir)/libegg/menu/libegg-menu.a\
	$(top_builddir)/libegg/toolbar/libegg-toolbar.a\
	$(top_builddir)/libegg/util/libeggutil.a\
	$(top_builddir)/libanjuta/libanjuta.la\
	$(top_builddir)/scintilla/libscintilla.a

#	$(LEXER_OBJS) \
#	$(top_builddir)/gdl/libgdl.a\

anjuta_DEPENDENCIES = \
	$(top_builddir)/libanjuta/libanjuta.la
#	$(top_builddir)/libegg/menu/libegg-menu.a \
#	$(top_builddir)/libegg/treeviewutils/libegg-treeview.a
