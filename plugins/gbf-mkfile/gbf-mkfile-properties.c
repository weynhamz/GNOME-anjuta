/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8; coding: utf-8 -*- */
/* gbf-mkfile-properties.c
 *
 * Copyright (C) 2005  Eric Greveson
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
 * Author: Eric Greveson
 * Based on the Autotools GBF backend (libgbf-am) by
 *   JP Rosevear
 *   Dave Camp
 *   Naba Kumar
 *   Gustavo Giráldez
 */

#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gbf-mkfile-config.h"
#include "gbf-mkfile-properties.h"

typedef enum {
	GBF_MKFILE_CONFIG_LABEL,
	GBF_MKFILE_CONFIG_ENTRY,
	GBF_MKFILE_CONFIG_TEXT,
	GBF_MKFILE_CONFIG_LIST,
} GbfConfigPropertyType;

static void
on_property_entry_changed (GtkEntry *entry, GbfMkfileConfigValue *value)
{
	gbf_mkfile_config_value_set_string (value, gtk_entry_get_text (entry));
}

static void
add_configure_property (GbfMkfileProject *project, GbfMkfileConfigMapping *config,
			GbfConfigPropertyType prop_type,
			const gchar *display_name, const gchar *direct_value,
			const gchar *config_key, GtkWidget *table,
			gint position)
{
	GtkWidget *label;
	const gchar *value;
	GtkWidget *widget;
	GbfMkfileConfigValue *config_value = NULL;
	
	value = "";
	if (direct_value)
	{
		value = direct_value;
	} else {
		config_value = gbf_mkfile_config_mapping_lookup (config,
							     config_key);
		if (!config_value) {
			config_value = gbf_mkfile_config_value_new (GBF_MKFILE_TYPE_STRING);
			gbf_mkfile_config_value_set_string (config_value, "");
			gbf_mkfile_config_mapping_insert (config, config_key,
						      config_value);
		}
		if (config_value && config_value->type == GBF_MKFILE_TYPE_STRING) {
			const gchar *val_str;
			val_str = gbf_mkfile_config_value_get_string (config_value);
			if (val_str)
				value = val_str;
		}
	}
	
	label = gtk_label_new (display_name);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, position, position+1,
			  GTK_FILL, GTK_FILL, 5, 3);
	switch (prop_type) {
		case GBF_MKFILE_CONFIG_LABEL:
			widget = gtk_label_new (value);
			gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
			break;
		case GBF_MKFILE_CONFIG_ENTRY:
			widget = gtk_entry_new ();
			gtk_entry_set_text (GTK_ENTRY (widget), value);
			if (config_value) {
				g_signal_connect (widget, "changed",
						  G_CALLBACK (on_property_entry_changed),
						  config_value);
			}
			break;
		default:
			g_warning ("Should not reach here");
			widget = gtk_label_new (_("Unknown"));
			gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
	}
	gtk_widget_show (widget);
	gtk_table_attach (GTK_TABLE (table), widget, 1, 2, position, position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
}

static void
recursive_config_foreach_cb (const gchar *key, GbfMkfileConfigValue *value,
			     gpointer user_data)
{
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *widget;
	gint position;
	
	table = GTK_WIDGET (user_data);
	position = g_list_length (GTK_TABLE (table)->children);
	
	label = gtk_label_new (key);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1,
			  position, position+1,
			  GTK_FILL, GTK_FILL, 5, 3);
		
	if (value->type == GBF_MKFILE_TYPE_STRING) {
		widget = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (widget),
				    gbf_mkfile_config_value_get_string (value));
		g_signal_connect (widget, "changed",
				  G_CALLBACK (on_property_entry_changed),
				  value);
	} else if (value->type == GBF_MKFILE_TYPE_LIST) {
		/* FIXME: */
		widget = gtk_label_new ("FIXME");
		gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
	} else if (value->type == GBF_MKFILE_TYPE_MAPPING) {
		/* FIXME: */
		widget = gtk_label_new ("FIXME");
		gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
	} else {
		g_warning ("Should not be here");
		widget = gtk_label_new (_("Unknown"));
		gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
	}
	gtk_widget_show (widget);
	gtk_table_attach (GTK_TABLE (table), widget, 1, 2,
			  position, position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
}

GtkWidget*
gbf_mkfile_properties_get_widget (GbfMkfileProject *project, GError **error)
{
	GbfMkfileConfigMapping *config;
	GtkWidget *table;
	GError *err = NULL;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	config = gbf_mkfile_project_get_config (project, &err);
	if (err)
	{
		g_propagate_error (error, err);
		return NULL;
	}
	table = gtk_table_new (7, 2, FALSE);
	
	/* Group name */
	add_configure_property (project, config, GBF_MKFILE_CONFIG_LABEL,
				_("Project:"), project->project_root_uri,
				NULL, table, 0);
	
	gtk_widget_show_all (table);
	return table;
}

