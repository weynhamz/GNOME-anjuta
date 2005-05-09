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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Tool editor dialog
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "editor.h"

#include "dialog.h"
#include "tool.h"
#include "variable.h"

#include <libgnomeui/libgnomeui.h>

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
	#if 0
	GtkEditable *location_en;
	GtkToggleButton *detached_tb;
	GtkToggleButton *file_tb;
	GtkToggleButton *project_tb;
	GtkToggleButton *params_tb;
	GtkEditable *input_type_en;
	GtkCombo *input_type_com;
	GtkEditable *input_en;
	GtkEditable *shortcut_en;
	GtkEditable *icon_en;
	gboolean editing;
	#endif
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

#define TOOL_VARIABLE "variable_dialog"
#define VARIABLE_TREEVIEW "variable_treeview"
#define VARIABLE_RESPONSE_SIGNAL "on_variable_dialog_response"
#define VARIABLE_ACTIVATE_SIGNAL "on_variable_activate_row"

#if 0
#define TOOL_CLIST "tools.clist"
#define TOOL_HELP_CLIST "tools.help.clist"
#define TOOL_LOCATION "tool.location"
#define TOOL_ENABLED "tool.enabled"
#define TOOL_DETACHED "tool.detached"
#define TOOL_TERMINAL "tool.run_in_terminal"
#define TOOL_USER_PARAMS "tool.user_params"
#define TOOL_AUTOSAVE "tool.autosave"
#define TOOL_FILE_LEVEL "tool.file_level"
#define TOOL_PROJECT_LEVEL "tool.project_level"
#define TOOL_INPUT_TYPE "tool.input.type"
#define TOOL_INPUT_TYPE_COMBO "tool.input.type.combo"
#define TOOL_INPUT "tool.input"
#define TOOL_OUTPUT "tool.output"
#define TOOL_OUTPUT_COMBO "tool.output.combo"
#define TOOL_ERROR "tool.error"
#define TOOL_ERROR_COMBO "tool.error.combo"
#define TOOL_SHORTCUT "tool.shortcut"
#define TOOL_ICON "tool.icon"
#endif

/* String for enumerate value
 *---------------------------------------------------------------------------*/

#if 0
StringMap input_type_strings[] = {
  { AN_TINP_NONE, "None" }
, { AN_TINP_BUFFER, "Current buffer" }
, { AN_TINP_STRING, "String" }
, { -1, NULL }
};

StringMap output_strings[] = {
  { MESSAGE_BUILD, "Build Pane" }
, { MESSAGE_DEBUG, "Debug Pane" }
, { MESSAGE_FIND, "Find Pane" }
, { MESSAGE_CVS, "CVS Pane" }
, { MESSAGE_TERMINAL, "Terminal Pane" }
, { MESSAGE_STDOUT, "Stdout Pane" }
, { MESSAGE_STDERR, "Stderr Pane" }
, { AN_TBUF_NEW, "New Buffer" }
, { AN_TBUF_REPLACE, "Replace Buffer" }
, { AN_TBUF_INSERT, "Insert in Buffer" }
, { AN_TBUF_APPEND, "Append to Buffer" }
, { AN_TBUF_REPLACESEL, "Replace current selection" }
, { AN_TBUF_POPUP, "Show as a popup message" }
, { -1, NULL }
};
#endif

/* Add helper function
 *---------------------------------------------------------------------------*/

void
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

gint
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
		anjuta_util_dialog_error (NULL, _("Unable to build user interface for tool variable"));
		return FALSE;
	}
	this->dialog = GTK_DIALOG (glade_xml_get_widget(xml, TOOL_VARIABLE));
	gtk_widget_show (GTK_WIDGET (this->dialog));
	gtk_window_set_transient_for (GTK_WINDOW (this->dialog), GTK_WINDOW (this->editor->dialog));

	/* Create variable list */
	this->view = (GtkTreeView *) glade_xml_get_widget(xml, VARIABLE_TREEVIEW);
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
atp_clear_tool_editor(ATPToolEditor* ted)
{
	g_return_if_fail (ted != NULL);

	gtk_editable_delete_text(ted->name_en, 0, -1);
	gtk_editable_delete_text(ted->command_en, 0, -1);
	gtk_editable_delete_text(ted->param_en, 0, -1);
	gtk_editable_delete_text(ted->dir_en, 0, -1);

	#if 0
	gtk_editable_delete_text(ted->location_en, 0, -1);
	gtk_toggle_button_set_active(ted->enabled_tb, TRUE);
	gtk_toggle_button_set_active(ted->detached_tb, FALSE);
	gtk_toggle_button_set_active(ted->terminal_tb, FALSE);
	gtk_toggle_button_set_active(ted->params_tb, FALSE);
	gtk_toggle_button_set_active(ted->file_tb, FALSE);
	gtk_toggle_button_set_active(ted->project_tb, FALSE);
	gtk_editable_delete_text(ted->input_type_en, 0, -1);
	gtk_editable_delete_text(ted->input_en, 0, -1);
	gtk_editable_delete_text(ted->output_en, 0, -1);
	gtk_editable_delete_text(ted->error_en, 0, -1);
	gtk_editable_delete_text(ted->shortcut_en, 0, -1);
	gtk_editable_delete_text(ted->icon_en, 0, -1);
	#endif
}

static void
atp_update_sensitivity(ATPToolEditor *ted)
{
	gboolean en;

	/* Deactivate output and input setting if a terminal is used */
	en = gtk_toggle_button_get_active (ted->terminal_tb);
	gtk_widget_set_sensitive((GtkWidget *) ted->output_com, !en);
	gtk_widget_set_sensitive((GtkWidget *) ted->error_com, !en);
	gtk_widget_set_sensitive((GtkWidget *) ted->input_com, !en);

	/* input value is available for a few input type only */
	if (!en)
	{
		switch (get_combo_box_value (ted->input_com))
		{
		case ATP_TIN_FILE:
		case ATP_TIN_STRING:
			en = TRUE;
			break;
		default:
			en = FALSE;
			break;
		}
		gtk_widget_set_sensitive((GtkWidget *) ted->input_en, en);
		gtk_widget_set_sensitive((GtkWidget *) ted->input_var_bt, en);
	}
	else
	{
		gtk_widget_set_sensitive((GtkWidget *) ted->input_en, FALSE);
		gtk_widget_set_sensitive((GtkWidget *) ted->input_var_bt, FALSE);
	}
}

