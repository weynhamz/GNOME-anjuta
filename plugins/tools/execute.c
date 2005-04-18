/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    execute.c
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
 * Execute an external tool
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "execute.h"

#include "variable.h"

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>

#include <glib.h>

/*---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-tools-plugin.png"
#define MAX_TOOL_PANES 4

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define TOOL_PARAMS "param_dialog"
#define TOOL_PARAMS_EN "tool.params"
#define TOOL_PARAMS_EN_COMBO "tool.params.combo"

/*---------------------------------------------------------------------------*/

/* Command processing */

typedef struct
{
	ATPOutputType type;
	struct _ATPExecutionContext* execution;
	IAnjutaMessageView* view;
	gboolean created;
	GString* buffer;
	IAnjutaEditor* editor;
	guint position;
} ATPOutputContext;

typedef struct _ATPExecutionContext
{
	gchar* name;
	ATPOutputContext output;
	ATPOutputContext error;
	AnjutaPlugin *plugin;
	AnjutaLauncher *launcher;
	gboolean busy;
} ATPExecutionContext;

/* Helper function
 *---------------------------------------------------------------------------*/

/* Return a copy of the name without mnemonic (underscore) */
static gchar*
remove_mnemonic (const gchar* label)
{
	const char *src;
	char *dst;
	char *without;

	without = g_new (char, strlen(label) + 1);
	dst = without;
	for (src = label; *src != '\0'; ++src)
	{
		if (*src == '_')
		{
			/* Remove single underscore */
			++src;
		}
		*dst++ = *src;
	}
	*dst = *src;

	return without;
}

/* Save all current anjuta files */
static gboolean
save_all_files (AnjutaPlugin *plugin)
{
	IAnjutaDocumentManager *docman;
	GList* node;
	gboolean save;

	save = FALSE;	
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell, IAnjutaDocumentManager, NULL);
	if (docman == NULL) return FALSE;

	for (node = ianjuta_document_manager_get_editors (docman, NULL); node != NULL; node = g_list_next (node))
	{
		IAnjutaFileSavable* ed;

		ed = IANJUTA_FILE_SAVABLE (node->data);
		if (ed)
		{
			if (ianjuta_file_savable_is_dirty (ed, NULL))
			{
				ianjuta_file_savable_save (ed, NULL);
				save = TRUE;
			}
		}
	}

	return save;
}

/* Output context
 *---------------------------------------------------------------------------*/

static void
on_message_buffer_flush (IAnjutaMessageView *view, const gchar *line,
						 ATPOutputContext *this)
{
	if (this->view)
		ianjuta_message_view_append (this->view, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line, "", NULL);
}

static gboolean
atp_output_context_print (ATPOutputContext *this, const gchar* text)
{
	const gchar* str;

	if (this->type == ATP_SAME)
	{
		this = &this->execution->output;
	}
	switch (this->type)
	{
	case ATP_SAME:
		/* output should not use this */
		g_return_val_if_reached (TRUE);
		break;
	case ATP_NULL:
		break;
	case ATP_COMMON_MESSAGE:
	case ATP_NEW_MESSAGE:
		/* Check if the view has already been created */
		if (this->created == FALSE)
		{
			IAnjutaMessageManager *man;
			gchar* title;
			guint i;

			man = anjuta_shell_get_interface (this->execution->plugin->shell, IAnjutaMessageManager, NULL);
			if (this->view == NULL)
			{
				this->view = ianjuta_message_manager_add_view (man, "", ICON_FILE, NULL);
				g_signal_connect (G_OBJECT (this->view), "buffer_flushed",
							  G_CALLBACK (on_message_buffer_flush), this);
				g_object_add_weak_pointer (G_OBJECT (this->view), (gpointer *)&this->view);
			}
			else
			{
				ianjuta_message_view_clear (this->view, NULL);
			}
			if (this->execution->error.type == ATP_SAME)
			{
				/* Same message used for all outputs */
				str = "";
			}
			else if (this == &this->execution->output)
			{
				/* Only for output data */
				str = _("(output)");
			}
			else
			{
				/* Only for error data */
				str = _("(error)");
			}
			title = g_strdup_printf ("%s %s", this->execution->name, str);
			
			ianjuta_message_manager_set_view_title (man, this->view, title, NULL);
			g_free (title);
			this->created = TRUE;
		}
		/* Display message */
		if (this->view)
		{
			ianjuta_message_view_buffer_append (this->view, text, NULL);
		}
		break;
	case ATP_NEW_BUFFER:
		ianjuta_editor_append (this->editor, text, strlen(text), NULL);
		break;
	case ATP_REPLACE_BUFFER:
		/* TODO: I don't know how to do this */
		break;
	case ATP_INSERT_BUFFER:
	case ATP_APPEND_BUFFER:
	case ATP_POPUP_DIALOG:
		g_string_append (this->buffer, text);	
		break;
	}

	return TRUE;
}

