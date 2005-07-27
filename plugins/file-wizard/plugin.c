/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <gtk/gtkactiongroup.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include "action-callbacks.h"
#include "file.h"
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-file-wizard.ui"
#define ICON_FILE "anjuta-file-wizard-plugin.png"

/*
static void
on_insert_custom_indent (GtkAction *action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
    aneditor_command (te->editor_id, ANE_CUSTOMINDENT, 0, 0);
}
*/
static GtkActionEntry actions_insert[] = {
  { "ActionMenuEditInsert", N_("_Insert text"), NULL, NULL, NULL, NULL},
  { "ActionMenuEditInsertCtemplate", N_("C _template"), NULL, NULL, NULL, NULL},
  { "ActionMenuEditInsertCvs", N_("_CVS keyword"), NULL, NULL, NULL, NULL},
  { "ActionMenuEditInsertGeneral", N_("_General"), NULL, NULL, NULL, NULL},
  { "ActionEditInsertHeader", N_("_Header"), NULL, NULL,
	N_("Insert a file header"),
    G_CALLBACK (on_insert_header)},
  { "ActionEditInsertCGPL", N_("/_* GPL Notice */"), NULL, NULL,
	N_("Insert GPL notice with C style comments"),
    G_CALLBACK (on_insert_c_gpl_notice)},
  { "ActionEditInsertCPPGPL", N_("/_/ GPL Notice"), NULL, NULL,
	N_("Insert GPL notice with C++ style comments"),
    G_CALLBACK (on_insert_cpp_gpl_notice)},
  { "ActionEditInsertPYGPL", N_("_# GPL Notice"), NULL, NULL,
	N_("Insert GPL notice with Python style comments"),
    G_CALLBACK (on_insert_py_gpl_notice)},
  { "ActionEditInsertUsername", N_("Current _Username"), NULL, NULL,
	N_("Insert name of current user"),
    G_CALLBACK (on_insert_username)},
  { "ActionEditInsertDateTime", N_("Current _Date & Time"), NULL, NULL,
	N_("Insert current date & time"),
    G_CALLBACK (on_insert_date_time)},
  { "ActionEditInsertHeaderTemplate", N_("Header File _Template"), NULL, NULL,
	N_("Insert a standard header file template"),
    G_CALLBACK (on_insert_header_template)},	
  { "ActionEditInsertChangelog", N_("ChangeLog entry"), NULL, NULL,
	N_("Insert a ChangeLog entry"),
    G_CALLBACK (on_insert_changelog_entry)},
  { "ActionEditInsertStatementSwitch", N_("_switch"), NULL, NULL,
	N_("Insert a switch template"),
    G_CALLBACK (on_insert_switch_template)},
  { "ActionEditInsertStatementFor", N_("_for"), NULL, NULL,
	N_("Insert a for template"),
    G_CALLBACK (on_insert_for_template)},
  { "ActionEditInsertStatementWhile", N_("_while"), NULL, NULL,
	N_("Insert a while template"),
    G_CALLBACK (on_insert_while_template)},
  { "ActionEditInsertStatementIfElse", N_("_if...else"), NULL, NULL,
	N_("Insert an if...else template"),
    G_CALLBACK (on_insert_ifelse_template)},
  { "ActionEditInsertCVSAuthor", N_("_Author"), NULL, NULL,
	N_("Insert the CVS Author keyword (author of the change)"),
    G_CALLBACK (on_insert_cvs_author)},
  { "ActionEditInsertCVSDate", N_("_Date"), NULL, NULL,
	N_("Insert the CVS Date keyword (date and time of the change)"),
    G_CALLBACK (on_insert_cvs_date)},
  { "ActionEditInsertCVSHeader", N_("_Header"), NULL, NULL,
	N_("Insert the CVS Header keyword (full path revision, date, author, state)"),
    G_CALLBACK (on_insert_cvs_header)},
  { "ActionEditInsertCVSID", N_("_Id"), NULL, NULL,
	N_("Insert the CVS Id keyword (file revision, date, author)"),
    G_CALLBACK (on_insert_cvs_id)},
  { "ActionEditInsertCVSLog", N_("_Log"), NULL, NULL,
	N_("Insert the CVS Log keyword (log message)"),
    G_CALLBACK (on_insert_cvs_log)},
  { "ActionEditInsertCVSName", N_("_Name"), NULL, NULL,
	N_("Insert the CVS Name keyword (name of the sticky tag)"),
    G_CALLBACK (on_insert_cvs_name)},
  { "ActionEditInsertCVSRevision", N_("_Revision"), NULL, NULL,
	N_("Insert the CVS Revision keyword (revision number)"),
    G_CALLBACK (on_insert_cvs_revision)},
  { "ActionEditInsertCVSSource", N_("_Source"), NULL, NULL,
	N_("Insert the CVS Source keyword (full path)"),
    G_CALLBACK (on_insert_cvs_source)},
};

