/*
 *	Copyright  2008	Ignacio Casal Quinteiro <nacho.resa@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include "starter.h"
#include <gio/gio.h>

#define LOGO_NAME "starter_logo.png"

static GObjectClass *parent_class = NULL;

struct StarterPrivate
{
	AnjutaShell *shell;

	GtkWidget *event_box;

	GdkPixbuf *logo;	

	gchar *filename;

	GtkWidget *new_file;
	GtkWidget *file_box;

	GtkWidget *recent_projects;

	GtkWidget *anjuta_page;
	GtkWidget *anjuta_manual;
	GtkWidget *gnome_library;
};

static GtkWidget *anjuta_starter_button_new (const gchar *label);

static void
destroy_notify (gpointer data)
{
	GFile *file = (GFile *)data;
	
	g_object_unref (file);
}

static void
recent_project_clicked_cb (GtkButton *button, Starter *wcm)
{
	GFile *file;
	IAnjutaFileLoader *loader = anjuta_shell_get_interface (wcm->priv->shell, IAnjutaFileLoader,
															NULL);

	file = g_object_get_data (G_OBJECT (button), "file");

	ianjuta_file_loader_load (IANJUTA_FILE_LOADER (loader),
							  file, FALSE, NULL);
}

static void
build_recent_projects (GtkWidget *box, Starter *wcm)
{
	GtkRecentManager *manager;
	GList *list;
	gint limit = 1000;
	gint i = 0;
	
	manager = gtk_recent_manager_get_default ();

	list = gtk_recent_manager_get_items (manager);

	while (i < limit && list != NULL)
	{
		if (strcmp (gtk_recent_info_get_mime_type (list->data), "application/x-anjuta") == 0)
		{
			GtkWidget *button;
			GFile *file;

			button = anjuta_starter_button_new (gtk_recent_info_get_display_name (list->data));
			gtk_widget_show (button);
			gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

			file = g_file_new_for_uri (gtk_recent_info_get_uri (list->data));
			g_object_set_data_full (G_OBJECT (button), "file", file,
									(GDestroyNotify)destroy_notify);

			g_signal_connect (button, "clicked",
							  G_CALLBACK (recent_project_clicked_cb),
							  wcm);
		}
		i++;
		list = g_list_next (list);
	}

	g_list_foreach (list, (GFunc)gtk_recent_info_unref, NULL);
	g_list_free (list);
}

static void
new_file_clicked_cb (GtkWidget *button, Starter *wcm)
{
	IAnjutaDocumentManager *docman = anjuta_shell_get_interface (wcm->priv->shell, 
																 IAnjutaDocumentManager,
																 NULL);
	if (docman)
		ianjuta_document_manager_add_buffer (docman, NULL, NULL, NULL);
}

static void
on_wizard_clicked (GtkButton *menuitem, Starter *starter)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaPluginDescription *desc;
	
	desc = g_object_get_data (G_OBJECT (menuitem), "__plugin_desc");
	plugin_manager = anjuta_shell_get_plugin_manager (starter->priv->shell,
													  NULL);
	if (desc)
	{
		gchar *id;
		GObject *plugin;
		
		if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &id))
		{
			plugin =
				anjuta_plugin_manager_get_plugin_by_id (plugin_manager, id);
			ianjuta_wizard_activate (IANJUTA_WIZARD (plugin), NULL);
		}
	}
}

static int
sort_wizards (gconstpointer wizard1, gconstpointer wizard2)
{
	gchar* name1, *name2;
	AnjutaPluginDescription* desc1 = (AnjutaPluginDescription*) wizard1;
	AnjutaPluginDescription* desc2 = (AnjutaPluginDescription*) wizard2;
	
	if ((anjuta_plugin_description_get_locale_string (desc1, "Wizard",
													  "Title", &name1) ||
			anjuta_plugin_description_get_locale_string (desc1, "Anjuta Plugin",
														 "Name", &name1)) &&
		(anjuta_plugin_description_get_locale_string (desc2, "Wizard",
													  "Title", &name2) ||
			anjuta_plugin_description_get_locale_string (desc2, "Anjuta Plugin",
														 "Name", &name2)))
	{
		return strcmp(name1, name2);
	}
	else
		return 0;
}

static void
add_wizard_buttons (Starter *starter)
{
	AnjutaPluginManager *plugin_manager;
	GList *node;
	gint count;
	GList *plugin_descs = NULL;
	
	plugin_manager = anjuta_shell_get_plugin_manager (starter->priv->shell,
													  NULL);
	
	plugin_descs = anjuta_plugin_manager_query (plugin_manager,
												"Anjuta Plugin",
												"Interfaces", "IAnjutaWizard",
												NULL);
	plugin_descs = g_list_sort (plugin_descs, sort_wizards);
	node = plugin_descs;
	count = 0;
	while (node)
	{
		AnjutaPluginDescription *desc;
		gchar *str, *name;
		
		desc = node->data;
		
		name = NULL;
		if (anjuta_plugin_description_get_locale_string (desc, "Wizard",
														 "Title", &str) ||
			anjuta_plugin_description_get_locale_string (desc, "Anjuta Plugin",
														 "Name", &str))
		{
			name = g_strdup (N_(str));
			g_free (str);
		}
		if (name)
		{
			GtkWidget *button;
			
			button = anjuta_starter_button_new (name);
			g_free(name);
			gtk_widget_show (button);
			g_object_set_data (G_OBJECT (button), "__plugin_desc", desc);
			g_signal_connect (G_OBJECT (button), "clicked",
							  G_CALLBACK (on_wizard_clicked),
							  starter);
							  
			gtk_box_pack_start (GTK_BOX (starter->priv->file_box), button,
								FALSE, FALSE, 0);
		}
		node = g_list_next (node);
	}
	g_list_free (plugin_descs);
}

static void
anjuta_manual_clicked_cb (GtkWidget *button, gpointer useless)
{
	anjuta_util_help_display (button, "anjuta-manual", "anjuta-manual.xml");
}

static void
anjuta_page_clicked_cb (GtkButton *button, gpointer useless)
{
	anjuta_res_url_show ("http://anjuta.sourceforge.net");
}

static void
gnome_library_clicked_cb (GtkButton *button, gpointer useless)
{
	anjuta_res_url_show ("http://library.gnome.org");
}

static gboolean
on_expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
					Starter *wcm)
{
	cairo_t *cr;
	cairo_pattern_t *pattern;
	
	cr = gdk_cairo_create (widget->window);
	
	pattern = cairo_pattern_create_linear (0, 0, 0, widget->allocation.height);
	
	if (gdk_screen_get_rgba_colormap (gtk_widget_get_screen (widget)) &&
	    gtk_widget_is_composited (widget))
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0); /* transparent */
	else
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* opaque white */
	
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
	
	cairo_pattern_add_color_stop_rgba (pattern, 0.0,
					  				   0.6, 0.7, 0.9, 1.0); /* solid orange */
	cairo_pattern_add_color_stop_rgba (pattern, 0.18,
					   				   1.0, 1.0, 1.0, 1.0); /* transparent orange */
	
	cairo_set_source (cr, pattern);
	cairo_pattern_destroy (pattern);
	
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	
	cairo_paint (cr);
	
	cairo_destroy (cr);

	cr = gdk_cairo_create (widget->window);
	
	gdk_cairo_set_source_pixbuf (cr, wcm->priv->logo, 20, 20);
	
	cairo_paint (cr);
	
	cairo_destroy (cr);	

	GList *l, *list = NULL;
	list = gtk_container_get_children (GTK_CONTAINER (widget));
	
	for (l = list; l != NULL; l = g_list_next (l))
		gtk_container_propagate_expose (GTK_CONTAINER (widget), l->data, event);
	
	g_list_free (list);
	
	return TRUE;
}

