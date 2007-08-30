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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gconf/gconf-client.h>
#include <libgnome/gnome-i18n.h>

#include "vghelgrindprefs.h"


#define PRIVATE_STACKS_KEY    "/apps/anjuta/valgrind/helgrind/private-stacks"
#define SHOW_LAST_ACCESS_KEY  "/apps/anjuta/valgrind/helgrind/show-last-access"

static void vg_helgrind_prefs_class_init (VgHelgrindPrefsClass *klass);
static void vg_helgrind_prefs_init (VgHelgrindPrefs *prefs);
static void vg_helgrind_prefs_destroy (GtkObject *obj);
static void vg_helgrind_prefs_finalize (GObject *obj);

static void helgrind_prefs_apply (VgToolPrefs *prefs);
static void helgrind_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv);


static VgToolPrefsClass *parent_class = NULL;


GType
vg_helgrind_prefs_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgHelgrindPrefsClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_helgrind_prefs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgHelgrindPrefs),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_helgrind_prefs_init,
		};
		
		type = g_type_register_static (VG_TYPE_TOOL_PREFS, "VgHelgrindPrefs", &info, 0);
	}
	
	return type;
}

static void
vg_helgrind_prefs_class_init (VgHelgrindPrefsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	VgToolPrefsClass *tool_class = VG_TOOL_PREFS_CLASS (klass);
	
	parent_class = g_type_class_ref (VG_TYPE_TOOL_PREFS);
	
	object_class->finalize = vg_helgrind_prefs_finalize;
	gtk_object_class->destroy = vg_helgrind_prefs_destroy;
	
	/* virtual methods */
	tool_class->apply = helgrind_prefs_apply;
	tool_class->get_argv = helgrind_prefs_get_argv;
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
	
	str = g_object_get_data (G_OBJECT (item), "value");
	gconf_client_set_string (gconf, key, str, NULL);
	
	g_object_unref (gconf);
}

static char *show_last_access_opts[] = { "no", "some", "all" };

static GtkWidget *
show_last_access_new (GConfClient *gconf)
{
	GtkWidget *omenu, *menu, *item;
	int history = 0;
	char *str;
	int i;
	
	str = gconf_client_get_string (gconf, SHOW_LAST_ACCESS_KEY, NULL);
	
	menu = gtk_menu_new ();
	for (i = 0; i < 3; i++) {
		if (str && !strcmp (show_last_access_opts[i], str))
			history = i;
		
		item = gtk_menu_item_new_with_label (show_last_access_opts[i]);
		g_object_set_data (G_OBJECT (item), "value", show_last_access_opts[i]);
		g_signal_connect (item, "activate", G_CALLBACK (menu_item_activated), SHOW_LAST_ACCESS_KEY);
		gtk_widget_show (item);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	}
	
	gtk_widget_show (menu);
	omenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), history);
	
	g_free (str);
	
	return omenu;
}

static void
vg_helgrind_prefs_init (VgHelgrindPrefs *prefs)
{
	GtkWidget *widget, *hbox;
	GConfClient *gconf;
	gboolean bool;
	
	gconf = gconf_client_get_default ();
	
	VG_TOOL_PREFS (prefs)->label = _("Helgrind");
	
	gtk_box_set_spacing (GTK_BOX (prefs), 6);
	
	bool = gconf_client_get_bool (gconf, PRIVATE_STACKS_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Assume thread stacks are used privately"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), PRIVATE_STACKS_KEY);
	prefs->private_stacks = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (prefs), widget, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	
	widget = gtk_label_new (_("Show location of last word access on error:"));
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	
	widget = show_last_access_new (gconf);
	prefs->show_last_access = GTK_OPTION_MENU (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (prefs), hbox, FALSE, FALSE, 0);
	
	g_object_unref (gconf);
}

static void
vg_helgrind_prefs_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_helgrind_prefs_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


static void
helgrind_prefs_apply (VgToolPrefs *prefs)
{
	;
}

enum {
	ARG_TYPE_BOOL,
	ARG_TYPE_STRING
};

static struct {
	const char *key;
	const char *arg;
	char *buf;
	int type;
	int dval;
} helgrind_args[] = {
	{ PRIVATE_STACKS_KEY,   "--private-stacks",   NULL, ARG_TYPE_BOOL,   0 },
	{ SHOW_LAST_ACCESS_KEY, "--show-last-access", NULL, ARG_TYPE_STRING, 0 },
};

static void
helgrind_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv)
{
	GConfClient *gconf;
	gboolean bool;
	char *str;
	int i;
	
	gconf = gconf_client_get_default ();
	
	for (i = 0; i < G_N_ELEMENTS (helgrind_args); i++) {
		const char *arg = helgrind_args[i].arg;
		const char *key = helgrind_args[i].key;
		
		g_free (helgrind_args[i].buf);
		if (helgrind_args[i].type == ARG_TYPE_BOOL) {
			bool = gconf_client_get_bool (gconf, key, NULL) ? 1 : 0;
			if (bool == helgrind_args[i].dval)
				continue;
			
			helgrind_args[i].buf = g_strdup_printf ("%s=%s", arg, bool ? "yes" : "no");
		} else {
			if (!(str = gconf_client_get_string (gconf, key, NULL)) || *str == '\0') {
				helgrind_args[i].buf = NULL;
				g_free (str);
				continue;
			}
			
			helgrind_args[i].buf = g_strdup_printf ("%s=%s", arg, str);
			g_free (str);
		}
		
		g_ptr_array_add (argv, helgrind_args[i].buf);
	}
	
	g_object_unref (gconf);
}