static
atp_editor_update_shortcut (ATPToolEditor* ted)
{
	if (ted->shortcut != NULL)
	{
		gtk_button_set_label (GTK_BUTTON (ted->shortcut_bt), ted->shortcut);
	}
	else
	{
		gtk_button_set_label (GTK_BUTTON (ted->shortcut_bt), _("Disabled"));
	}
}

static void
atp_populate_tool_editor(ATPToolEditor* ted)
{
	int pos;
	const gchar* value;
	guint accel_key;
	GdkModifierType accel_mods;

	g_return_if_fail (ted != NULL);

	/* Nothing to fill */
	if (ted->tool == NULL) return;

	value = atp_user_tool_get_name (ted->tool);
	if (value)
	{
		gtk_editable_insert_text(ted->name_en, value
			  , strlen(value), &pos);
	}
	value = atp_user_tool_get_command (ted->tool);
	if (value)
	{
		gtk_editable_insert_text(ted->command_en, value
		  , strlen(value), &pos);
	}
	value = atp_user_tool_get_param (ted->tool);
	if (value)
	{
		gtk_editable_insert_text(ted->param_en, value
		  , strlen(value), &pos);
	}
	value = atp_user_tool_get_working_dir (ted->tool);
	if (value)
	{
		gtk_editable_insert_text(ted->dir_en, value
		  , strlen(value), &pos);
	}
	gtk_toggle_button_set_active (ted->enabled_tb, atp_user_tool_get_flag (ted->tool, ATP_TOOL_ENABLE));
	gtk_toggle_button_set_active (ted->autosave_tb, atp_user_tool_get_flag (ted->tool, ATP_TOOL_AUTOSAVE));
	gtk_toggle_button_set_active (ted->terminal_tb, atp_user_tool_get_flag (ted->tool, ATP_TOOL_TERMINAL));

	set_combo_box_value (ted->output_com, atp_user_tool_get_output (ted->tool));
	set_combo_box_value (ted->error_com, atp_user_tool_get_error (ted->tool));
	set_combo_box_value (ted->input_com, atp_user_tool_get_input (ted->tool));
	switch (atp_user_tool_get_input (ted->tool))
	{
	case ATP_TIN_FILE:
	case ATP_TIN_STRING:
		value = atp_user_tool_get_input_string (ted->tool);
		if (value)
		{
			gtk_editable_insert_text(ted->input_en, value, strlen(value), &pos);
		}
		break;
	}
	atp_update_sensitivity (ted);

	if (ted->shortcut != NULL) g_free (ted->shortcut);
	if (atp_user_tool_get_accelerator (ted->tool, &accel_key, &accel_mods))
	{
		ted->shortcut = gtk_accelerator_name (accel_key, accel_mods);
	}
	else
	{
		ted->shortcut = NULL;
	}
	atp_editor_update_shortcut (ted);

	gnome_icon_entry_set_filename (ted->icon_en, atp_user_tool_get_icon (ted->tool));
	
	#if 0
	if (ted->tool->location)
	{
		gtk_editable_insert_text(ted->location_en, ted->tool->location
		  , strlen(ted->tool->location), &pos);
	}
	gtk_toggle_button_set_active(ted->detached_tb, ted->tool->detached);
	gtk_toggle_button_set_active(ted->params_tb, ted->tool->user_params);
	gtk_toggle_button_set_active(ted->file_tb, ted->tool->file_level);
	gtk_toggle_button_set_active(ted->project_tb, ted->tool->project_level);
	if (ted->tool->input_type)
	{
		const char *s = string_from_type(input_type_strings
		  , ted->tool->input_type);
		if (s)
			gtk_editable_insert_text(ted->input_type_en, s, strlen(s), &pos);
	}
	if (ted->tool->input)
	{
		gtk_editable_insert_text(ted->input_en, ted->tool->input
		  , strlen(ted->tool->input), &pos);
	}
	if (ted->tool->output >= 0)
	{
		const char *s = string_from_type(output_strings
		  , ted->tool->output);
		if (s)
			gtk_editable_insert_text(ted->output_en, s, strlen(s), &pos);
	}
	if (ted->tool->error >= 0)
	{
		const char *s = string_from_type(output_strings
		  , ted->tool->error);
		if (s)
			gtk_editable_insert_text(ted->error_en, s, strlen(s), &pos);
	}
	if (ted->tool->shortcut)
	{
		gtk_editable_insert_text(ted->shortcut_en, ted->tool->shortcut
		  , strlen(ted->tool->shortcut), &pos);
	}
	if (ted->tool->icon)
	{
		gtk_editable_insert_text(ted->icon_en, ted->tool->icon
		  , strlen(ted->tool->icon), &pos);
	}
	#endif
}



#if 0


static void set_tool_editor_widget_sensitivity(gboolean s1, gboolean s2)
{
	gtk_widget_set_sensitive((GtkWidget *) ted->input_type_en, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->input_type_com, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->input_en, !s1 && s2);
	gtk_widget_set_sensitive((GtkWidget *) ted->output_en, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->output_com, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->error_en, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->error_com, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->terminal_tb, s1);	
}

/* Callbacks for the tool editor GUI */