static gpointer parent_class;

/* FIXME: There was a change in data representation in GtkActionEntry from
EggActionGroupEntry. The actual entries should be fixed and this hack removed */
static void
swap_label_and_stock (GtkActionEntry* actions, gint size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		const gchar *stock_id = actions[i].label;
		actions[i].label = actions[i].stock_id;
		actions[i].stock_id = stock_id;
		if (actions[i].name == NULL)
			g_warning ("Name is null: %s", actions[i].label);
	}
}

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaFileWizardPlugin *w_plugin;
	const gchar *root_uri;

	w_plugin = (AnjutaFileWizardPlugin*) plugin;
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		if (root_dir)
			w_plugin->top_dir = g_strdup(root_dir);
		else
			w_plugin->top_dir = NULL;
		g_free (root_dir);
	}
	else
		w_plugin->top_dir = NULL;
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	AnjutaFileWizardPlugin *w_plugin;
	w_plugin = (AnjutaFileWizardPlugin*) plugin;
	
	g_free (w_plugin->top_dir);
	w_plugin->top_dir = NULL;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaFileWizardPlugin *w_plugin;
	static gboolean initialized = FALSE;
	
	DEBUG_PRINT ("AnjutaFileWizardPlugin: Activating File wizard plugin ...");
	w_plugin = (AnjutaFileWizardPlugin*) plugin;
	w_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	w_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	/* Action groups */
	if (!initialized)
		swap_label_and_stock (actions_insert, G_N_ELEMENTS (actions_insert));
	
	w_plugin->action_group = 
		anjuta_ui_add_action_group_entries (w_plugin->ui,
											"ActionGroupFileWizard",
											_("File wizard operations"),
											actions_insert,
											G_N_ELEMENTS(actions_insert),
											GETTEXT_PACKAGE, plugin);
	/* Merge UI */
	w_plugin->merge_id = 
		anjuta_ui_merge (w_plugin->ui, UI_FILE);
	
	/* set up project directory watch */
	w_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
													   "project_root_uri",
													   project_root_added,
													   project_root_removed,
													   NULL);
	initialized = TRUE;
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaFileWizardPlugin *w_plugin;
	w_plugin = (AnjutaFileWizardPlugin*) plugin;
	anjuta_plugin_remove_watch (plugin, w_plugin->root_watch_id, TRUE);
	anjuta_ui_unmerge (w_plugin->ui, w_plugin->merge_id);
	anjuta_ui_remove_action_group (w_plugin->ui, w_plugin->action_group);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
file_wizard_plugin_instance_init (GObject *obj)
{
	AnjutaFileWizardPlugin *plugin = (AnjutaFileWizardPlugin*) obj;
	plugin->top_dir = NULL;
}

static void
file_wizard_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	AnjutaFileWizardPlugin *plugin;
	IAnjutaDocumentManager *docman;
	
	plugin = (AnjutaFileWizardPlugin *)ANJUTA_PLUGIN (wiz);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (wiz)->shell,
										 IAnjutaDocumentManager, NULL);
	display_new_file(plugin, docman);
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (AnjutaFileWizardPlugin, file_wizard_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaFileWizardPlugin, file_wizard_plugin);
