/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2004 Naba Kumar

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

#include <config.h>

/*#define DEBUG*/

#include "plugin.h"

#include "breakpoints.h"
#include "watch.h"
#include "locals.h"
#include "stack_trace.h"
#include "threads.h"
#include "info.h"
#include "memory.h"
#include "disassemble.h"
#include "signals.h"
#include "sharedlib.h"
#include "registers.h"
#include "utilities.h"
#include "start.h"
#include "queue.h"

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-debug-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

#include <libgnomevfs/gnome-vfs.h>

/* Contants defintion
 *---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-debug-manager-plugin-48.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-debug-manager.ui"
#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-debug-manager.glade"

/* Plugin type
 *---------------------------------------------------------------------------*/

struct _DebugManagerPlugin
{
	AnjutaPlugin parent;

	/* Debugger queue */
	DmaDebuggerQueue *queue;
	
	/* Menu item */
	gint uiid;
	GtkActionGroup *start_group;
	GtkActionGroup *loaded_group;
	GtkActionGroup *stopped_group;
	GtkActionGroup *running_group;

	/* Project */
	gchar *project_root_uri;
	guint project_watch_id;

	/* Editor */
	IAnjutaEditor *current_editor;
	guint editor_watch_id;
	IAnjutaEditor *pc_editor;
	guint pc_line;
	guint pc_address;
	gboolean busy;
	
	/* Debugger components */
	Locals *locals;
	ExprWatch *watch;
	BreakpointsDBase *breakpoints;
	DmaStart *start;
	StackTrace *stack;
	CpuRegisters *registers;
	Sharedlibs *sharedlibs;
	Signals *signals;	
	DmaMemory *memory;
	DmaDisassemble *disassemble;
	DmaThreads *thread;
	
	
	/* Logging view */
	IAnjutaMessageView* view;
};

struct _DebugManagerPluginClass
{
	AnjutaPluginClass parent_class;
};

/* Private functions
 *---------------------------------------------------------------------------*/

static void
register_stock_icons (AnjutaPlugin *plugin)
{
        static gboolean registered = FALSE;

        if (registered)
                return;
        registered = TRUE;

        /* Register stock icons */
		BEGIN_REGISTER_ICON (plugin)
		REGISTER_ICON (ICON_FILE, "debugger-icon");
        REGISTER_ICON ("stack.png", "gdb-stack-icon");
        REGISTER_ICON ("locals.png", "gdb-locals-icon");
        REGISTER_ICON_FULL ("anjuta-watch", "gdb-watch-icon");
        REGISTER_ICON_FULL ("anjuta-breakpoint-toggle", ANJUTA_STOCK_BREAKPOINT_TOGGLE);
        REGISTER_ICON_FULL ("anjuta-breakpoint-clear", ANJUTA_STOCK_BREAKPOINT_CLEAR);		
    	/* We have no -24 version for the next two */
		REGISTER_ICON ("anjuta-breakpoint-disabled-16.png", ANJUTA_STOCK_BREAKPOINT_DISABLED);
        REGISTER_ICON ("anjuta-breakpoint-enabled-16.png", ANJUTA_STOCK_BREAKPOINT_ENABLED);				
		REGISTER_ICON_FULL ("anjuta-attach", "debugger-attach");
		REGISTER_ICON_FULL ("anjuta-step-into", "debugger-step-into");
		REGISTER_ICON_FULL ("anjuta-step-out", "debugger-step-out");
		REGISTER_ICON_FULL ("anjuta-step-over", "debugger-step-over");
		REGISTER_ICON_FULL ("anjuta-run-to-cursor", "debugger-run-to-cursor");
		END_REGISTER_ICON
}

/* Program counter functions
 *---------------------------------------------------------------------------*/

static void
show_program_counter_in_editor(DebugManagerPlugin *self)
{
	IAnjutaEditor *editor = self->current_editor;
	
	if ((editor != NULL) && (self->pc_editor == editor))
	{
		if (IANJUTA_IS_MARKABLE (editor))
		{
			ianjuta_markable_mark(IANJUTA_MARKABLE (editor), self->pc_line, IANJUTA_MARKABLE_PROGRAM_COUNTER, NULL);
		}
		if (IANJUTA_IS_INDICABLE(editor))
		{
			gint begin = ianjuta_editor_get_line_begin_position(editor, self->pc_line, NULL);
			gint end = ianjuta_editor_get_line_end_position(editor, self->pc_line, NULL);
			
			ianjuta_indicable_set(IANJUTA_INDICABLE(editor), begin, end, IANJUTA_INDICABLE_IMPORTANT,
				NULL);
		}		
	}
}

