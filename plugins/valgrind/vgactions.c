/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Massimo Cora' 2005 <maxcvs@gmail.com>
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            51 Franklin Street, Fifth Floor,
 *            Boston,  MA  02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <libanjuta/anjuta-debug.h>

#include "vgactions.h"
#include "vgtoolview.h"
#include "vgdefaultview.h"

#define EXE_PATH     	"/apps/anjuta/valgrind/exe-path"
 
static void vg_actions_class_init(VgActionsClass *klass);
static void vg_actions_init(VgActions *sp);
static void vg_actions_finalize(GObject *object);

static GObjectClass *parent_class = NULL;

struct _VgActionsPriv {
	const gchar *program;
	const char **srcdir;
	SymTab *symtab;
	
	GtkWidget *view;
	
	GIOChannel *gio;
	guint watch_id;
	pid_t pid;
	
	AnjutaValgrindPlugin *anjuta_plugin;	/* mainly for valgrind_update_ui () */
	ValgrindPluginPrefs **prefs;
};


GType
vg_actions_get_type (void)
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (VgActionsClass),
			NULL,
			NULL,
			(GClassInitFunc)vg_actions_class_init,
			NULL,
			NULL,
			sizeof (VgActions),
			0,
			(GInstanceInitFunc)vg_actions_init,
		};

		type = g_type_register_static(G_TYPE_OBJECT, 
			"VgActions", &our_info, 0);
	}

	return type;
}

static void
vg_actions_class_init(VgActionsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = vg_actions_finalize;
}

static void
vg_actions_init(VgActions *obj)
{
	VgActionsPriv *priv;
	obj->priv = g_new0(VgActionsPriv, 1);
	
	priv = obj->priv;
	
	priv->srcdir = NULL;
	priv->symtab = NULL;
	
	priv->view = NULL;
	
	priv->gio = NULL;
	priv->watch_id = 0;
	priv->pid = (pid_t) -1;
	
	priv->prefs = NULL;
}

