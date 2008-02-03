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
#include <limits.h>

#include <gconf/gconf-client.h>
#include <libgnome/gnome-i18n.h>

#include "vgcachegrindprefs.h"


static const char *cache_labels[] = {
	"I1 Cache", "D1 Cache", "L2 Cache"
};

static const char *override_keys[] = {
	"/apps/anjuta/valgrind/cachegrind/I1/override",
	"/apps/anjuta/valgrind/cachegrind/D1/override",
	"/apps/anjuta/valgrind/cachegrind/L2/override"
};

static const char *cache_keys[] = {
	"/apps/anjuta/valgrind/cachegrind/I1/settings",
	"/apps/anjuta/valgrind/cachegrind/D1/settings",
	"/apps/anjuta/valgrind/cachegrind/L2/settings"
};

static void vg_cachegrind_prefs_class_init (VgCachegrindPrefsClass *klass);
static void vg_cachegrind_prefs_init (VgCachegrindPrefs *prefs);
static void vg_cachegrind_prefs_destroy (GtkObject *obj);
static void vg_cachegrind_prefs_finalize (GObject *obj);

static void cachegrind_prefs_apply (VgToolPrefs *prefs);
static void cachegrind_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv);


static VgToolPrefsClass *parent_class = NULL;


GType
vg_cachegrind_prefs_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgCachegrindPrefsClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_cachegrind_prefs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgCachegrindPrefs),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_cachegrind_prefs_init,
		};
		
		type = g_type_register_static (VG_TYPE_TOOL_PREFS, "VgCachegrindPrefs", &info, 0);
	}
	
	return type;
}

static void
vg_cachegrind_prefs_class_init (VgCachegrindPrefsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	VgToolPrefsClass *tool_class = VG_TOOL_PREFS_CLASS (klass);
	
	parent_class = g_type_class_ref (VG_TYPE_TOOL_PREFS);
	
	object_class->finalize = vg_cachegrind_prefs_finalize;
	gtk_object_class->destroy = vg_cachegrind_prefs_destroy;
	
	/* virtual methods */
	tool_class->apply = cachegrind_prefs_apply;
	tool_class->get_argv = cachegrind_prefs_get_argv;
}


static void
toggle_button_toggled (GtkToggleButton *toggle, const char *key)
{
	GConfClient *gconf;
	GtkWidget *hbox;
	gboolean bool;
	
	gconf = gconf_client_get_default ();
	
	bool = gtk_toggle_button_get_active (toggle);
	gconf_client_set_bool (gconf, key, bool, NULL);
	
	g_object_unref (gconf);
	
	hbox = g_object_get_data (G_OBJECT (toggle), "hbox");
	gtk_widget_set_sensitive (hbox, bool);
}

static const char *
cache_settings_get (GtkEntry *entry)
{
	register const char *inptr;
	gboolean fixed = FALSE;
	const char *settings;
	char *out, *outptr;
	GtkWidget *parent;
	GtkWidget *dialog;
	int offset, i;
	
	inptr = settings = gtk_entry_get_text (entry);
	outptr = out = g_malloc (strlen (settings) + 1);
	
	if (*inptr == '\0') {
		g_free (out);
		return settings;
	}
	
	for (i = 0; i < 3; i++) {
		while (*inptr == ' ' || *inptr == '\t') {
			fixed = TRUE;
			inptr++;
		}
		
		if (!(*inptr >= '0' && *inptr < '9')) {
			if (i == 0 && *inptr == '\0') {
				gtk_entry_set_text (entry, "");
				g_free (out);
				return "";
			}
			
			goto invalid;
		}
		
		while (*inptr >= '0' && *inptr <= '9')
			*outptr++ = *inptr++;
		
		while (*inptr == ' ' || *inptr == '\t') {
			fixed = TRUE;
			inptr++;
		}
		
		if ((i < 2 && *inptr != ',') || (i == 2 && *inptr != '\0'))
			goto invalid;
		
		*outptr++ = *inptr++;
	}
	
	if (fixed)
		gtk_entry_set_text (entry, out);
	
	g_free (out);
	
	return fixed ? gtk_entry_get_text (entry) : settings;
	
 invalid:
	
	offset = outptr - out;
	while (*inptr)
		*outptr++ = *inptr++;
	*outptr = '\0';
	
	if (fixed)
		gtk_entry_set_text (entry, out);
	
	gtk_editable_select_region (GTK_EDITABLE (entry), offset, offset + 1);
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (entry));
	parent = GTK_WIDGET_TOPLEVEL (parent) ? parent : NULL;
	dialog = gtk_message_dialog_new (GTK_WINDOW (parent), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					 _("Invalid syntax in settings '%s'.\nPlease enter a value "
					   "of the form \"<integer>,<integer>,<integer>\"."), out);
	g_free (out);
	
	g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);
	gtk_widget_show (dialog);
	
	return NULL;
}