void
on_user_tool_edit_detached_toggled(GtkToggleButton *tb, gpointer user_data)
{
	int input_type = AN_TINP_NONE;
	char *s = gtk_editable_get_chars(ted->input_type_en, 0, -1);
	gboolean state = gtk_toggle_button_get_active(tb);
	/* For a tool in detached mode, input, output and error cannot be
	** redirected. Also, unless the input type is AN_TINP_STRING,
	** input string should be disabled.	*/
	if (s)
	{
		input_type = type_from_string(input_type_strings, s);
		g_free(s);
	}
	set_tool_editor_widget_sensitivity(state, (AN_TINP_STRING == input_type));
}

void
on_user_tool_edit_input_type_changed(GtkEditable *editable, gboolean user_data)
{
	int input_type = AN_TINP_NONE;
	char *s = gtk_editable_get_chars(editable, 0, -1);
	if (s)
	{
		input_type = type_from_string(input_type_strings, s);
		g_free(s);
	}
	gtk_widget_set_sensitive((GtkWidget *) ted->input_en
	  , (AN_TINP_STRING == input_type));
}

void
on_user_tool_edit_response (GtkDialog *dialog, gint response,
							gpointer data)
{
	GtkTreeModel *model;
	AnUserTool *tool, *tool1 = NULL;
	GSList *tmp;

	switch (response)
	{
	case GTK_RESPONSE_HELP:
		anjuta_tools_show_variables ();
		return;
	
	case GTK_RESPONSE_OK:
		tool = an_user_tool_from_gui();
		
		/* Check for all mandatory fields */
		if (!tool->name || '\0' == tool->name[0])
		{
			anjuta_error(_("You must provide a tool name!"));
			an_user_tool_free(tool, FALSE);
			return;
		}
		if (!tool->command || '\0' == tool->command[0])
		{
			anjuta_error(_("You must provide a tool command!"));
			an_user_tool_free(tool, FALSE);
			return;
		}
		if (NULL == tool_hash)
			tool_hash = g_hash_table_new(g_str_hash, g_str_equal);
		else
		{
			tool1 = g_hash_table_lookup(tool_hash, tool->name);
			if (tool1)
			{
				/* A tool with the same name exists */
				if (!ted->editing || tool1 != ted->tool)
				{
					anjuta_error_parented(ted->dialog,
						_("A tool with the name '%s' already exists!"),
					  tool->name);
					return;
				}
			}
		}
		if (ted->editing)
			an_user_tool_free(ted->tool, TRUE);
		g_hash_table_insert(tool_hash, tool->name, tool);
		tool_list = g_slist_append(tool_list, tool);
		ted->tool = NULL;
		
		if (tl->dialog)
		{
			gtk_dialog_set_response_sensitive((GtkDialog *) tl->dialog,
											  GTK_RESPONSE_OK, FALSE);
			gtk_dialog_set_response_sensitive((GtkDialog *) tl->dialog,
											  GTK_RESPONSE_NO, FALSE);
			model = gtk_tree_view_get_model (GTK_TREE_VIEW(tl->clist));
			gtk_list_store_clear (GTK_LIST_STORE (model));
			for (tmp = tool_list; tmp; tmp = g_slist_next(tmp))
			{
				GtkTreeIter iter;
				gtk_list_store_append (GTK_LIST_STORE (model), &iter);
				gtk_list_store_set (GTK_LIST_STORE (model), &iter,
									AN_TOOLS_ENABLED_COLUMN,
									((AnUserTool *) tmp->data)->enabled,
									AN_TOOLS_NAME_COLUMN,
									((AnUserTool *) tmp->data)->name,
									AN_TOOLS_DATA_COLUMN, tmp->data,
									-1);
			}
		}
	}
	gtk_widget_destroy (ted->dialog);
	g_free (ted);
	ted = NULL;
}

static gboolean
on_user_tool_edit_delete_event (GtkWindow *window, GdkEvent* event,
								gpointer user_data)
{
	g_message ("Tools editor: Destroy event called");
	g_free (ted);
	ted = NULL;
	return FALSE;
}

