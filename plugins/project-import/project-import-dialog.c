/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Carl-Anton Ingmarsson 2009 <ca.ingmarsson@gmail.com>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "project-import-dialog.h"
#include <glib/gi18n.h>

#define BUILDER_FILE PACKAGE_DATA_DIR"/glade/project-import.glade"

G_DEFINE_TYPE (ProjectImportDialog, project_import_dialog, GTK_TYPE_DIALOG);

typedef struct _ProjectImportDialogPrivate ProjectImportDialogPrivate;

struct _ProjectImportDialogPrivate {
	GtkEntry *name_entry;
	GtkWidget *source_dir_button;
	GtkWidget *vcs_entry;
	GtkWidget *dest_dir_button;
	GtkWidget *import_button;
	GtkWidget *folder_radio;
	GtkWidget *vcs_combo;
	GtkListStore *vcs_store;
};

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), PROJECT_IMPORT_TYPE_DIALOG, ProjectImportDialogPrivate))

static void
vcs_entry_changed (GtkEditable *editable, gpointer user_data)
{
	ProjectImportDialog *project_import = (ProjectImportDialog *)user_data;
	ProjectImportDialogPrivate *priv = GET_PRIVATE (project_import);

	if (gtk_entry_get_text_length (GTK_ENTRY (editable)))
	{
		if (gtk_entry_get_text_length (priv->name_entry))
		{
			gtk_widget_set_sensitive (priv->import_button, TRUE);
		}
	}
	else
		gtk_widget_set_sensitive (priv->import_button, FALSE);
}

static void
name_entry_changed (GtkEditable *editable, gpointer user_data)
{
	ProjectImportDialog *project_import = (ProjectImportDialog *)user_data;
	ProjectImportDialogPrivate *priv = GET_PRIVATE (project_import);

	if (gtk_entry_get_text_length (GTK_ENTRY (editable)))
	{
		if ((gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->folder_radio)) ||
		     gtk_entry_get_text_length (GTK_ENTRY (priv->vcs_entry))))
		{
			gtk_widget_set_sensitive (priv->import_button, TRUE);
		}
	}
	else
		gtk_widget_set_sensitive (priv->import_button, FALSE);
}

static void
folder_radio_toggled (GtkToggleButton *button, gpointer user_data)
{
	ProjectImportDialog *project_import = (ProjectImportDialog *)user_data;
	ProjectImportDialogPrivate *priv = GET_PRIVATE (project_import);

	gtk_widget_set_sensitive (priv->source_dir_button, TRUE);
	gtk_widget_set_sensitive (priv->vcs_entry, FALSE);
	gtk_widget_set_sensitive (priv->dest_dir_button, FALSE);
	gtk_widget_set_sensitive (priv->vcs_combo, FALSE);
	
	if (gtk_entry_get_text_length (priv->name_entry))
	{
		gtk_widget_set_sensitive (priv->import_button, TRUE);
	}
	else
		gtk_widget_set_sensitive (priv->import_button, FALSE);
}

static void
vcs_radio_toggled (GtkToggleButton *button, gpointer user_data)
{
	ProjectImportDialog *project_import = (ProjectImportDialog *)user_data;
	ProjectImportDialogPrivate *priv = GET_PRIVATE (project_import);

	gtk_widget_set_sensitive (priv->vcs_entry, TRUE);
	gtk_widget_set_sensitive (priv->dest_dir_button, TRUE);
	gtk_widget_set_sensitive (priv->vcs_combo, TRUE);
	gtk_widget_set_sensitive (priv->source_dir_button, FALSE);
	
	if ((gtk_entry_get_text_length (GTK_ENTRY (priv->vcs_entry))
	    && gtk_entry_get_text (priv->name_entry)))
	{
		gtk_widget_set_sensitive (priv->import_button, TRUE);
	}
	else
		gtk_widget_set_sensitive (priv->import_button, FALSE);
}

gchar *
project_import_dialog_get_vcs_id (ProjectImportDialog *import_dialog)
{
	ProjectImportDialogPrivate *priv = GET_PRIVATE (import_dialog);
	GtkTreeIter iter;
	gchar *vcs_id;

	g_assert (PROJECT_IS_IMPORT_DIALOG (import_dialog));
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->folder_radio)))
		return NULL;

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->vcs_combo), &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->vcs_store), &iter, 1, &vcs_id, -1);

	return vcs_id;
}

gchar *
project_import_dialog_get_vcs_uri (ProjectImportDialog *import_dialog)
{
	ProjectImportDialogPrivate *priv = GET_PRIVATE (import_dialog);

	g_assert (PROJECT_IS_IMPORT_DIALOG (import_dialog));
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->folder_radio)))
		return NULL;
	
	return g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->vcs_entry)));
}

GFile *
project_import_dialog_get_dest_dir (ProjectImportDialog *import_dialog)
{
	ProjectImportDialogPrivate *priv = GET_PRIVATE (import_dialog);

	g_assert (PROJECT_IS_IMPORT_DIALOG (import_dialog));
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->folder_radio)))
		return NULL;
	
	return gtk_file_chooser_get_file (GTK_FILE_CHOOSER (priv->dest_dir_button));
}

