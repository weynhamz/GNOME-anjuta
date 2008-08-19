/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    editor.c
    Copyright (C) 2003 Biswapesh Chattopadhyay

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * Tool editor dialog
 * This is used to set all tool informations.
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "editor.h"

#include "dialog.h"
#include "tool.h"
#include "variable.h"

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <gtk/gtk.h>

#include <string.h>

/*---------------------------------------------------------------------------*/

/* Variable dialog */

typedef enum {
	ATP_VARIABLE_DEFAULT = 0,
	ATP_VARIABLE_REPLACE = 1 << 1
} ATPVariableType;

typedef struct _ATPVariableDialog
{
	GtkDialog* dialog;
	GtkTreeView* view;
	ATPToolEditor* editor;
	GtkEditable* entry;
	ATPVariableType type;
} ATPVariableDialog;	

enum {
	ATP_VARIABLE_NAME_COLUMN,
	ATP_VARIABLE_MEAN_COLUMN,
	ATP_VARIABLE_VALUE_COLUMN,
	ATP_N_VARIABLE_COLUMNS,
};

/* Structure containing the required properties of the tool editor GUI */
struct _ATPToolEditor
{
	GtkWidget *dialog;
	GtkEditable *name_en;
	GtkEditable *command_en;
	GtkEditable *param_en;
	ATPVariableDialog param_var;
	GtkEditable *dir_en;
	ATPVariableDialog dir_var;
	GtkToggleButton *enabled_tb;
	GtkToggleButton *terminal_tb;
	GtkToggleButton *autosave_tb;
	GtkToggleButton *script_tb;
	GtkComboBox *output_com;
	GtkComboBox *error_com;
	GtkComboBox *input_com;
	GtkEditable *input_en;
	GtkButton *input_var_bt;
	ATPVariableDialog input_file_var;
	ATPVariableDialog input_string_var;
	GtkToggleButton *shortcut_bt;
	GnomeIconEntry *icon_en;
	gchar* shortcut;
	ATPUserTool *tool;
	ATPToolDialog* parent;
	ATPToolEditorList* owner;
	ATPToolEditor* next;
};

/*---------------------------------------------------------------------------*/

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define TOOL_EDITOR "editor_dialog"
#define TOOL_NAME "name_entry"
#define TOOL_COMMAND "command_entry"
#define TOOL_PARAM "parameter_entry"
#define TOOL_WORKING_DIR "directory_entry"
#define TOOL_ENABLED "enable_checkbox"
#define TOOL_AUTOSAVE "save_checkbox"
#define TOOL_TERMINAL "terminal_checkbox"
#define TOOL_SCRIPT "script_checkbox"
#define TOOL_OUTPUT "output_combo"
#define TOOL_ERROR "error_combo"
#define TOOL_INPUT "input_combo"
#define TOOL_INPUT_VALUE "input_entry"
#define TOOL_INPUT_VARIABLE "input_button"
#define TOOL_SHORTCUT "shortcut_bt"
#define TOOL_ICON "icon_entry"

#define EDITOR_RESPONSE_SIGNAL "on_editor_dialog_response"
#define EDITOR_PARAM_VARIABLE_SIGNAL "on_variable_parameter"
#define EDITOR_DIR_VARIABLE_SIGNAL "on_variable_directory"
#define EDITOR_INPUT_VARIABLE_SIGNAL "on_variable_input"
#define EDITOR_INPUT_CHANGED_SIGNAL "on_input_changed"
#define EDITOR_TOGGLE_TERMINAL_SIGNAL "on_toggle_terminal"
#define EDITOR_TOGGLE_SHORCUT_SIGNAL "on_toggle_shorcut"
#define EDITOR_TOGGLE_SCRIPT_SIGNAL "on_toggle_script"

#define TOOL_VARIABLE "variable_dialog"
#define VARIABLE_TREEVIEW "variable_treeview"
#define VARIABLE_RESPONSE_SIGNAL "on_variable_dialog_response"
#define VARIABLE_ACTIVATE_SIGNAL "on_variable_activate_row"

/* Add helper function
 *---------------------------------------------------------------------------*/

static gboolean
make_directory (gchar* path)
{
	gchar c;
	gchar* ptr;

	for (ptr = path; *ptr;)
	{
		/* Get next directory */
		for (c = '\0'; *ptr; ++ptr)
		{
			if (*ptr == G_DIR_SEPARATOR)
			{
				/* Strip leading directory separator */
				do
				{
					++ptr;
				} while (*ptr == G_DIR_SEPARATOR);
				c = *ptr;
				*ptr = '\0';
				break;
			}
		}

		/* Create it */
		if (mkdir (path, 0755) < 0)
		{
			/* Error creating directory */
			*ptr = c;
			if (errno != EEXIST)
			{
		 		/* An already existing directory is not an error */		
				return FALSE;
			}
		}
		else
		{
			/* New directory created */
			*ptr = c;
		}
	}

	return TRUE;
}

