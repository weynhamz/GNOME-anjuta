/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  window.c
 *	Copyright (C) 2006 Armin Burgmeier
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU Library General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "window.h"
#include "transform.h"
#include "validator.h"
#include "element-editor.h"

#include <libanjuta/anjuta-plugin.h>
#include <stdlib.h>
#include <glib.h>

#include <ctype.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-class-gen-plugin.glade"

#define CC_HEADER_TEMPLATE PACKAGE_DATA_DIR"/class-templates/cc-header.tpl"
#define CC_SOURCE_TEMPLATE PACKAGE_DATA_DIR"/class-templates/cc-source.tpl"

#define GO_HEADER_TEMPLATE PACKAGE_DATA_DIR"/class-templates/go-header.tpl"
#define GO_SOURCE_TEMPLATE PACKAGE_DATA_DIR"/class-templates/go-source.tpl"

typedef struct _CgWindowPrivate CgWindowPrivate;
struct _CgWindowPrivate
{
	GladeXML *gxml;
	GtkWidget *window;
	
	CgElementEditor *editor_cc;
	CgElementEditor *editor_go_members;
	CgElementEditor *editor_go_properties;
	CgElementEditor *editor_go_signals;
	
	CgValidator *validator;
};

#define CG_WINDOW_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE( \
		(o), \
		CG_TYPE_WINDOW, \
		CgWindowPrivate \
	))

enum {
	PROP_0,

	/* Construct only */
	PROP_GLADE_XML
};

static GObjectClass *parent_class = NULL;

static const gchar *CC_SCOPE_LIST[] =
{
	"public",
	"protected",
	"private",
	NULL
};

static const gchar *CC_IMPLEMENTATION_LIST[] =
{
	"normal",
	"static",
	"virtual",
	NULL
};

static const gchar *GO_SCOPE_LIST[] =
{
	"public",
	"private",
	NULL
};

static const gchar *GO_PARAMSPEC_LIST[] =
{
	"Guess from type",
	"g_param_spec_object",
	"g_param_spec_pointer",
	"g_param_spec_enum",
	"g_param_spec_flags",
	"g_param_spec_boxed",
	NULL
};

static const CgElementEditorFlags GO_PROPERTY_FLAGS[] =
{
	{ "G_PARAM_READABLE", "R" },
	{ "G_PARAM_WRITABLE", "W" },
	{ "G_PARAM_CONSTRUCT", "C" },
	{ "G_PARAM_CONSTRUCT_ONLY", "CO" },
	{ "G_PARAM_LAX_VALIDATION", "LV" },
	{ "G_PARAM_STATIC_NAME", "SNA" },
	{ "G_PARAM_STATIC_NICK", "SNI" },
	{ "G_PARAM_STATIC_BLURB", "SBL" },
	{ NULL, NULL }
};

static const CgElementEditorFlags GO_SIGNAL_FLAGS[] =
{
	{ "G_SIGNAL_RUN_FIRST", "RF" },
	{ "G_SIGNAL_RUN_LAST", "RL" },
	{ "G_SIGNAL_RUN_CLEANUP", "RC" },
	{ "G_SIGNAL_NO_RECURSE", "NR" },
	{ "G_SIGNAL_DETAILED", "D" },
	{ "G_SIGNAL_ACTION", "A" },
	{ "G_SIGNAL_NO_HOOKS", "NH" },
	{ NULL, NULL }
};

#if 0
static void
cg_window_browse_button_clicked_cb (GtkButton *button,
                                    gpointer user_data)
{
	GtkWidget *entry;
	GtkFileChooserDialog *dialog;
	const gchar *text;
	gchar *filename;

	entry = GTK_WIDGET (user_data);
	
	dialog = GTK_FILE_CHOOSER_DIALOG (
		gtk_file_chooser_dialog_new (
			"Select A File", /* TODO: Better context for caption */
			GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL));

	gtk_file_chooser_set_do_overwrite_confirmation (
		GTK_FILE_CHOOSER(dialog), TRUE);

	text = gtk_entry_get_text (GTK_ENTRY (entry));
	if (text != NULL)
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), text);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gtk_entry_set_text (GTK_ENTRY (entry), filename);
		g_free (filename);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
}
#endif

