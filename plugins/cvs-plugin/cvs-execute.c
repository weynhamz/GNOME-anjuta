/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    cvs-execute.c
    Copyright (C) 2004 Johannes Schmid

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

#include "cvs-execute.h"

#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/anjuta-launcher.h>

#define CVS_ICON ""

static void
on_cvs_terminated (AnjutaLauncher *launcher,
					 gint child_pid, gint status, gulong time_taken,
					 CVSPlugin *plugin)
{
	g_return_if_fail (plugin != NULL);
	
	plugin->executing_command = FALSE;
	/* We do not care about this view any longer, it will be freed when 
	the users closes it */
	plugin->mesg_view = NULL;
	if (status != 0)
	{
		anjuta_util_dialog_error
			(NULL,_("CVS command failed! - See messages for details"), NULL);
	}
}

static void
on_cvs_message (AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar * mesg, gpointer user_data)
{
	CVSPlugin* plugin = (CVSPlugin*)user_data;
	ianjuta_message_view_append (plugin->mesg_view, IANJUTA_MESSAGE_VIEW_TYPE_INFO,
		mesg, "", NULL);
}

void 
cvs_execute(CVSPlugin* plugin, const gchar* command, const gchar* dir)
{
	IAnjutaMessageManager *mesg_manager;
	
	g_return_if_fail (command != NULL);
	g_return_if_fail (dir != NULL);	
	
	if (plugin->executing_command)
	{
		anjuta_util_dialog_error
			(NULL,_("CVS command is running - please wait until it finishes!"), NULL);
		return;
	}
		
		
	mesg_manager = anjuta_shell_get_interface 
		(ANJUTA_PLUGIN (plugin)->shell,	IAnjutaMessageManager, NULL);
	plugin->mesg_view =
		ianjuta_message_manager_add_view (mesg_manager, _("CVS"), 
		CVS_ICON, NULL);

	plugin->launcher = anjuta_launcher_new ();
	
	g_signal_connect (G_OBJECT (plugin->launcher), "child-exited",
					  G_CALLBACK (on_cvs_terminated), plugin);
	chdir (dir);
	plugin->executing_command = TRUE;
	g_message("Executing: %s", command);	
	anjuta_launcher_execute (plugin->launcher, command,
							 on_cvs_message, plugin);
}