static void
set_combo_box_enum_model (GtkComboBox* combo_box, const ATPEnumType* list)
{
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));

	for (; list->id != -1;++list)
	{
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, _(list->name), 1, list->id, -1);
	}
	gtk_combo_box_set_model (combo_box, model);
}

static gint
get_combo_box_value (GtkComboBox* combo_box)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint value = -1;

	if (gtk_combo_box_get_active_iter (combo_box, &iter))
	{
		model = gtk_combo_box_get_model (combo_box);
		gtk_tree_model_get (model, &iter, 1, &value, -1);
	}

	return value;
}

static gboolean
set_combo_box_value (GtkComboBox* combo_box, gint value)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint current;

	if (value != -1)
	{
		model = gtk_combo_box_get_model (combo_box);
		if (gtk_tree_model_get_iter_first (model, &iter))
		{
			do
			{
				gtk_tree_model_get (model, &iter, 1, &current, -1);
				if (value == current)
				{
					gtk_combo_box_set_active_iter (combo_box, &iter);
	
					return TRUE;
				}
			}
			while (gtk_tree_model_iter_next (model, &iter));
		}
	}

	gtk_combo_box_set_active (combo_box, 0);

	return FALSE;
}

/* Tool variable dialog
 * Display a list of variables useable as command line parameters or working
 * directory with their values. Clicking on a variable add it in the current
 * entry widget.
 *---------------------------------------------------------------------------*/

static ATPVariableDialog*
atp_variable_dialog_construct (ATPVariableDialog* this, ATPToolEditor* editor, ATPVariableType type)
{
	this->dialog = NULL;
	this->editor = editor;
	this->type = type;

	return this;
}

static void
atp_variable_dialog_destroy (ATPVariableDialog* this)
{
	if (this->dialog)
	{
		gtk_widget_destroy (GTK_WIDGET (this->dialog));
		this->dialog = NULL;
	}
}

static void
atp_variable_dialog_set_entry (ATPVariableDialog *this, GtkEditable* entry)
{
	this->entry = entry;
}

static void
atp_variable_dialog_populate (ATPVariableDialog* this, ATPFlags flag)
{
	GtkTreeModel *model;
	ATPVariable* variable;
	guint i;

	variable = atp_tool_dialog_get_variable (this->editor->parent);
	model = gtk_tree_view_get_model (this->view);
	gtk_list_store_clear (GTK_LIST_STORE(model));

	for (i = atp_variable_get_count(variable); i > 0;)
	{
		GtkTreeIter iter;
		gchar* value;
		const gchar* value_col;

		--i;
		if ((flag == ATP_DEFAULT_VARIABLE) || (flag & atp_variable_get_flag (variable, i)))
		{
			if (atp_variable_get_flag (variable, i) & ATP_INTERACTIVE_VARIABLE)
			{
				value = NULL;
				value_col = _("ask at runtime");
			}
			else
			{
				value = atp_variable_get_value_from_id (variable, i);
				value_col = (value == NULL) ? _("undefined") : value;
			}
			gtk_list_store_append (GTK_LIST_STORE(model), &iter);
			gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				ATP_VARIABLE_NAME_COLUMN,
				atp_variable_get_name(variable, i),
				ATP_VARIABLE_MEAN_COLUMN,
				_(atp_variable_get_help(variable, i)),
				ATP_VARIABLE_VALUE_COLUMN,
				value_col,
				-1);
			if (value) g_free (value);
		}
	}
}

static void
atp_variable_dialog_add_variable(ATPVariableDialog *this, const gchar* text)
{
	gint pos;

	g_return_if_fail (this->entry);

	if (text != NULL)
	{
		gchar* next;

		if (this->type == ATP_VARIABLE_REPLACE)
		{
			gtk_editable_delete_text (this->entry, 0, -1);
		}
	
		pos = gtk_editable_get_position(this->entry);
		/* Add space before if useful */
		if (pos != 0)
		{
			next = gtk_editable_get_chars (this->entry, pos - 1, pos);

			if (!g_ascii_isspace (*next))
			{
				gtk_editable_insert_text (this->entry, " ", 1, &pos);
			}
			g_free (next);
		}
		gtk_editable_insert_text (this->entry, "$(", 2, &pos);
		gtk_editable_insert_text (this->entry, text, strlen(text), &pos);
		gtk_editable_insert_text (this->entry, ")", 1, &pos);
		/* Add space after if useful */
		next = gtk_editable_get_chars (this->entry, pos, pos + 1);
		if (next != NULL)
		{
			if ((*next != '\0') && (!g_ascii_isspace (*next)))
			{
				gtk_editable_insert_text (this->entry, " ",1, &pos);
			}
			g_free (next);
		}
	}
}