static gboolean
atp_output_context_print_command (ATPOutputContext *this, const gchar* command)
{
	gboolean ok;

	ok = TRUE;
	switch (this->type)
	{
	case ATP_NULL:
	case ATP_SAME:
		break;
	case ATP_COMMON_MESSAGE:
	case ATP_NEW_MESSAGE:
		
		ok = atp_output_context_print (this, _("Running command: "));
		ok &= atp_output_context_print (this, command);
		ok &= atp_output_context_print (this, "...\n");	
		break;
	case ATP_NEW_BUFFER:
	case ATP_REPLACE_BUFFER:
	case ATP_INSERT_BUFFER:
	case ATP_APPEND_BUFFER:
	case ATP_POPUP_DIALOG:
		/* Do nothing for all these cases */
		break;
	}

	return ok;
};

static gboolean
atp_output_context_print_result (ATPOutputContext *this, gint error)
{
	gboolean ok;
	char buffer[33];
	IAnjutaMessageManager *man;

	ok = TRUE;
	switch (this->type)
	{
	case ATP_NULL:
	case ATP_SAME:
		break;
	case ATP_COMMON_MESSAGE:
	case ATP_NEW_MESSAGE:
		if (this == &this->execution->output)
		{
			if (error)
			{
				ok = atp_output_context_print (this, _("Completed ... unsuccessful with "));
				sprintf (buffer, "%d", error);
				ok &= atp_output_context_print (this, buffer);
			}
			else
			{
				ok = atp_output_context_print (this, _("Completed ... successful"));
			}
			ok &= atp_output_context_print (this, "\n");
			if (this->view)
			{
				man = anjuta_shell_get_interface (this->execution->plugin->shell, IAnjutaMessageManager, NULL);
				ianjuta_message_manager_set_current_view (man, this->view, NULL);
			}

		}
		break;
	case ATP_NEW_BUFFER:
	case ATP_REPLACE_BUFFER:
		/* Do nothing  */
		break;
	case ATP_INSERT_BUFFER:
		ianjuta_editor_insert (this->editor, this->position, this->buffer->str, this->buffer->len, NULL);
		g_string_free (this->buffer, TRUE);
		this->buffer = NULL;
		break;
	case ATP_APPEND_BUFFER:
		ianjuta_editor_append (this->editor, this->buffer->str, this->buffer->len, NULL);
		g_string_free (this->buffer, TRUE);
		this->buffer = NULL;
		break;
	case ATP_POPUP_DIALOG:
		if (this->buffer->len)
		{
			if (this == &this->execution->output)
			{
				anjuta_util_dialog_error (NULL, this->buffer->str);
			}
			else
			{
				anjuta_util_dialog_info (NULL, this->buffer->str);
			}
			g_string_free (this->buffer, TRUE);
		}
		break;
	}

	return ok;
};

static ATPOutputContext*
atp_output_context_initialize (ATPOutputContext *this, ATPExecutionContext *execution, ATPOutputType type)
{
	IAnjutaDocumentManager *docman;
	IAnjutaEditor *ed;

	switch (this->type)
	{
	case ATP_NULL:
	case ATP_SAME:
		break;
	case ATP_COMMON_MESSAGE:
	case ATP_NEW_MESSAGE:
		this->created = FALSE;
		break;
	case ATP_NEW_BUFFER:
		docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (this->execution->plugin)->shell, IAnjutaDocumentManager, NULL);
		this->editor = ianjuta_document_manager_add_buffer (docman, this->execution->name,"", NULL);
	case ATP_REPLACE_BUFFER:
		/* TODO: I don't know how to do this */
		break;
	case ATP_INSERT_BUFFER:
	case ATP_APPEND_BUFFER:
		docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (this->execution->plugin)->shell, IAnjutaDocumentManager, NULL);
		this->editor = ianjuta_document_manager_get_current_editor (docman, NULL);
		if (this->editor == NULL)
		{
			anjuta_util_dialog_warning (NULL, _("No document currently open, command aborted"));
			return NULL;
		}
		this->position = ianjuta_editor_get_position (this->editor, NULL);
		/* No break, need a buffer too */
	case ATP_POPUP_DIALOG:
		if (this->buffer == NULL)
		{
			this->buffer = g_string_new ("");
		}
		else
		{
			g_string_erase (this->buffer, 0, -1);
		}
		break;
	}

	return this;
}


