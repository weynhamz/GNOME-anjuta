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

static GtkActionEntry menu_entries_file[] = {
  { "ActionMenuFile", NULL, N_("_File")},
  { "ActionExit", GTK_STOCK_QUIT, N_("_Quit"), "<control>q",
	N_("Quit Anjuta IDE"),
    G_CALLBACK (on_exit1_activate)}
};

static GtkActionEntry menu_entries_edit[] = {
  { "ActionMenuEdit", NULL, N_("_Edit")}
};

static GtkActionEntry menu_entries_view[] = {
  { "ActionMenuView", NULL, N_("_View")},
  { "ActionViewToolbars", NULL, N_("_Toolbars")}
};

static GtkActionEntry menu_entries_settings[] = {
  { "ActionMenuSettings", NULL, N_("_Settings")},
  { "ActionSettingsPreferences", GTK_STOCK_PROPERTIES, 
	N_("_Preferences ..."), NULL,
	N_("Do you prefer coffee to tea? Check it out."),
    G_CALLBACK (on_set_preferences1_activate)},
  { "ActionSettingsDefaults", GTK_STOCK_PROPERTIES,
	N_("Set _Default Preferences"), NULL,
	N_("But I prefer tea."),
    G_CALLBACK (on_set_default_preferences1_activate)},
  { "ActionSettingsShortcuts", NULL,
    N_("C_ustomize shortcuts"), NULL,
	N_("Customize shortcuts associated with menu items"),
    G_CALLBACK (on_customize_shortcuts_activate)},
  { "ActionSettingsLayout", NULL,
    N_("Layout manager"), NULL,
	N_("Manipulate layout manager items"),
    G_CALLBACK (on_layout_manager_activate)},
  { "ActionSettingsPlugins", NULL,
    N_("Plugins manager"), NULL,
	N_("Manipulate plugins manager items"),
    G_CALLBACK (on_show_plugins_activate)}
};

static GtkActionEntry menu_entries_help[] = {
  { "ActionMenuHelp", NULL, N_("_Help")},
  { "ActionHelpMan", GTK_STOCK_DIALOG_INFO,
    N_("Browse _Man Pages"), NULL,
	N_("The good old manual pages"),
    G_CALLBACK (on_url_man_activate)},
  { "ActionHelpInfo", GTK_STOCK_DIALOG_INFO,
    N_("Browse _Info Pages"), NULL,
	N_("The good info pages"),
    G_CALLBACK (on_url_info_activate)},
  { "ActionHelpAnjutaHome", GTK_STOCK_HOME,
    N_("Anjuta _Home Page"), NULL,
	N_("Online documentation and resources"),
    G_CALLBACK (on_url_home_activate)},
  { "ActionHelpLidn", GTK_STOCK_JUMP_TO,
    N_("_Libraries API references"), NULL,
	N_("Online reference library for GDK, GLib, GNOME etc.."),
    G_CALLBACK (on_url_libs_activate)},
  { "ActionHelpBugReport", GTK_STOCK_JUMP_TO,
    N_("Report _Bugs"), NULL,
	N_("Submit a bug report for Anjuta"),
    G_CALLBACK (on_url_bugs_activate)},
  { "ActionHelpFeatureRequest", GTK_STOCK_JUMP_TO,
    N_("Request _Features"), NULL,
	N_("Submit a feature request for Anjuta"),
    G_CALLBACK (on_url_features_activate)},
  { "ActionHelpSubmitPatch", GTK_STOCK_JUMP_TO,
    N_("Submit _patches"), NULL,
	N_("Submit patches for Anjuta"),
    G_CALLBACK (on_url_patches_activate)},
  { "ActionHelpFaq", GTK_STOCK_JUMP_TO,
    N_("Ask a _question"), NULL,
	N_("Submit a question for FAQs"),
    G_CALLBACK (on_url_faqs_activate)},
  { "ActionAboutAnjuta", GNOME_STOCK_ABOUT,
    N_("_About"), NULL,
	N_("About Anjuta"),
    G_CALLBACK (on_about1_activate)}
};