static GtkWidget *
anjuta_starter_button_new (const gchar *label)
{
	GtkWidget *button;
	gchar *markup;
	GtkWidget *widget;
	
	markup = g_markup_printf_escaped ("<span underline=\"single\" foreground=\"#5a7ac7\">%s</span>", label);
	
	button = gtk_button_new_with_label (markup);
	widget = gtk_bin_get_child (GTK_BIN (button));
	gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
	
	g_free (markup);
	
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_button_set_alignment (GTK_BUTTON (button), 0.0, 0.5);
	
	return button;
}

static gchar *
get_header_text (const gchar *text)
{
	gchar *markup;
	
	markup = g_markup_printf_escaped ("<span size=\"large\"  weight=\"bold\" foreground=\"#2525a6\">%s</span>", text);
	
	return markup;
}

static void 
starter_instance_init (Starter* wcm)
{
	GtkWidget *main_box;
	GtkWidget *alignment;
	GtkWidget *vbox1, *vbox2, *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GError *error = NULL;
	gchar *logo;
	gchar *aux;
	const guint logo_offset = 20;

	wcm->priv = g_slice_new0 (StarterPrivate);

	wcm->priv->filename = g_strdup (_("Starter"));

	logo = g_build_filename (PACKAGE_PIXMAPS_DIR, LOGO_NAME, NULL);
	
	wcm->priv->logo = gdk_pixbuf_new_from_file (logo, &error);
	
	if (error != NULL)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
		return;
	}

	wcm->priv->event_box = gtk_event_box_new ();
	gtk_widget_show (wcm->priv->event_box);
	g_signal_connect (wcm->priv->event_box, "expose-event",
					  G_CALLBACK (on_expose_event_cb), wcm);

	alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment),
							   logo_offset + gdk_pixbuf_get_height (wcm->priv->logo) + logo_offset,
							   0, logo_offset, 0);

	gtk_widget_show (alignment);
	gtk_container_add (GTK_CONTAINER (wcm->priv->event_box), alignment);
	
	main_box = gtk_hbox_new (FALSE, 35);
	gtk_widget_show (main_box);
	gtk_container_add (GTK_CONTAINER (alignment), main_box);

	/*
	 * vbox1
	 */
	vbox1 = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (main_box), vbox1, FALSE, FALSE, 0);
	
	/*
	 * FILE/PROJECT
	 */
	label = gtk_label_new (NULL);
	aux = get_header_text (_("Create File/Project"));
	gtk_label_set_markup (GTK_LABEL (label), aux);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	g_free (aux);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox1), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new ("    ");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	wcm->priv->file_box = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (wcm->priv->file_box);
	gtk_box_pack_start (GTK_BOX (hbox), wcm->priv->file_box, FALSE, FALSE, 0);
	
	wcm->priv->new_file = anjuta_starter_button_new (_("New File"));
	gtk_widget_show (wcm->priv->new_file);
	gtk_box_pack_start (GTK_BOX (wcm->priv->file_box), wcm->priv->new_file, TRUE, TRUE, 0);
	g_signal_connect (wcm->priv->new_file, "clicked",
					  G_CALLBACK (new_file_clicked_cb), wcm);
	
	/* Separator */
	label = gtk_label_new ("");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox1), label, FALSE, FALSE, 0);
	
	/*
	 * Recent Projects
	 */
	label = gtk_label_new (NULL);
	aux = get_header_text (_("Recent Projects"));
	gtk_label_set_markup (GTK_LABEL (label), aux);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	g_free (aux);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox1), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new ("    ");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	wcm->priv->recent_projects = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (wcm->priv->recent_projects);
	gtk_box_pack_start (GTK_BOX (hbox), wcm->priv->recent_projects, FALSE, FALSE, 0);
	build_recent_projects (wcm->priv->recent_projects, wcm);
	
	/*
	 * vbox2
	 */
	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (main_box), vbox2, FALSE, FALSE, 0);

	/*
	 * Links
	 */
	label = gtk_label_new (NULL);
	aux = get_header_text (_("Links"));
	gtk_label_set_markup (GTK_LABEL (label), aux);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	g_free (aux);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new ("    ");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	wcm->priv->anjuta_page = anjuta_starter_button_new (_("Anjuta Home Page"));
	gtk_widget_show (wcm->priv->anjuta_page);
	gtk_box_pack_start (GTK_BOX (vbox), wcm->priv->anjuta_page, TRUE, TRUE, 0);
	g_signal_connect (wcm->priv->anjuta_page, "clicked",
					  G_CALLBACK (anjuta_page_clicked_cb), NULL);

	wcm->priv->anjuta_manual = anjuta_starter_button_new (_("Anjuta Manual"));
	gtk_widget_show (wcm->priv->anjuta_manual);
	gtk_box_pack_start (GTK_BOX (vbox), wcm->priv->anjuta_manual, TRUE, TRUE, 0);
	g_signal_connect (wcm->priv->anjuta_manual, "clicked",
					  G_CALLBACK (anjuta_manual_clicked_cb), NULL);

	wcm->priv->gnome_library = anjuta_starter_button_new (_("Gnome Online API Documentation"));
	gtk_widget_show (wcm->priv->gnome_library);
	gtk_box_pack_start (GTK_BOX (vbox), wcm->priv->gnome_library, TRUE, TRUE, 0);
	g_signal_connect (wcm->priv->gnome_library, "clicked",
					  G_CALLBACK (gnome_library_clicked_cb), NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (wcm),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
}

static void
starter_dispose (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
starter_finalize (GObject *object)
{
	Starter *cobj;
	cobj = ANJUTA_STARTER (object);
	
	g_free (cobj->priv->filename);

	g_object_unref (cobj->priv->logo);

	g_slice_free (StarterPrivate, cobj->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
	DEBUG_PRINT("=========== finalise =============");
}

static void
starter_class_init (StarterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = starter_dispose;
	object_class->finalize = starter_finalize;
}

/* Plublic funcs */
Starter *
starter_new (AnjutaShell *shell)
{
	Starter *starter = ANJUTA_STARTER (g_object_new (ANJUTA_TYPE_STARTER, NULL));

	starter->priv->shell = shell;
	
	/*
	 * Add buttons after setting the shell
	 */
	add_wizard_buttons (starter);
	
	/*
	 * Add the event box after the scroll is realized
	 */
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (starter),
									       starter->priv->event_box);

	return starter;
}

ANJUTA_TYPE_BEGIN(Starter, starter, GTK_TYPE_SCROLLED_WINDOW);
ANJUTA_TYPE_END;
