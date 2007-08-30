/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *  Copyright (C) Massimo Cora' 2005 <maxcvs@gmail.com>  
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <gconf/gconf-client.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/anjuta-debug.h>

#include "vggeneralprefs.h"


#define DEMANGLE_KEY         "/apps/anjuta/valgrind/general/demangle"
#define NUM_CALLERS_KEY      "/apps/anjuta/valgrind/general/num-callers"
#define ERROR_LIMIT_KEY      "/apps/anjuta/valgrind/general/error-limit"
#define SLOPPY_MALLOC_KEY    "/apps/anjuta/valgrind/general/sloppy-malloc"
#define TRACE_CHILDREN_KEY   "/apps/anjuta/valgrind/general/trace-children"
#define TRACK_FDS_KEY        "/apps/anjuta/valgrind/general/track-fds"
#define TIME_STAMP_KEY       "/apps/anjuta/valgrind/general/time-stamp"
#define RUN_LIBC_FREERES_KEY "/apps/anjuta/valgrind/general/run-libc-freeres"
#define SUPPRESSIONS_KEY     "/apps/anjuta/valgrind/general/suppressions"

#define SUPPRESSIONS_DEFAULT_FILE	".anjuta/valgrind.supp"

static void vg_general_prefs_class_init (VgGeneralPrefsClass *klass);
static void vg_general_prefs_init (VgGeneralPrefs *prefs);
static void vg_general_prefs_destroy (GtkObject *obj);
static void vg_general_prefs_finalize (GObject *obj);

static void general_prefs_apply (VgToolPrefs *prefs);
static void general_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv);


static VgToolPrefsClass *parent_class = NULL;


GType
vg_general_prefs_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgGeneralPrefsClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_general_prefs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgGeneralPrefs),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_general_prefs_init,
		};
		
		type = g_type_register_static (VG_TYPE_TOOL_PREFS, "VgGeneralPrefs", &info, 0);
	}
	
	return type;
}

static void
vg_general_prefs_class_init (VgGeneralPrefsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	VgToolPrefsClass *tool_class = VG_TOOL_PREFS_CLASS (klass);
	
	parent_class = g_type_class_ref (VG_TYPE_TOOL_PREFS);
	
	object_class->finalize = vg_general_prefs_finalize;
	gtk_object_class->destroy = vg_general_prefs_destroy;
	
	/* virtual methods */
	tool_class->apply = general_prefs_apply;
	tool_class->get_argv = general_prefs_get_argv;
}


static void
toggle_button_toggled (GtkToggleButton *toggle, const char *key)
{
	GConfClient *gconf;
	gboolean bool;
	
	gconf = gconf_client_get_default ();
	
	bool = gtk_toggle_button_get_active (toggle);
	gconf_client_set_bool (gconf, key, bool, NULL);
	
	g_object_unref (gconf);
}

static gboolean
spin_focus_out (GtkSpinButton *spin, GdkEventFocus *event, const char *key)
{
	GConfClient *gconf;
	int num;
	
	gconf = gconf_client_get_default ();
	
	num = gtk_spin_button_get_value_as_int (spin);
	gconf_client_set_int (gconf, key, num, NULL);
	
	g_object_unref (gconf);
	
	return FALSE;
}

static void
file_entry_changed (GtkFileChooser *chooser, const char *key)
{
	GConfClient *gconf;
	gchar *str;
	
	gconf = gconf_client_get_default ();
	
	str = gtk_file_chooser_get_filename (chooser);
	
	DEBUG_PRINT ("str is %s key is %s", str, key);
	
	gconf_client_set_string (gconf, key, str ? str : "", NULL);
	g_free (str);

	g_object_unref (gconf);
}