static void
hide_program_counter_in_editor(DebugManagerPlugin *self)
{
	IAnjutaEditor *editor = self->current_editor;
	
	if ((editor != NULL) && (self->pc_editor == editor))
	{
		if (IANJUTA_IS_MARKABLE (editor))
		{
			ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (editor), IANJUTA_MARKABLE_PROGRAM_COUNTER, NULL);
		}
		if (IANJUTA_IS_INDICABLE(editor))
		{
			ianjuta_indicable_clear(IANJUTA_INDICABLE(editor), NULL);			
		}		
	}
}

static void
set_program_counter(DebugManagerPlugin *self, const gchar* file, guint line, guint address)
{
	IAnjutaDocumentManager *docman = NULL;
	gchar *file_uri;

	/* Remove previous marker */
	hide_program_counter_in_editor (self);
	if (self->pc_editor != NULL)
	{
		g_object_remove_weak_pointer (G_OBJECT (self->pc_editor), (gpointer *)(gpointer)&self->pc_editor);
		self->pc_editor = NULL;
	}
	self->pc_address = address;

	if (file != NULL)
	{
		docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (self)->shell, IAnjutaDocumentManager, NULL);
		file_uri = g_strconcat ("file://", file, NULL);
		if (docman)
		{
			IAnjutaEditor* editor;
		
			editor = ianjuta_document_manager_goto_file_line(docman, file_uri, line, NULL);
		
			if (editor != NULL)
			{
				self->pc_editor = editor;
				g_object_add_weak_pointer (G_OBJECT (editor), (gpointer)(gpointer)&self->pc_editor);
				self->pc_line = line;
				show_program_counter_in_editor (self);
			}
		}		
		g_free (file_uri);
	}
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	DebugManagerPlugin *dm_plugin;
	const gchar *root_uri;

	dm_plugin = ANJUTA_PLUGIN_DEBUG_MANAGER (plugin);
	
	if (dm_plugin->project_root_uri)
		g_free (dm_plugin->project_root_uri);
	dm_plugin->project_root_uri = NULL;
	
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		dm_plugin->project_root_uri = g_strdup (root_uri);
	}
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer user_data)
{
	DebugManagerPlugin *dm_plugin;

	dm_plugin = ANJUTA_PLUGIN_DEBUG_MANAGER (plugin);
	
	if (dm_plugin->project_root_uri)
		g_free (dm_plugin->project_root_uri);
	dm_plugin->project_root_uri = NULL;
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	GObject *editor;
	DebugManagerPlugin *self;

	self = ANJUTA_PLUGIN_DEBUG_MANAGER (plugin);

	editor = g_value_get_object (value);
	DEBUG_PRINT("add value current editor %p",  editor);
	
	if (!IANJUTA_IS_EDITOR(editor))
	{
		self->current_editor = NULL;
		return;
	}
							 
	self->current_editor = IANJUTA_EDITOR (editor);
	g_object_add_weak_pointer (G_OBJECT (self->current_editor), (gpointer *)(gpointer)&self->current_editor);
		
	/* Restore program counter marker */
	show_program_counter_in_editor (self);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	DebugManagerPlugin *self = ANJUTA_PLUGIN_DEBUG_MANAGER (plugin);

	DEBUG_PRINT("remove value current editor %p", self->current_editor);
	if (self->current_editor)
	{
		hide_program_counter_in_editor (self);

		g_object_remove_weak_pointer (G_OBJECT (self->current_editor), (gpointer *)(gpointer)&self->current_editor);
	}
	self->current_editor = NULL;
}

static void
enable_log_view (DebugManagerPlugin *this, gboolean enable)
{
	if (enable)
	{
		if (this->view == NULL)
		{
			IAnjutaMessageManager* man;

			man = anjuta_shell_get_interface (ANJUTA_PLUGIN (this)->shell, IAnjutaMessageManager, NULL);
			this->view = ianjuta_message_manager_add_view (man, _("Debugger Log"), ICON_FILE, NULL);
			if (this->view != NULL)
			{
				/*g_signal_connect (G_OBJECT (this->view), "buffer_flushed", G_CALLBACK (on_message_buffer_flushed), this);*/
				g_object_add_weak_pointer (G_OBJECT (this->view), (gpointer *)(gpointer)&this->view);
				dma_queue_enable_log (this->queue, this->view);
			}
		}
		else
		{
			ianjuta_message_view_clear (this->view, NULL);
		}
	}
	else
	{
		if (this->view != NULL)
		{
			IAnjutaMessageManager* man;

			man = anjuta_shell_get_interface (ANJUTA_PLUGIN (this)->shell, IAnjutaMessageManager, NULL);
			ianjuta_message_manager_remove_view (man, this->view, NULL);
			this->view = NULL;
		}
		if (this->queue != NULL)
		{
			dma_queue_disable_log (this->queue);
		}
	}
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
                                 AnjutaSession *session, DebugManagerPlugin *plugin)
{
	if (phase == ANJUTA_SESSION_PHASE_FIRST)
		enable_log_view (plugin, FALSE);
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	/* Close debugger when session changed */
	if (plugin->queue)
	{
		dma_queue_abort (plugin->queue);
	}
}