static void
vg_actions_finalize(GObject *object)
{
	VgActions *cobj;
	cobj = VG_ACTIONS(object);

	g_object_unref (G_OBJECT (cobj->priv->anjuta_plugin));
	g_object_unref (G_OBJECT (cobj->priv->view));
	
	/* Free private members, etc. */
	/* wouldn't be necessary.. anyway */
	cobj->priv->anjuta_plugin = NULL;
	cobj->priv->prefs = NULL;
	
	g_free(cobj->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

VgActions *
vg_actions_new (AnjutaValgrindPlugin *anjuta_plugin, 
		ValgrindPluginPrefs **prefs, GtkWidget *vg_default_view)
{
	VgActions *obj;
    
	g_return_val_if_fail(prefs != NULL, NULL);
	
	obj = VG_ACTIONS(g_object_new(VG_TYPE_ACTIONS, NULL));
	
	/* set the anjuta plugin object */
	obj->priv->anjuta_plugin = anjuta_plugin;	
	
	/* set the prefs object */
	obj->priv->prefs = prefs;
		
	/* and the view object */
	obj->priv->view = GTK_WIDGET (vg_default_view);
	
	g_object_ref (G_OBJECT (obj->priv->anjuta_plugin));
	g_object_ref (G_OBJECT (obj->priv->view));
	
	return obj;
}

static gboolean
io_ready_cb (GIOChannel *gio, GIOCondition condition, gpointer user_data)
{
	VgActions *actions = user_data;
	VgActionsPriv *priv;

	priv = actions->priv;

	if ((condition & G_IO_IN) && vg_tool_view_step (VG_TOOL_VIEW (priv->view)) <= 0) {
		DEBUG_PRINT ("%s", "child program exited or error in GIOChannel [IO_IN], killing");
		anjuta_util_dialog_info (NULL, _("Reached the end of the input file or error "
								"in parsing valgrind output."));
		vg_actions_kill (actions);
		priv->watch_id = 0;
		return FALSE;
	}

	if (condition & G_IO_HUP) {
		DEBUG_PRINT ("%s", "child program exited or error in GIOChannel [IO_HUP], killing");
		anjuta_util_dialog_info (NULL, _("Process exited."));
		vg_actions_kill (actions);
		priv->watch_id = 0;
		return FALSE;
	}
	
	return TRUE;
}

static gboolean
check_valgrind_binary() 
{
	GConfClient *gconf;
	gchar *str_valgrind_file;
	GError *err = NULL;
	
	gconf = gconf_client_get_default ();	
	if (!(str_valgrind_file = 
			gconf_client_get_string (gconf, EXE_PATH, &err)) || err != NULL) {
		anjuta_util_dialog_error (NULL, 
				_("Could not get the right valgrind-binary gconf key:"));
		g_free (str_valgrind_file);
		return FALSE;
	}
	
	if ( g_file_test (str_valgrind_file, 
				G_FILE_TEST_EXISTS | G_FILE_TEST_IS_SYMLINK) == FALSE ) {;
		anjuta_util_dialog_error (NULL, 
			_("Valgrind binary [%s] does not exist. Please check "
			"the preferences or install Valgrind package."), 
			str_valgrind_file);
			
		g_free (str_valgrind_file);
		return FALSE;
	}
	
	g_free (str_valgrind_file);
	return TRUE;
}

void
vg_actions_run (VgActions *actions, gchar* prg_to_debug, gchar* tool, GError **err)
{
	char logfd_arg[30];
	GPtrArray *args;
	int logfd[2];
	char **argv;
	int i;
	VgActionsPriv *priv;
	
	g_return_if_fail (actions != NULL);
	
	priv = actions->priv;

	g_return_if_fail (priv->prefs != NULL);

	/* check the valgrind binary availability */
	if (!check_valgrind_binary ())
        return;
	
	priv->program = g_strdup (prg_to_debug);

	if (priv->pid != (pid_t) -1) {
		anjuta_util_dialog_error (NULL, 
				_("Could not get the right pipe for the process."));
		
		return;
	}
	
	if (pipe (logfd) == -1) {
		anjuta_util_dialog_error (NULL, 
				_("Could not get the right pipe for the process."));
		return;
	}

	args = valgrind_plugin_prefs_create_argv (*priv->prefs, tool);

	sprintf (logfd_arg, "--log-fd=%d", logfd[1]);
	g_ptr_array_add (args, logfd_arg);

	for ( i=0; i < args->len; i++ ) {
		DEBUG_PRINT ("arg %d is %s", i, (char*)g_ptr_array_index (args, i));
	}

	g_ptr_array_add (args, (gpointer)priv->program);
	
	DEBUG_PRINT("program noticed is %s", priv->program);
	g_ptr_array_add (args, NULL);
	
	argv = (char **) args->pdata;
	
	priv->pid = process_fork (argv[0], argv, TRUE, logfd[1], NULL, NULL, NULL, err);

	if (priv->pid == (pid_t) -1) {
		close (logfd[0]);
		close (logfd[1]);
		return;
	}
	
	g_ptr_array_free (args, TRUE);
	close (logfd[1]);
	
	vg_tool_view_clear(VG_TOOL_VIEW (priv->view));
	vg_tool_view_connect (VG_TOOL_VIEW (priv->view), logfd[0]);

	priv->gio = g_io_channel_unix_new (logfd[0]);
	priv->watch_id = g_io_add_watch (priv->gio, G_IO_IN | G_IO_HUP, 
										io_ready_cb, actions);

	/* let's update our menu status */
	valgrind_set_busy_status (priv->anjuta_plugin, TRUE);
	valgrind_update_ui (priv->anjuta_plugin);	
}

void
vg_actions_kill (VgActions *actions)
{
	VgActionsPriv *priv;
	
	g_return_if_fail (actions != NULL);
	priv = actions->priv;
	
	vg_tool_view_disconnect (VG_TOOL_VIEW (priv->view));
	
	if (priv->gio) {
		g_io_channel_close (priv->gio);
		g_io_channel_unref (priv->gio);
		priv->watch_id = 0;
		priv->gio = NULL;
	}
	
	if (priv->pid != (pid_t) -1) {
		process_kill (priv->pid);
		priv->pid = (pid_t) -1;
	}

	/* let's set the correct sensitive menu */
	valgrind_set_busy_status (priv->anjuta_plugin, FALSE);
	valgrind_update_ui (priv->anjuta_plugin);	
}

void vg_actions_set_pid (VgActions *actions, pid_t pid) 
{
	VgActionsPriv *priv;
	
	g_return_if_fail (actions != NULL);
	priv = actions->priv;

	priv->pid = (pid_t) pid;

}

void vg_actions_set_giochan (VgActions *actions, GIOChannel*gio) 
{
	VgActionsPriv *priv;
	
	g_return_if_fail (actions != NULL);
	priv = actions->priv;

	priv->gio = gio;	
	
	priv->watch_id = g_io_add_watch (priv->gio, G_IO_IN | G_IO_HUP, 
								io_ready_cb, actions);	
}