/* List of variables understood by the tools editor and their values.
** It is grossly incomplete and I need help here ! - Biswa
*/
static struct
{
	char *name;
	char *value;
} variable_list[] = {
  {CURRENT_FULL_FILENAME_WITH_EXT, "Current full filename with extension" }
, {CURRENT_FULL_FILENAME, "Current full filename without extension" }
, {CURRENT_FILENAME_WITH_EXT, "Current filename with extension" }
, {CURRENT_FILENAME, "Current filename without extension" }
, {CURRENT_FILE_DIRECTORY, "Directory of the current file" }
, {CURRENT_FILE_EXTENSION, "Extension of the current file" }
, {"current.file.selection", "Selected/focussed word in the current buffer"}
, {"top.proj.dir", "Top project directory" }
, {"project.name", "Name of the project" }
, {"project.type", "Type of project, e.g. GNOME"}
, {"project.target.type", "Type of target of the project"}
, {"project.version", "Project version"}
, {"project.author", "Author of the project"}
, {"project.source.target", "Main target executable/library for the project"}
, {"project.source.paths", "Directories where project sources are present"}
, {"project.has.gettext", "Whether gettext (INTL) support is enabled for the project"}
, {"project.programming.language", "Programming languages used in the project"}
, {"project.excluded.modules", "Modules (directories) excluded from the project"}
, {"project.config.extra.modules.before", "Modules to configure before source module"}
, {"project.config.extra.modules.after", "Modules to configure after source module"}
, {"project.menu.entry", "Project menu entry (i.e. project name)"}
, {"project.menu.group", "Project menu group (e.g. Applications)"}
, {"project.menu.comment", "Desktop entry descriptive comment"}
, {"project.menu.icon", "Icon file for the project"}
, {"project.menu.need.terminal", "Whether the program should run in a terminal"}
, {"project.configure.options", "Options to pass to project configure script"}
, {"anjuta.program.arguments", "Arguments anjuta passes while executing the program"}
, {"module.*.name", "Name of project module ('*' can be include, source, pixmap, data, help, doc and po"}
, {"module.*.files", "Files under the given project module (source, include, etc.)"}
, {"compiler.options.supports", "Compiler support options (needs better explaination"}
, {"compiler.options.include.paths", "Include paths to pass to the compiler"}
, {"compiler.options.library.paths", "Library paths to pass to the linker"}
, {"compiler.options.libraries", "Libraries to link against"}
, {"compiler.options.defines", "Defines to pass to the compiler"}
, {"compiler.options.other.c.flags", "Other options to pass to the compiler"}
, {"compiler.options.other.l.flags", "Other library flags to pass to the linker"}
, {"compiler.options.opther.l.libs", "Other libraries to link with the application"}
, {"anjuta.last.open.project", "Last open project"}
, {"Generic user preferences", ""}
, {PROJECTS_DIRECTORY, "Directory where projects are created by default"}
, {TARBALLS_DIRECTORY, "Default tarballs creation directory"}
, {RPMS_DIRECTORY, "RPMs directory"}
, {SRPMS_DIRECTORY, "SRPMs directory"}
, {MAXIMUM_RECENT_PROJECTS, "Maximum recent projects to show" }
, {MAXIMUM_RECENT_FILES, "Maximum recent files to show" }
, {MAXIMUM_COMBO_HISTORY, "Maximum combo history size" }
, {DIALOG_ON_BUILD_COMPLETE, "Whether to show dialog on completion of build" }
, {BEEP_ON_BUILD_COMPLETE, "Whether to beep on completion of build" }
, {RELOAD_LAST_PROJECT, "If the last project is to be reloaded at startup" }
, {BUILD_OPTION_KEEP_GOING, "Whether to continue building on errors" }
, {BUILD_OPTION_DEBUG, "Enable debugging of build" }
, {BUILD_OPTION_SILENT, "Build silently" }
, {BUILD_OPTION_WARN_UNDEF, "Warn on undefined variables during build" }
, {BUILD_OPTION_JOBS, "Maximum number of build jobs" }
, {BUILD_OPTION_AUTOSAVE, "Autosave before build"}
, {DISABLE_SYNTAX_HILIGHTING, "Disable syntax highlighting for source files"}
, {SAVE_AUTOMATIC, "Save automatically on a periodic basis"}
, {INDENT_AUTOMATIC, "Auto-indent"}
, {USE_TABS, "Use tabs for indentation"}
, {BRACES_CHECK, "Check brace matching"}
, {"braces.sloppy", "Sloppy braces"}
, {DOS_EOL_CHECK, "Use DOS style end-of-line characters"}
, {WRAP_BOOKMARKS, "Wrap around bookmarks Previous/Next"}
, {TAB_SIZE, "Tab size"}
, {INDENT_SIZE, "Indentation size"}
, {INDENT_OPENING, "Whether to indent opening brace"}
, {INDENT_CLOSING, "Whether to indent closing brace"}
, {AUTOSAVE_TIMER, "Time in minutes after which autosave occurs"}
, {SAVE_SESSION_TIMER, "use save session timer"}
, {AUTOFORMAT_DISABLE, "Disable autoformatting"}
, {AUTOFORMAT_CUSTOM_STYLE, "Custom autoformat style (indent parameters)"}
, {AUTOFORMAT_STYLE, "Predefined autoformat style"}
, {EDITOR_TABS_POS, "Editor tab position"}
, {EDITOR_TABS_HIDE, "Whether to hide editor tabs"}
, {EDITOR_TABS_ORDERING, "Whether to order editor tabs by file name"}
, {EDITOR_TABS_RECENT_FIRST, "Bring recent tabs first on paeg switch"}
, {STRIP_TRAILING_SPACES, "Whether to strip trailing spaces"}
, {"edge.columns", "Maximum length of line suggested by the editor"}
, {"edge.mode", "How the editor marks lines exceeding recommended length"}
, {"edge.color", "Color of edge line"}
, {"margin.linenumber.visible", "Whether linenumber margin is visible"}
, {"margin.marker.visible", "Whether marker margin is visible"}
, {"margin.fold.visible", "Whether the fold margin is visible"}
, {"margin.linenumber.width", "Width of the linenumber margin"}
, {"margin.marker.width", "Width of the marker margin"}
, {"margin.fold.width", "Width of the fold margin"}
, {"buffered.draw", "Use double-buffering for editor draw"}
, {"default.file.ext", "Default extension for new files"}
, {"vc.home.key", "Should the editor use the VC_HOME key"}
, {"highlight.indentation.guides", "Highlight indentation guides"}
, {"dwell.period", "Dwell period (used for showing editor tips"}
, {"fold", "Enable folding"}
, {"fold.flags", "Fold flags"}
, {"fold.compact", "Compact folding"}
, {"fold.use.plus", "User +/- signs for folding"}
, {"fold.comment", "Fold comments"}
, {"fold.comment.python", "Fold python comments"}
, {"fold.quotes.python", "Fold python quotes"}
, {"fold.html", "Fold HTML"}
, {"fold.symbols", "Fold symbols"}
, {"styling.within.preporcessor", "Enable styling within preprocessor"}
, {"xml.auto.close.tags", "Automatically close XML tags"}
, {"asp.default.language", "Default language for ASP pages"}
, {"calltip.*.ignorecase", "Whether calltip should be case-insensitive"}
, {"autocomplete.*.ignorecase", "Whether autocomplete should be case-insensitive"}
, {"autocomplete.choose.single", "Whether single autocomplete option is chosen automatically"}
, {"autocompleteword.automatic", "Try to autocomplete after these many characters"}
, {FOLD_ON_OPEN, "Whether to close folds on opening a new file"}
, {CARET_FORE_COLOR, "Caret foreground color"}
, {CALLTIP_BACK_COLOR, "Background color of calltip"}
, {SELECTION_FORE_COLOR, "Foreground color of selection"}
, {SELECTION_BACK_COLOR, "Background color of selection"}
, {TEXT_ZOOM_FACTOR, "Current text zoom factor"}
, {TRUNCAT_MESSAGES, "Whether to truncate long messages"}
, {TRUNCAT_MESG_FIRST, "Number of characters to truncate after"}
, {TRUNCAT_MESG_LAST, "No of trailing characters to show"}
, {MESSAGES_TAG_POS, "Position of messages window tabs"}
, {MESSAGES_COLOR_ERROR, "Color of error message lines"}
, {MESSAGES_COLOR_WARNING, "Color of warning message lines"}
, {MESSAGES_COLOR_MESSAGES1, "Color of program messages"}
, {MESSAGES_COLOR_MESSAGES2, "Color of other messages"}
, {MESSAGES_INDICATORS_AUTOMATIC, "Enable automatic message indicators"}
, {"indicator.0.style", "Style for general indicators"}
, {"indicator.0.color", "Color for general indicators"}
, {"indicator.1.style", "Style for warning indicators"}
, {"indicator.1.color", "Color for warning indicators"}
, {"indicator.2.style", "Style for error indicators"}
, {"indicator.2.color", "Color for error indicators"}
, {"blank.margin.left", "Width of left editor margin"}
, {"blank.margin.right", "Width of right editor margin"}
, {"horizontal.scrollbar", "Enable horizontal scrollbar"}
, {"chars.alpha", "Alphabetic characters"}
, {"chars.numeric", "Numeric characters"}
, {"chars.accented", "Accented characters"}
, {"source.files", "These extensions will be treated as source files"}
, {"view.eol", "Should the EOL character be visible"}
, {"view.whitespace", "Should the whitespace be visible"}
, {"view.indentation.whitespace", "Should indentation whitespace be visible"}
, {"view.indentation.guides", "Should indentation guides be visible"}
, {"view.line.wrap", "Should line wrap be on by default"}
, {"use.monospaced", "Use monospaced fonts by default"}
, {"statusbar.visible", "Should the statusbar be visible"}
, {"main.toolbar.visible", "Should the main toolbar be visible"}
, {"browser.toolbar.visible", "Should the browser toolbar be visible"}
, {"extended.toolbar.visible", "Should the extended toolbar be visible"}
, {"format.toolbar.visible", "Should the format toolbar be visible"}
, {"debug.toolbar.visible", "Should the debug toolbar be visible"}
, {"font.base", "Default base font"}
, {"font.monospace", "Default monospace font"}
, {"font.big", "Default big font"}
, {"font.medium", "Default medium font"}
, {"font.small", "Default small font"}
, {"font.comment", "Default comment font"}
, {"style.default.whitespace", "Default style for whitespaces"}
, {"style.default.comment", "Default style for comments"}
, {"style.default.number", "Default style for number"}
, {"style.default.keyword", "Default style for keywords"}
, {"style.default.syskeyword", "Default style for system keywords"}
, {"style.default.localkeyword", "Default style for local keywords"}
, {"style.default.doublequote", "Default style for doubel quoted strings"}
, {"style.default.singlequote", "Default style for single quote strings"}
, {"style.default.preprocessor", "Default style for preprocessor"}
, {"style.default.operator", "Default style for operators"}
, {"style.default.unclosedstring", "Default style for unclosed strings"}
, {"style.default.identifier", "Default style for identifiers"}
, {"style.default.definition", "Default style for definition name"}
, {"style.default.function", "Default style for function name"}
, {"messages.is.docked", "Should be messages window be docked"}
, {"project.is.docked", "Should be project window be docked"}
, {AUTOMATIC_TAGS_UPDATE, "Update tag image automatically"}
, {BUILD_SYMBOL_BROWSER, "Build symbol browsxer automatically"}
, {BUILD_FILE_BROWSER, "Build file browser automatically"}
, {SHOW_TOOLTIPS, "Show tooltips"}
, {PRINT_PAPER_SIZE, "Paper size"}
, {PRINT_HEADER, "Print header"}
, {PRINT_WRAP, "Enable wrapping while printing"}
, {PRINT_LINENUM_COUNT, "Print line number after these many lines"}
, {PRINT_LANDSCAPE, "Use landscape printing mode"}
, {PRINT_MARGIN_LEFT, "Left margin for printing"}
, {PRINT_MARGIN_RIGHT, "Right margin for printing"}
, {PRINT_MARGIN_TOP, "Top margin for printing"}
, {PRINT_MARGIN_BOTTOM, "Bottom margin for printing"}
, {PRINT_COLOR, "Enable color printing"}
, {USE_COMPONENTS, "Use components"}
, {IDENT_NAME, "User name"}
, {IDENT_EMAIL, "User e-mail address"}
, {"anjuta.home.directory", "Home directory for Anjuta"}
, {"anjuta.data.directory", "Data directory for anjuta"}
, {"anjuta.pixmap.directory", "Pixmap directory for anjuta"}
, {"anjuta.version", "Anjuta version"}
, {"make", "Command for executing make"}
, {"anjuta.make.options", "Options to pass to the make command"}
, {"command.build.module", "Command to build a module"}
, {"command.build.project", "Command to build the full project"}
, {"command.build.tarball", "Command to build the project tarball"}
, {"command.build.install", "Command to install a project"}
, {"command.build.clean", "Command to clean up temporary project files"}
, {"command.build.clean.all", "Command to clean all generated files"}
, {"command.execute.project", "Command to run the project executable"}
, {"command.terminal", "Command to execute a program in a terminal"}
, {"anjuta.terminal", "Terminal program to be used (deprecated)"}
, {"anjuta.compiler.flags", "Default compiler flags"}
, {"anjuta.linker.flags", "Default linler flags"}
, {NULL, NULL}
};