/* State functions
 *---------------------------------------------------------------------------*/

/* Called when the debugger is started but no program is loaded */

static void
dma_plugin_debugger_started (DebugManagerPlugin *this)
{
	AnjutaUI *ui;
	GtkAction *action;
	AnjutaStatus* status;

	DEBUG_PRINT ("DMA: dma_plugin_debugger_started");
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (this)->shell, NULL);
	
	/* Update ui */
	action = gtk_action_group_get_action (this->start_group, "ActionDebuggerStop");
	gtk_action_set_sensitive (action, TRUE);
	gtk_action_group_set_visible (this->loaded_group, TRUE);
	gtk_action_group_set_sensitive (this->loaded_group, FALSE);
	gtk_action_group_set_visible (this->stopped_group, TRUE);
	gtk_action_group_set_sensitive (this->stopped_group, FALSE);
	gtk_action_group_set_visible (this->running_group, TRUE);
	gtk_action_group_set_sensitive (this->running_group, FALSE);
	
	status = anjuta_shell_get_status(ANJUTA_PLUGIN (this)->shell, NULL);
	anjuta_status_set_default (status, _("Debugger"), _("Started"));
}

/* Called when a program is loaded */

static void
dma_plugin_program_loaded (DebugManagerPlugin *this)
{
	AnjutaUI *ui;
	AnjutaStatus* status;

	DEBUG_PRINT ("DMA: dma_plugin_program_loaded");
	
	if (this->sharedlibs == NULL)
	{
		this->sharedlibs = sharedlibs_new (this);
	}
	if (this->signals == NULL)
	{
		this->signals = signals_new (this);
	}

	/* Update ui */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (this)->shell, NULL);
	gtk_action_group_set_sensitive (this->loaded_group, TRUE);
	gtk_action_group_set_sensitive (this->stopped_group, FALSE);
	gtk_action_group_set_sensitive (this->running_group, FALSE);
	
	status = anjuta_shell_get_status(ANJUTA_PLUGIN (this)->shell, NULL);
	anjuta_status_set_default (status, _("Debugger"), _("Loaded"));
}

/* Called when the program is running */

static void
dma_plugin_program_running (DebugManagerPlugin *this)
{
	AnjutaUI *ui;
	AnjutaStatus* status;

	DEBUG_PRINT ("DMA: dma_plugin_program_running");
	
	/* Update ui */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (this)->shell, NULL);
	gtk_action_group_set_sensitive (this->loaded_group, TRUE);
	gtk_action_group_set_sensitive (this->stopped_group, FALSE);
	gtk_action_group_set_sensitive (this->running_group, TRUE);
	
	status = anjuta_shell_get_status(ANJUTA_PLUGIN (this)->shell, NULL);
	anjuta_status_set_default (status, _("Debugger"), _("Running..."));

	set_program_counter(this, NULL, 0, 0);
}

/* Called when the program is stopped */

static void
dma_plugin_program_stopped (DebugManagerPlugin *this)
{
	AnjutaUI *ui;
	AnjutaStatus* status;
	
	DEBUG_PRINT ("DMA: dma_plugin_program_broken");
	
	/* Update ui */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (this)->shell, NULL);
	gtk_action_group_set_sensitive (this->loaded_group, TRUE);
	gtk_action_group_set_sensitive (this->stopped_group, TRUE);
	gtk_action_group_set_sensitive (this->running_group, FALSE);

	status = anjuta_shell_get_status(ANJUTA_PLUGIN (this)->shell, NULL);
	anjuta_status_set_default (status, _("Debugger"), _("Stopped"));
}

/* Called when the program postion change */

static void
dma_plugin_program_moved (DebugManagerPlugin *this, guint pid, guint tid, guint address, const gchar* file, guint line)
{
	DEBUG_PRINT ("DMA: dma_plugin_program_moved %s %d %x", file, line, address);

	set_program_counter (this, file, line, address);
}

/* Called when a program is unloaded */
static void
dma_plugin_program_unload (DebugManagerPlugin *this)
{
	AnjutaUI *ui;
	AnjutaStatus* status;

	DEBUG_PRINT ("DMA: dma_plugin_program_unload");
	
	if (this->sharedlibs != NULL)
	{
		sharedlibs_free (this->sharedlibs);
		this->sharedlibs = NULL;
	}
	if (this->signals == NULL)
	{
		signals_free (this->signals);
		this->signals = NULL;
	}

	/* Update ui */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (this)->shell, NULL);
	gtk_action_group_set_visible (this->start_group, TRUE);
	gtk_action_group_set_sensitive (this->start_group, TRUE);
	gtk_action_group_set_visible (this->loaded_group, TRUE);
	gtk_action_group_set_sensitive (this->loaded_group, FALSE);
	gtk_action_group_set_visible (this->stopped_group, TRUE);
	gtk_action_group_set_sensitive (this->stopped_group, FALSE);
	gtk_action_group_set_visible (this->running_group, TRUE);
	gtk_action_group_set_sensitive (this->running_group, FALSE);
	
	status = anjuta_shell_get_status(ANJUTA_PLUGIN (this)->shell, NULL);
	anjuta_status_set_default (status, _("Debugger"), _("Unloaded"));
}