static gchar *
cg_window_fetch_string (CgWindow *window,
                        const gchar *id)
{
	GtkWidget *widget;
	CgWindowPrivate *priv;

	priv = CG_WINDOW_PRIVATE (window);
	widget = glade_xml_get_widget (priv->gxml, id);

	g_return_val_if_fail (widget != NULL, NULL);

	if (GTK_IS_ENTRY (widget))
		return g_strdup (gtk_entry_get_text(GTK_ENTRY(widget)));
	else if (GTK_IS_COMBO_BOX (widget))
		return gtk_combo_box_get_active_text (GTK_COMBO_BOX(widget));
	else
		return NULL;
}

static gint
cg_window_fetch_integer (CgWindow *window,
                         const gchar *id)
{
	GtkWidget *widget;
	CgWindowPrivate *priv;
	
	priv = CG_WINDOW_PRIVATE(window);
	widget = glade_xml_get_widget(priv->gxml, id);
	
	g_return_val_if_fail(widget != NULL, 0);
	
	if (GTK_IS_SPIN_BUTTON(widget))
		return gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON( widget));
	else if (GTK_IS_ENTRY (widget))
		return strtol (gtk_entry_get_text (GTK_ENTRY (widget)), NULL, 0);
	else if (GTK_IS_COMBO_BOX (widget))
		return gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	else
		return 0;
}

static gboolean
cg_window_fetch_boolean (CgWindow *window,
                         const gchar *id)
{
	GtkWidget *widget;
	CgWindowPrivate *priv;
	
	priv = CG_WINDOW_PRIVATE (window);
	widget = glade_xml_get_widget (priv->gxml, id);
	
	g_return_val_if_fail (widget != NULL, FALSE);
	
	if (GTK_IS_TOGGLE_BUTTON (widget))
		return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	else
		return FALSE;
}

static void
cg_window_set_heap_value (CgWindow *window,
                          GHashTable *values,
                          GType type,
                          const gchar *name,
                          const gchar *id)
{
	gchar int_buffer[16];
	gint int_value;

	gchar *text;
	NPWValue *value;

	value = npw_value_heap_find_value (values, name);

	switch (type)
	{
	case G_TYPE_STRING:
		text = cg_window_fetch_string (window, id);
		npw_value_set_value (value, text, NPW_VALID_VALUE);
		g_free (text);
		break;
	case G_TYPE_INT:
		int_value = cg_window_fetch_integer (window, id);
		sprintf (int_buffer, "%d", int_value);
		npw_value_set_value (value, int_buffer, NPW_VALID_VALUE);
		break;
	case G_TYPE_BOOLEAN:
		npw_value_set_value (value,
			cg_window_fetch_boolean (window, id) ? "1" : "0", NPW_VALID_VALUE);

		break;
	default:
		break;
	}
}

static void
cg_window_validate_cc (CgWindow *window)
{
	CgWindowPrivate *priv;
	priv = CG_WINDOW_PRIVATE(window);
	
	if (priv->validator != NULL) g_object_unref (G_OBJECT (priv->validator));
	
	priv->validator = cg_validator_new (
		glade_xml_get_widget (priv->gxml, "create_button"),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "cc_name")),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "header_file")),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "source_file")), NULL);
}

static void
cg_window_validate_go (CgWindow *window)
{
	CgWindowPrivate *priv;
	priv = CG_WINDOW_PRIVATE (window);
	
	if (priv->validator != NULL) g_object_unref (G_OBJECT (priv->validator));
	
	priv->validator = cg_validator_new (
		glade_xml_get_widget (priv->gxml, "create_button"),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "go_name")),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "go_prefix")),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "go_type")),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "go_func_prefix")),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "header_file")),
		GTK_ENTRY (glade_xml_get_widget (priv->gxml, "source_file")), NULL);
}