static gchar*
get_current_name (GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	gchar* name;

	model = gtk_tree_view_get_model (view);
	sel = gtk_tree_view_get_selection (view);
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, ATP_VARIABLE_NAME_COLUMN, &name, -1);

		return name;
	}

	return NULL;
}

static void
on_variable_activate (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	ATPVariableDialog *this = (ATPVariableDialog*)user_data;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar* name;

	/* Get Selected variable name */
	model = gtk_tree_view_get_model (treeview);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, ATP_VARIABLE_NAME_COLUMN, &name, -1);

	atp_variable_dialog_add_variable (this, name);	

	gtk_widget_hide (GTK_WIDGET (this->dialog));
}

static void
on_variable_response (GtkDialog *dialog, gint response, gpointer user_data)
{
	ATPVariableDialog *this = (ATPVariableDialog *)user_data;
	gchar* name;

	switch (response)
	{
	case GTK_RESPONSE_OK:
		name = get_current_name (this->view);
		atp_variable_dialog_add_variable (this, name);
		break;
	default:
		break;
	}

	gtk_widget_hide (GTK_WIDGET (this->dialog));
}

static gboolean
atp_variable_dialog_show (ATPVariableDialog* this, ATPFlags flag)
{
	GladeXML *xml;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	if (this->dialog != NULL)
	{
		/* Display dialog box */
		if (this->dialog) gtk_window_present (GTK_WINDOW (this->dialog));
		return TRUE;
	}
	
	if (NULL == (xml = glade_xml_new(GLADE_FILE, TOOL_VARIABLE, NULL)))
	{
		anjuta_util_dialog_error (GTK_WINDOW (this->editor->dialog), _("Unable to build user interface for tool variable"));
		return FALSE;
	}
	this->dialog = GTK_DIALOG (glade_xml_get_widget(xml, TOOL_VARIABLE));
	gtk_widget_show (GTK_WIDGET (this->dialog));
	gtk_window_set_transient_for (GTK_WINDOW (this->dialog), GTK_WINDOW (this->editor->dialog));

	/* Create variable list */
	this->view = GTK_TREE_VIEW (glade_xml_get_widget(xml, VARIABLE_TREEVIEW));
	model = GTK_TREE_MODEL (gtk_list_store_new (ATP_N_VARIABLE_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
	gtk_tree_view_set_model (this->view, model);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Variable"), renderer, "text", ATP_VARIABLE_NAME_COLUMN, NULL);
	gtk_tree_view_append_column (this->view, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Meaning"), renderer, "text", ATP_VARIABLE_MEAN_COLUMN, NULL);
	gtk_tree_view_append_column (this->view, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Value"), renderer, "text", ATP_VARIABLE_VALUE_COLUMN, NULL);
	gtk_tree_view_append_column (this->view, column);
	g_object_unref (model);
	atp_variable_dialog_populate (this, flag);

	/* Connect all signals */	
	glade_xml_signal_connect_data (xml, VARIABLE_RESPONSE_SIGNAL, GTK_SIGNAL_FUNC (on_variable_response), this);
	glade_xml_signal_connect_data (xml, VARIABLE_ACTIVATE_SIGNAL, GTK_SIGNAL_FUNC (on_variable_activate), this);
	g_signal_connect (G_OBJECT (this->dialog), "delete_event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

	g_object_unref (xml);

	return TRUE;
}

/* Tool editor dialog
 *---------------------------------------------------------------------------*/

static void
atp_clear_tool_editor(ATPToolEditor* this)
{
	g_return_if_fail (this != NULL);

	gtk_editable_delete_text(this->name_en, 0, -1);
	gtk_editable_delete_text(this->command_en, 0, -1);
	gtk_editable_delete_text(this->param_en, 0, -1);
	gtk_editable_delete_text(this->dir_en, 0, -1);
}

static void
atp_update_sensitivity(ATPToolEditor *this)
{
	gboolean en;

	/* Deactivate output and input setting if a terminal is used */
	en = gtk_toggle_button_get_active (this->terminal_tb);
	gtk_widget_set_sensitive(GTK_WIDGET (this->output_com), !en);
	gtk_widget_set_sensitive(GTK_WIDGET (this->error_com), !en);
	gtk_widget_set_sensitive(GTK_WIDGET (this->input_com), !en);

	/* input value is available for a few input type only */
	if (!en)
	{
		switch (get_combo_box_value (this->input_com))
		{
		case ATP_TIN_FILE:
		case ATP_TIN_STRING:
			en = TRUE;
			break;
		default:
			en = FALSE;
			break;
		}
		gtk_widget_set_sensitive(GTK_WIDGET (this->input_en), en);
		gtk_widget_set_sensitive(GTK_WIDGET (this->input_var_bt), en);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET (this->input_en), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET (this->input_var_bt), FALSE);
	}
}

static void
atp_editor_update_shortcut (ATPToolEditor* this)
{
	if (this->shortcut != NULL)
	{
		gtk_button_set_label (GTK_BUTTON (this->shortcut_bt), this->shortcut);
	}
	else
	{
		gtk_button_set_label (GTK_BUTTON (this->shortcut_bt), _("Disabled"));
	}
}

static void
atp_populate_tool_editor(ATPToolEditor* this)
{
	gint pos;
	const gchar* value;
	guint accel_key;
	GdkModifierType accel_mods;

	g_return_if_fail (this != NULL);

	/* Nothing to fill */
	if (this->tool == NULL) return;

	value = atp_user_tool_get_name (this->tool);
	if (value)
	{
		gtk_editable_insert_text(this->name_en, value
			  , strlen(value), &pos);
	}
	value = atp_user_tool_get_command (this->tool);
	if (value)
	{
		gtk_editable_insert_text(this->command_en, value
		  , strlen(value), &pos);
	}
	value = atp_user_tool_get_param (this->tool);
	if (value)
	{
		gtk_editable_insert_text(this->param_en, value
		  , strlen(value), &pos);
	}
	value = atp_user_tool_get_working_dir (this->tool);
	if (value)
	{
		gtk_editable_insert_text(this->dir_en, value
		  , strlen(value), &pos);
	}
	gtk_toggle_button_set_active (this->enabled_tb, atp_user_tool_get_flag (this->tool, ATP_TOOL_ENABLE));
	gtk_toggle_button_set_active (this->autosave_tb, atp_user_tool_get_flag (this->tool, ATP_TOOL_AUTOSAVE));
	gtk_toggle_button_set_active (this->terminal_tb, atp_user_tool_get_flag (this->tool, ATP_TOOL_TERMINAL));

	set_combo_box_value (this->output_com, atp_user_tool_get_output (this->tool));
	set_combo_box_value (this->error_com, atp_user_tool_get_error (this->tool));
	set_combo_box_value (this->input_com, atp_user_tool_get_input (this->tool));
	switch (atp_user_tool_get_input (this->tool))
	{
	case ATP_TIN_FILE:
	case ATP_TIN_STRING:
		value = atp_user_tool_get_input_string (this->tool);
		if (value)
		{
			gtk_editable_insert_text(this->input_en, value, strlen(value), &pos);
		}
		break;
	default:
		break;
	}
	atp_update_sensitivity (this);

	if (this->shortcut != NULL) g_free (this->shortcut);
	if (atp_user_tool_get_accelerator (this->tool, &accel_key, &accel_mods))
	{
		this->shortcut = gtk_accelerator_name (accel_key, accel_mods);
	}
	else
	{
		this->shortcut = NULL;
	}
	atp_editor_update_shortcut (this);

	gnome_icon_entry_set_filename (this->icon_en, atp_user_tool_get_icon (this->tool));
}

static void
on_editor_terminal_toggle (GtkToggleButton *tb, gpointer user_data)
{
	ATPToolEditor *this = (ATPToolEditor *)user_data;

	atp_update_sensitivity (this);
}

static void
on_editor_script_toggle (GtkToggleButton *tb, gpointer user_data)
{
	ATPToolEditor *this = (ATPToolEditor *)user_data;
	gchar* command;

	if (gtk_toggle_button_get_active(tb))
	{
		/* Get current command */
		command = gtk_editable_get_chars(this->command_en, 0, -1);

		if ((command == NULL) || (*command == '\0'))
		{
			gchar* name;
			gint pos;

			if (command) g_free (command);
			/* Generate a new script file name */
			command = gtk_editable_get_chars(this->name_en, 0, -1);
			if ((command == NULL) || (*command == '\0'))
			{
				command = g_strdup("script");
			}
			name = atp_remove_mnemonic (command);
			g_free (command);

			command = anjuta_util_get_user_data_file_path ("scripts/", name, NULL);
			g_free (name);

			/* Find a new file name */
			name = command;
			pos = 0;
			while (g_file_test (command, G_FILE_TEST_EXISTS))
			{
				if (command != name) g_free (command);
				command = g_strdup_printf("%s%d", name, pos); 
				pos++;	
			}
			if (command != name) g_free (name);

			/* Fill command line */
			gtk_editable_delete_text(this->command_en, 0, -1);
			gtk_editable_insert_text(this->command_en, command,
									 strlen(command), &pos);
		}
		if (command) g_free (command);
	}
}

static void
on_editor_input_changed (GtkComboBox *combo, gpointer user_data)
{
	ATPToolEditor *this = (ATPToolEditor *)user_data;

	atp_update_sensitivity (this);
}

static void
on_editor_response (GtkDialog *dialog, gint response, gpointer user_data)
{
	ATPToolEditor* this = (ATPToolEditor*)user_data;
	gchar* name;
	gchar* data;
	ATPInputType in_type;
	gchar* value;
	guint accel_key;
	GdkModifierType accel_mods;
	GtkAccelGroup* group;
	AnjutaUI* ui;

	if (response == GTK_RESPONSE_OK)
	{
		/* Check for all mandatory fields */
		name = gtk_editable_get_chars(this->name_en, 0, -1);
		if (!name || '\0' == name[0])
		{
			if (name) g_free (name);
			anjuta_util_dialog_error(GTK_WINDOW (this->dialog), _("You must provide a tool name!"));
			return;
		}
		data = gtk_editable_get_chars(this->command_en, 0, -1);
		if (!data || '\0' == data[0])
		{
			if (name) g_free (name);
			if (data) g_free (data);
			anjuta_util_dialog_error(GTK_WINDOW (this->dialog), _("You must provide a tool command!"));
			return;
		}

		if (!atp_user_tool_set_name (this->tool, name))
		{
			if (name) g_free (name);
			if (data) g_free (data);
			anjuta_util_dialog_error(GTK_WINDOW (this->dialog), _("A tool with the same name already exists!"));
			return;
		}
		g_free (name);
		
		if (this->shortcut == NULL)
		{
			accel_key = 0;
			accel_mods = 0;
		}
		else
		{
			gtk_accelerator_parse (this->shortcut, &accel_key, &accel_mods);
			ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(this->parent->plugin)->shell, NULL);
			group = anjuta_ui_get_accel_group(ui);
			if (gtk_accel_group_query (group, accel_key, accel_mods, NULL) != NULL)
			{
				if (!anjuta_util_dialog_boolean_question (GTK_WINDOW (this->dialog), _("The shortcut is already used by another component in Anjuta. Do you want to keep it anyway?")))
				{
					return;
				}
			}
		}

		/* Set new tool data */
		atp_user_tool_set_command (this->tool, data);
		g_free (data);

		data = gtk_editable_get_chars(this->param_en, 0, -1);
		atp_user_tool_set_param (this->tool, data);
		g_free (data);

		data = gtk_editable_get_chars(this->dir_en, 0, -1);
		atp_user_tool_set_working_dir (this->tool, data);
		g_free (data);

		atp_user_tool_set_flag (this->tool, ATP_TOOL_ENABLE | (gtk_toggle_button_get_active(this->enabled_tb) ? ATP_SET : ATP_CLEAR));

		atp_user_tool_set_flag (this->tool, ATP_TOOL_AUTOSAVE | (gtk_toggle_button_get_active(this->autosave_tb) ? ATP_SET : ATP_CLEAR));

		atp_user_tool_set_flag (this->tool, ATP_TOOL_TERMINAL | (gtk_toggle_button_get_active(this->terminal_tb) ? ATP_SET : ATP_CLEAR));


		atp_user_tool_set_output (this->tool, get_combo_box_value (this->output_com));
		atp_user_tool_set_error (this->tool, get_combo_box_value (this->error_com));
		in_type = get_combo_box_value (this->input_com);
		switch (in_type)
		{
		case ATP_TIN_FILE:
		case ATP_TIN_STRING:
			data = gtk_editable_get_chars(this->input_en, 0, -1);
			atp_user_tool_set_input (this->tool, in_type, data);
			g_free (data);
			break;
		default:
			atp_user_tool_set_input (this->tool, in_type, NULL);
			break;
		}

		atp_user_tool_set_accelerator (this->tool, accel_key, accel_mods);

		value = gnome_icon_entry_get_filename (this->icon_en);	
		atp_user_tool_set_icon (this->tool, value);
		g_free (value);	

		/* Open script in editor if requested */
		if (gtk_toggle_button_get_active (this->script_tb))
		{
			IAnjutaDocumentManager *docman;
			IAnjutaDocument *doc;
			GFile* file;

			/* Check that default script directory exist */
			data = anjuta_util_get_user_data_file_path ("scripts/", NULL);

			// TODO: Replace with g_mkdir_with_parents
			make_directory (data);
			g_free (data);

			data = gtk_editable_get_chars(this->command_en, 0, -1);

			if (!g_file_test (data, G_FILE_TEST_EXISTS))
			{
				FILE* sh;

				/* Create default script */
				sh = fopen (data, "wt");
				if (sh != NULL)
				{
					gint previous;

					fprintf(sh, "#!\n#\tScript template generated by Anjuta.\n#\tYou can pass argument using command line parameters\n#\n\n");
					fclose (sh);

					/* Make this file executable */
					previous = umask (0666);
					chmod (data, 0777 & ~previous);
					umask (previous);
				}
			}

			/* Load the script in an editor window */
			docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (this->parent->plugin)->shell, IAnjutaDocumentManager, NULL);
			if (docman == NULL)
			{	       
				anjuta_util_dialog_error(GTK_WINDOW (this->dialog), _("Unable to edit script"));
				return;
			}

			file = g_file_new_for_path (data);
			g_free (data);
			doc =
				ianjuta_document_manager_find_document_with_file 
							   (docman, file, NULL);
			if (doc == NULL)
			{
				IAnjutaFileLoader* loader;

				/* Not found, load file */
				loader = IANJUTA_FILE_LOADER (anjuta_shell_get_interface (ANJUTA_PLUGIN (this->parent->plugin)->shell, IAnjutaFileLoader, NULL));
				ianjuta_file_loader_load (loader, file, FALSE, NULL);
			}
			else
			{
				/* Set as current */
				ianjuta_document_manager_set_current_document (docman, doc, NULL);
			}
			g_object_unref (file);
		}
	}
	
	atp_tool_dialog_refresh (this->parent, atp_user_tool_get_name (this->tool));
	
	atp_tool_editor_free (this);
}

static void
on_editor_param_variable_show (GtkButton *button, gpointer user_data)
{
	ATPToolEditor* this = (ATPToolEditor*)user_data;

	atp_variable_dialog_show (&this->param_var, ATP_DEFAULT_VARIABLE);	
}

static void
on_editor_dir_variable_show (GtkButton *button, gpointer user_data)
{
	ATPToolEditor* this = (ATPToolEditor*)user_data;

	atp_variable_dialog_show (&this->dir_var, ATP_DIRECTORY_VARIABLE);
}

static void
on_editor_input_variable_show (GtkButton *button, gpointer user_data)
{
	ATPToolEditor* this = (ATPToolEditor*)user_data;

	switch (get_combo_box_value (this->input_com))
	{
	case ATP_TIN_FILE:
		atp_variable_dialog_show (&this->input_file_var, ATP_FILE_VARIABLE);
		break;
	case ATP_TIN_STRING:
		atp_variable_dialog_show (&this->input_string_var, ATP_DEFAULT_VARIABLE);
		break;
	}
}

static gboolean
on_editor_get_keys(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	ATPToolEditor *this = (ATPToolEditor*)user_data;
  	GdkDisplay *display;
  	guint accel_key = 0;
	GdkModifierType	accel_mods = 0;
  	GdkModifierType consumed_mods = 0;
	gboolean delete = FALSE;
	gboolean edited = FALSE;

  	switch (event->keyval)
    	{
	case GDK_Shift_L:
	case GDK_Shift_R:
	case GDK_Control_L:
	case GDK_Control_R:
	case GDK_Alt_L:
	case GDK_Alt_R:
		return TRUE;
	case GDK_Escape:
		break;
	case GDK_Delete:
	case GDK_KP_Delete:
	case GDK_BackSpace:
		delete = TRUE;
		break;
	default:
  		display = gtk_widget_get_display (widget);
		gdk_keymap_translate_keyboard_state (gdk_keymap_get_for_display (display),
                                       event->hardware_keycode,
                                       event->state,
                                       event->group,
                                       NULL, NULL, NULL, &consumed_mods);

		accel_key = gdk_keyval_to_lower (event->keyval);
		accel_mods = (event->state & gtk_accelerator_get_default_mod_mask () & ~consumed_mods);

		/* If lowercasing affects the keysym, then we need to include SHIFT
		* in the modifiers, We re-upper case when we match against the
		* keyval, but display and save in caseless form.
		*/
		if (accel_key != event->keyval)
			accel_mods |= GDK_SHIFT_MASK;

		edited = gtk_accelerator_valid (accel_key, accel_mods);
		break;
	}

	if (delete || edited)
	{
		/* Remove previous shortcut */
		if (this->shortcut != NULL) g_free (this->shortcut);

		/* Set new one */
		this->shortcut = delete ? NULL : gtk_accelerator_name (accel_key, accel_mods);
	}
	
	gtk_toggle_button_set_active (this->shortcut_bt, FALSE);

	return TRUE;
}

static void
on_editor_shortcut_toggle (GtkToggleButton *tb, gpointer user_data)
{
	ATPToolEditor *this = (ATPToolEditor *)user_data;

	if (gtk_toggle_button_get_active (tb))
	{
		gtk_grab_add(GTK_WIDGET(tb));

  		g_signal_connect (G_OBJECT (tb), "key_press_event", G_CALLBACK (on_editor_get_keys), this);
  		gtk_button_set_label (GTK_BUTTON (tb), _("New accelerator..."));
	}
	else
	{
		g_signal_handlers_disconnect_by_func (G_OBJECT (this->shortcut_bt), G_CALLBACK (on_editor_get_keys), this);
		gtk_grab_remove (GTK_WIDGET(this->shortcut_bt));

		atp_editor_update_shortcut (this);
	}
}

gboolean
atp_tool_editor_show (ATPToolEditor* this)
{
	GladeXML *xml;

	if (this->dialog != NULL)
	{
		/* dialog is already displayed */
		gtk_window_present (GTK_WINDOW (this->dialog));
		return TRUE;
	}

	if (NULL == (xml = glade_xml_new(GLADE_FILE, TOOL_EDITOR, NULL)))
	{
		anjuta_util_dialog_error (atp_tool_dialog_get_window (this->parent), _("Unable to build user interface for tool editor"));
		g_free(this);

		return FALSE;
	}
	this->dialog = glade_xml_get_widget(xml, TOOL_EDITOR);
	gtk_widget_show (this->dialog);
	gtk_window_set_transient_for (GTK_WINDOW (this->dialog), atp_plugin_get_app_window (this->parent->plugin));

	this->name_en = GTK_EDITABLE (glade_xml_get_widget (xml, TOOL_NAME));
	this->command_en = GTK_EDITABLE (glade_xml_get_widget (xml, TOOL_COMMAND));
	this->param_en = GTK_EDITABLE (glade_xml_get_widget (xml, TOOL_PARAM));
	atp_variable_dialog_set_entry (&this->param_var, this->param_en);
	this->dir_en = GTK_EDITABLE (glade_xml_get_widget (xml, TOOL_WORKING_DIR));
	atp_variable_dialog_set_entry (&this->dir_var, this->dir_en);
	this->enabled_tb = GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, TOOL_ENABLED));
	this->terminal_tb = GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, TOOL_TERMINAL));
	this->autosave_tb = GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, TOOL_AUTOSAVE));
	this->script_tb = GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, TOOL_SCRIPT));
	this->output_com = GTK_COMBO_BOX (glade_xml_get_widget(xml, TOOL_OUTPUT));
	this->error_com = GTK_COMBO_BOX (glade_xml_get_widget(xml, TOOL_ERROR));
	this->input_com = GTK_COMBO_BOX (glade_xml_get_widget(xml, TOOL_INPUT));
	this->input_en = GTK_EDITABLE (glade_xml_get_widget(xml, TOOL_INPUT_VALUE));
	this->input_var_bt = GTK_BUTTON (glade_xml_get_widget(xml, TOOL_INPUT_VARIABLE));
	this->shortcut_bt = GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, TOOL_SHORTCUT));
	atp_variable_dialog_set_entry (&this->input_file_var, this->input_en);
	atp_variable_dialog_set_entry (&this->input_string_var, this->input_en);
	this->icon_en = GNOME_ICON_ENTRY (glade_xml_get_widget(xml, TOOL_ICON));

	/* Add combox box value */
	set_combo_box_enum_model (this->error_com, atp_get_error_type_list());
	set_combo_box_enum_model (this->output_com, atp_get_output_type_list());
	set_combo_box_enum_model (this->input_com, atp_get_input_type_list());

	atp_clear_tool_editor (this);
	atp_populate_tool_editor (this);
	atp_update_sensitivity (this);

	/* Connect all signals */	
	glade_xml_signal_connect_data (xml, EDITOR_RESPONSE_SIGNAL, GTK_SIGNAL_FUNC (on_editor_response), this);
	glade_xml_signal_connect_data (xml, EDITOR_PARAM_VARIABLE_SIGNAL, GTK_SIGNAL_FUNC (on_editor_param_variable_show), this);
	glade_xml_signal_connect_data (xml, EDITOR_DIR_VARIABLE_SIGNAL, GTK_SIGNAL_FUNC (on_editor_dir_variable_show), this);
	glade_xml_signal_connect_data (xml, EDITOR_TOGGLE_SHORCUT_SIGNAL, GTK_SIGNAL_FUNC (on_editor_shortcut_toggle), this);
	glade_xml_signal_connect_data (xml, EDITOR_TOGGLE_TERMINAL_SIGNAL, GTK_SIGNAL_FUNC (on_editor_terminal_toggle), this);
	glade_xml_signal_connect_data (xml, EDITOR_TOGGLE_SCRIPT_SIGNAL, GTK_SIGNAL_FUNC (on_editor_script_toggle), this);
	glade_xml_signal_connect_data (xml, EDITOR_INPUT_VARIABLE_SIGNAL, GTK_SIGNAL_FUNC (on_editor_input_variable_show), this);
	glade_xml_signal_connect_data (xml, EDITOR_INPUT_CHANGED_SIGNAL, GTK_SIGNAL_FUNC (on_editor_input_changed), this);

	g_object_unref (xml);

	return TRUE;
}