/* Called when the debugger is stopped */

static void
dma_plugin_debugger_stopped (DebugManagerPlugin *self, GError *err)
{
	AnjutaUI *ui;
	GtkAction *action;
	AnjutaStatus* state;

	DEBUG_PRINT ("DMA: dma_plugin_debugger_stopped");

	dma_plugin_program_unload (self);
	
	/* Update ui */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (self)->shell, NULL);
	gtk_action_group_set_visible (self->start_group, TRUE);
	gtk_action_group_set_sensitive (self->start_group, TRUE);
	action = gtk_action_group_get_action (self->start_group, "ActionDebuggerStop");
	gtk_action_set_sensitive (action, FALSE);
	gtk_action_group_set_visible (self->loaded_group, TRUE);
	gtk_action_group_set_sensitive (self->loaded_group, FALSE);
	gtk_action_group_set_visible (self->stopped_group, TRUE);
	gtk_action_group_set_sensitive (self->stopped_group, FALSE);
	gtk_action_group_set_visible (self->running_group, TRUE);
	gtk_action_group_set_sensitive (self->running_group, FALSE);

	/* clear indicator */
	set_program_counter (self, NULL, 0, 0);
	
	enable_log_view (self, FALSE);
	
	state = anjuta_shell_get_status(ANJUTA_PLUGIN (self)->shell, NULL);
	anjuta_status_set_default (state, _("Debugger"), NULL);
	
	/* Display a warning if debugger stop unexpectedly */
	if (err != NULL)
	{
		GtkWindow *parent = GTK_WINDOW (ANJUTA_PLUGIN(self)->shell);
		anjuta_util_dialog_error (parent, _("Debugger terminated with error %d: %s\n"), err->code, err->message);
	}
		
}

static void
dma_plugin_signal_received (DebugManagerPlugin *self, const gchar *name, const gchar *description)
{
	GtkWindow *parent = GTK_WINDOW (ANJUTA_PLUGIN (self)->shell);
	
	/* Skip SIGINT signal */
	if (strcmp(name, "SIGINT") != 0)
	{
		anjuta_util_dialog_warning (parent, _("Program has received signal: %s\n"), description);
	}
}

/* Called when the user want to go to another location */

static void
dma_plugin_location_changed (DebugManagerPlugin *self, guint address, const gchar* uri, guint line)
{
	/* Go to location in editor */
	if (uri != NULL)
	{
		IAnjutaDocumentManager *docman;

        docman = anjuta_shell_get_interface (ANJUTA_PLUGIN(self)->shell, IAnjutaDocumentManager, NULL);
        if (docman)
        {
			ianjuta_document_manager_goto_file_line (docman, uri, line, NULL);
        }
	}
}

/* Start/Stop menu functions
 *---------------------------------------------------------------------------*/
	
static void
on_start_debug_activate (GtkAction* action, DebugManagerPlugin* this)
{
	enable_log_view (this, TRUE);
	if (dma_run_target (this->start))
	{
		GtkAction *action;
		action = gtk_action_group_get_action (this->start_group, "ActionDebuggerRestartTarget");
		gtk_action_set_sensitive (action, TRUE);
	}

}

static void
on_restart_debug_activate (GtkAction* action, DebugManagerPlugin* this)
{
	enable_log_view (this, TRUE);
	dma_rerun_target (this->start);
}

static void
on_attach_to_project_action_activate (GtkAction* action, DebugManagerPlugin* this)
{
	enable_log_view (this, TRUE);
	dma_attach_to_process (this->start);
}

static void
on_debugger_stop_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (plugin->start)
	{
		dma_quit_debugger (plugin->start);
	}
}

/* Execute call back
 *---------------------------------------------------------------------------*/

static void
on_run_continue_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (plugin->queue)
		dma_queue_run (plugin->queue);
}

static void
on_step_in_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (plugin->queue)
	{
		if ((plugin->disassemble != NULL) && (dma_disassemble_is_focus (plugin->disassemble)))
		{
			dma_queue_stepi_in (plugin->queue);
		}
		else
		{
			dma_queue_step_in (plugin->queue);
		}
	}
}

static void
on_step_over_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (plugin->queue)
	{
		if ((plugin->disassemble != NULL) && (dma_disassemble_is_focus (plugin->disassemble)))
		{
			dma_queue_stepi_over (plugin->queue);
		}
		else
		{
			dma_queue_step_over (plugin->queue);
		}
	}
}