static void
cg_window_top_notebook_switch_page_cb (G_GNUC_UNUSED GtkNotebook *notebook,
                                       G_GNUC_UNUSED GtkNotebookPage *page,
                                       guint page_num,
                                       gpointer user_data)
{
	CgWindow *window;
	window = CG_WINDOW(user_data);

	switch(page_num)
	{
	case 0:
		cg_window_validate_cc (window);
		break;
	case 1:
		cg_window_validate_go (window);
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

static gchar *
cg_window_class_name_to_file_name (const gchar *class_name)
{
	GString *str;
	const gchar *pos;

	str = g_string_sized_new (128);
	for (pos = class_name; *pos != '\0'; ++ pos)
	{
		if (isupper(*pos))
		{
			if (str->len > 0) g_string_append_c (str, '-');
			g_string_append_c (str, tolower (*pos));
		}
		else if (islower (*pos) || isdigit (*pos))
		{
			g_string_append_c (str, *pos);
		}
	}
	
	return g_string_free (str, FALSE);
}

static void
cg_window_add_project_toggled_cb (GtkToggleButton *button,
                                  gpointer user_data)
{
	CgWindow *window;
	CgWindowPrivate *priv;
	
	window = CG_WINDOW (user_data);
	priv = CG_WINDOW_PRIVATE (window);

	if (gtk_toggle_button_get_active (button) == FALSE)
	{
		GtkWidget* widget = glade_xml_get_widget(priv->gxml, "add_repository");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget),
		                              FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (glade_xml_get_widget (
			priv->gxml, "add_repository"), TRUE);
	}
}

static void
cg_window_cc_name_changed_cb (GtkEntry *entry,
                              gpointer user_data)
{
	CgWindow *window;
	CgWindowPrivate *priv;

	GtkWidget *file_header;
	GtkWidget *file_source;
	gchar* str_filebase;
	gchar* str_fileheader;
	gchar* str_filesource;

	window = CG_WINDOW (user_data);
	priv = CG_WINDOW_PRIVATE (window);

	file_header = glade_xml_get_widget (priv->gxml, "header_file");
	file_source = glade_xml_get_widget (priv->gxml, "source_file");

	str_filebase = cg_window_class_name_to_file_name (
		gtk_entry_get_text (GTK_ENTRY (entry)));

	str_fileheader = g_strconcat (str_filebase, ".h", NULL);
	str_filesource = g_strconcat (str_filebase, ".cc", NULL);
	g_free (str_filebase);
	
	gtk_entry_set_text (GTK_ENTRY (file_header), str_fileheader);
	gtk_entry_set_text (GTK_ENTRY (file_source), str_filesource);
	
	g_free (str_fileheader);
	g_free (str_filesource);
}

static void
cg_window_go_name_changed_cb (GtkEntry *entry,
                              gpointer user_data)
{
	CgWindow *window;
	CgWindowPrivate *priv;

	GtkWidget *type_prefix;
	GtkWidget *type_name;
	GtkWidget *func_prefix;

	gchar *str_type_prefix;
	gchar *str_type_name;
	gchar *str_func_prefix;
	const gchar *name;

	GtkWidget *file_header;
	GtkWidget *file_source;
	gchar* str_filebase;
	gchar* str_fileheader;
	gchar* str_filesource;

	window = CG_WINDOW (user_data);
	priv = CG_WINDOW_PRIVATE (window);
	
	type_prefix = glade_xml_get_widget (priv->gxml, "go_prefix");
	type_name = glade_xml_get_widget (priv->gxml, "go_type");
	func_prefix = glade_xml_get_widget (priv->gxml, "go_func_prefix");

	file_header = glade_xml_get_widget (priv->gxml, "header_file");
	file_source = glade_xml_get_widget (priv->gxml, "source_file");

	name = gtk_entry_get_text (GTK_ENTRY (entry));
	cg_transform_custom_c_type_to_g_type (name, &str_type_prefix,
	                                      &str_type_name, &str_func_prefix);

	gtk_entry_set_text (GTK_ENTRY (type_prefix), str_type_prefix);
	gtk_entry_set_text (GTK_ENTRY (type_name), str_type_name);
	gtk_entry_set_text (GTK_ENTRY (func_prefix), str_func_prefix);
	
	g_free (str_type_prefix);
	g_free (str_type_name);
	g_free (str_func_prefix);
	
	str_filebase = cg_window_class_name_to_file_name (name);
	str_fileheader = g_strconcat (str_filebase, ".h", NULL);
	str_filesource = g_strconcat (str_filebase, ".c", NULL);
	g_free (str_filebase);
	
	gtk_entry_set_text (GTK_ENTRY (file_header), str_fileheader);
	gtk_entry_set_text (GTK_ENTRY (file_source), str_filesource);
	
	g_free (str_fileheader);
	g_free (str_filesource);
}

#if 0
static void
cg_window_associate_browse_button (GladeXML *xml,
                                   const gchar *button_id,
                                   const gchar *entry_id)
{
	GtkWidget *button;
	GtkWidget *entry;
	
	button = glade_xml_get_widget (xml, button_id);
	entry = glade_xml_get_widget (xml, entry_id);

	g_return_if_fail (GTK_IS_BUTTON (button) && GTK_IS_ENTRY (entry));

	g_signal_connect(G_OBJECT(button), "clicked",
	                 G_CALLBACK(cg_window_browse_button_clicked_cb), entry);
}
#endif

static void
cg_window_set_glade_xml (CgWindow *window,
                         GladeXML *xml)
{
	CgWindowPrivate *priv;
	priv = CG_WINDOW_PRIVATE (window);

	priv->gxml = xml;
	g_object_ref (priv->gxml);

	priv->window = glade_xml_get_widget (priv->gxml, "classgen_main");

#if 0
	cg_window_associate_browse_button (priv->gxml, "browse_header",
	                                   "header_file");

	cg_window_associate_browse_button (priv->gxml, "browse_source",
	                                   "source_file");
#endif

	priv->editor_cc = cg_element_editor_new (
		GTK_TREE_VIEW (glade_xml_get_widget (priv->gxml, "cc_elements")),
		GTK_BUTTON (glade_xml_get_widget (priv->gxml, "cc_elements_add")),
		GTK_BUTTON (glade_xml_get_widget (priv->gxml, "cc_elements_remove")),
		5,
		"Scope", CG_ELEMENT_EDITOR_COLUMN_LIST, CC_SCOPE_LIST,
		"Implementation", CG_ELEMENT_EDITOR_COLUMN_LIST, CC_IMPLEMENTATION_LIST,
		"Type", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Name", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Arguments", CG_ELEMENT_EDITOR_COLUMN_ARGUMENTS);
	
	priv->editor_go_members = cg_element_editor_new (
		GTK_TREE_VIEW (glade_xml_get_widget (priv->gxml, "go_members")),
		GTK_BUTTON (glade_xml_get_widget (priv->gxml, "go_members_add")),
		GTK_BUTTON (glade_xml_get_widget (priv->gxml, "go_members_remove")),
		4,
		"Scope", CG_ELEMENT_EDITOR_COLUMN_LIST, GO_SCOPE_LIST,
		"Type", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Name", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Arguments", CG_ELEMENT_EDITOR_COLUMN_ARGUMENTS);
	
	priv->editor_go_properties = cg_element_editor_new(
		GTK_TREE_VIEW (glade_xml_get_widget (priv->gxml, "go_properties")),
		GTK_BUTTON (glade_xml_get_widget (priv->gxml, "go_properties_add")),
		GTK_BUTTON (glade_xml_get_widget (priv->gxml, "go_properties_remove")),
		7,
		"Name", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Nick", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Blurb", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"GType", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"ParamSpec", CG_ELEMENT_EDITOR_COLUMN_LIST, GO_PARAMSPEC_LIST,
		"Default", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Flags", CG_ELEMENT_EDITOR_COLUMN_FLAGS, GO_PROPERTY_FLAGS);
	
	priv->editor_go_signals = cg_element_editor_new(
		GTK_TREE_VIEW (glade_xml_get_widget (priv->gxml, "go_signals")),
		GTK_BUTTON (glade_xml_get_widget (priv->gxml, "go_signals_add")),
		GTK_BUTTON (glade_xml_get_widget (priv->gxml, "go_signals_remove")),
		5,
		"Type", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Name", CG_ELEMENT_EDITOR_COLUMN_STRING,
		"Arguments", CG_ELEMENT_EDITOR_COLUMN_ARGUMENTS, /* Somehow redundant with marshaller, but required for default handler */
		"Flags", CG_ELEMENT_EDITOR_COLUMN_FLAGS, GO_SIGNAL_FLAGS,
		"Marshaller", CG_ELEMENT_EDITOR_COLUMN_STRING);

	/* Active item property in glade cannot be set because no GtkTreeModel
	 * is assigned. */
	gtk_combo_box_set_active (
		GTK_COMBO_BOX (glade_xml_get_widget (priv->gxml, "license")), 0);

	gtk_combo_box_set_active (
		GTK_COMBO_BOX (glade_xml_get_widget (priv->gxml, "cc_inheritance")),
		0);

	/* This revalidates the appropriate validator */
	g_signal_connect (
		G_OBJECT (glade_xml_get_widget (priv->gxml, "top_notebook")),
		"switch-page", G_CALLBACK (cg_window_top_notebook_switch_page_cb),
		window);
	
	g_signal_connect (
		G_OBJECT (glade_xml_get_widget (priv->gxml, "go_name")), "changed",
		G_CALLBACK (cg_window_go_name_changed_cb), window);

	g_signal_connect (
		G_OBJECT (glade_xml_get_widget (priv->gxml, "cc_name")), "changed",
		G_CALLBACK (cg_window_cc_name_changed_cb), window);

	g_signal_connect (
		G_OBJECT (glade_xml_get_widget(priv->gxml, "add_project")), "toggled",
		G_CALLBACK (cg_window_add_project_toggled_cb), window);

	cg_window_add_project_toggled_cb (GTK_TOGGLE_BUTTON (
		glade_xml_get_widget (priv->gxml, "add_project")), window);

	/* Selected page is CC */
	cg_window_validate_cc (window);
}

static void
cg_window_cc_transform_func (GHashTable *table,
                             G_GNUC_UNUSED gpointer user_data)
{
	cg_transform_arguments (table, "Arguments", FALSE);
}

static void
cg_window_go_members_transform_func (GHashTable *table,
                                     gpointer user_data)
{
	CgWindow *window;
	gchar *name;
	gchar *func_prefix;

	/* Strip of func prefix of members if they contain any, the prefix
	 * is added by the autogen template. */
	window = CG_WINDOW (user_data);
	name = g_hash_table_lookup (table, "Name");
	func_prefix = cg_window_fetch_string (window, "go_func_prefix");

	if (g_str_has_prefix (name, func_prefix))
	{
		name = g_strdup (name + strlen (func_prefix) + 1);
		g_hash_table_insert (table, "Name", name);
	}
	
	g_free (func_prefix);
	cg_transform_arguments (table, "Arguments", TRUE);
}

static void
cg_window_go_properties_transform_func (GHashTable *table,
                                        G_GNUC_UNUSED gpointer user_data)
{
	gchar *paramspec;

	cg_transform_string (table, "Name");
	cg_transform_string (table, "Nick");
	cg_transform_string (table, "Blurb");

	cg_transform_guess_paramspec (table, "ParamSpec",
	                              "Type", GO_PARAMSPEC_LIST[0]);

	cg_transform_flags (table, "Flags", GO_PROPERTY_FLAGS);

	paramspec = g_hash_table_lookup (table, "ParamSpec");
	if (paramspec && (strcmp (paramspec, "g_param_spec_string") == 0))
		cg_transform_string (table, "Default");
}

static void
cg_window_go_signals_transform_func (GHashTable *table,
                                     gpointer user_data)
{
	CgWindow *window;

	gchar *type;
	guint arg_count;
	
	gchar *gtype_prefix;
	gchar *gtype_suffix;
	gchar *name;
	gchar *self_type;
	
	window = CG_WINDOW (user_data);

	cg_transform_string (table, "Name");

	/* Provide GType of return type */
	type = g_hash_table_lookup (table, "Type");
	if (type != NULL)
	{
		cg_transform_c_type_to_g_type (type, &gtype_prefix, &gtype_suffix);
		g_hash_table_insert (table, "GTypePrefix", gtype_prefix);
		g_hash_table_insert (table, "GTypeSuffix", gtype_suffix);
	}

	cg_transform_arguments (table, "Arguments", TRUE);

	/* Add self as signal's first argument */
	name = cg_window_fetch_string (window, "go_name");
	self_type = g_strconcat (name, "*", NULL);
	g_free (name);
	
	cg_transform_first_argument (table, "Arguments", self_type);
	g_free (self_type);

	/* Provide GTypes and amount of arguments */
	arg_count = cg_transform_arguments_to_gtypes (table, "Arguments",
	                                              "ArgumentGTypes");

	g_hash_table_insert (table, "ArgumentCount",
	                     g_strdup_printf ("%u", arg_count));

	cg_transform_flags (table, "Flags", GO_SIGNAL_FLAGS);
}

#if 0
static gboolean
cg_window_scope_condition_func (const gchar **elements,
                                gpointer user_data)
{
	/* Matches all members in the given scope */
	if (elements[0] == NULL) return FALSE;
	if (strcmp (elements[0], (const gchar *) user_data) == 0) return TRUE;
	return FALSE;
}
#endif

static gboolean
cg_window_scope_with_args_condition_func (const gchar **elements,
                                          gpointer user_data)
{
	/* Matches all members in the given scope that have arguments set */
	if (elements[0] == NULL) return FALSE;
	if (elements[3] == NULL || *(elements[3]) == '\0') return FALSE;
	if (strcmp (elements[0], (const gchar *) user_data) != 0) return FALSE;
	return TRUE;
}

static gboolean
cg_window_scope_without_args_condition_func (const gchar **elements,
                                             gpointer user_data)
{
	/* Matches all members in the given scope that have no arguments set */
	if (elements[0] == NULL) return FALSE;
	if (elements[3] != NULL && *(elements[3]) != '\0') return FALSE;
	if (strcmp(elements[0], (const gchar *) user_data) != 0) return FALSE;
	return TRUE;
}

static void
cg_window_init (CgWindow *window)
{
	CgWindowPrivate *priv;
	priv = CG_WINDOW_PRIVATE (window);

	priv->gxml = NULL;
	priv->window = NULL;
	
	priv->editor_cc = NULL;
	priv->editor_go_members = NULL;
	priv->editor_go_properties = NULL;
	priv->editor_go_signals = NULL;
	
	priv->validator = NULL;
}

static void 
cg_window_finalize (GObject *object)
{
	CgWindow *window;
	CgWindowPrivate *priv;
	
	window = CG_WINDOW (object);
	priv = CG_WINDOW_PRIVATE (window);

	if (priv->editor_cc != NULL)
		g_object_unref (G_OBJECT (priv->editor_cc));
	if (priv->editor_go_members != NULL)
		g_object_unref (G_OBJECT (priv->editor_go_members));
	if (priv->editor_go_properties != NULL)
		g_object_unref (G_OBJECT (priv->editor_go_properties));
	if (priv->editor_go_signals != NULL)
		g_object_unref (G_OBJECT (priv->editor_go_signals));

	if (priv->validator != NULL)
		g_object_unref (G_OBJECT (priv->validator));

	if (priv->gxml != NULL)
		g_object_unref (G_OBJECT (priv->gxml));

	gtk_widget_destroy(priv->window);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cg_window_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
	CgWindow *window;
	CgWindowPrivate *priv;

	g_return_if_fail (CG_IS_WINDOW (object));

	window = CG_WINDOW (object);
	priv = CG_WINDOW_PRIVATE (window);

	switch (prop_id)
	{
	case PROP_GLADE_XML:
		cg_window_set_glade_xml (window,
		                         GLADE_XML (g_value_get_object (value)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_window_get_property (GObject *object,
                        guint prop_id,
                        GValue *value, 
                        GParamSpec *pspec)
{
	CgWindow *window;
	CgWindowPrivate *priv;

	g_return_if_fail (CG_IS_WINDOW (object));

	window = CG_WINDOW (object);
	priv = CG_WINDOW_PRIVATE (window);

	switch (prop_id)
	{
	case PROP_GLADE_XML:
		g_value_set_object (value, G_OBJECT (priv->gxml));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_window_class_init (CgWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	g_type_class_add_private (klass, sizeof (CgWindowPrivate));

	object_class->finalize = cg_window_finalize;
	object_class->set_property = cg_window_set_property;
	object_class->get_property = cg_window_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_GLADE_XML,
	                                 g_param_spec_object ("glade-xml",
	                                                      "Glade XML description",
	                                                      _("XML description of the user interface"),
	                                                      GLADE_TYPE_XML,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

GType
cg_window_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (CgWindowClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) cg_window_class_init,
			NULL,
			NULL,
			sizeof (CgWindow),
			0,
			(GInstanceInitFunc) cg_window_init,
			NULL
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "CgWindow",
		                                   &our_info, 0);
	}

	return our_type;
}

CgWindow *
cg_window_new (void)
{
	GladeXML *gxml;
	GObject *window;

	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	if (gxml == NULL) return NULL;
	
	window = g_object_new (CG_TYPE_WINDOW, "glade-xml", gxml, NULL);
	return CG_WINDOW (window);
}

GtkDialog *
cg_window_get_dialog (CgWindow *window)
{
	return GTK_DIALOG (CG_WINDOW_PRIVATE (window)->window);
}

GHashTable *
cg_window_create_value_heap (CgWindow *window)
{
	static const gchar *LICENSES[] = {
		"GPL",
		"LGPL",
		"NONE"
	};

	CgWindowPrivate *priv;
	GHashTable *values;
	NPWValue *value;
	GError *error;
	gint license_index;
	
	GtkNotebook *notebook;

	gchar *header_file;
	gchar *source_file;

	gchar *text;	
	gchar *base_prefix;
	gchar *base_suffix;

	priv = CG_WINDOW_PRIVATE (window);
	notebook = GTK_NOTEBOOK (glade_xml_get_widget (priv->gxml,
	                                               "top_notebook"));

	values = npw_value_heap_new ();
	error = NULL;

	switch (gtk_notebook_get_current_page (notebook))
	{
	case 0: /* cc */
		cg_window_set_heap_value (window, values, G_TYPE_STRING,
		                          "ClassName", "cc_name");
		cg_window_set_heap_value (window, values, G_TYPE_STRING,
		                          "BaseClass", "cc_base");
		cg_window_set_heap_value (window, values, G_TYPE_STRING,
		                          "Inheritance", "cc_inheritance");
		cg_window_set_heap_value (window, values, G_TYPE_BOOLEAN,
		                          "Headings", "cc_headings");
		cg_window_set_heap_value (window, values, G_TYPE_BOOLEAN,
		                          "Inline", "cc_inline");

		cg_element_editor_set_values (priv->editor_cc, "Elements", values,
		                              cg_window_cc_transform_func, window,
		                              "Scope", "Implementation", "Type",
		                              "Name", "Arguments");

		break;
	case 1: /* GObject */
		cg_window_set_heap_value (window, values, G_TYPE_STRING,
		                          "ClassName", "go_name");
		cg_window_set_heap_value(window, values, G_TYPE_STRING,
		                         "BaseClass", "go_base");
		cg_window_set_heap_value(window, values, G_TYPE_STRING,
		                         "TypePrefix", "go_prefix");
		cg_window_set_heap_value(window, values, G_TYPE_STRING,
		                         "TypeSuffix", "go_type");

		/* Store GType of base class which is also required */
		text = cg_window_fetch_string (window, "go_base");
		cg_transform_custom_c_type_to_g_type (text, &base_prefix,
		                                      &base_suffix, NULL);

		g_free (text);

		value = npw_value_heap_find_value (values, "BaseTypePrefix");
		npw_value_set_value (value, base_prefix, NPW_VALID_VALUE);
		
		value = npw_value_heap_find_value (values, "BaseTypeSuffix");
		npw_value_set_value (value, base_suffix, NPW_VALID_VALUE);
		
		g_free (base_prefix);
		g_free (base_suffix);

		cg_window_set_heap_value (window, values, G_TYPE_STRING,
		                          "FuncPrefix", "go_func_prefix");

		cg_window_set_heap_value (window, values, G_TYPE_BOOLEAN,
		                          "Headings", "go_headings");

		cg_element_editor_set_values (priv->editor_go_members, "Members",
		                              values,
		                              cg_window_go_members_transform_func,
		                              window, "Scope", "Type", "Name",
		                              "Arguments");

		/* These count the amount of members that match certain conditions
		 * and that would be relatively hard to find out in the autogen
		 * file (at least with my limited autogen skills). These are the
		 * number of private respectively public member functions (arguments
		 * set) and variables (no arguments set). */
		cg_element_editor_set_value_count (priv->editor_go_members,
			"PrivateFunctionCount", values,
			cg_window_scope_with_args_condition_func, "private");

		cg_element_editor_set_value_count (priv->editor_go_members,
			"PrivateVariableCount", values,
			cg_window_scope_without_args_condition_func, "private");

		cg_element_editor_set_value_count (priv->editor_go_members,
			"PublicFunctionCount", values,
			cg_window_scope_with_args_condition_func, "public");
		
		cg_element_editor_set_value_count (priv->editor_go_members,
			"PublicVariableCount", values,
			cg_window_scope_without_args_condition_func, "public");

		cg_element_editor_set_values (priv->editor_go_properties, "Properties",
		                              values,
		                              cg_window_go_properties_transform_func,
		                              window, "Name", "Nick", "Blurb", "Type",
		                              "ParamSpec", "Default", "Flags");

		cg_element_editor_set_values (priv->editor_go_signals, "Signals",
		                              values,
		                              cg_window_go_signals_transform_func,
		                              window, "Type", "Name", "Arguments",
		                              "Flags", "Marshaller");

		break;
	default:
		g_assert_not_reached ();
		break;
	}

	cg_window_set_heap_value (window, values, G_TYPE_STRING,
	                          "AuthorName", "author_name");

	cg_window_set_heap_value(window, values, G_TYPE_STRING,
	                         "AuthorEmail", "author_email");

	license_index = cg_window_fetch_integer (window, "license");
	value = npw_value_heap_find_value (values, "License");

	npw_value_set_value(value, LICENSES[license_index],
	                         NPW_VALID_VALUE);

	header_file = g_path_get_basename (cg_window_get_header_file (window));
	source_file = g_path_get_basename (cg_window_get_source_file (window));
	
	value = npw_value_heap_find_value (values, "HeaderFile");
	npw_value_set_value (value, header_file, NPW_VALID_VALUE);
	
	value = npw_value_heap_find_value (values, "SourceFile");
	npw_value_set_value (value, source_file, NPW_VALID_VALUE);
	
	g_free (header_file);
	g_free (source_file);
	
	return values;
}

const gchar *
cg_window_get_header_template (CgWindow *window)
{
	CgWindowPrivate *priv;
	GtkNotebook *notebook;

	priv = CG_WINDOW_PRIVATE (window);
	notebook = GTK_NOTEBOOK (glade_xml_get_widget (priv->gxml,
	                                               "top_notebook"));
	
	g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), NULL);

	switch(gtk_notebook_get_current_page (notebook))
	{
	case 0:
		return CC_HEADER_TEMPLATE;
	case 1:
		return GO_HEADER_TEMPLATE;
	default:
		g_assert_not_reached ();
		return NULL;
	}
}

const gchar *
cg_window_get_header_file(CgWindow *window)
{
	CgWindowPrivate *priv;
	GtkEntry *entry;

	priv = CG_WINDOW_PRIVATE (window);
	entry = GTK_ENTRY (glade_xml_get_widget (priv->gxml, "header_file"));
	
	g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);
	return gtk_entry_get_text (entry);
}

const gchar *
cg_window_get_source_template(CgWindow *window)
{
	CgWindowPrivate *priv;
	GtkNotebook *notebook;

	priv = CG_WINDOW_PRIVATE (window);
	notebook = GTK_NOTEBOOK (glade_xml_get_widget (priv->gxml,
	                                               "top_notebook"));
	
	g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), NULL);

	switch(gtk_notebook_get_current_page (notebook))
	{
	case 0:
		return CC_SOURCE_TEMPLATE;
	case 1:
		return GO_SOURCE_TEMPLATE;
	default:
		g_assert_not_reached ();
		return NULL;
	}
}

const gchar *
cg_window_get_source_file (CgWindow *window)
{
	CgWindowPrivate *priv;
	GtkEntry *entry;

	priv = CG_WINDOW_PRIVATE (window);
	entry = GTK_ENTRY (glade_xml_get_widget (priv->gxml, "source_file"));
	
	g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);
	return gtk_entry_get_text (entry);
}

void
cg_window_set_add_to_project (CgWindow *window,
                              gboolean enable)
{
	CgWindowPrivate *priv;
	GtkCheckButton *button;

	priv = CG_WINDOW_PRIVATE (window);
	button = GTK_CHECK_BUTTON (glade_xml_get_widget (priv->gxml,
	                                                 "add_project"));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), enable);
}

