/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <limits.h>

#include <gconf/gconf-client.h>
#include <libgnome/gnome-i18n.h>

#include "vgmemcheckprefs.h"


#define LEAK_CHECK_KEY             "/apps/anjuta/valgrind/memcheck/leak-check"
#define SHOW_REACHABLE_KEY         "/apps/anjuta/valgrind/memcheck/show-reachable"
#define LEAK_RESOLUTION_KEY        "/apps/anjuta/valgrind/memcheck/leak-resolution"
#define FREELIST_VOL_KEY           "/apps/anjuta/valgrind/memcheck/freelist-vol"
#define WORKAROUND_GCC296_BUGS_KEY "/apps/anjuta/valgrind/memcheck/workaround-gcc296-bugs"
#define AVOID_STRLEN_ERRORS_KEY    "/apps/anjuta/valgrind/memcheck/avoid-strlen-errors"

static void vg_memcheck_prefs_class_init (VgMemcheckPrefsClass *klass);
static void vg_memcheck_prefs_init (VgMemcheckPrefs *prefs);
static void vg_memcheck_prefs_destroy (GtkObject *obj);
static void vg_memcheck_prefs_finalize (GObject *obj);

static void memcheck_prefs_apply (VgToolPrefs *prefs);
static void memcheck_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv);


static VgToolPrefsClass *parent_class = NULL;


GType
vg_memcheck_prefs_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgMemcheckPrefsClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_memcheck_prefs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgMemcheckPrefs),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_memcheck_prefs_init,
		};
		
		type = g_type_register_static (VG_TYPE_TOOL_PREFS, "VgMemcheckPrefs", &info, 0);
	}
	
	return type;
}

