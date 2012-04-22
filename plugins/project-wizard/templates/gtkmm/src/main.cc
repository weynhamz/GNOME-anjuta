[+ autogen5 template +]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
[+INCLUDE (string-append "indent.tpl") \+]
/* [+INVOKE EMACS-MODELINE MODE="C" \+] */
[+INVOKE START-INDENT\+]
/*
 * main.cc
 * Copyright (C) [+(shell "date +%Y")+] [+Author+] <[+Email+]>
 * 
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "Name") OWNER=(get "Author") \+]
 */

#include <gtkmm.h>
#include <iostream>

#include "config.h"

[+IF (=(get "HaveI18n") "1")+]
#ifdef ENABLE_NLS
#  include <libintl.h>
#endif
[+ENDIF+]

[+IF (=(get "HaveBuilderUI") "1")+]
/* For testing propose use the local (not installed) ui file */
/* #define UI_FILE PACKAGE_DATA_DIR"/ui/[+NameHLower+].ui" */
#define UI_FILE "src/[+NameHLower+].ui"
[+ENDIF+]
   
int
main (int argc, char *argv[])
{
	Gtk::Main kit(argc, argv);

[+IF (=(get "HaveBuilderUI") "1")+]
	//Load the Glade file and instiate its widgets:
	Glib::RefPtr<Gtk::Builder> builder;
	try
	{
		builder = Gtk::Builder::create_from_file(UI_FILE);
	}
	catch (const Glib::FileError & ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	Gtk::Window* main_win = 0;
	builder->get_widget("main_window", main_win);
[+ELSE+]
	Gtk::Window* main_win = new Gtk::Window (Gtk::WINDOW_TOPLEVEL);
	main_win->set_title ("[+Name+]");
[+ENDIF+]

	if (main_win)
	{
		kit.run(*main_win);
	}
	return 0;
}
[+INVOKE END-INDENT\+]
