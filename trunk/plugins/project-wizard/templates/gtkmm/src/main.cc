[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.cc
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  "main.cc" (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl "main.cc" (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  "main.cc"                " * ")+]
[+ESAC+] */

#include <libglademm/xml.h>
#include <gtkmm.h>
#include <iostream>

[+IF (=(get "HaveI18n") "1")+]
#ifdef ENABLE_NLS
#  include <libintl.h>
#endif
[+ENDIF+]

/* For testing propose use the local (not installed) glade file */
/* #define GLADE_FILE PACKAGE_DATA_DIR"/[+NameHLower+]/glade/[+NameHLower+].glade" */
#define GLADE_FILE "[+NameHLower+].glade"
   
int
main (int argc, char *argv[])
{
	Gtk::Main kit(argc, argv);
	
	//Load the Glade file and instiate its widgets:
	Glib::RefPtr<Gnome::Glade::Xml> refXml;
	try
	{
		refXml = Gnome::Glade::Xml::create(GLADE_FILE);
	}
	catch(const Gnome::Glade::XmlError& ex)
    {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	Gtk::Window* main_win = 0;
	refXml->get_widget("main_window", main_win);
	if (main_win)
	{
		kit.run(*main_win);
	}
	return 0;
}