enum {
	AN_TOOLS_HELP_VAR_COLUMN,
	AN_TOOLS_HELP_MEAN_COLUMN,
	AN_TOOLS_HELP_VALUE_COLUMN,
	N_AN_TOOLS_HELP_COLUMNS,
};

void anjuta_tools_show_variables()
{
	int len;
	int i = 0;
	char *s[4];
	GladeXML* xml;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *dialog;
	GtkWidget *clist;

	if (NULL == (xml = glade_xml_new(GLADE_FILE_ANJUTA, TOOL_HELP, NULL)))
	{
		anjuta_error(_("Unable to build user interface for tool help"));
		return;
	}
	dialog = glade_xml_get_widget(xml, TOOL_HELP);
	gtk_widget_show (dialog);
	gtk_window_set_transient_for (GTK_WINDOW(dialog)
	  , GTK_WINDOW(app));
	
	clist = (GtkWidget *) glade_xml_get_widget(xml, TOOL_HELP_CLIST);
	model = GTK_TREE_MODEL(gtk_list_store_new (N_AN_TOOLS_HELP_COLUMNS,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING));
	gtk_tree_view_set_model (GTK_TREE_VIEW (clist), model);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Variable"), renderer,
													   "text",
													   AN_TOOLS_HELP_VAR_COLUMN,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(clist), column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Meaning"), renderer,
													   "text",
													   AN_TOOLS_HELP_MEAN_COLUMN,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(clist), column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Value"), renderer,
													   "text",
													   AN_TOOLS_HELP_VALUE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(clist), column);
	g_object_unref (model);
	g_object_unref (xml);
	
	gtk_list_store_clear (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(clist))));
	if (app->current_text_editor)
	{
		/* Update file level properties */
		gchar *word;
		anjuta_set_file_properties(app->current_text_editor->full_filename);
		word = text_editor_get_current_word(app->current_text_editor);
		anjuta_preferences_set (ANJUTA_PREFERENCES (app->preferences),
								"current.file.selection", word?word:"");
		if (word)
			g_free(word);
	}
	s[3] = "";
	s[1] = "";
	while (NULL != variable_list[i].name)
	{
		GtkTreeModel *model;
		GtkTreeIter iter;
		
		model = gtk_tree_view_get_model (GTK_TREE_VIEW(clist));
		s[0] = variable_list[i].name;
		s[1] = variable_list[i].value;
		s[2] = prop_get(app->project_dbase->props, s[0]);
		if (s[2])
		{
			len = strlen(s[2]);
			if (len > 20)
				s[20] = '\0';
		}
		else
			s[2] = g_strdup("Undefined");
		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				AN_TOOLS_HELP_VAR_COLUMN, s[0],
				AN_TOOLS_HELP_MEAN_COLUMN, s[1],
				AN_TOOLS_HELP_VALUE_COLUMN, s[2],
				-1);
		g_free(s[2]);
		++ i;
	}
	return;
}
#endif

