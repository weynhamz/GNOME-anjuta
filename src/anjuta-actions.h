/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta.c Copyright (C) 2000 Naba Kumar
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <libanjuta/anjuta-stock.h>
#include "action-callbacks.h"

static EggActionGroupEntry menu_entries_file[] = {
  { "ActionMenuFile", N_("_File"), NULL, NULL, NULL, NULL, NULL },
  { "ActionExit", N_("_Quit"), GTK_STOCK_QUIT, "<control>q",
	N_("Quit Anjuta IDE"),
    G_CALLBACK (on_exit1_activate), NULL },
};

static EggActionGroupEntry menu_entries_edit[] = {
  { "ActionMenuEdit", N_("_Edit"), NULL, NULL, NULL, NULL, NULL },
  { "ActionMenuEditGoto", N_("G_oto"), NULL, NULL, NULL, NULL, NULL },
};

static EggActionGroupEntry menu_entries_view[] = {
  { "ActionMenuView", N_("_View"), NULL, NULL, NULL, NULL, NULL },
};

static EggActionGroupEntry menu_entries_settings[] = {
  { "ActionMenuSettings", N_("_Settings"), NULL, NULL, NULL, NULL, NULL },
  { "ActionSettingsPreferences", N_("_Preferences ..."),
	GTK_STOCK_PROPERTIES, NULL,
	N_("Do you prefer coffee to tea? Check it out."),
    G_CALLBACK (on_set_preferences1_activate), NULL},
  { "ActionSettingsDefaults", N_("Set _Default Preferences"),
	GTK_STOCK_PROPERTIES, NULL,
	N_("But I prefer tea."),
    G_CALLBACK (on_set_default_preferences1_activate), NULL},
  { "ActionSettingsShortcuts", N_("C_ustomize shortcuts"),
	NULL, NULL,
	N_("Customize shortcuts associated with menu items"),
    G_CALLBACK (on_customize_shortcuts_activate), NULL},
};

static EggActionGroupEntry menu_entries_help[] = {
  { "ActionMenuHelp", N_("_Help"), NULL, NULL, NULL, NULL, NULL },
  { "ActionHelpGnome", N_("The GNOME API pages"),
	GTK_STOCK_DIALOG_INFO, NULL,
	N_("Browse GNOME _API Pages"),
    G_CALLBACK (on_gnome_pages1_activate), NULL},
  { "ActionHelpMan", N_("Browse _Man Pages"),
	GTK_STOCK_DIALOG_INFO, NULL,
	N_("The good old manual pages"),
    G_CALLBACK (on_url_activate), "man:man"},
  { "ActionHelpInfo", N_("Browse _Info Pages"),
	GTK_STOCK_DIALOG_INFO, NULL,
	N_("The good info pages"),
    G_CALLBACK (on_url_activate), "info:info"},
  { "ActionHelpContext", N_("_Context Help"),
	GTK_STOCK_HELP, "<control>h",
	N_("Search help for the current word in the editor"),
    G_CALLBACK (on_context_help_activate), NULL},
  { "ActionHelpSearch", N_("_Search a topic"), GTK_STOCK_FIND, NULL,
	N_("May I help you?"),
    G_CALLBACK (on_search_a_topic1_activate), NULL},
  { "ActionHelpAnjutaHome", N_("Anjuta _Home Page"), GTK_STOCK_HOME, NULL,
	N_("Online documentation and resources"),
    G_CALLBACK (on_url_activate), "http://anjuta.org"},
  { "ActionHelpLidn", N_("_Libraries API references"), GTK_STOCK_JUMP_TO, NULL,
	N_("Online reference library for GDK, GLib, GNOME etc.."),
    G_CALLBACK (on_url_activate), "http://lidn.sourceforge.net"},
  { "ActionHelpBugReport", N_("Report _Bugs"), GTK_STOCK_JUMP_TO, NULL,
	N_("Submit a bug report for Anjuta"),
    G_CALLBACK (on_url_activate), "http://sourceforge.net/tracker/?atid=114222&group_id=14222&func=browse"},
  { "ActionHelpFeatureRequest", N_("Request _Features"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Submit a feature request for Anjuta"),
    G_CALLBACK (on_url_activate), "http://sourceforge.net/tracker/?atid=364222&group_id=14222&func=browse"},
  { "ActionHelpSubmitPatch", N_("Submit _patches"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Submit patches for Anjuta"),
    G_CALLBACK (on_url_activate), "http://sourceforge.net/tracker/?atid=314222&group_id=14222&func=browse"},
  { "ActionHelpFaq", N_("Ask a _question"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Submit a question for FAQs"),
    G_CALLBACK (on_url_activate), "mailto:anjuta-list@lists.sourceforge.net"},
  { "ActionAboutAnjuta", N_("_About"),
	GNOME_STOCK_ABOUT, NULL,
	N_("About Anjuta"),
    G_CALLBACK (on_about1_activate), NULL},
};
