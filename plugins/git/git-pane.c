/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-pane.h"



G_DEFINE_ABSTRACT_TYPE (GitPane, git_pane, ANJUTA_TYPE_DOCK_PANE);

static void
git_pane_init (GitPane *object)
{
	/* TODO: Add initialization code here */
}

static void
git_pane_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (git_pane_parent_class)->finalize (object);
}

static void
git_pane_class_init (GitPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
#if 0
	AnjutaDockPaneClass* parent_class = ANJUTA_DOCK_PANE_CLASS (klass);
#endif

	object_class->finalize = git_pane_finalize;
}

static void
on_message_view_destroyed (Git* plugin, gpointer destroyed_view)
{
	plugin->message_view = NULL;
}

void
git_pane_create_message_view (Git *plugin)
{
	IAnjutaMessageManager *message_manager; 
		
		
	message_manager = anjuta_shell_get_interface  (ANJUTA_PLUGIN (plugin)->shell,	
												   IAnjutaMessageManager, NULL);
	plugin->message_view =  ianjuta_message_manager_get_view_by_name (message_manager, 
																	  _("Git"), 
																	  NULL);
	if (!plugin->message_view)
	{
		plugin->message_view = ianjuta_message_manager_add_view (message_manager, 
																 _("Git"), 
																 ICON_FILE, 
																 NULL);
		g_object_weak_ref (G_OBJECT (plugin->message_view), 
						   (GWeakNotify) on_message_view_destroyed, plugin);
	}
	
	ianjuta_message_view_clear (plugin->message_view, NULL);
	ianjuta_message_manager_set_current_view (message_manager, 
											  plugin->message_view, 
											  NULL);
}

void 
git_pane_on_command_info_arrived (AnjutaCommand *command, Git *plugin)
{
	GQueue *info;
	gchar *message;
	
	info = git_command_get_info_queue (GIT_COMMAND (command));
	
	while (g_queue_peek_head (info))
	{
		message = g_queue_pop_head (info);
		ianjuta_message_view_append (plugin->message_view, 
								     IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
									 message, "", NULL);
		g_free (message);
	}
}