static ATPOutputContext*
atp_output_context_construct (ATPOutputContext *this, ATPExecutionContext *execution, ATPOutputType type)
{

	this->type = type;
	this->execution = execution;
	this->view = NULL;
	this->buffer = NULL;

	return atp_output_context_initialize (this, execution, type);
}

static void
atp_output_context_destroy (ATPOutputContext *this)
{
	if (this->view)
	{
		IAnjutaMessageManager *man;

		man = anjuta_shell_get_interface (this->execution->plugin->shell, IAnjutaMessageManager, NULL);
		ianjuta_message_manager_remove_view (man, this->view, NULL);
		this->view = NULL;
	}
	if (this->buffer)
	{
		g_string_free (this->buffer, TRUE);
	}
}

/* Execute context
 *---------------------------------------------------------------------------*/

static void
on_run_terminated (AnjutaLauncher* launcher, gint pid, gint status, gulong time, gpointer user_data)
{
	ATPExecutionContext *this = (ATPExecutionContext *)user_data;

	/* TODO: add all code */
	atp_output_context_print_result (&this->output, status);
	atp_output_context_print_result (&this->error, status);
	this->busy = FALSE;
}

static void
on_run_output (AnjutaLauncher* launcher, AnjutaLauncherOutputType type, const gchar* output, gpointer user_data)
{
	ATPExecutionContext* this = (ATPExecutionContext*)user_data;

	switch (type)
	{
	case ANJUTA_LAUNCHER_OUTPUT_STDOUT:
		atp_output_context_print (&this->output, output);
		break;
	case ANJUTA_LAUNCHER_OUTPUT_STDERR:
		atp_output_context_print (&this->error, output);
		break;
	case ANJUTA_LAUNCHER_OUTPUT_PTY:
		/* TODO: Do nothing ? */
		break;
	}
}

static ATPExecutionContext*
atp_execution_context_reuse (ATPExecutionContext* this, const gchar *name, ATPOutputType output, ATPOutputType error)
{
	if (this->name) g_free (this->name);
	this->name = remove_mnemonic (name);

	if (atp_output_context_initialize (&this->output, this, output) == NULL)
	{
		return NULL;
	}
	if (atp_output_context_initialize (&this->error, this, error) == NULL)
	{
		return NULL;
	}
	
	return this;
}

static ATPExecutionContext*
atp_execution_context_new (AnjutaPlugin *plugin, const gchar *name, guint id, ATPOutputType output, ATPOutputType error)
{
	ATPExecutionContext *this;

	this = g_new0 (ATPExecutionContext, 1);

	this->plugin = plugin;
	this->launcher =  anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (this->launcher), "child-exited", G_CALLBACK (on_run_terminated), this);
	this->name = remove_mnemonic (name);

	if (atp_output_context_construct (&this->output, this, output) == NULL)
	{
 		g_free (this);
		return NULL;
	}
	if (atp_output_context_construct (&this->error, this, error) == NULL)
	{
		g_free (this);
		return NULL;
	}
	
	return this;
}

static void
atp_execution_context_free (ATPExecutionContext* this)
{
	atp_output_context_destroy (&this->output);
	atp_output_context_destroy (&this->error);

	if (this->launcher)
	{
		if (anjuta_launcher_is_busy (this->launcher))
			anjuta_launcher_reset (this->launcher);
		g_object_unref (this->launcher);
	}
	if (this->name) g_free (this->name);

	g_free (this);
}

static void
atp_execution_context_execute (ATPExecutionContext* this, const gchar* command)
{
	atp_output_context_print_command (&this->output, command);
	anjuta_launcher_execute (this->launcher, command, on_run_output, this);
	this->busy = TRUE;
}

