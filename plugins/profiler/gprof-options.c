/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-options.c
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 * 
 * gprof-options.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#include "gprof-options.h"

/* Internal structure to hold information on individual preference keys */
typedef struct
{
	gchar *name;
	gchar *widget_name;
	GProfOptions *options;  /* Need this to set/get data from callbacks */
	OptionWidgetType widget_type;
} Key;

struct _GProfOptionsPriv
{
	GHashTable *default_options;
	GHashTable *targets;
	GHashTable *current_target;
	GHashTable *key_data_table;
};

static void
free_key (Key *key)
{
	g_free (key->name);
	g_free (key->widget_name);
	g_free (key);
}

static void
gprof_options_init (GProfOptions *self)
{
	self->priv = g_new0 (GProfOptionsPriv, 1);
	
	self->priv->default_options = g_hash_table_new_full (g_str_hash, 
														 g_str_equal,
														 g_free, g_free);
	
	self->priv->key_data_table = g_hash_table_new_full (g_str_hash, 
														g_str_equal,
														g_free, 
														(GDestroyNotify) free_key);
	
	self->priv->targets = g_hash_table_new_full (g_str_hash,
												 g_str_equal,
												 g_free, 
												 (GDestroyNotify) g_hash_table_destroy);
}

static void
gprof_options_finalize (GObject *obj)
{
	GProfOptions *self;
	
	self = (GProfOptions *) obj;
	
	g_hash_table_destroy (self->priv->default_options);
	g_hash_table_destroy (self->priv->key_data_table);
	g_hash_table_destroy (self->priv->targets);
	
	g_free (self->priv);
}

static void 
gprof_options_class_init (GProfOptionsClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	object_class->finalize = gprof_options_finalize;
}

/* Copy defaults into a new target specifc settings table */
static void
copy_default_key (gchar *key, gchar *value, GHashTable *new_settings)
{
	g_hash_table_insert (new_settings, g_strdup (key), g_strdup (value));
}

static void
gprof_options_parse_tree (GProfOptions *self, xmlNodePtr node)
{
	xmlNodePtr current_node;
	xmlChar *target_name;
	xmlChar *key_name;
	xmlChar *key_value;
	
	current_node = node;
	
	while (current_node)
	{
		if (strcmp ((gchar *) current_node->name, "target") == 0)
		{
			target_name = xmlGetProp (current_node, BAD_CAST "name");
			gprof_options_set_target (self, (gchar *) target_name); 
			
			xmlFree (target_name);
		}
		else if (strcmp ((gchar *) current_node->name, "key") == 0)
		{
			key_name = xmlGetProp (current_node, BAD_CAST "name");
			key_value = xmlNodeGetContent (current_node);
			
			/* Filter out deprecated keys (those that aren't registered) */
			if (g_hash_table_lookup_extended (self->priv->key_data_table,
											  (gchar *) key_name,
											  NULL, NULL))
			{
				gprof_options_set_string (self, (gchar *) key_name, 
									  	  (gchar *) key_value);
			}
			
			xmlFree (key_name);
			xmlFree (key_value);
		}
		
		gprof_options_parse_tree (self, current_node->children);
		current_node = current_node->next;
		
	}
}

static void
build_key_elements (gchar *key, gchar *value, xmlNodePtr parent)
{
	xmlNodePtr new_key;
	
	new_key = xmlNewChild (parent, NULL, BAD_CAST "key", BAD_CAST value);
	xmlNewProp (new_key, BAD_CAST "name", BAD_CAST key);
}

static void
build_target_elements (gchar *key, GHashTable *target_settings, 
					   xmlNodePtr parent)
{
	xmlNodePtr new_target;
	
	new_target = xmlNewChild (parent, NULL, BAD_CAST "target", NULL);
	xmlNewProp (new_target, BAD_CAST "name", BAD_CAST key);
	
	g_hash_table_foreach (target_settings, (GHFunc) build_key_elements, new_target);
}

/* GUI functions: callbacks and various setup functions */
static void
on_option_toggled (GtkToggleButton *button, Key *key)
{
	if (gtk_toggle_button_get_active (button))
		gprof_options_set_int (key->options, key->name, 1);
	else
		gprof_options_set_int (key->options, key->name, 0);
}

static gboolean
on_option_focus_out (GtkWidget *text_view, GdkEventFocus *event,
					 Key *key)
{
	GtkTextBuffer *text_buffer;
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *string;
	
	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
	gtk_text_buffer_get_start_iter (text_buffer, &start_iter);
	gtk_text_buffer_get_end_iter (text_buffer, &end_iter);
	string = gtk_text_buffer_get_text (text_buffer, &start_iter, &end_iter, FALSE);
	
	gprof_options_set_string (key->options, key->name, string);
	
	g_free (string);
	
	return FALSE;
}

static void
on_option_changed (GtkEditable *editable, Key *key)
{
	gchar *string;
	
	string = gtk_editable_get_chars (editable, 0, -1);
	gprof_options_set_string (key->options, key->name, string);
	
	g_free (string);
}