void
cg_window_set_add_to_repository (CgWindow *window,
                                 gboolean enable)
{
	CgWindowPrivate *priv;
	GtkCheckButton *button;

	priv = CG_WINDOW_PRIVATE (window);
	button = GTK_CHECK_BUTTON (glade_xml_get_widget (priv->gxml,
	                                                 "add_repository"));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), enable);
}

gboolean
cg_window_get_add_to_project(CgWindow *window)
{
	CgWindowPrivate *priv;
	priv = CG_WINDOW_PRIVATE (window);

	return cg_window_fetch_boolean (window, "add_project");
}

gboolean
cg_window_get_add_to_repository (CgWindow *window)
{
	CgWindowPrivate *priv;
	priv = CG_WINDOW_PRIVATE (window);

	return cg_window_fetch_boolean (window, "add_repository");
}

void
cg_window_enable_add_to_project (CgWindow *window,
                                 gboolean enable)
{
	CgWindowPrivate *priv;
	GtkWidget *widget;
	
	priv = CG_WINDOW_PRIVATE (window);
	widget = glade_xml_get_widget (priv->gxml, "add_project");
	
	gtk_widget_set_sensitive (widget, enable);
}

void
cg_window_enable_add_to_repository (CgWindow *window,
                                    gboolean enable)
{
	CgWindowPrivate *priv;
	GtkWidget *widget;
	
	priv = CG_WINDOW_PRIVATE (window);
	widget = glade_xml_get_widget (priv->gxml, "add_repository");
	
	gtk_widget_set_sensitive (widget, enable);
}

void
cg_window_set_author (CgWindow *window,
                      const gchar *author)
{
	CgWindowPrivate* priv;
	GtkEntry* entry;
	
	priv = CG_WINDOW_PRIVATE (window);
	entry = GTK_ENTRY(glade_xml_get_widget (priv->gxml, "author_name"));
	
	gtk_entry_set_text (entry, author);
}

void
cg_window_set_email (CgWindow *window,
                     const gchar *email)
{
	CgWindowPrivate* priv;
	GtkEntry* entry;
	
	priv = CG_WINDOW_PRIVATE (window);
	entry = GTK_ENTRY(glade_xml_get_widget (priv->gxml, "author_email"));
	
	gtk_entry_set_text (entry, email);
}