/* Execute context list
 *---------------------------------------------------------------------------*/

ATPContextList *
atp_context_list_construct (ATPContextList *this)
{
	this->list = NULL;
	return this;
}

void
atp_context_list_destroy (ATPContextList *this)
{
	GList *item;

	for (item = this->list; item != NULL;)
	{
		this->list = g_list_remove_link (this->list, item);
		atp_execution_context_free ((ATPExecutionContext *)item->data);
		g_list_free (item);
	}
}

static ATPExecutionContext*
atp_context_list_find_context (ATPContextList *this, AnjutaPlugin *plugin, const gchar* name, ATPOutputType output, ATPOutputType error)
{
	ATPExecutionContext* context;
	GList* reuse;
	guint best;
	guint pane;
	GList* node;
	gboolean new_pane;
	gboolean output_pane;
	gboolean error_pane;

	pane = 0;
	best = 0;
	context = NULL;
	new_pane = (output == ATP_NEW_MESSAGE) || (error == ATP_NEW_MESSAGE);
	output_pane = (output == ATP_NEW_MESSAGE) || (output == ATP_COMMON_MESSAGE);
	error_pane = (error == ATP_NEW_MESSAGE) || (error == ATP_COMMON_MESSAGE);
	for (node = this->list; node != NULL; node = g_list_next (node))
	{
		ATPExecutionContext* current;
		guint score;

		current = (ATPExecutionContext *)node->data;

		/* Count number of used message panes */
		if (current->output.view != NULL) pane++;
		if (current->error.view != NULL) pane++;

		score = 1;
		if ((current->output.view != NULL) == output_pane) score++;
		if ((current->error.view != NULL) == error_pane) score++;

		if (!current->busy)
		{
			if ((score > best) || ((score == best) && (new_pane)))
			{
				/* Reuse this context */
				context = current;
				reuse = node;
				best = score;
			}
		}
	}

	if ((new_pane) && (pane < MAX_TOOL_PANES))
	{
		/* Not too many message pane, we can create a new one */
		context = NULL;
	}

	if (context == NULL)
	{
		/* Create a new node */
		context = atp_execution_context_new (plugin, name, 0, output, error);
		if (context != NULL)
		{
			this->list = g_list_prepend (this->list, context);
		}
	}
	else
	{
		this->list = g_list_remove_link (this->list, reuse);
		context = atp_execution_context_reuse (context, name, output, error);
		if (context != NULL)
		{
			this->list = g_list_concat (reuse, this->list);
		}
	}
	
	return context;
}

/* Execute tools
 *---------------------------------------------------------------------------*/