#if 0
static
atp_tool_editor_fill_from_gui(ATPToolEditor *this)
{
	this->
	char *s;
	AnUserTool *tool = g_new0(AnUserTool, 1);
	tool->name = gtk_editable_get_chars(ted->name_en, 0, -1);
	tool->location = gtk_editable_get_chars(ted->location_en, 0, -1);
	tool->command = gtk_editable_get_chars(ted->command_en, 0, -1);
	tool->working_dir = gtk_editable_get_chars(ted->dir_en, 0, -1);
	tool->enabled = gtk_toggle_button_get_active(ted->enabled_tb);
	tool->detached = gtk_toggle_button_get_active(ted->detached_tb);
	tool->run_in_terminal = gtk_toggle_button_get_active(ted->terminal_tb);
	tool->user_params = gtk_toggle_button_get_active(ted->params_tb);
	tool->autosave = gtk_toggle_button_get_active(ted->autosave_tb);
	tool->file_level = gtk_toggle_button_get_active(ted->file_tb);
	tool->project_level = gtk_toggle_button_get_active(ted->project_tb);
	s = gtk_editable_get_chars(ted->input_type_en, 0, -1);
	tool->input_type = type_from_string(input_type_strings, s);
	g_free(s);
	tool->input = gtk_editable_get_chars(ted->input_en, 0, -1);
	s = gtk_editable_get_chars(ted->output_en, 0, -1);
	tool->output = type_from_string(output_strings, s);
	g_free(s);
	s = gtk_editable_get_chars(ted->error_en, 0, -1);
	tool->error = type_from_string(output_strings, s);
	g_free(s);
	tool->shortcut = gtk_editable_get_chars(ted->shortcut_en, 0, -1);
	tool->icon = gtk_editable_get_chars(ted->icon_en, 0, -1);
	return tool;
}
#endif

static void
on_editor_terminal_toggle (GtkToggleButton *tb, gpointer user_data)
{
	ATPToolEditor *ted = (ATPToolEditor *)user_data;

	atp_update_sensitivity (ted);
}

static void
on_editor_input_changed (GtkComboBox *combo, gpointer user_data)
{
	ATPToolEditor *ted = (ATPToolEditor *)user_data;

	atp_update_sensitivity (ted);
}