static void
on_step_out_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (plugin->queue)
		dma_queue_step_out (plugin->queue);
}

static void
on_run_to_cursor_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (plugin->queue)
	{
		if ((plugin->disassemble != NULL) && (dma_disassemble_is_focus (plugin->disassemble)))
		{
			guint address;
			
			address = dma_disassemble_get_current_address (plugin->disassemble);
			dma_queue_run_to_address (plugin->queue, address);
		}
		else
		{
			IAnjutaDocumentManager *docman;
			IAnjutaEditor *editor;
			IAnjutaDocument *document;
			gchar *uri;
			gchar *file;
			gint line;

			docman = IANJUTA_DOCUMENT_MANAGER (anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell, "IAnjutaDocumentManager", NULL));
			if (docman == NULL)
				return;
	
			document = ianjuta_document_manager_get_current_document (docman, NULL);
			if (IANJUTA_IS_DOCUMENT(document))
			{	
				editor = IANJUTA_EDITOR(document);
			}
			else
				return;
			uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
			if (uri == NULL)
				return;
	
			file = gnome_vfs_get_local_path_from_uri (uri);
	
			line = ianjuta_editor_get_lineno (editor, NULL);
			dma_queue_run_to (plugin->queue, file, line);
			g_free (uri);
			g_free (file);
		}
	}
}

static void
on_debugger_interrupt_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (plugin->queue)
		dma_queue_interrupt (plugin->queue);
}

/* Custom command
 *---------------------------------------------------------------------------*/

static void
on_debugger_command_entry_activate (GtkEntry *entry, DebugManagerPlugin *plugin)
{
        const gchar *command;

        command = gtk_entry_get_text (GTK_ENTRY (entry));
        if (command && strlen (command))
                dma_queue_send_command (plugin->queue, command);
        gtk_entry_set_text (entry, "");
}

static void
on_debugger_custom_command_activate (GtkAction * action, DebugManagerPlugin *plugin)
{
        GladeXML *gxml;
        GtkWidget *win, *entry;

        gxml = glade_xml_new (GLADE_FILE, "debugger_command_dialog", NULL);
        win = glade_xml_get_widget (gxml, "debugger_command_dialog");
        entry = glade_xml_get_widget (gxml, "debugger_command_entry");

        gtk_window_set_transient_for (GTK_WINDOW (win),
                                                                  GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell));
        g_signal_connect_swapped (win, "response",
                                                          G_CALLBACK (gtk_widget_destroy),
                                                          win);
        g_signal_connect (entry, "activate",
                                          G_CALLBACK (on_debugger_command_entry_activate),
                                          plugin);
        g_object_unref (gxml);
}

/* Info callbacks
 *---------------------------------------------------------------------------*/

static void
on_debugger_dialog_message (const gpointer data, gpointer user_data, GError* error)
{
	const GList *cli_result = data;
	GtkWindow *parent = GTK_WINDOW (user_data);
	if (g_list_length ((GList*)cli_result) < 1)
		return;
	gdb_info_show_list (parent, (GList*)cli_result, 0, 0);
}

static void
on_info_targets_activate (GtkAction *action, DebugManagerPlugin *plugin)
{
	dma_queue_info_target (plugin->queue, on_debugger_dialog_message, plugin);
}

static void
on_info_program_activate (GtkAction *action, DebugManagerPlugin *plugin)
{
	dma_queue_info_program (plugin->queue, on_debugger_dialog_message, plugin);
}

static void
on_info_udot_activate (GtkAction *action, DebugManagerPlugin *plugin)
{
	dma_queue_info_udot (plugin->queue, on_debugger_dialog_message, plugin);
}

static void
on_info_variables_activate (GtkAction *action, DebugManagerPlugin *plugin)
{
	dma_queue_info_variables (plugin->queue, on_debugger_dialog_message, plugin);
}

static void
on_info_frame_activate (GtkAction *action, DebugManagerPlugin *plugin)
{
	dma_queue_info_frame (plugin->queue, 0, on_debugger_dialog_message, plugin);
}

static void
on_info_args_activate (GtkAction *action, DebugManagerPlugin *plugin)
{
	dma_queue_info_args (plugin->queue, on_debugger_dialog_message, plugin);
}

/* Other informations
 *---------------------------------------------------------------------------*/

/*static void
on_info_memory_activate (GtkAction * action, DebugManagerPlugin *plugin)
{
	GtkWidget *win_memory;

	win_memory = memory_info_new (plugin->debugger,
								  GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  NULL);
	gtk_widget_show(win_memory);
}*/

static void
on_debugger_sharedlibs_activate (GtkAction * action, DebugManagerPlugin *plugin)
{
	sharedlibs_show (plugin->sharedlibs);
}