static gboolean
entry_focus_out (GtkEntry *entry, GdkEventFocus *event, const char *key)
{
	const char *settings;
	GConfClient *gconf;
	
	gconf = gconf_client_get_default ();
	
	if ((settings = cache_settings_get (entry)))
		gconf_client_set_string (gconf, key, settings, NULL);
	
	g_object_unref (gconf);
	
	return FALSE;
}

static GtkWidget *
cache_settings_new (VgCachegrindPrefs *prefs, const char *name, int index,
		    gboolean override, const char *settings)
{
	GtkWidget *frame, *vbox, *hbox;
	GtkWidget *widget, *label;
	
	frame = gtk_frame_new (name);
	
	vbox = gtk_vbox_new (FALSE, 6);
	
	widget = gtk_check_button_new_with_label (_("Override default settings"));
	prefs->cache[index].override = GTK_TOGGLE_BUTTON (widget);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), override);
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), (void *) override_keys[index]);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget,FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	g_object_set_data (G_OBJECT (prefs->cache[index].override), "hbox", hbox);
	
	/* This is the format of the preference, simply translate the words
	 * inside the <> */
	label = gtk_label_new (_("Enter <size>,<assoc>,<line_size>:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	widget = gtk_entry_new ();
	prefs->cache[index].settings = GTK_ENTRY (widget);
	gtk_entry_set_text (GTK_ENTRY (widget), settings ? settings : "");
	g_signal_connect (widget, "focus-out-event", G_CALLBACK (entry_focus_out), (void *) cache_keys[index]);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
	
	gtk_widget_show (hbox);
	gtk_widget_set_sensitive (hbox, override);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	return frame;
}

static void
vg_cachegrind_prefs_init (VgCachegrindPrefs *prefs)
{
	GConfClient *gconf;
	GtkWidget *widget;
	gboolean override;
	char *settings;
	int i;
	
	gconf = gconf_client_get_default ();
	
	VG_TOOL_PREFS (prefs)->label = _("Cachegrind");
	
	gtk_box_set_spacing (GTK_BOX (prefs), 6);
	
	for (i = 0; i < 3; i++) {
		override = gconf_client_get_bool (gconf, override_keys[i], NULL);
		settings = gconf_client_get_string (gconf, cache_keys[i], NULL);
		
		widget = cache_settings_new (prefs, cache_labels[i], i, override, settings);
		g_free (settings);
		
		gtk_widget_show (widget);
		gtk_box_pack_start (GTK_BOX (prefs), widget, FALSE, FALSE, 0);
	}
	
	g_object_unref (gconf);
}

static void
vg_cachegrind_prefs_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_cachegrind_prefs_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


static void
cachegrind_prefs_apply (VgToolPrefs *prefs)
{
	;
}


static struct {
	const char *arg;
	char *buf;
} cachegrind_args[] = {
	{ "--I1", NULL },
	{ "--D1", NULL },
	{ "--L2", NULL }
};

static void
cachegrind_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv)
{
	GConfClient *gconf;
	char *str;
	int i;
	
	gconf = gconf_client_get_default ();
	
	for (i = 0; i < 3; i++) {
		const char *arg = cachegrind_args[i].arg;
		
		g_free (cachegrind_args[i].buf);
		
		if (gconf_client_get_bool (gconf, override_keys[i], NULL)) {
			if (!(str = gconf_client_get_string (gconf, cache_keys[i], NULL)) || *str == '\0') {
				cachegrind_args[i].buf = NULL;
				g_free (str);
				continue;
			}
			
			cachegrind_args[i].buf = g_strdup_printf ("%s=%s", arg, str);
			g_free (str);
			
			g_ptr_array_add (argv, cachegrind_args[i].buf);
		} else {
			cachegrind_args[i].buf = NULL;
		}
	}
	
	g_object_unref (gconf);
}