static void
vg_general_prefs_init (VgGeneralPrefs *prefs)
{
	GtkWidget *vbox, *hbox, *label;
	GConfClient *gconf;
	GError *err = NULL;
	GtkWidget *widget;
	gboolean bool;
	gchar *str_file;
	int num;
	
	gconf = gconf_client_get_default ();
	
	VG_TOOL_PREFS (prefs)->label = _("General");
	
	vbox = GTK_WIDGET (prefs);
	gtk_box_set_spacing (GTK_BOX (vbox), 6);
	
	bool = gconf_client_get_bool (gconf, DEMANGLE_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Demangle c++ symbol names"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), DEMANGLE_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->demangle = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Show"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	num = gconf_client_get_int (gconf, NUM_CALLERS_KEY, NULL);
	widget = gtk_spin_button_new_with_range (0, (gdouble) 1024, 1);
	gtk_widget_show (widget);
	prefs->num_callers = GTK_SPIN_BUTTON (widget);
	gtk_spin_button_set_digits (prefs->num_callers, 0);
	gtk_spin_button_set_numeric (prefs->num_callers, TRUE);
	gtk_spin_button_set_value (prefs->num_callers, (gdouble) num);
	g_signal_connect (widget, "focus-out-event", G_CALLBACK (spin_focus_out), NUM_CALLERS_KEY);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	label = gtk_label_new (_("callers in stack trace"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, ERROR_LIMIT_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Stop showing errors if there are too many"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), ERROR_LIMIT_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->error_limit = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, SLOPPY_MALLOC_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Round malloc sizes to next word"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), SLOPPY_MALLOC_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->sloppy_malloc = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, TRACE_CHILDREN_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Trace any child processes forked off by the program being debugged"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), TRACE_CHILDREN_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->trace_children = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, TRACK_FDS_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Track open file descriptors"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), TRACK_FDS_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->track_fds = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, TIME_STAMP_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Add time stamps to log messages"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), TIME_STAMP_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->time_stamp = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, RUN_LIBC_FREERES_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Call __libc_freeres() at exit before checking for memory leaks"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), RUN_LIBC_FREERES_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->run_libc_freeres = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Suppressions File:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	if (!(str_file = gconf_client_get_string (gconf, SUPPRESSIONS_KEY, &err)) || err != NULL) {
		int fd;
		
		str_file = g_build_filename (g_get_home_dir (), SUPPRESSIONS_DEFAULT_FILE, NULL);
		if ((fd = open (str_file, O_WRONLY | O_CREAT, 0666)) == -1) {
			g_free (str_file);
			str_file = NULL;
		} else {
			close (fd);
		}
		
		g_clear_error (&err);
	}
	
	
	widget = 
		gtk_file_chooser_button_new (_("Choose Valgrind Suppressions File..."), 
								GTK_FILE_CHOOSER_ACTION_OPEN);

	if ( gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (widget), str_file) == FALSE )
		DEBUG_PRINT ("error: could not select file uri with gtk_file_chooser_select_filename ()");
	
	/* grab every change in file selection */
	g_signal_connect (widget, "selection-changed", G_CALLBACK (file_entry_changed), SUPPRESSIONS_KEY);

	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
	
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	g_object_unref (gconf);
}

static void
vg_general_prefs_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_general_prefs_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


static void
general_prefs_apply (VgToolPrefs *prefs)
{
	;
}


enum {
	ARG_TYPE_BOOL,
	ARG_TYPE_INT,
	ARG_TYPE_STRING
};

static struct {
	const char *key;
	const char *arg;
	char *buf;
	int type;
	int dval;
} general_args[] = {
	{ DEMANGLE_KEY,         "--demangle",         NULL, ARG_TYPE_BOOL,   1 },
	{ NUM_CALLERS_KEY,      "--num-callers",      NULL, ARG_TYPE_INT,    4 },
	{ ERROR_LIMIT_KEY,      "--error-limit",      NULL, ARG_TYPE_BOOL,   1 },
	{ SLOPPY_MALLOC_KEY,    "--sloppy-malloc",    NULL, ARG_TYPE_BOOL,   0 },
	{ TRACE_CHILDREN_KEY,   "--trace-children",   NULL, ARG_TYPE_BOOL,   0 },
	{ TRACK_FDS_KEY,        "--track-fds",        NULL, ARG_TYPE_BOOL,   0 },
	{ TIME_STAMP_KEY,       "--time-stamp",       NULL, ARG_TYPE_BOOL,   0 },
	{ RUN_LIBC_FREERES_KEY, "--run-libc-freeres", NULL, ARG_TYPE_BOOL,   0 },
	{ SUPPRESSIONS_KEY,     "--suppressions",     NULL, ARG_TYPE_STRING, 0 },
};

static void
general_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv)
{
	GConfClient *gconf;
	int bool, num, i;
	char *str;
	
	gconf = gconf_client_get_default ();
	
	g_ptr_array_add (argv, "--alignment=8");
	
	for (i = 0; i < G_N_ELEMENTS (general_args); i++) {
		const char *arg = general_args[i].arg;
		const char *key = general_args[i].key;
		struct stat st;
		
		g_free (general_args[i].buf);
		if (general_args[i].type == ARG_TYPE_INT) {
			num = gconf_client_get_int (gconf, key, NULL);
			if (num == general_args[i].dval)
				continue;
			
			general_args[i].buf = g_strdup_printf ("%s=%d", arg, num);
		} else if (general_args[i].type == ARG_TYPE_BOOL) {
			bool = gconf_client_get_bool (gconf, key, NULL) ? 1 : 0;
			if (bool == general_args[i].dval)
				continue;
			
			general_args[i].buf = g_strdup_printf ("%s=%s", arg, bool ? "yes" : "no");
		} else {
			if (!(str = gconf_client_get_string (gconf, key, NULL)) || *str == '\0') {
				general_args[i].buf = NULL;
				g_free (str);
				continue;
			}
			
			if (general_args[i].key == SUPPRESSIONS_KEY &&
			    (stat (str, &st) == -1 || !S_ISREG (st.st_mode))) {
				general_args[i].buf = NULL;
				g_free (str);
				continue;
			}
			
			general_args[i].buf = g_strdup_printf ("%s=%s", arg, str);
			g_free (str);
		}
		
		g_ptr_array_add (argv, general_args[i].buf);
	}
	
	g_object_unref (gconf);
}
