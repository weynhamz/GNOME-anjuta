/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-dialogs.c
 *
 * Copyright (C) 2009  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "am-dialogs.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

#define GLADE_FILE  PACKAGE_DATA_DIR "/glade/am-project.ui"

/* Types
  *---------------------------------------------------------------------------*/

typedef struct _AmpConfigureProjectDialog
{
	AmpProject *project;

	GtkWidget *top_level;
} AmpConfigureProjectDialog;


/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static void
on_project_widget_destroy (GtkWidget *wid, AmpConfigureProjectDialog *dlg)
{
	g_object_unref (dlg->top_level);
	g_free (dlg);
}

static void
add_entry (AmpProject *project, AnjutaProjectNode *node, AnjutaProjectPropertyInfo *info, GtkWidget *table, gint position)
{
	GtkWidget *label;
	GtkWidget *entry;
	const gchar *value;

	label = gtk_label_new (_(info->name));
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, position, position+1,
			  GTK_FILL, GTK_FILL, 5, 3);
	
	entry = gtk_entry_new ();
	value = info->value;
	gtk_entry_set_text (GTK_ENTRY (entry), value);
	gtk_misc_set_alignment (GTK_MISC (entry), 0, -1);
	gtk_widget_show (entry);
	gtk_table_attach (GTK_TABLE (table), entry, 1, 2, position, position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
}

static void
add_label (const gchar *display_name, const gchar *value, GtkWidget *table, gint position)
{
	GtkWidget *label;
	
	label = gtk_label_new (display_name);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, position, position+1,
			  GTK_FILL, GTK_FILL, 5, 3);

	label = gtk_label_new (value);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, position, position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
}

/* Public functions
 *---------------------------------------------------------------------------*/

GtkWidget *
amp_configure_project_dialog (AmpProject *project, GError **error)
{
	GtkBuilder *bxml = gtk_builder_new ();
	GtkWidget *top_level;
	AmpConfigureProjectDialog *dlg;
	GtkWidget *table;
	gint pos;
	gchar *name;
	AnjutaProjectPropertyItem *prop;
	AnjutaProjectPropertyList *list;

	bxml = anjuta_util_builder_new (GLADE_FILE, NULL);
	if (!bxml) return NULL;

	dlg = g_new0 (AmpConfigureProjectDialog, 1);
	anjuta_util_builder_get_objects (bxml,
	                                "top_level", &top_level,
	    							"general_properties_table", &table,
	                                NULL);
	dlg->top_level = top_level;
	g_object_ref (top_level);
	g_signal_connect (top_level, "destroy", G_CALLBACK (on_project_widget_destroy), dlg);

	name = g_file_get_parse_name (amp_project_get_file (project));
	add_label (_("Path:"), name, table, 0);
	g_free (name);

	pos = 1;
	list = amp_project_get_property_list (project);
	for (prop = anjuta_project_property_first (list); prop != NULL; prop = anjuta_project_property_next (prop))
	{
		AnjutaProjectPropertyInfo *info;

		info = anjuta_project_property_lookup (list, prop);
		if (info == NULL) info = anjuta_project_property_get_info (prop);
		add_entry (project, NULL, info, table, pos++);
	}
	
	/*add_entry (project, NULL, _("Name:"), AMP_PROPERTY_NAME, table, 1);
	add_entry (project, NULL, _("Version:"), AMP_PROPERTY_VERSION, table, 2);
	add_entry (project, NULL, _("Bug report URL:"), AMP_PROPERTY_BUG_REPORT, table, 3);
	add_entry (project, NULL, _("Package name:"), AMP_PROPERTY_TARNAME, table, 4);
	add_entry (project, NULL, _("URL:"), AMP_PROPERTY_URL, table, 5);*/
	
	gtk_widget_show_all (top_level);
	g_object_unref (bxml);

	return top_level;
}