static void
on_editor_response (GtkDialog *dialog, gint response, gpointer user_data)
{
	ATPToolEditor* ted = (ATPToolEditor*)user_data;
	const gchar* name;
	const gchar* data;
	ATPInputType in_type;
	gchar* value;
	guint accel_key;
	GdkModifierType accel_mods;
	GtkAccelGroup* group;
	AnjutaUI* ui;

	if (response == GTK_RESPONSE_OK)
	{
		/* Check for all mandatory fields */
		name = gtk_editable_get_chars(ted->name_en, 0, -1);
		if (!name || '\0' == name[0])
		{
			anjuta_util_dialog_error(NULL, _("You must provide a tool name!"));
			return;
		}
		data = gtk_editable_get_chars(ted->command_en, 0, -1);
		if (!data || '\0' == data[0])
		{
			anjuta_util_dialog_error(NULL, _("You must provide a tool command!"));
			return;
		}

		if (!atp_user_tool_set_name (ted->tool, name))
		{
			anjuta_util_dialog_error(NULL, _("A tool with the same name already exists!"));
			return;
		}

		
		if (ted->shortcut == NULL)
		{
			accel_key = 0;
			accel_mods = 0;
		}
		else
		{
			gtk_accelerator_parse (ted->shortcut, &accel_key, &accel_mods);
			ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(ted->parent->plugin)->shell, NULL);
			group = anjuta_ui_get_accel_group(ui);
			if (gtk_accel_group_query (group, accel_key, accel_mods, NULL) != NULL)
			{
				if (!anjuta_util_dialog_boolean_question (NULL, _("The shortcut is already used by another component in Anjuta. Do you want to keep it anyway ?")))
				{
					return;
				}
			}
		}

		/* Set new tool data */
		atp_user_tool_set_command (ted->tool, data);

		data = gtk_editable_get_chars(ted->param_en, 0, -1);
		atp_user_tool_set_param (ted->tool, data);

		data = gtk_editable_get_chars(ted->dir_en, 0, -1);
		atp_user_tool_set_working_dir (ted->tool, data);

		atp_user_tool_set_flag (ted->tool, ATP_TOOL_ENABLE | (gtk_toggle_button_get_active(ted->enabled_tb) ? ATP_SET : ATP_CLEAR));

		atp_user_tool_set_flag (ted->tool, ATP_TOOL_AUTOSAVE | (gtk_toggle_button_get_active(ted->autosave_tb) ? ATP_SET : ATP_CLEAR));

		atp_user_tool_set_flag (ted->tool, ATP_TOOL_TERMINAL | (gtk_toggle_button_get_active(ted->terminal_tb) ? ATP_SET : ATP_CLEAR));

		atp_user_tool_set_output (ted->tool, get_combo_box_value (ted->output_com));
		atp_user_tool_set_error (ted->tool, get_combo_box_value (ted->error_com));
		in_type = get_combo_box_value (ted->input_com);
		switch (in_type)
		{
		case ATP_TIN_FILE:
		case ATP_TIN_STRING:
			data = gtk_editable_get_chars(ted->input_en, 0, -1);
			atp_user_tool_set_input (ted->tool, in_type, data);
			break;
		default:
			atp_user_tool_set_input (ted->tool, in_type, NULL);
			break;
		}

		atp_user_tool_set_accelerator (ted->tool, accel_key, accel_mods);

		value = gnome_icon_entry_get_filename (ted->icon_en);	
		atp_user_tool_set_icon (ted->tool, value);
		g_free (value);	

		atp_tool_dialog_refresh (ted->parent, name);
	}

	atp_tool_editor_free (ted);
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
	ATPToolEditor *ted = (ATPToolEditor*)user_data;
  	GdkDisplay *display;
  	guint accel_key;
	GdkModifierType	accel_mods;
  	GdkModifierType consumed_mods;
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
		if (ted->shortcut != NULL) g_free (ted->shortcut);

		/* Set new one */
		ted->shortcut = delete ? NULL : gtk_accelerator_name (accel_key, accel_mods);
	}
	
	gtk_toggle_button_set_active (ted->shortcut_bt, FALSE);

	return TRUE;
}

static void
on_editor_shortcut_toggle (GtkToggleButton *tb, gpointer user_data)
{
	ATPToolEditor *ted = (ATPToolEditor *)user_data;

	if (gtk_toggle_button_get_active (tb))
	{
		gtk_grab_add(GTK_WIDGET(tb));

  		g_signal_connect (G_OBJECT (tb), "key_press_event", G_CALLBACK (on_editor_get_keys), ted);
  		gtk_button_set_label (GTK_BUTTON (tb), _("New accelerator..."));
	}
	else
	{
		g_signal_handlers_disconnect_by_func (G_OBJECT (ted->shortcut_bt), G_CALLBACK (on_editor_get_keys), ted);
		gtk_grab_remove (GTK_WIDGET(ted->shortcut_bt));

		atp_editor_update_shortcut (ted);
	}
}