static void
on_debugger_signals_activate (GtkAction * action, DebugManagerPlugin *plugin)
{
	signals_show (plugin->signals);
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_start[] =
{
	{
		"ActionMenuDebug",                        /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Debug"),                             /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		NULL                                      /* action callback */
	},
	{
		"ActionMenuStart",
		"debugger-icon",
		N_("_Start Debugger"),
		NULL,
		NULL,
		NULL
	},
	{
		"ActionDebuggerRunTarget",
		NULL,
		N_("Run Target..."),
		"<shift>F12",
		N_("load and start the target for debugging"),
		G_CALLBACK (on_start_debug_activate)
	},
	{
		"ActionDebuggerRestartTarget",
		NULL,
		N_("Restart Target"),
		NULL,
		N_("restart the same target for debugging"),
		G_CALLBACK (on_restart_debug_activate)
	},
	{
		"ActionDebuggerAttachProcess",
		"debugger-attach",
		N_("_Attach to Process..."),
		NULL,
		N_("Attach to a running program"),
		G_CALLBACK (on_attach_to_project_action_activate)
	},
	{
		"ActionDebuggerStop",
		GTK_STOCK_STOP,
		N_("Stop Debugger"), 
		NULL,
		N_("Say goodbye to the debugger"),
		G_CALLBACK (on_debugger_stop_activate)
	},
};

static GtkActionEntry actions_loaded[] =
{
	{
		"ActionGdbCommand",                              /* Action name */
		NULL,                                            /* Stock icon, if any */
		N_("Debugger Command..."),                       /* Display label */
		NULL,                                            /* short-cut */
		N_("Custom debugger command"),                   /* Tooltip */ 
		G_CALLBACK (on_debugger_custom_command_activate) /* action callback */
	},
	{
		"ActionMenuGdbInformation",
		NULL,
		N_("_Info"),
		NULL,
		NULL,
		NULL
	},
	{
		"ActionGdbInfoTargetFiles",
		NULL,
		N_("Info _Target Files"),
		NULL,
		N_("Display information on the files the debugger is active with"),
		G_CALLBACK (on_info_targets_activate)
	},
	{
		"ActionGdbInfoProgram",
		NULL,
		N_("Info _Program"),
		NULL,
		N_("Display information on the execution status of the program"),
		G_CALLBACK (on_info_program_activate)
	},
	{
		"ActionGdbInfoKernelUserStruct",
		NULL,
		N_("Info _Kernel User Struct"),
		NULL,
		N_("Display the contents of kernel 'struct user' for current child"),
		G_CALLBACK (on_info_udot_activate)
	},
/*	{
		"ActionGdbExamineMemory",
		NULL,
		N_("Examine _Memory"),
		NULL,
		N_("Display accessible memory"),
		G_CALLBACK (on_info_memory_activate)
	},*/
	{
		"ActionGdbViewSharedlibs",
		NULL,
		N_("Shared Libraries"),
		NULL,
		N_("Show shared libraries mappings"),
		G_CALLBACK (on_debugger_sharedlibs_activate)
	},
	{
		"ActionGdbViewSignals",
		NULL,
		N_("Kernel Signals"),
		NULL,
		N_("Show kernel signals"),
		G_CALLBACK (on_debugger_signals_activate)
	}
};

static GtkActionEntry actions_stopped[] =
{
	{
		"ActionDebuggerRunContinue",                   /* Action name */
		GTK_STOCK_EXECUTE,                             /* Stock icon, if any */
		N_("Run/_Continue"),                           /* Display label */
		"F4",                                          /* short-cut */
		N_("Continue the execution of the program"),   /* Tooltip */
		G_CALLBACK (on_run_continue_action_activate)   /* action callback */
	},
	{
		"ActionDebuggerStepIn",
		"debugger-step-into",
		N_("Step _In"),
		"F5",
		N_("Single step into function"),
		G_CALLBACK (on_step_in_action_activate) 
	},
	{
		"ActionDebuggerStepOver", 
		"debugger-step-over",
		N_("Step O_ver"),
		"F6",
		N_("Single step over function"),
		G_CALLBACK (on_step_over_action_activate)
	},
	{
		"ActionDebuggerStepOut",
		"debugger-step-out",
		N_("Step _Out"),                    
		"F7",                              
		N_("Single step out of the function"), 
		G_CALLBACK (on_step_out_action_activate) 
	},
	{
		"ActionDebuggerRunToPosition",    
		"debugger-run-to-cursor",                             
		N_("_Run to Cursor"),           
		"F8",                              
		N_("Run to the cursor"),              
		G_CALLBACK (on_run_to_cursor_action_activate) 
	},
	{
		"ActionGdbCommand",
		NULL,
		N_("Debugger Command..."),
		NULL,
		N_("Custom debugger command"),
		G_CALLBACK (on_debugger_custom_command_activate)
	},
	{
		"ActionMenuGdbInformation",
		NULL,
		N_("_Info"),
		NULL,
		NULL,
		NULL
	},
	{
		"ActionGdbInfoGlobalVariables",
		NULL,
		N_("Info _Global Variables"),
		NULL,
		N_("Display all global and static variables of the program"),
		G_CALLBACK (on_info_variables_activate)
	},
	{
		"ActionGdbInfoCurrentFrame",
		NULL,
		N_("Info _Current Frame"),
		NULL,
		N_("Display information about the current frame of execution"),
		G_CALLBACK (on_info_frame_activate)
	},
	{
		"ActionGdbInfoFunctionArgs",
		NULL,
		N_("Info Function _Arguments"),
		NULL,
		N_("Display function arguments of the current frame"),
		G_CALLBACK (on_info_args_activate)
	},
	{
		"ActionGdbViewSharedlibs",
		NULL,
		N_("Shared Libraries"),
		NULL,
		N_("Show shared libraries mappings"),
		G_CALLBACK (on_debugger_sharedlibs_activate)
	},
	{
		"ActionGdbViewSignals",
		NULL,
		N_("Kernel Signals"),
		NULL,
		N_("Show kernel signals"),
		G_CALLBACK (on_debugger_signals_activate)
	}
};

static GtkActionEntry actions_running[] =
{
    {
		"ActionGdbPauseProgram",                       /* Action name */
		GTK_STOCK_MEDIA_PAUSE,                        /* Stock icon, if any */
		N_("Pa_use Program"),                          /* Display label */
		NULL,                                          /* short-cut */
		N_("Pauses the execution of the program"),     /* Tooltip */
		G_CALLBACK (on_debugger_interrupt_activate)    /* action callback */
	},
};

/* AnjutaPlugin functions
 *---------------------------------------------------------------------------*/

static gboolean
dma_plugin_activate (AnjutaPlugin* plugin)
{
	DebugManagerPlugin *this;
    static gboolean initialized = FALSE;
	AnjutaUI *ui;
	GtkAction *action;
	
	DEBUG_PRINT ("DebugManagerPlugin: Activating Debug Manager plugin...");
	this = ANJUTA_PLUGIN_DEBUG_MANAGER (plugin);
	
    if (!initialized)
    {
		initialized = TRUE;
		register_stock_icons (ANJUTA_PLUGIN (plugin));
	}

	/* Load debugger */
	this->queue = dma_debugger_queue_new (plugin);
	g_signal_connect (this, "debugger-started", G_CALLBACK (dma_plugin_debugger_started), this);
	g_signal_connect (this, "debugger-stopped", G_CALLBACK (dma_plugin_debugger_stopped), this);
	g_signal_connect (this, "program-loaded", G_CALLBACK (dma_plugin_program_loaded), this);
	g_signal_connect (this, "program-running", G_CALLBACK (dma_plugin_program_running), this);
	g_signal_connect (this, "program-stopped", G_CALLBACK (dma_plugin_program_stopped), this);
	g_signal_connect (this, "program-exited", G_CALLBACK (dma_plugin_program_loaded), this);
	g_signal_connect (this, "program-moved", G_CALLBACK (dma_plugin_program_moved), this);
	g_signal_connect (this, "signal-received", G_CALLBACK (dma_plugin_signal_received), this);
	g_signal_connect (this, "location_changed", G_CALLBACK (dma_plugin_location_changed), this);
	
	/* Add all our debug manager actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	this->start_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupDebug",
											_("Debugger operations"),
											actions_start,
											G_N_ELEMENTS (actions_start),
											GETTEXT_PACKAGE, TRUE, this);
	this->loaded_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupDebug",
											_("Debugger operations"),
											actions_loaded,
											G_N_ELEMENTS (actions_loaded),
											GETTEXT_PACKAGE, TRUE, this);
	this->stopped_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupDebug",
											_("Debugger operations"),
											actions_stopped,
											G_N_ELEMENTS (actions_stopped),
											GETTEXT_PACKAGE, TRUE, this);
	this->running_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupDebug",
											_("Debugger operations"),
											actions_running,
											G_N_ELEMENTS (actions_running),
											GETTEXT_PACKAGE, TRUE, this);	
	this->uiid = anjuta_ui_merge (ui, UI_FILE);

	
	/* Watch expression */
	this->watch = expr_watch_new (ANJUTA_PLUGIN (this));
	
	/* Local window */
	this->locals = locals_new (this);

	/* Stack trace */
	this->stack = stack_trace_new (this);

	/* Thread list */
	this->thread = dma_threads_new (this);
	
	/* Create breakpoints list */
	this->breakpoints = breakpoints_dbase_new (this);

	/* Register list */
	this->registers = cpu_registers_new (this);

	/* Memory window */
	this->memory = dma_memory_new (this);

	/* Disassembly window */
	this->disassemble = dma_disassemble_new (this);

	/* Start debugger part */
	this->start = dma_start_new (this);
	

	dma_plugin_debugger_stopped (this, 0);
	action = gtk_action_group_get_action (this->start_group, "ActionDebuggerRestartTarget");
	gtk_action_set_sensitive (action, FALSE);
	
	/* Add watches */
	this->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	this->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
						 
    /* Connect to save session */
	g_signal_connect (G_OBJECT (plugin->shell), "save_session",
                                 G_CALLBACK (on_session_save), plugin);							 

	return TRUE;
}

static gboolean
dma_plugin_deactivate (AnjutaPlugin* plugin)
{
	DebugManagerPlugin *this;
	AnjutaUI *ui;

	DEBUG_PRINT ("DebugManagerPlugin: Deactivating Debug Manager plugin...");

	this = ANJUTA_PLUGIN_DEBUG_MANAGER (plugin);

	/* Stop debugger */
	dma_plugin_debugger_stopped (this, 0);

	g_signal_handlers_disconnect_matched (plugin->shell, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, plugin);	
	g_signal_handlers_disconnect_matched (plugin, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, plugin);	

	anjuta_plugin_remove_watch (plugin, this->project_watch_id, FALSE);
	anjuta_plugin_remove_watch (plugin, this->editor_watch_id, FALSE);
	
	dma_debugger_queue_free (this->queue);
	this->queue = NULL;

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, this->uiid);

    expr_watch_destroy (this->watch);
	this->watch = NULL;
	
	breakpoints_dbase_destroy (this->breakpoints);
	this->breakpoints = NULL;
	
	locals_free (this->locals);
	this->locals = NULL;
	
	stack_trace_free (this->stack);
	this->stack = NULL;

	cpu_registers_free (this->registers);
	this->registers = NULL;
	
	dma_memory_free (this->memory);
	this->memory = NULL;
	
	dma_disassemble_free (this->disassemble);
	this->disassemble = NULL;
	
	dma_threads_free (this->thread);
	this->thread = NULL;
	
	dma_start_free (this->start);
	this->start = NULL;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (this)->shell, NULL);
	anjuta_ui_remove_action_group (ui, this->start_group);
	anjuta_ui_remove_action_group (ui, this->loaded_group);
	anjuta_ui_remove_action_group (ui, this->stopped_group);
	anjuta_ui_remove_action_group (ui, this->running_group);

	if (this->view != NULL)
	{
		g_object_remove_weak_pointer (G_OBJECT (this->view), (gpointer*)(gpointer)&this->view);
        this->view = NULL;
	}
	
	return TRUE;
}