static void
on_group_widget_destroy (GtkWidget *wid, GtkWidget *table)
{
	GError *err = NULL;
	
	GbfMkfileProject *project = g_object_get_data (G_OBJECT (table), "__project");
	GbfMkfileConfigMapping *new_config = g_object_get_data (G_OBJECT (table), "__config");
	const gchar *group_id = g_object_get_data (G_OBJECT (table), "__group_id");
	gbf_mkfile_project_set_group_config (project, group_id, new_config, &err);
	if (err)
	{
		g_warning ("%s", err->message);
		g_error_free (err);
	}
	g_object_unref (table);
}

GtkWidget*
gbf_mkfile_properties_get_group_widget (GbfMkfileProject *project,
				    const gchar *group_id,
				    GError **error)
{
	GbfProjectGroup *group;
	GbfMkfileConfigMapping *config;
	GbfMkfileConfigValue *value;
	GtkWidget *table;
	GError *err = NULL;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	group = gbf_project_get_group (GBF_PROJECT (project), group_id, &err);
	if (err)
	{
		g_propagate_error (error, err);
		return NULL;
	}
	config = gbf_mkfile_project_get_group_config (project, group_id, &err);
	if (err)
	{
		g_propagate_error (error, err);
		return NULL;
	}
	
	g_return_val_if_fail (group != NULL, NULL);
	g_return_val_if_fail (config != NULL, NULL);
	
	table = gtk_table_new (7, 2, FALSE);
	g_object_ref (table);
	g_object_set_data (G_OBJECT (table), "__project", project);
	g_object_set_data_full (G_OBJECT (table), "__config", config,
				(GDestroyNotify)gbf_mkfile_config_mapping_destroy);
	g_object_set_data_full (G_OBJECT (table), "__group_id",
				g_strdup (group_id),
				(GDestroyNotify)g_free);
	g_signal_connect (table, "destroy",
			  G_CALLBACK (on_group_widget_destroy), table);
	
	/* Group name */
	add_configure_property (project, config, GBF_MKFILE_CONFIG_LABEL,
				_("Group name:"), group->name, NULL, table, 0);
	/* Includes */
	add_configure_property (project, config, GBF_MKFILE_CONFIG_ENTRY,
				_("Includes:"), NULL, "includes",
				table, 1);
	/* Install directories */
	value = gbf_mkfile_config_mapping_lookup (config, "installdirs");
	if (value)
	{
		GtkWidget *table2, *frame, *lab;
		char *text;
		frame = gtk_frame_new ("");
		
		lab = gtk_frame_get_label_widget (GTK_FRAME(frame));
		text = g_strdup_printf ("<b>%s</b>", _("Install directories:"));
		gtk_label_set_markup (GTK_LABEL(lab), text);
		g_free (text);
		gtk_frame_set_shadow_type (GTK_FRAME (frame),
					   GTK_SHADOW_NONE);
		gtk_widget_show (frame);
		gtk_table_attach (GTK_TABLE (table), frame, 0, 2, 2, 3,
				  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
		table2 = gtk_table_new (0, 0, FALSE);
		gtk_widget_show (table2);
		gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
		gtk_container_add (GTK_CONTAINER(frame), table2);
		gbf_mkfile_config_mapping_foreach (value->mapping,
					       recursive_config_foreach_cb,
					       table2);
	}
	gtk_widget_show_all (table);
	gbf_project_group_free (group);
	return table;
}

static void
on_target_widget_destroy (GtkWidget *wid, GtkWidget *table)
{
	GError *err = NULL;
	
	GbfMkfileProject *project = g_object_get_data (G_OBJECT (table), "__project");
	GbfMkfileConfigMapping *new_config = g_object_get_data (G_OBJECT (table), "__config");
	const gchar *target_id = g_object_get_data (G_OBJECT (table), "__target_id");
	gbf_mkfile_project_set_target_config (project, target_id, new_config, &err);
	if (err)
	{
		g_warning ("%s", err->message);
		g_error_free (err);
	}
	g_object_unref (table);
}

GtkWidget*
gbf_mkfile_properties_get_target_widget (GbfMkfileProject *project,
				     const gchar *target_id, GError **error)
{
	GbfProjectGroup *group;
	GbfMkfileConfigMapping *group_config;
	GbfProjectTarget *target;
	GbfMkfileConfigMapping *config;
	GbfMkfileConfigValue *value;
	GbfMkfileConfigMapping *installdirs;
	GbfMkfileConfigValue *installdirs_val, *dir_val;
	GtkWidget *table;
	GError *err = NULL;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	target = gbf_project_get_target (GBF_PROJECT (project), target_id, &err);
	if (err)
	{
		g_propagate_error (error, err);
		return NULL;
	}
	config = gbf_mkfile_project_get_target_config (project, target_id, &err);
	if (err)
	{
		g_propagate_error (error, err);
		return NULL;
	}
	g_return_val_if_fail (target != NULL, NULL);
	g_return_val_if_fail (config != NULL, NULL);
	
	group = gbf_project_get_group (GBF_PROJECT (project),
				       target->group_id, NULL);
	group_config = gbf_mkfile_project_get_group_config (project,
							target->group_id,
							NULL);
	
	table = gtk_table_new (7, 2, FALSE);
	g_object_ref (table);
	g_object_set_data (G_OBJECT (table), "__project", project);
	g_object_set_data_full (G_OBJECT (table), "__config", config,
				(GDestroyNotify)gbf_mkfile_config_mapping_destroy);
	g_object_set_data_full (G_OBJECT (table), "__target_id",
				g_strdup (target_id),
				(GDestroyNotify)g_free);
	g_signal_connect (table, "destroy",
			  G_CALLBACK (on_target_widget_destroy), table);
	
	/* Target name */
	add_configure_property (project, config, GBF_MKFILE_CONFIG_LABEL,
				_("Target name:"), target->name, NULL, table, 0);
	/* Target type */
	add_configure_property (project, config, GBF_MKFILE_CONFIG_LABEL,
				_("Type:"),
				gbf_project_name_for_type (GBF_PROJECT (project),
							   target->type),
				NULL, table, 1);
	/* Target group */
	add_configure_property (project, config, GBF_MKFILE_CONFIG_LABEL,
				_("Group:"), group->name,
				NULL, table, 2);
	
	/* Target primary */
	/* FIXME: Target config 'installdir' actually stores target primary,
	 * and not what it really seems to mean, i.e the primary prefix it
	 * belongs to. The actual install directory of a target is the
	 * install directory of primary prefix it belongs to and is
	 * configured in group properties.
	 */
	value = gbf_mkfile_config_mapping_lookup (config, "installdir");
	installdirs_val = gbf_mkfile_config_mapping_lookup (group_config,
							"installdirs");
	if (installdirs_val)
		installdirs = gbf_mkfile_config_value_get_mapping (installdirs_val);
	
	if (!value || !installdirs_val) {
		add_configure_property (project, config, GBF_MKFILE_CONFIG_LABEL,
					_("Install directory:"), NULL, "installdir",
					table, 3);
	} else {
		const gchar *primary_prefix;
		gchar *installdir;
		
		primary_prefix = gbf_mkfile_config_value_get_string (value);
		installdirs = gbf_mkfile_config_value_get_mapping (installdirs_val);
		dir_val = gbf_mkfile_config_mapping_lookup (installdirs,
							primary_prefix);
		if (dir_val)
		{
			installdir = g_strconcat (primary_prefix, " = ",
						  gbf_mkfile_config_value_get_string (dir_val),
						  NULL);
			add_configure_property (project, config,
						GBF_MKFILE_CONFIG_LABEL,
						_("Install directory:"),
						installdir, NULL,
						table, 3);
			g_free (installdir);
		} else {
			add_configure_property (project, config,
						GBF_MKFILE_CONFIG_LABEL,
						_("Install directory:"),
						NULL, "installdir",
						table, 3);
		}
	}

	if (target->type && (strcmp (target->type, "program") == 0 ||
			     strcmp (target->type, "shared_lib") == 0 ||
			     strcmp (target->type, "static_lib") == 0)) {
		/* LDFLAGS */
		add_configure_property (project, config,
					GBF_MKFILE_CONFIG_ENTRY,
					_("Linker flags:"), NULL,
					"ldflags", table, 4);
		
		/* LDADD */
		add_configure_property (project, config,
					GBF_MKFILE_CONFIG_ENTRY,
					_("Libraries:"), NULL,
					"ldadd", table, 5);
		
		/* DEPENDENCIES */
		add_configure_property (project, config,
					GBF_MKFILE_CONFIG_ENTRY,
					_("Dependencies:"), NULL,
					"explicit_deps", table, 6);
	}
	gtk_widget_show_all (table);
	gbf_project_target_free (target);
	return table;
}