GFile *
project_import_dialog_get_source_dir (ProjectImportDialog *import_dialog)
{
	ProjectImportDialogPrivate *priv = GET_PRIVATE (import_dialog);

	g_assert (PROJECT_IS_IMPORT_DIALOG (import_dialog));
	
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->folder_radio)))
		return NULL;
	
	return gtk_file_chooser_get_file (GTK_FILE_CHOOSER (priv->source_dir_button));
}

gchar *
project_import_dialog_get_name (ProjectImportDialog *import_dialog)
{
	ProjectImportDialogPrivate *priv = GET_PRIVATE (import_dialog);

	g_assert (PROJECT_IS_IMPORT_DIALOG (import_dialog));

	return g_strdup (gtk_entry_get_text (priv->name_entry));
}

ProjectImportDialog *
project_import_dialog_new (AnjutaPluginManager *plugin_manager, const gchar *name, GFile *dir)
{
	ProjectImportDialog *import_dialog;
	ProjectImportDialogPrivate *priv;
	GList *plugin_descs, *l_iter;

	import_dialog = g_object_new (PROJECT_IMPORT_TYPE_DIALOG, NULL);
	priv = GET_PRIVATE (import_dialog);

	if (name)
		gtk_entry_set_text (priv->name_entry, name);
	if (dir)
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (priv->source_dir_button), dir, NULL);

	plugin_descs = anjuta_plugin_manager_query (plugin_manager,
	                                            "Anjuta Plugin",
	                                            "Interfaces",
	                                            "IAnjutaVcs",
	                                            NULL);
	for (l_iter = plugin_descs; l_iter; l_iter = l_iter->next)
	{
		gchar *vcs_name, *plugin_id;
		GtkTreeIter iter;
		
		anjuta_plugin_description_get_string (l_iter->data, "Vcs", "System",
		                                      &vcs_name);
		anjuta_plugin_description_get_string (l_iter->data, "Anjuta Plugin", "Location",
		                                      &plugin_id);

		gtk_list_store_append (priv->vcs_store, &iter);
		gtk_list_store_set (priv->vcs_store, &iter, 0, vcs_name, 1, plugin_id, -1);

		g_free (vcs_name);
		g_free (plugin_id);

		gtk_combo_box_set_active (GTK_COMBO_BOX (priv->vcs_combo), 0);
	}
	
	g_list_free (plugin_descs);

	return import_dialog;
}

static void
project_import_dialog_init (ProjectImportDialog *import_dialog)
{
	ProjectImportDialogPrivate *priv;
	GtkBuilder *builder;
	gchar *object_ids[] = {"top_level", "vcs_store", NULL};
	GtkWidget *image;

	priv = GET_PRIVATE (import_dialog);
	
	builder = gtk_builder_new ();
	gtk_builder_add_objects_from_file (builder, BUILDER_FILE, object_ids, NULL);

	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (import_dialog))),
	                   GTK_WIDGET (gtk_builder_get_object (builder, "top_level")));

	priv->name_entry =  GTK_ENTRY (gtk_builder_get_object (builder, "name_entry"));
	priv->source_dir_button = GTK_WIDGET (gtk_builder_get_object (builder, "source_dir_button"));
	priv->vcs_entry = GTK_WIDGET (gtk_builder_get_object (builder, "vcs_entry"));
	priv->dest_dir_button = GTK_WIDGET (gtk_builder_get_object (builder, "dest_dir_button"));
	priv->folder_radio = GTK_WIDGET (gtk_builder_get_object (builder, "folder_radio"));
	priv->vcs_combo = GTK_WIDGET (gtk_builder_get_object (builder, "vcs_combo"));
	priv->vcs_store = GTK_LIST_STORE (gtk_builder_get_object (builder, "vcs_store"));

	g_signal_connect (priv->name_entry, "changed", G_CALLBACK (name_entry_changed),
	                  import_dialog);
	g_signal_connect (priv->vcs_entry, "changed", G_CALLBACK (vcs_entry_changed),
	                  import_dialog);
	
	g_signal_connect (priv->folder_radio, "toggled",
	                  G_CALLBACK (folder_radio_toggled), import_dialog);
	g_signal_connect (gtk_builder_get_object (builder, "vcs_radio"), "toggled",
	                  G_CALLBACK (vcs_radio_toggled), import_dialog);

	g_object_unref (builder);

	gtk_window_set_title (GTK_WINDOW (import_dialog), _("Import project"));

	gtk_dialog_add_button (GTK_DIALOG (import_dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);

	priv->import_button = gtk_dialog_add_button (GTK_DIALOG (import_dialog), _("Import"), GTK_RESPONSE_ACCEPT);
	image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image (GTK_BUTTON (priv->import_button), image);
	gtk_widget_set_sensitive (priv->import_button, FALSE);
}

static void
project_import_dialog_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (project_import_dialog_parent_class)->finalize (object);
}

static void
project_import_dialog_class_init (ProjectImportDialogClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkDialogClass* parent_class = GTK_DIALOG_CLASS (klass);

	object_class->finalize = project_import_dialog_finalize;

	g_type_class_add_private (klass, sizeof (ProjectImportDialogPrivate));
}

