/*  patch-plugin.c (C) 2002 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "../../src/anjuta.h"
#include "../../src/launcher.h"
#include "../../src/anjuta_info.h"

gchar   *GetDescr       (void);
glong    GetVersion     (void);
gboolean Init           (GModule *self, void **pUserData, AnjutaApp* p);
void     CleanUp        (GModule *self, void *pUserData, AnjutaApp* p);
void     Activate       (GModule *self, void *pUserData, AnjutaApp* p);
gchar   *GetMenuTitle   (GModule *self, void *pUserData);
gchar   *GetTooltipText (GModule *self, void *pUserData);

typedef struct _PatchPluginGUI PatchPluginGUI;

static void patch_level_changed (GtkAdjustment *adj);

static void on_ok_clicked (GtkButton *button, PatchPluginGUI* gui);
static void on_cancel_clicked (GtkButton *button, PatchPluginGUI* gui);

static void on_msg_arrived (gchar* line);
static void on_patch_terminate (int status, time_t time);

struct _PatchPluginGUI
{
	GtkWidget* dialog;
	
	GtkWidget* ok_button;
	GtkWidget* cancel_button;
	
	GtkWidget* entry_patch_dir;
	GtkWidget* entry_patch_file;
	
	GtkWidget* hscale_patch_level;
};

gint patch_level = 0;

/* Get module description */
gchar *
GetDescr()
{
	return g_strdup(_("Interface to patch Projects/Files"));
}
	/* GetModule Version hi/low word 1.02 0x10002 */
glong
GetVersion()
{
	return 0x100000;
}

gboolean
Init( GModule *self, void **pUserData, AnjutaApp* p )
{
	return TRUE ;
}

void
CleanUp( GModule *self, void *pUserData, AnjutaApp* p )
{
}

static void
patch_level_changed (GtkAdjustment *adj)
{
	patch_level = (gint) adj->value;
}

void
Activate( GModule *self, void *pUserData, AnjutaApp* p)
{
	PatchPluginGUI* gui;
	GtkObject* adj;
	GtkWidget* dir_label = gtk_label_new (_("File/Dir to patch"));
	GtkWidget* file_label = gtk_label_new (_("Patch file"));
	GtkWidget* level_label = gtk_label_new (_("Patch level (-p)"));
	GtkWidget* table = gtk_table_new (3, 2, FALSE);

	gui = g_new0 (PatchPluginGUI, 1);
	
	gui->dialog = gnome_dialog_new (_("Patch Plugin"), _("Cancel"), _("Patch"), NULL);
	gui->entry_patch_dir = gnome_file_entry_new ("patch-dir", 
	_("Selected directory to patch"));
	gui->entry_patch_file = gnome_file_entry_new ("patch-file",
			_("Selected patch file"));

	adj = gtk_adjustment_new (patch_level, 0, 10, 1, 1, 1);
	gtk_signal_connect (adj, "value_changed",
			    GTK_SIGNAL_FUNC (patch_level_changed), NULL);

	gui->hscale_patch_level = gtk_hscale_new (GTK_ADJUSTMENT (adj));
	gtk_scale_set_digits (GTK_SCALE (gui->hscale_patch_level), 0);
	
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 10);

	gtk_table_attach_defaults (GTK_TABLE(table), dir_label, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE(table), file_label, 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE(table), level_label, 0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE(table), gui->entry_patch_dir, 1, 2, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE(table), gui->entry_patch_file, 1, 2, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE(table), gui->hscale_patch_level, 1, 2, 2, 3);
	
	gui->ok_button = g_list_last (GNOME_DIALOG (gui->dialog)->buttons)->data;
	gui->cancel_button = g_list_first (GNOME_DIALOG (gui->dialog)->buttons)->data;
	
	gtk_signal_connect (GTK_OBJECT (gui->ok_button), "clicked", 
			GTK_SIGNAL_FUNC(on_ok_clicked), gui);
	gtk_signal_connect (GTK_OBJECT (gui->cancel_button), "clicked", 
			GTK_SIGNAL_FUNC(on_cancel_clicked), gui);

	gtk_container_set_border_width (GTK_CONTAINER (GNOME_DIALOG (gui->dialog)->vbox), 5);
	gtk_box_pack_start_defaults (GTK_BOX (GNOME_DIALOG (gui->dialog)->vbox), table);

	gtk_widget_show_all (gui->dialog);
}

gchar
*GetMenuTitle( GModule *self, void *pUserData )
{
	return g_strdup(_("Patch plugin"));
}

gchar
*GetTooltipText( GModule *self, void *pUserData ) 
{
   return g_strdup(_("The patch plugin"));
}

static void on_ok_clicked (GtkButton *button, PatchPluginGUI* gui)
{
	gchar* directory;
	gchar* patch_file;
	GString* command = g_string_new (NULL);
	
	directory = gtk_entry_get_text(GTK_ENTRY(
		gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(gui->entry_patch_dir))));
	patch_file = gtk_entry_get_text(GTK_ENTRY(
		gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(gui->entry_patch_file))));

	if (!file_is_directory (directory))
	{
		g_string_free (command, TRUE);
		gnome_ok_dialog (_("You have to select the directory where the patch should be applied"));
		return;
	}
	
	g_string_sprintf (command, "patch -d %s -p %d -f -i %s",
			  directory, patch_level, patch_file);
	
	anjuta_message_manager_show (app->messages, MESSAGE_BUILD);
	anjuta_message_manager_append (app->messages,
			_("Patching...\n"), MESSAGE_BUILD);
	
	if (!launcher_is_busy())
		launcher_execute (command->str, on_msg_arrived, on_msg_arrived, 
			on_patch_terminate);
	else
		gnome_ok_dialog (
			_("There are unfinished jobs, please wait until they are finished"));
	g_string_free(command, TRUE);
	on_cancel_clicked (GTK_BUTTON(gui->cancel_button), gui);
}

static void on_cancel_clicked (GtkButton *button, PatchPluginGUI* gui)
{
	gtk_widget_hide (gui->dialog);
	gtk_widget_destroy (gui->dialog);
	g_free(gui);
}

static void on_msg_arrived (gchar* line)
{
	g_return_if_fail (line != NULL);
	anjuta_message_manager_append (app->messages, line, MESSAGE_BUILD);
}

static void on_patch_terminate (int status, time_t time)
{
	if (status)
	{
		anjuta_message_manager_append (app->messages,
			_("Patch failed\n"), MESSAGE_BUILD);
	}
	else
	{
		anjuta_message_manager_append (app->messages,
			_("Patch successful\n"), MESSAGE_BUILD);	}
}