void
atp_user_tool_execute (GtkMenuItem *item, ATPUserTool* this)
{
	ATPPlugin* plugin;
	ATPVariable* variable;
	ATPContextList* list;
	ATPExecutionContext* context;
	GString* string;
	gchar* dir;
	gchar* cmd;
	const gchar* param;
	gchar* val;
	guint len;

	plugin = atp_user_tool_get_plugin (this);
	variable = atp_plugin_get_variable (plugin);

	/* Save files if requested */
	if (atp_user_tool_get_flag (this, ATP_TOOL_AUTOSAVE))
	{
		save_all_files (ANJUTA_PLUGIN (plugin));
	}
	
	/* Make command line */
	string = g_string_new (atp_user_tool_get_command (this));
	g_string_append_c (string, ' ');

	/* Add argument and replace variable*/
	param = atp_user_tool_get_param (this); 
	if (param != NULL)
	{
		for (; *param != '\0'; param += len)
		{
			for (len = 0; (param[len] != '\0') && g_ascii_isspace (param[len]); len++);
			g_string_append_len (string, param, len);
			param += len;
			for (len = 0; (param[len] != '\0') && !g_ascii_isspace (param[len]); len++);
			if (len)
			{
				val = atp_variable_get_value_from_name_part (variable, param, len);
				if (val)
				{
					g_string_append (string, val);
				}
				else
				{
					g_string_append_len (string, param, len);
				}
				g_free (val);
			}
		}
	}
	/* Remove leading space, trailing space and empty string */
	cmd = g_string_free (string, FALSE);
	g_strstrip (cmd);
	if ((cmd != NULL) && (*cmd == '\0'))
	{
		g_free (cmd);
		cmd = NULL;
	}

	/* Get working directory and replace variable */
	for(param = atp_user_tool_get_working_dir (this);g_ascii_isspace(*param); param++);
	for(len = 0; param[len] != '\0';++len)
	{
		if (param[len] == G_DIR_SEPARATOR) break;
	}
	dir = atp_variable_get_value_from_name_part (variable, param, len);
	if (dir == NULL)
	{	       
		dir = g_strdup (param);
	}
	else if (param[len] != '\0')
	{
		/* Append additional directory */
		val = g_strconcat(dir, param + len, NULL);
		g_free (dir);
		dir = val;
	}
	/* Remove leading space, trailing space and empty string */
	g_strstrip (dir);
	if ((dir != NULL) && (*dir == '\0'))
	{
		g_free (dir);
		dir = NULL;
	}

	if (atp_user_tool_get_flag (this, ATP_TOOL_TERMINAL))
	{
		/* Run in a terminal */
		/* don't need a execution context, launch and forget */

		gnome_execute_terminal_shell (dir, cmd);
	}
	else
	{
		list = atp_plugin_get_context_list (plugin);

		context = atp_context_list_find_context (list, ANJUTA_PLUGIN(plugin),
			       atp_user_tool_get_name (this),
			       atp_user_tool_get_output (this),
			       atp_user_tool_get_error (this));

		if (context)
		{
			/* Set working directory */
			if (dir != NULL)
			{
				val = g_get_current_dir();
				chdir (dir);
			}
	
			/* Run command */
			atp_execution_context_execute (context, cmd);

			/* Restore previous current directory */
			if (dir != NULL)
			{
				chdir (val);
				g_free (val);
			}
		}
	}

	if (dir != NULL) g_free (dir);
	if (cmd != NULL) g_free (cmd);
}


#if 0
/* Popup a dialog to ask for user parameters */
static const gchar *
get_user_params(AnUserTool *tool, gint *response_ptr)
{
	char title[256];
	GladeXML *xml;
	GtkWidget *dialog;
	GtkEntry *params_en;
	GtkCombo *params_com;
	
	if (NULL == (xml = glade_xml_new(GLADE_FILE_ANJUTA, TOOL_PARAMS, NULL)))
	{
		anjuta_error(_("Unable to build user interface for tool parameters"));
		return FALSE;
	}
	dialog = glade_xml_get_widget(xml, TOOL_PARAMS);
	snprintf(title, 256, _("%s: Command line parameters"), tool->name);
	gtk_window_set_title (GTK_WINDOW(dialog), title);
	gtk_window_set_transient_for (GTK_WINDOW(dialog)
	  , GTK_WINDOW(app));
	params_en = (GtkEntry *) glade_xml_get_widget(xml, TOOL_PARAMS_EN);
	params_com = (GtkCombo *) glade_xml_get_widget(xml, TOOL_PARAMS_EN_COMBO);
	gtk_combo_disable_activate (GTK_COMBO(params_com));
	gtk_entry_set_activates_default (GTK_ENTRY (GTK_COMBO(params_com)->entry),
									 TRUE);
	glade_xml_signal_autoconnect(xml);
	g_object_unref (xml);
	*response_ptr = gtk_dialog_run ((GtkDialog *) dialog);
	gtk_widget_destroy (dialog);
	return gtk_entry_get_text(params_en);
}


/* Simplistic output handler - needs to be enhanced */
static void
tool_stdout_handler (const gchar *line)
{
	if (line && current_tool)
	{
		if (current_tool->output <= MESSAGE_MAX  && current_tool->output >= 0)
		{
			/* Send the message to the proper message pane */
			an_message_manager_append (app->messages, line
			  ,current_tool->output);
		}
		else
		{
			/* Store the line so that proper action can be taken once
			the tool has finished running */
			if (NULL == current_tool_output)
				current_tool_output = g_string_new(line);
			else
				g_string_append(current_tool_output, line);
		}
	}
}

/* Simplistic error handler - needs to be enhanced */
static void
tool_stderr_handler (const gchar *line)
{
	if (line && current_tool)
	{
		if (current_tool->error <= MESSAGE_MAX  && current_tool->error >= 0)
		{
			/* Simply send the message to the proper message pane */
			an_message_manager_append (app->messages, line
			  , current_tool->error);
		}
		else
		{
			/* Store the line so that proper action can be taken once
			the tool has finished running */
			if (NULL == current_tool_error)
				current_tool_error = g_string_new(line);
			else
				g_string_append(current_tool_error, line);
		}
	}
}