static void
vg_memcheck_prefs_class_init (VgMemcheckPrefsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	VgToolPrefsClass *tool_class = VG_TOOL_PREFS_CLASS (klass);
	
	parent_class = g_type_class_ref (VG_TYPE_TOOL_PREFS);
	
	object_class->finalize = vg_memcheck_prefs_finalize;
	gtk_object_class->destroy = vg_memcheck_prefs_destroy;
	
	/* virtual methods */
	tool_class->apply = memcheck_prefs_apply;
	tool_class->get_argv = memcheck_prefs_get_argv;
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

static void
menu_item_activated (GtkMenuItem *item, const char *key)
{
	GConfClient *gconf;
	const char *str;
	
	gconf = gconf_client_get_default ();
	
	str = g_object_get_data ((GObject *) item, "value");
	gconf_client_set_string (gconf, key, str, NULL);
	
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

static GtkWidget *
option_menu_new (GConfClient *gconf, char *key, char **values, int n, int def)
{
	GtkWidget *omenu, *menu, *item;
	int history = def;
	char *str;
	int i;
	
	str = gconf_client_get_string (gconf, key, NULL);
	
	menu = gtk_menu_new ();
	for (i = 0; i < n; i++) {
		if (str && !strcmp (values[i], str))
			history = i;
		
		item = gtk_menu_item_new_with_label (values[i]);
		g_object_set_data ((GObject *) item, "value", values[i]);
		g_signal_connect (item, "activate", G_CALLBACK (menu_item_activated), key);
		gtk_widget_show (item);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	}
	
	gtk_widget_show (menu);
	omenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu ((GtkOptionMenu *) omenu, menu);
	gtk_option_menu_set_history ((GtkOptionMenu *) omenu, history);
	
	g_free (str);
	
	return omenu;
}

static char *leak_checks[] = { "no", "summary", "full" };
static char *leak_resolutions[] = { "low", "med", "high" };

static void
vg_memcheck_prefs_init (VgMemcheckPrefs *prefs)
{
	GtkWidget *vbox, *hbox, *label, *frame;
	GConfClient *gconf;
	GtkWidget *widget;
	gboolean bool;
	int num;
	
	gconf = gconf_client_get_default ();
	
	((VgToolPrefs *) prefs)->label = _("Memcheck");
	
	vbox = (GtkWidget *) prefs;
	gtk_box_set_spacing ((GtkBox *) vbox, 6);
	
	frame = gtk_frame_new (_("Memory leaks"));
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width ((GtkContainer *) vbox, 6);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Leak check:"));
	gtk_widget_show (label);
	gtk_box_pack_start ((GtkBox *) hbox, label, FALSE, FALSE, 0);
	widget = option_menu_new (gconf, LEAK_CHECK_KEY, leak_checks, G_N_ELEMENTS (leak_checks), 1);
	prefs->leak_check = (GtkOptionMenu *) widget;
	gtk_widget_show (widget);
	gtk_box_pack_start ((GtkBox *) hbox, widget, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, SHOW_REACHABLE_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Show reachable blocks in leak check"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), SHOW_REACHABLE_KEY);
	gtk_toggle_button_set_active ((GtkToggleButton *) widget, bool);
	prefs->show_reachable = (GtkToggleButton *) widget;
	gtk_widget_show (widget);
	gtk_box_pack_start ((GtkBox *) vbox, widget, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Leak resolution:"));
	gtk_widget_show (label);
	gtk_box_pack_start ((GtkBox *) hbox, label, FALSE, FALSE, 0);
	widget = option_menu_new (gconf, LEAK_RESOLUTION_KEY, leak_resolutions, G_N_ELEMENTS (leak_resolutions), 0);
	prefs->leak_resolution = (GtkOptionMenu *) widget;
	gtk_widget_show (widget);
	gtk_box_pack_start ((GtkBox *) hbox, widget, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);
	
	gtk_widget_show (vbox);
	gtk_container_add ((GtkContainer *) frame, vbox);
	vbox = (GtkWidget *) prefs;
	
	gtk_widget_show (frame);
	gtk_box_pack_start ((GtkBox *) vbox, frame, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Keep up to"));
	gtk_widget_show (label);
	gtk_box_pack_start ((GtkBox *) hbox, label, FALSE, FALSE, 0);
	num = gconf_client_get_int (gconf, FREELIST_VOL_KEY, NULL);
	widget = gtk_spin_button_new_with_range (0, (gdouble) INT_MAX, 4);
	gtk_widget_show (widget);
	prefs->freelist_vol = (GtkSpinButton *) widget;
	gtk_spin_button_set_digits (prefs->freelist_vol, 0);
	gtk_spin_button_set_numeric (prefs->freelist_vol, TRUE);
	gtk_spin_button_set_value (prefs->freelist_vol, (gdouble) num);
	g_signal_connect (widget, "focus-out-event", G_CALLBACK (spin_focus_out), FREELIST_VOL_KEY);
	gtk_box_pack_start ((GtkBox *) hbox, widget, FALSE, FALSE, 0);
	label = gtk_label_new (_("bytes in the queue after being free()'d"));
	gtk_widget_show (label);
	gtk_box_pack_start ((GtkBox *) hbox, label, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, WORKAROUND_GCC296_BUGS_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Work around bugs generated by gcc 2.96"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), WORKAROUND_GCC296_BUGS_KEY);
	gtk_toggle_button_set_active ((GtkToggleButton *) widget, bool);
	prefs->workaround_gcc296_bugs = (GtkToggleButton *) widget;
	gtk_widget_show (widget);
	gtk_box_pack_start ((GtkBox *) vbox, widget, FALSE, FALSE, 0);
/*/	
	bool = gconf_client_get_bool (gconf, AVOID_STRLEN_ERRORS_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Ignore errors produced by inline strlen() calls"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), AVOID_STRLEN_ERRORS_KEY);
	gtk_toggle_button_set_active ((GtkToggleButton *) widget, bool);
	prefs->avoid_strlen_errors = (GtkToggleButton *) widget;
	gtk_widget_show (widget);
	gtk_box_pack_start ((GtkBox *) vbox, widget, FALSE, FALSE, 0);
/*/	
	g_object_unref (gconf);
}

static void
vg_memcheck_prefs_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_memcheck_prefs_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


static void
memcheck_prefs_apply (VgToolPrefs *prefs)
{
	;
}


enum {
	ARG_TYPE_BOOL,
	ARG_TYPE_INT,
	ARG_TYPE_STRING
};

enum {
	ADDRCHECK = 1,
	MEMCHECK  = 2,
	BOTH      = 3
};

static struct {
	const char *key;
	const char *arg;
	unsigned int mask;
	char *buf;
	int type;
	int dval;
} memcheck_args[] = {
	{ LEAK_CHECK_KEY,             "--leak-check",             BOTH,     NULL, ARG_TYPE_STRING, 0       },
	{ SHOW_REACHABLE_KEY,         "--show-reachable",         BOTH,     NULL, ARG_TYPE_BOOL,   0       },
	{ LEAK_RESOLUTION_KEY,        "--leak-resolution",        BOTH,     NULL, ARG_TYPE_STRING, 0       },
	{ FREELIST_VOL_KEY,           "--freelist-vol",           BOTH,     NULL, ARG_TYPE_INT,    1000000 },
	{ WORKAROUND_GCC296_BUGS_KEY, "--workaround-gcc296-bugs", BOTH,     NULL, ARG_TYPE_BOOL,   0       }/*,
	{ AVOID_STRLEN_ERRORS_KEY,    "--avoid-strlen-errors",    MEMCHECK, NULL, ARG_TYPE_BOOL,   1       },*/
};

static void
memcheck_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv)
{
	GConfClient *gconf;
	unsigned int mode;
        int bool, num, i;
	char *str;
	
	if (tool != NULL && !strcmp (tool, "addrcheck"))
		mode = ADDRCHECK;
	else
		mode = MEMCHECK;
	
	gconf = gconf_client_get_default ();
	
	for (i = 0; i < G_N_ELEMENTS (memcheck_args); i++) {
		const char *arg = memcheck_args[i].arg;
		const char *key = memcheck_args[i].key;
		
		g_free (memcheck_args[i].buf);
		if (memcheck_args[i].mask & mode) {
			if (memcheck_args[i].type == ARG_TYPE_INT) {
				num = gconf_client_get_int (gconf, key, NULL);
				if (num == memcheck_args[i].dval)
					continue;
				
				memcheck_args[i].buf = g_strdup_printf ("%s=%d", arg, num);
			} else if (memcheck_args[i].type == ARG_TYPE_BOOL) {
				bool = gconf_client_get_bool (gconf, key, NULL) ? 1 : 0;
				if (bool == memcheck_args[i].dval)
					continue;
				
				memcheck_args[i].buf = g_strdup_printf ("%s=%s", arg, bool ? "yes" : "no");
			} else {
				if (!(str = gconf_client_get_string (gconf, key, NULL)) || *str == '\0') {
					memcheck_args[i].buf = NULL;
					g_free (str);
					continue;
				}
				
				memcheck_args[i].buf = g_strdup_printf ("%s=%s", arg, str);
				g_free (str);
			}
			
			g_ptr_array_add (argv, memcheck_args[i].buf);
		} else {
			memcheck_args[i].buf = NULL;
		}
	}
	
	g_object_unref (gconf);
}