GtkWidget *
amp_configure_group_dialog (AmpProject *project, AmpGroup *group, GError **error)
{
	GtkBuilder *bxml = gtk_builder_new ();
	GtkWidget *properties;
	GtkWidget *main_table;
	gint main_pos;
	GtkWidget *extra_table;
	gint extra_pos;
	AmpConfigureProjectDialog *dlg;
	gchar *name;
	AnjutaProjectPropertyList *list;
	AnjutaProjectPropertyItem *prop;

	bxml = anjuta_util_builder_new (GLADE_FILE, NULL);
	if (!bxml) return NULL;

	dlg = g_new0 (AmpConfigureProjectDialog, 1);
	anjuta_util_builder_get_objects (bxml,
	                                "properties", &properties,
	    							"main_table", &main_table,
	    							"extra_table", &extra_table,
	                                NULL);
	dlg->top_level = properties;
	g_object_ref (properties);
	g_signal_connect (properties, "destroy", G_CALLBACK (on_project_widget_destroy), dlg);

	name = g_file_get_parse_name (amp_group_get_directory (group));
	add_label (_("Name:"), name, main_table, 0);
	g_free (name);

	main_pos = 1;
	extra_pos = 0;
	list = anjuta_project_node_get_property_list ((AnjutaProjectNode *)group);
	for (prop = anjuta_project_property_first (list); prop != NULL; prop = anjuta_project_property_next (prop))
	{
		AnjutaProjectPropertyInfo *info;

		info = anjuta_project_property_lookup (list, prop);
		if (info != NULL)
		{
			add_entry (project, (AnjutaProjectNode *)group, info, main_table, main_pos++);
		}
		else
		{
			info = anjuta_project_property_get_info (prop);
			add_entry (project, (AnjutaProjectNode *)group, info, extra_table, extra_pos++);
		}
	}
	
	gtk_widget_show_all (properties);
	g_object_unref (bxml);
	
	return properties;
}

GtkWidget *
amp_configure_target_dialog (AmpProject *project, AmpTarget *target, GError **error)
{
	GtkBuilder *bxml = gtk_builder_new ();
	GtkWidget *properties;
	GtkWidget *main_table;
	gint main_pos;
	GtkWidget *extra_table;
	gint extra_pos;
	AmpConfigureProjectDialog *dlg;
	AnjutaProjectTargetType type;
	const gchar *name;
	AnjutaProjectPropertyList *list;
	AnjutaProjectPropertyItem *prop;

	bxml = anjuta_util_builder_new (GLADE_FILE, NULL);
	if (!bxml) return NULL;

	dlg = g_new0 (AmpConfigureProjectDialog, 1);
	anjuta_util_builder_get_objects (bxml,
	                                "properties", &properties,
	    							"main_table", &main_table,
	    							"extra_table", &extra_table,
	                                NULL);
	dlg->top_level = properties;
	g_object_ref (properties);
	g_signal_connect (properties, "destroy", G_CALLBACK (on_project_widget_destroy), dlg);

	name = amp_target_get_name (target);
	add_label (_("Name:"), name, main_table, 0);
	type = anjuta_project_target_get_type (target);
	add_label (_("Type:"), anjuta_project_target_type_name (type), main_table, 1);

	main_pos = 2;
	extra_pos = 0;
	list = anjuta_project_node_get_property_list ((AnjutaProjectNode *)target);
	for (prop = anjuta_project_property_first (list); prop != NULL; prop = anjuta_project_property_next (prop))
	{
		AnjutaProjectPropertyInfo *info;

		info = anjuta_project_property_lookup (list, prop);
		if (info != NULL)
		{
			add_entry (project, (AnjutaProjectNode *)target, info, main_table, main_pos++);
		}
		else
		{
			info = anjuta_project_property_get_info (prop);
			add_entry (project, (AnjutaProjectNode *)target, info, extra_table, extra_pos++);
		}
	}
	
	gtk_widget_show_all (properties);
	g_object_unref (bxml);
	
	return properties;
}

GtkWidget *
amp_configure_source_dialog (AmpProject *project, AmpSource *target, GError **error)
{
	return NULL;
}