gboolean
atp_tool_editor_show (ATPToolEditor* ted)
{
	GladeXML *xml;
	guint i;

	if (ted->dialog != NULL)
	{
		/* dialog is already displayed */
		gtk_window_present (GTK_WINDOW (ted->dialog));
		return TRUE;
	}

	if (NULL == (xml = glade_xml_new(GLADE_FILE, TOOL_EDITOR, NULL)))
	{
		anjuta_util_dialog_error (NULL, _("Unable to build user interface for tool editor"));
		g_free(ted);

		return FALSE;
	}
	ted->dialog = glade_xml_get_widget(xml, TOOL_EDITOR);
	gtk_widget_show (ted->dialog);
	gtk_window_set_transient_for (GTK_WINDOW (ted->dialog), atp_tool_dialog_get_window (ted->parent));

	ted->name_en = (GtkEditable *) glade_xml_get_widget (xml, TOOL_NAME);
	ted->command_en = (GtkEditable *) glade_xml_get_widget (xml, TOOL_COMMAND);
	ted->param_en = (GtkEditable *) glade_xml_get_widget (xml, TOOL_PARAM);
	atp_variable_dialog_set_entry (&ted->param_var, ted->param_en);
	ted->dir_en = (GtkEditable *) glade_xml_get_widget (xml, TOOL_WORKING_DIR);
	atp_variable_dialog_set_entry (&ted->dir_var, ted->dir_en);
	ted->enabled_tb = (GtkToggleButton *) glade_xml_get_widget (xml, TOOL_ENABLED);
	ted->terminal_tb = (GtkToggleButton *) glade_xml_get_widget(xml, TOOL_TERMINAL);
	ted->autosave_tb = (GtkToggleButton *) glade_xml_get_widget(xml, TOOL_AUTOSAVE);
	ted->output_com = (GtkComboBox *) glade_xml_get_widget(xml, TOOL_OUTPUT);
	ted->error_com = (GtkComboBox *) glade_xml_get_widget(xml, TOOL_ERROR);
	ted->input_com = (GtkComboBox *) glade_xml_get_widget(xml, TOOL_INPUT);
	ted->input_en = (GtkEditable *) glade_xml_get_widget(xml, TOOL_INPUT_VALUE);
	ted->input_var_bt = (GtkButton *) glade_xml_get_widget(xml, TOOL_INPUT_VARIABLE);
	ted->shortcut_bt = (GtkToggleButton *) glade_xml_get_widget(xml, TOOL_SHORTCUT);
	atp_variable_dialog_set_entry (&ted->input_file_var, ted->input_en);
	atp_variable_dialog_set_entry (&ted->input_string_var, ted->input_en);
	ted->icon_en = (GnomeIconEntry *) glade_xml_get_widget(xml, TOOL_ICON);

	/* Add combox box value */
	set_combo_box_enum_model (ted->error_com, atp_get_error_type_list());
	set_combo_box_enum_model (ted->output_com, atp_get_output_type_list());
	set_combo_box_enum_model (ted->input_com, atp_get_input_type_list());

	#if 0
	ted->detached_tb = (GtkToggleButton *) glade_xml_get_widget(xml, TOOL_DETACHED);
	ted->params_tb = (GtkToggleButton *) glade_xml_get_widget(xml, TOOL_USER_PARAMS);
	ted->file_tb = (GtkToggleButton *) glade_xml_get_widget(xml, TOOL_FILE_LEVEL);
	ted->project_tb = (GtkToggleButton *) glade_xml_get_widget(xml, TOOL_PROJECT_LEVEL);
	ted->input_type_en = (GtkEditable *) glade_xml_get_widget(xml, TOOL_INPUT_TYPE);
	ted->input_type_com = (GtkCombo *) glade_xml_get_widget(xml, TOOL_INPUT_TYPE_COMBO);
	ted->input_en = (GtkEditable *) glade_xml_get_widget(xml, TOOL_INPUT);
	ted->shortcut_en = (GtkEditable *) glade_xml_get_widget(xml, TOOL_SHORTCUT);
	ted->icon_en = (GtkEditable *) glade_xml_get_widget(xml, TOOL_ICON);
	strlist = glist_from_map(input_type_strings);
	gtk_combo_set_popdown_strings(ted->input_type_com, strlist);
	g_list_free(strlist);
	strlist = glist_from_map(output_strings);
	gtk_combo_set_popdown_strings(ted->output_com, strlist);
	gtk_combo_set_popdown_strings(ted->error_com, strlist);
	g_list_free(strlist);
	#endif

	atp_clear_tool_editor (ted);
	atp_populate_tool_editor (ted);
	atp_update_sensitivity (ted);

	#if 0
	set_tool_editor_widget_sensitivity(ted->tool->detached
	  , (ted->tool->input_type == AN_TINP_STRING));
	#endif

	/* Connect all signals */	
	glade_xml_signal_connect_data (xml, EDITOR_RESPONSE_SIGNAL, GTK_SIGNAL_FUNC (on_editor_response), ted);
	glade_xml_signal_connect_data (xml, EDITOR_PARAM_VARIABLE_SIGNAL, GTK_SIGNAL_FUNC (on_editor_param_variable_show), ted);
	glade_xml_signal_connect_data (xml, EDITOR_DIR_VARIABLE_SIGNAL, GTK_SIGNAL_FUNC (on_editor_dir_variable_show), ted);
	glade_xml_signal_connect_data (xml, EDITOR_TOGGLE_SHORCUT_SIGNAL, GTK_SIGNAL_FUNC (on_editor_shortcut_toggle), ted);
	glade_xml_signal_connect_data (xml, EDITOR_TOGGLE_TERMINAL_SIGNAL, GTK_SIGNAL_FUNC (on_editor_terminal_toggle), ted);
	glade_xml_signal_connect_data (xml, EDITOR_INPUT_VARIABLE_SIGNAL, GTK_SIGNAL_FUNC (on_editor_input_variable_show), ted);
	glade_xml_signal_connect_data (xml, EDITOR_INPUT_CHANGED_SIGNAL, GTK_SIGNAL_FUNC (on_editor_input_changed), ted);

	#if 0
	g_signal_connect (G_OBJECT (ted->dialog), "delete_event",
					  G_CALLBACK (on_user_tool_edit_delete_event), NULL);
	g_signal_connect (G_OBJECT (ted->detached_tb), "toggled",
					  G_CALLBACK (on_user_tool_edit_detached_toggled), NULL);
	g_signal_connect (G_OBJECT (ted->input_type_en), "changed",
					  G_CALLBACK (on_user_tool_edit_input_type_changed), NULL);
	#endif
	g_object_unref (xml);

	return TRUE;
}

ATPToolEditor* 
atp_tool_editor_new (ATPUserTool *tool, ATPToolEditorList *list, struct _ATPToolDialog *dialog)
{
	ATPToolEditor *ted;

	/* Search a already existing tool editor with same name */
	for (ted = list->first; ted != NULL; ted = ted->next)
	{
		/* Name use the same string, so a string comparaison is not necessary */
		if (atp_user_tool_get_name (ted->tool) == atp_user_tool_get_name (tool))
		{
			return ted;
		}
	}

	/* Not found, create a new object */
	ted = g_new0(ATPToolEditor, 1);
	ted->parent = dialog;
	ted->owner = list;
	ted->tool = tool;
	atp_variable_dialog_construct (&ted->param_var, ted, ATP_VARIABLE_DEFAULT);
	atp_variable_dialog_construct (&ted->dir_var, ted, ATP_VARIABLE_REPLACE);
	atp_variable_dialog_construct (&ted->input_file_var, ted, ATP_VARIABLE_REPLACE);
	atp_variable_dialog_construct (&ted->input_string_var, ted, ATP_VARIABLE_REPLACE);

	/* Add it in the list */
	if (list != NULL)
	{
		ted->next = list->first;
		list->first = ted;
	}

	return ted;	
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

