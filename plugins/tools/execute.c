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

/*---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-tools-plugin.png"

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
	union
	{
		IAnjutaMessageView* view;
		gint id;
	};
} ATPOutputContext;

typedef struct
{
	ATPOutputContext output;
	ATPOutputContext error;
	AnjutaPlugin *plugin;
	AnjutaLauncher *launcher;
	gboolean busy;
} ATPExecutionContext;

/* Output context
 *---------------------------------------------------------------------------*/

static void
atp_output_context_append_view (ATPOutputContext* this, const gchar* text)
{
	if (this->view)
	{
		ianjuta_message_view_buffer_append (this->view, text, NULL);
	}
}

static void
atp_output_context_print_view (ATPOutputContext* this, IAnjutaMessageViewType type, const gchar* summary, const gchar* details)
{
	if (this->view)
	{
		ianjuta_message_view_append (this->view, type, summary, details, NULL);
	}
}

static void
on_message_buffer_flush (IAnjutaMessageView *view, const gchar *line,
						 ATPOutputContext *this)
{
	atp_output_context_print_view (this, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line, "");
}

static ATPOutputContext*
atp_output_context_construct (ATPOutputContext *this, AnjutaPlugin *plugin)
{
	IAnjutaMessageManager* man;

	man = anjuta_shell_get_interface (plugin->shell, IAnjutaMessageManager, NULL);

	this->view = ianjuta_message_manager_add_view (man, _("Anjuta Tools"), ICON_FILE, NULL);
	if (this->view != NULL)
	{
		g_signal_connect (G_OBJECT (this->view), "buffer_flushed",
						  G_CALLBACK (on_message_buffer_flush), this);
		g_object_add_weak_pointer (G_OBJECT (this->view), (gpointer *)&this->view);
	}
	else
	{
		ianjuta_message_view_clear (this->view, NULL);
	}

	return this;
}

static void
atp_output_context_destroy (ATPOutputContext *this, AnjutaPlugin* plugin)
{
	switch (this->type)
	{
	case ATP_NULL:
	case ATP_SAME:
		break;
	case ATP_COMMON_MESSAGE:
	case ATP_PRIVATE_MESSAGE:
		if (this->view)
		{
			IAnjutaMessageManager *man;

			man = anjuta_shell_get_interface (plugin->shell, IAnjutaMessageManager, NULL);
			ianjuta_message_manager_remove_view (man, this->view, NULL);
			this->view = NULL;
		}
		break;
	case ATP_NEW_BUFFER:
	case ATP_REPLACE_BUFFER:
	case ATP_INSERT_BUFFER:
	case ATP_APPEND_BUFFER:
	case ATP_POPUP_DIALOG:
		/* TODO: Work in progress */
		break;
	}
}

/* Execute context
 *---------------------------------------------------------------------------*/

static void
on_run_terminated (AnjutaLauncher* launcher, gint pid, gint status, gulong time, gpointer user_data)
{
	ATPExecutionContext *this = (ATPExecutionContext *)user_data;

	/* TODO: add all code */
	this->busy = FALSE;
}

static void
on_run_output (AnjutaLauncher* launcher, AnjutaLauncherOutputType type, const gchar* output, gpointer user_data)
{
	ATPExecutionContext* this = (ATPExecutionContext*)user_data;
	
	/* TODO: add all code */
	atp_output_context_append_view (&this->output, output);
}

static ATPExecutionContext*
atp_execution_context_new (AnjutaPlugin *plugin)
{
	ATPExecutionContext *this;

	this = g_new0 (ATPExecutionContext, 1);

	this->plugin = plugin;
	this->launcher =  anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (this->launcher), "child-exited", G_CALLBACK (on_run_terminated), this);

	atp_output_context_construct (&this->output, plugin);

	return this;
}

static void
atp_execution_context_free (ATPExecutionContext* this)
{
	atp_output_context_destroy (&this->output, this->plugin);
	atp_output_context_destroy (&this->error, this->plugin);

	if (this->launcher)
	{
		if (anjuta_launcher_is_busy (this->launcher))
			anjuta_launcher_reset (this->launcher);
		g_object_unref (this->launcher);
	}

	g_free (this);
}

static void
atp_execution_context_execute (ATPExecutionContext* this, const gchar* command)
{
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
	GSList *item;

	for (item = this->list; item != NULL;)
	{
		this->list = g_slist_remove_link (this->list, item);

		atp_execution_context_free ((ATPExecutionContext *)item->data);
		g_slist_free (item);
	}
}

static ATPExecutionContext*
atp_context_list_find_context (ATPContextList *this, AnjutaPlugin *plugin)
{
	ATPExecutionContext* context;

	context = atp_execution_context_new (plugin);
	this->list = g_slist_prepend (this->list, context);

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
	param = atp_user_tool_get_working_dir (this);
	dir = atp_variable_get_value (variable, param);
	if (dir == NULL) dir = g_strdup (param);
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

		context = atp_context_list_find_context (list, ANJUTA_PLUGIN(plugin));

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