/* Public functions
 *---------------------------------------------------------------------------*/

DmaDebuggerQueue* 
dma_debug_manager_get_queue (DebugManagerPlugin *self)
{
	return self->queue;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static gpointer parent_class;

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
dma_plugin_instance_init (GObject* obj)
{
	DebugManagerPlugin *plugin = ANJUTA_PLUGIN_DEBUG_MANAGER (obj);
	
	plugin->uiid = 0;
	
	plugin->project_root_uri = NULL;
	plugin->queue = NULL;
	plugin->current_editor = NULL;
	plugin->pc_editor = NULL;
	plugin->editor_watch_id = 0;
	plugin->project_watch_id = 0;
	plugin->breakpoints = NULL;
	plugin->watch = NULL;
	plugin->locals = NULL;
	plugin->registers = NULL;
	plugin->signals = NULL;
	plugin->sharedlibs = NULL;
	plugin->view = NULL;
	
	/* plugin->uri = NULL; */
}

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
dma_plugin_dispose (GObject* obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT (obj)));
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
dma_plugin_finalize (GObject* obj)
{
	DebugManagerPlugin *self = ANJUTA_PLUGIN_DEBUG_MANAGER (obj);

	if (self->pc_editor != NULL)
	{
		g_object_remove_weak_pointer (G_OBJECT (self->pc_editor), (gpointer *)(gpointer)&self->pc_editor);
	}
	if (self->current_editor != NULL)
	{
		g_object_remove_weak_pointer (G_OBJECT (self->current_editor), (gpointer *)(gpointer)&self->current_editor);
	}
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT (obj)));
}

/* class_init intialize the class itself not the instance */

static void
dma_plugin_class_init (GObjectClass* klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = dma_plugin_activate;
	plugin_class->deactivate = dma_plugin_deactivate;
	klass->dispose = dma_plugin_dispose;
	klass->finalize = dma_plugin_finalize;
}

/* Implementation of IAnjutaDebugManager interface
 *---------------------------------------------------------------------------*/

static void
idebug_manager_iface_init (IAnjutaDebugManagerIface *iface)
{
}

ANJUTA_PLUGIN_BEGIN (DebugManagerPlugin, dma_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(idebug_manager, IANJUTA_TYPE_DEBUG_MANAGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (DebugManagerPlugin, dma_plugin);