static void
tool_output_handler (AnjutaLauncher *launcher,
					 AnjutaLauncherOutputType output_type,
					 const gchar * mesg, gpointer data)
{
	switch (output_type)
	{
	case ANJUTA_LAUNCHER_OUTPUT_STDERR:
		tool_stderr_handler (mesg);
		break;
	default:
		tool_stdout_handler (mesg);
	}
}

/* Handle non-message output and error lines from current tool */
static void
handle_tool_output(int type, GString *s, gboolean is_error)
{
	TextEditor *te;
	int sci_message = -1;

	if (AN_TBUF_POPUP == type)
	{
		if (is_error)
			anjuta_error (s->str);
		else
			anjuta_information (s->str);
		return;
	}
	if (AN_TBUF_NEW == type || (NULL == app->current_text_editor))
		te = anjuta_append_text_editor(NULL, NULL);
	else
		te = app->current_text_editor;
	if (NULL == te)
	{
		anjuta_error("Unable to create new text buffer");
		return;
	}

	switch(type)
	{
		case AN_TBUF_NEW:
		case AN_TBUF_INSERT:
			sci_message = SCI_ADDTEXT;
			break;
		case AN_TBUF_REPLACE:
			sci_message = SCI_SETTEXT;
			break;
		case AN_TBUF_APPEND:
			sci_message = SCI_APPENDTEXT;
			break;
		case AN_TBUF_REPLACESEL:
			sci_message = SCI_REPLACESEL;
			break;
		default:
			anjuta_error("Got invalid tool output/error redirection message");
			return;
	}
	scintilla_send_message(SCINTILLA(te->widgets.editor), sci_message
	  , s->len, (long) s->str);
}

/* Termination handler
** Nothing much to do if output and error messages have been sent to the
** message panes. Otherwise, we need to decide what to do with the output
** and error and do it.
*/
static void
tool_terminate_handler (AnjutaLauncher *launcher,
						gint child_pid, gint status,
						gulong time_taken, gpointer data)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
										  G_CALLBACK (tool_terminate_handler),
										  data);
	if (current_tool)
	{
		if (current_tool->error <= MESSAGE_MAX  && current_tool->error >= 0)
		{
			char line[BUFSIZ];
			snprintf(line, BUFSIZ, "Tool terminated with status %d\n", status);
			an_message_manager_append(app->messages, line
			  , current_tool->error);
		}
		else if (current_tool_error && (0 < current_tool_error->len)  && current_tool->error >= 0)
		{
			handle_tool_output(current_tool->error, current_tool_error, TRUE);
		}
		if (current_tool->output <= MESSAGE_MAX  && current_tool->output >= 0)
		{
			/* Nothing to do here */
		}
		else if (current_tool_output && (0 < current_tool_output->len)  && current_tool->output >= 0)
		{
			handle_tool_output(current_tool->output, current_tool_output, FALSE);
		}
		if ('\0' != tmp_file[0])
		{
			unlink(tmp_file);
			tmp_file[0] = '\0';
		}
		current_tool = NULL;
	}
}

/* Menu activate handler which executes the tool. It should do command
** substitution, input, output and error redirection, setting the
** working directory, etc. Currently, it just executes the tool and
** appends output and error to output and error panes of the message
** manager.
*/
static const gchar *get_user_params(AnUserTool *tool, gint *response_ptr);