static void
setup_widgets (gchar *key_name, Key *key, GladeXML *gxml)
{
	GtkWidget *widget;
	GtkWidget *file_chooser_dialog;
	GtkTextBuffer *buffer;
	gchar *string;
	
	widget = glade_xml_get_widget (gxml, key->widget_name);
	
	if (widget)
	{
		switch (key->widget_type)
		{
			case OPTION_TYPE_TOGGLE:
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
											  gprof_options_get_int (key->options,
																	 key_name));
				g_signal_connect (widget, "toggled", G_CALLBACK (on_option_toggled),
								  key);
				break;
			case OPTION_TYPE_TEXT_ENTRY: 
				buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
				string = gprof_options_get_string (key->options, key_name);	
				gtk_text_buffer_set_text (buffer, string, -1);
			
				g_signal_connect (widget, "focus-out-event", G_CALLBACK (on_option_focus_out),
								  key);
			
				g_free (string);
				break;
			case OPTION_TYPE_ENTRY:
				string = gprof_options_get_string (key->options, key->name);
				gtk_entry_set_text (GTK_ENTRY (widget), string);
				
				g_signal_connect (widget, "changed", G_CALLBACK (on_option_changed),
								  key);
				
				g_free (string);
				break;
			default:
				break;
		}
	}
}

GType 
gprof_options_get_type (void)
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfOptionsClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_options_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfOptions),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_options_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "GProfOptions", &obj_info, 0);
	}
	return obj_type;
}

GProfOptions *
gprof_options_new ()
{
	return g_object_new (GPROF_OPTIONS_TYPE, NULL);
}

void
gprof_options_destroy (GProfOptions *self)
{
	g_object_unref (self);
}

void
gprof_options_set_target (GProfOptions *self, gchar *target)
{
	GHashTable *new_table;
	
	/* First, the target must have an entry in the target table. If we don't
	 * have one, set one up and copy the defaults into it. */
	
	if (!g_hash_table_lookup_extended (self->priv->targets, target,
									   NULL, NULL))
	{
		new_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, 
										   g_free);
		g_hash_table_foreach (self->priv->default_options, (GHFunc) copy_default_key,
							  new_table);
		g_hash_table_insert (self->priv->targets, g_strdup (target), new_table);
	}
	
	self->priv->current_target = g_hash_table_lookup (self->priv->targets, 
													  target);
}

/* Free returned string */
gchar *
gprof_options_get_string (GProfOptions *self, const gchar *key)
{	
	if (self->priv->current_target)
	{
		return g_strdup (g_hash_table_lookup (self->priv->current_target,
											  key));
	}
	else
	{
		g_warning ("GProfOptions: Trying to get option value with "
				   "no target.\n");
	}
	
	return NULL;
}

gint
gprof_options_get_int (GProfOptions *self, const gchar *key)
{
	gchar *value_string;
	gint value;
	
	value_string = gprof_options_get_string (self, key);
	
	if (value_string)
	{
		value = atoi (value_string);
		g_free (value_string);
	}
	else
		value = 0;
	
	return value;
}

void 
gprof_options_set_string (GProfOptions *self, gchar *key, gchar *value)
{
	if (self->priv->current_target)
	{
		g_hash_table_insert (self->priv->current_target, 
							 g_strdup (key), g_strdup (value));
	}
	else
	{
		g_warning ("GProfOptions: Trying to set option value with "
				   "no target.\n");
	}	
}

void 
gprof_options_set_int (GProfOptions *self, gchar *key, gint value)
{
	gchar *value_string;
	
	value_string = g_strdup_printf ("%i", value);
	gprof_options_set_string (self, key, value_string);
	
	g_free (value_string);
}

void
gprof_options_register_key (GProfOptions *self, gchar *key_name, 
							const gchar *default_value,
							const gchar *widget_name,
							OptionWidgetType widget_type)
{
	Key *new_key;
	
	new_key = g_new0 (Key, 1);
	
	new_key->name = g_strdup (key_name);
	new_key->widget_name = g_strdup (widget_name);
	new_key->options = self;
	new_key->widget_type = widget_type;
	
	g_hash_table_insert (self->priv->key_data_table, g_strdup (key_name), 
						 new_key);
	g_hash_table_insert (self->priv->default_options, g_strdup (key_name), 
						 g_strdup (default_value));
}

void
gprof_options_create_window (GProfOptions *self, GladeXML *gxml)
{
	g_hash_table_foreach (self->priv->key_data_table, (GHFunc) setup_widgets,
						  gxml);
}

void
gprof_options_load (GProfOptions *self, const gchar *path)
{	
	if (g_file_test (path, G_FILE_TEST_EXISTS))
	{
		xmlDocPtr settings_doc;
		xmlNodePtr root;
		
		settings_doc = xmlReadFile (path, NULL, 0);
		root = xmlDocGetRootElement (settings_doc);
		
		gprof_options_parse_tree (self, root);
		
		xmlFreeDoc (settings_doc);
		xmlCleanupParser ();
	}
}

void
gprof_options_save (GProfOptions *self, const gchar *path)
{
	xmlDocPtr settings_doc;
	xmlNodePtr root_node;
	
	settings_doc = xmlNewDoc (BAD_CAST "1.0");
	root_node = xmlNewNode (NULL, BAD_CAST "profiler-settings");
	xmlDocSetRootElement (settings_doc, root_node);
	
	g_hash_table_foreach (self->priv->targets, (GHFunc) build_target_elements, 
						  root_node);
	
	xmlSaveFormatFile (path, settings_doc, TRUE);
	
	xmlFreeDoc (settings_doc);
	xmlCleanupParser ();
}
