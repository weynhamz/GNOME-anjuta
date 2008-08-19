/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  preferences.c
 *
 *  Copyright (C) Massimo Cora' 2006 <maxcvs@gmail.com> 
 * 
 * preferences.c is free software.
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
 
#include "vggeneralprefs.h"
#include "vgmemcheckprefs.h"
#include "vgcachegrindprefs.h"
#include "vghelgrindprefs.h"
#include "preferences.h" 

#include <string.h>
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/anjuta-debug.h>

#define EDITOR_KEY      "/apps/anjuta/valgrind/editor"
#define NUM_LINES_KEY   "/apps/anjuta/valgrind/num-lines"
#define EXE_PATH     	"/apps/anjuta/valgrind/exe-path"

#define VALGRIND_DEFAULT_BIN	"/usr/bin/valgrind"

enum {
	PAGE_GENERAL    = 0,
	PAGE_MEMCHECK   = 1,
	PAGE_ADDRCHECK  = PAGE_MEMCHECK,
	PAGE_CACHEGRIND = 2,
	PAGE_HELGRIND   = 3,
};

struct _ValgrindPluginPrefsPriv {
	GtkWidget * pages[4];	/* must be used only to retrieve gconf parameters.
							 * Do NOT return these as widgets that can be destroyed */
};

 
static void valgrind_plugin_prefs_class_init(ValgrindPluginPrefsClass *klass);
static void valgrind_plugin_prefs_init(ValgrindPluginPrefs *sp);
static void valgrind_plugin_prefs_finalize(GObject *object);

static GObjectClass *parent_class = NULL;

static gboolean 
spin_changed (GtkSpinButton *spin, GdkEvent *event, const char *key) 
{
	GConfClient *gconf;
	gint num;
	
	gconf = gconf_client_get_default ();
	
	num = gtk_spin_button_get_value_as_int (spin);
	gconf_client_set_int (gconf, key, num, NULL);
	
	g_object_unref (gconf);
	
	return FALSE;
}


/*-----------------------------------------------------------------------------
 * Callback for valgrind exe file path selection
 */

static void
on_exe_path_entry_changed (GtkFileChooser *chooser, const char *key)
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


/*-----------------------------------------------------------------------------
 * build and returns a widget containing the general prefs of alleyoop/valgrind
 */

static GtkWidget *
build_general_prefs () 
{
	GConfClient *gconf;
	GtkWidget *vbox, *hbox, *label, *main_label, *gen_page, *widget;
	GtkSpinButton *numlines;
	GError *err = NULL;
	gint num;
	gchar *str_file;

	gconf = gconf_client_get_default ();	

	vbox = gtk_vbox_new (FALSE, 6);

	hbox = gtk_hbox_new (FALSE, 6);

	main_label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (main_label), _("<b>Valgrind general preferences</b>"));	

	gtk_box_pack_start (GTK_BOX (hbox), main_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Valgrind binary file path:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	str_file = gconf_client_get_string (gconf, EXE_PATH, &err);

	if (str_file == NULL || err != NULL || strlen (str_file) <= 0) {
		str_file = g_strdup (VALGRIND_DEFAULT_BIN);
	}
	
	if (!g_path_is_absolute(str_file))
		DEBUG_PRINT("Not absolute");
	
	widget = 
		gtk_file_chooser_button_new (_("Choose Valgrind Binary File Path..."), 
								GTK_FILE_CHOOSER_ACTION_OPEN);
								
	if ( gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (widget), str_file) == FALSE )
		DEBUG_PRINT ("error: could not select file uri with gtk_file_chooser_select_filename ()");
		
	g_free (str_file);

	/* grab every change in file selection */
	g_signal_connect (widget, "selection-changed", G_CALLBACK (on_exe_path_entry_changed), EXE_PATH);

	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	
	label = gtk_label_new (_("Preview"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	num = gconf_client_get_int (gconf, NUM_LINES_KEY, NULL);
	numlines = GTK_SPIN_BUTTON (gtk_spin_button_new_with_range (0, (gdouble) INT_MAX, 1));
	gtk_spin_button_set_digits (numlines, 0);
	gtk_spin_button_set_numeric (numlines, TRUE);
	gtk_spin_button_set_value (numlines, (gdouble) num);

	g_signal_connect (numlines, "focus-out-event", G_CALLBACK (spin_changed), NUM_LINES_KEY);
	gtk_widget_show (GTK_WIDGET (numlines));
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (numlines), FALSE, FALSE, 0);

	label = gtk_label_new (_("lines above and below the target line."));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	/* create a fresh new general prefs widget and add it to the vbox */
	gen_page = g_object_new (VG_TYPE_GENERAL_PREFS, NULL);
	gtk_box_pack_start (GTK_BOX (vbox), gen_page, FALSE, FALSE, 0);
	
	gtk_widget_show_all (vbox);
	return vbox;
}

