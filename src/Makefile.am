## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES = \
	-I$(top_srcdir)/intl \
	$(GNOME_INCLUDEDIR)\
	-I$(srcdir)/../scintilla/include \
	-I$(srcdir)/../widgets \
	-I$(srcdir)/../ccview \
	-I$(srcdir)/../pixmaps \
	-I.. -I. \
	-Wall -Wimplicit -Wreturn-type -Wunused -Wswitch -Wcomment\
	-Wuninitialized -Wparentheses -Wtraditional -Wpointer-arith\
	-Wmissing-prototypes\
 	-O1\
 	-g

bin_PROGRAMS = anjuta

anjuta_SOURCES = \
	aneditor.cxx\
	aneditor.h\
	anjuta.h\
	anjuta_cbs.c\
	anjuta_dirs.c\
	anjuta_dirs.h\
	anjuta_info.c\
	anjuta_info.h\
	appwiz_page1.c\
	appwiz_page2.c\
	appwiz_page3.c\
	appwizard.c\
	appwizard.h\
	appwizard_cbs.c\
	appwizard_cbs.h\
	appwizard_gui.c\
	attach_process.c\
	attach_process.h\
	attach_process_cbs.c\
	attach_process_cbs.h\
	attach_process_gui.c\
	breakpoints.c\
	breakpoints.h\
	breakpoints_cbs.c\
	breakpoints_cbs.h\
	breakpoints_gui.c\
	build_file.c\
	build_file.h\
	build_project.c\
	build_project.h\
	clean_project.c\
	clean_project.h\
	commands.c\
	commands.h\
	compile.c\
	compile.h\
	compiler_options.c\
	compiler_options.h\
	compiler_options_cbs.c\
	compiler_options_cbs.h\
	compiler_options_gui.c\
	configurer.c\
	configurer.h\
	controls.c\
	controls.h\
	debugger.c\
	debugger.h\
	dnd.c\
	dnd.h\
	executer.c\
	executer.h\
	fileselection.c\
	fileselection.h\
	find_in_files.c\
	find_in_files.h\
	find_in_files_cbs.c\
	find_in_files_cbs.h\
	find_in_files_gui.c\
	find_replace.c\
	find_replace.h\
	find_text.c\
	find_text.h\
	global.h\
	gnome_project.c\
	gnome_project.h\
	goto_line.c\
	goto_line.h\
	help.c\
	help.h\
	launcher.c\
	launcher.h\
	lexer.h\
	main.c\
	main_menubar.c\
	main_menubar.h\
	main_menubar_def.h\
	mainmenu_callbacks.c\
	mainmenu_callbacks.h\
	messagebox.c\
	messagebox.h\
	messages.c\
	messages.h\
	pixmaps.h\
	preferences.c\
	preferences.h\
	preferences_gui.c\
	project_dbase.c\
	project_dbase.h\
	project_config.c\
	project_config.h\
	project_dbase_gui.c\
	properties.cxx\
	properties.h\
	registers.c\
	registers.h\
	registers_cbs.c\
	registers_cbs.h\
	registers_gui.c\
	sharedlib.c\
	sharedlib.h\
	sharedlib_cbs.c\
	sharedlib_cbs.h\
	sharedlib_gui.c\
	signals.c\
	signals.h\
	signals_cbs.c\
	signals_cbs.h\
	signals_gui.c\
	splash.c\
	splash.h\
	src_paths.c\
	src_paths.h\
	src_paths_cbs.c\
	src_paths_cbs.h\
	src_paths_gui.c\
	stack_trace.c\
	stack_trace.h\
	stack_trace_cbs.c\
	stack_trace_cbs.h\
	stack_trace_gui.c\
	syntax.h\
	tags_manager.c\
	tags_manager.h\
	text_editor.c\
	text_editor.h\
	text_editor_cbs.c\
	text_editor_cbs.h\
	text_editor_gui.c\
	text_editor_gui.h\
	text_editor_menu.c\
	text_editor_menu.h\
	toolbar.c\
	toolbar.h\
	toolbar_callbacks.c\
	toolbar_callbacks.h\
	utilities.c\
	utilities.h\
	watch.c\
	watch.h\
	watch_cbs.c\
	watch_cbs.h\
	watch_gui.c\
	about.c\
	appwiz_page4.c\
	source.c\
	anjuta.c\
	anjuta_gui.c\
	source.h\
	glade_iface.c\
	glade_iface.h\
	compatibility_0.c\
	compatibility_0.h\
	defaults.c\
	defaults.h\
	about.h\
	resources.c\
	resources.h\
	notebook.c\
	notebook.h\
	CORBA-Server.c\
	CORBA-Server.h\
	Gladen-common.c\
	Gladen.h\
	Prj.h\
	glades.c\
	Prj-common.c\
	Gladen-stubs.c\
	session.c\
	session.h

anjuta_LDADD =  \
	$(INTLLIBS) \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS)\
	-L../scintilla -L../widgets -L../ccview\
	../scintilla/LexCPP.o ../scintilla/LexConf.o ../scintilla/LexHTML.o\
	../scintilla/LexLua.o ../scintilla/LexOthers.o ../scintilla/LexPerl.o\
	../scintilla/LexPython.o ../scintilla/LexSQL.o ../scintilla/LexVB.o\
	../scintilla/LexPascal.o ../scintilla/LexAVE.o -lwidgets -lccview -lxml\
	-lscintilla -lORBit -lIIOP -lORBitutil -lgnorba