static void
execute_tool(GtkMenuItem *item, gpointer data)
{
	AnUserTool *tool = (AnUserTool *) data;
	const gchar *params = NULL;
	gchar *command;
	gchar *working_dir;
	
#ifdef TOOL_DEBUG
	g_message("Tool: %s (%s)\n", tool->name, tool->command);
#endif
	/* Ask for user parameters if required */
	if (tool->user_params)
	{
		gint response;
		params = get_user_params(tool, &response);
		if (response != GTK_RESPONSE_OK)
			return;
		/*
		if (!anjuta_get_user_params(tool->name, &params))
			return;
		*/
	}
	if (tool->autosave)
	{
		anjuta_save_all_files();
	}
	/* Expand variables to get the full command */
	anjuta_set_editor_properties();
	if (params)
	{
		gchar *cmd = g_strconcat(tool->command, " ", params, NULL);
		command = prop_expand(app->project_dbase->props, cmd);
		g_free(cmd);
	}
	else
		command = prop_expand(app->project_dbase->props, tool->command);
	if (NULL == command)
		return;
#ifdef TOOL_DEBUG
	g_message("Command is '%s'", command);
#endif

	/* Set the current working directory */
	if (tool->working_dir)
	{
		working_dir = prop_expand(app->project_dbase->props
		  , tool->working_dir);
	} else {
		working_dir = g_strdup (getenv("HOME"));
	}
#ifdef TOOL_DEBUG
	g_message("Working dir is %s", working_dir);
#endif
	anjuta_set_execution_dir(working_dir);
	chdir(working_dir);
	if (tool->detached)
	{
		/* Detached mode - execute and forget about it */
		if(tool->run_in_terminal)
		{
			gchar* escaped_cmd = anjuta_util_escape_quotes(command);
			prop_set_with_key (app->project_dbase->props
			  , "anjuta.current.command", escaped_cmd);
			g_free(escaped_cmd);
			g_free(command);
			command = command_editor_get_command (app->command_editor
			  , COMMAND_TERMINAL);
		}
		else
		{
			prop_set_with_key (app->project_dbase->props
			  , "anjuta.current.command", command);
		}
#ifdef TOOL_DEBUG
		g_message("Final command: '%s'\n", command);
#endif
		gnome_execute_shell(working_dir, command);
	}
	else
	{
		/* Attached mode - run through launcher */
		char *buf = NULL;
		current_tool = tool;
		if (current_tool_output)
			g_string_truncate(current_tool_output, 0);
		if (current_tool_error)
			g_string_truncate(current_tool_error, 0);		
		if (tool->error <= MESSAGE_MAX  && tool->error >= 0)
			an_message_manager_clear(app->messages, tool->error);
		if (tool->input_type == AN_TINP_BUFFER && app->current_text_editor)
		{
			long len = scintilla_send_message(
			  SCINTILLA(app->current_text_editor->widgets.editor)
			, SCI_GETLENGTH, 0, 0);
			buf = g_new(char, len+1);
			scintilla_send_message(
			  SCINTILLA(app->current_text_editor->widgets.editor)
			, SCI_GETTEXT, len+1, (long) buf);
		}
		else if ((tool->input_type == AN_TINP_STRING) && (tool->input)
		  && '\0' != tool->input[0])
		{
			buf = prop_expand(app->project_dbase->props, tool->input);
			if (NULL == buf)
				buf = g_strdup("");
		}
		if (buf)
		{
			int fd;
			int buflen = strlen(buf);
			gchar* escaped_cmd, *real_tmp_file;
			
			real_tmp_file = get_a_tmp_file();
			strncpy (tmp_file, real_tmp_file, PATH_MAX);
			g_free (real_tmp_file);
			
			if (0 > (fd = mkstemp(tmp_file)))
			{
				anjuta_system_error(errno, "Unable to create temporary file %s!"
				  , tmp_file);
				return;
			}
			if (buflen != write(fd, buf, buflen))
			{
				anjuta_system_error(errno, "Unable to write to temporary file %s."
				  , tmp_file);
				return;
			}
			close(fd);
			escaped_cmd = anjuta_util_escape_quotes(command);
			g_free(command);
			command = g_strconcat("sh -c \"", escaped_cmd, "<", tmp_file, "\"", NULL);
			g_free(buf);
		}
		if (tool->output <= MESSAGE_MAX  && tool->output >= 0)
		{
			an_message_manager_clear(app->messages, tool->output);
			an_message_manager_show(app->messages, tool->output);
		}
#ifdef TOOL_DEBUG
	g_message("Final command: '%s'\n", command);
#endif
		g_signal_connect (app->launcher, "child-exited",
						  G_CALLBACK (tool_terminate_handler), NULL);
		
		if (FALSE == anjuta_launcher_execute(app->launcher, command,
											 tool_output_handler, NULL))
		{
			anjuta_error("%s: Unable to launch!", command);
		}
		g_free(command);
	}
	g_free(working_dir);
}
#endif