GPtrArray *
valgrind_plugin_prefs_create_argv (ValgrindPluginPrefs *valprefs, const char *tool)
{
	GPtrArray *argv;
	int page;
	ValgrindPluginPrefsPriv *priv;
	GConfClient *gconf;
	gchar *str_file;
	
	g_return_val_if_fail (valprefs != NULL, NULL);
	
	priv = valprefs->priv;
	
	argv = g_ptr_array_new ();
	
	gconf = gconf_client_get_default ();
	str_file = gconf_client_get_string (gconf, EXE_PATH, NULL);
	
	g_ptr_array_add (argv, str_file);
		
	if (tool != NULL) {
		if (!strcmp (tool, "memcheck")) {
			g_ptr_array_add (argv, "--tool=memcheck");
			page = PAGE_MEMCHECK;
		} else if (!strcmp (tool, "addrcheck")) {
			g_ptr_array_add (argv, "--tool=addrcheck");
			page = PAGE_ADDRCHECK;
		} else if (!strcmp (tool, "cachegrind")) {
			g_ptr_array_add (argv, "--tool=cachegrind");
			page = PAGE_CACHEGRIND;
		} else if (!strcmp (tool, "helgrind")) {
			g_ptr_array_add (argv, "--tool=helgrind");
			page = PAGE_HELGRIND;
		} else {
			g_assert_not_reached ();
		}
	} else {
		/* default tool */
		g_ptr_array_add (argv, "--tool=memcheck");
		page = PAGE_MEMCHECK;
	}

	/* next, apply the general prefs */
	vg_tool_prefs_get_argv (VG_TOOL_PREFS (priv->pages[PAGE_GENERAL]), tool, argv);

	/* finally, apply the current view's prefs */
	vg_tool_prefs_get_argv (VG_TOOL_PREFS (priv->pages[page]), tool, argv);

	return argv;
}

GtkWidget *
valgrind_plugin_prefs_get_anj_prefs (void)
{
	return build_general_prefs ();
}
 
GtkWidget * 
valgrind_plugin_prefs_get_general_widget (void) 
{
	return g_object_new (VG_TYPE_GENERAL_PREFS, NULL);
}

GtkWidget * 
valgrind_plugin_prefs_get_memcheck_widget (void) 
{
	return g_object_new (VG_TYPE_MEMCHECK_PREFS, NULL);
}

GtkWidget * 
valgrind_plugin_prefs_get_cachegrind_widget (void) 
{
	return g_object_new (VG_TYPE_CACHEGRIND_PREFS, NULL);
}

GtkWidget * 
valgrind_plugin_prefs_get_helgrind_widget (void) 
{
	return g_object_new (VG_TYPE_HELGRIND_PREFS, NULL);
}


GType
valgrind_plugin_prefs_get_type(void)
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (ValgrindPluginPrefsClass),
			NULL,
			NULL,
			(GClassInitFunc)valgrind_plugin_prefs_class_init,
			NULL,
			NULL,
			sizeof (ValgrindPluginPrefs),
			0,
			(GInstanceInitFunc)valgrind_plugin_prefs_init,
		};

		type = g_type_register_static(G_TYPE_OBJECT, 
			"ValgrindPluginPrefs", &our_info, 0);
	}

	return type;
}

static void
valgrind_plugin_prefs_class_init(ValgrindPluginPrefsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = valgrind_plugin_prefs_finalize;
}

static void
valgrind_plugin_prefs_init(ValgrindPluginPrefs *obj)
{
	ValgrindPluginPrefsPriv *priv;
	obj->priv = g_new0(ValgrindPluginPrefsPriv, 1);

	priv = obj->priv;
	
	/* build our own widgets. These will be used only by VgActions to retrieve
	 * the configs */
	priv->pages[PAGE_GENERAL] = g_object_new (VG_TYPE_GENERAL_PREFS, NULL);	
	priv->pages[PAGE_MEMCHECK] = g_object_new (VG_TYPE_MEMCHECK_PREFS, NULL);
	priv->pages[PAGE_CACHEGRIND] = g_object_new (VG_TYPE_CACHEGRIND_PREFS, NULL);
	priv->pages[PAGE_HELGRIND] = g_object_new (VG_TYPE_HELGRIND_PREFS, NULL);		
}

static void
valgrind_plugin_prefs_finalize(GObject *object)
{
	ValgrindPluginPrefs *cobj;
	cobj = VALGRIND_PLUGINPREFS(object);
	
	/* FIXME: Free private members, etc. */
	g_free(cobj->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

ValgrindPluginPrefs *
valgrind_plugin_prefs_new(void)
{
	ValgrindPluginPrefs *obj;
	
	obj = VALGRIND_PLUGINPREFS(g_object_new(VALGRIND_TYPE_PLUGINPREFS, NULL));
	
	return obj;
}