ATPToolEditor* 
atp_tool_editor_new (ATPUserTool *tool, ATPToolEditorList *list, struct _ATPToolDialog *dialog)
{
	ATPToolEditor *this;

	/* Search a already existing tool editor with same name */
	for (this = list->first; this != NULL; this = this->next)
	{
		/* Name use the same string, so a string comparaison is not necessary */
		if (atp_user_tool_get_name (this->tool) == atp_user_tool_get_name (tool))
		{
			return this;
		}
	}

	/* Not found, create a new object */
	this = g_new0(ATPToolEditor, 1);
	this->parent = dialog;
	this->owner = list;
	this->tool = tool;
	atp_variable_dialog_construct (&this->param_var, this, ATP_VARIABLE_DEFAULT);
	atp_variable_dialog_construct (&this->dir_var, this, ATP_VARIABLE_REPLACE);
	atp_variable_dialog_construct (&this->input_file_var, this, ATP_VARIABLE_REPLACE);
	atp_variable_dialog_construct (&this->input_string_var, this, ATP_VARIABLE_REPLACE);

	/* Add it in the list */
	if (list != NULL)
	{
		this->next = list->first;
		list->first = this;
	}

	return this;	
}

gboolean
atp_tool_editor_free (ATPToolEditor *this)
{
	ATPToolEditor **prev;

	atp_variable_dialog_destroy (&this->input_string_var);
	atp_variable_dialog_destroy (&this->input_file_var);
	atp_variable_dialog_destroy (&this->dir_var);
	atp_variable_dialog_destroy (&this->param_var);

	if (this->shortcut != NULL) g_free (this->shortcut);

	if (this->owner == NULL)
	{
		/* tool editor is not in a list */
		gtk_widget_destroy (GTK_WIDGET (this->dialog));
		g_free (this);

		return TRUE;
	}

	/* Search tool editor in list */
	for (prev = &this->owner->first; *prev != NULL; prev = &((*prev)->next))
	{
		if (*prev == this)
		{
			/* remove tool editor from list */
			*prev = this->next;
			/* delete tool editor object */
			gtk_widget_destroy (GTK_WIDGET (this->dialog));
			g_free (this);

			return TRUE;
		}
	}

	/* tool editor not found in list */
	return FALSE;
}

/* Tool editor list
 * This list all the current active tool editors, it is mainly useful for
 * avoiding to have two dialogs editing the same tool. It could be possible
 * because the tool editing dialog is not modal.
 *---------------------------------------------------------------------------*/

ATPToolEditorList*
atp_tool_editor_list_construct (ATPToolEditorList* this)
{
	this->first = NULL;

	return this;
}

void
atp_tool_editor_list_destroy (ATPToolEditorList* this)
{
	ATPToolEditor *ted;

	for ( ; (ted = this->first) != NULL;)
	{
		atp_tool_editor_free (ted);
		
	}		
}

