/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/* plugin.c
 * Copyright (C) 2000 Naba Kumar
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"
#include "anjuta-design-document.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-glade.xml"

static gpointer parent_class;

/* This is variable required from libdevhelp */
gchar *geometry = NULL;

struct _GladePluginPriv
{
	gint uiid;
	GtkActionGroup *action_group;
	GladeApp  *app;

	GtkWidget *inspector;
	GtkWidget *palette;
	GtkWidget *editor;

	GtkWidget *view_box;
	GtkWidget *paned;
	GtkWidget *palette_box;
	gint editor_watch_id;
	gboolean destroying;

	GtkWidget *selector_toggle;
	GtkWidget *resize_toggle;

	/* File count, disable plugin when NULL */
	guint file_count;

	/* for status */
	gboolean add_ticks;
};

enum {
	NAME_COL,
	PROJECT_COL,
	N_COLUMNS
};

static void
on_pointer_mode_changed (GladeProject *project,
                         GParamSpec   *pspec,
                         GladePlugin  *plugin);

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
                            const GValue *value, gpointer data)
{
	GladePlugin* glade_plugin = ANJUTA_PLUGIN_GLADE(plugin);
	GladePluginPriv* priv = glade_plugin->priv;
	GObject *editor;
	editor = g_value_get_object (value);
	if (ANJUTA_IS_DESIGN_DOCUMENT(editor))
	{
		AnjutaDesignDocument* view = ANJUTA_DESIGN_DOCUMENT(editor);
		GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(view));
		if (!view->is_project_added)
		{
			glade_app_add_project (project);
			g_signal_connect (G_OBJECT (project), "notify::pointer-mode",
			                  G_CALLBACK (on_pointer_mode_changed), glade_plugin);
			view->is_project_added = TRUE;
		}
		/* Change view components */
		glade_palette_set_project (GLADE_PALETTE (priv->palette), project);
		glade_inspector_set_project (GLADE_INSPECTOR (priv->inspector), project);
	}
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
                              const char *name, gpointer data)
{

}

static void
signal_editor_signal_activated_cb (GladeSignalEditor* seditor, 
                                   GladeSignal *signal,
                                   GladePlugin *plugin)
{
	IAnjutaEditor* current_editor;
    GladeWidget *gwidget = glade_signal_editor_get_widget (seditor);
    GladeProject *project = glade_widget_get_project (gwidget);
    gchar *path = glade_project_get_path (project);
	
    IAnjutaDocumentManager *docman;
    IAnjutaDocument *doc;

    docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
                                         IAnjutaDocumentManager, NULL);
    if (!docman)
        return;

	doc = ianjuta_document_manager_get_current_document (docman, NULL);
	if(!doc)
		return;

    if (!IANJUTA_IS_EDITOR (doc))
        return;

	current_editor = IANJUTA_EDITOR (doc);

    if(!current_editor)
        return;

    g_signal_emit_by_name (G_OBJECT (current_editor), "glade-callback-add",
                                                      G_OBJECT_TYPE_NAME (glade_widget_get_object (gwidget)),
                                                      glade_signal_get_name (signal),
                                                      glade_signal_get_handler (signal),
                                                      glade_signal_get_userdata (signal),
                                                      glade_signal_get_swapped (signal)?"1":"0",
                                                      glade_signal_get_after (signal)?"1":"0",
                                                      path);
}

static void
on_signal_editor_created (GladeApp* app,
                          GladeSignalEditor* seditor,
                          gpointer data)
{
	glade_signal_editor_enable_dnd (seditor, TRUE);

	g_signal_connect (seditor, "signal-activated",
	                  G_CALLBACK (signal_editor_signal_activated_cb),
	                  data);
}

static void
on_api_help (GladeEditor* editor,
             const gchar* book,
             const gchar* page,
             const gchar* search,
             GladePlugin* plugin)
{

	AnjutaPlugin* aplugin = ANJUTA_PLUGIN(plugin);
	AnjutaShell* shell = aplugin->shell;
	IAnjutaHelp* help;

	help = anjuta_shell_get_interface(shell, IAnjutaHelp, NULL);

	/* No API Help Plugin */
	if (help == NULL)
		return;


	if (search)
		ianjuta_help_search(help, search, NULL);
}

static void
glade_do_close (GladePlugin *plugin, GladeProject *project)
{
	glade_app_remove_project (project);
}

static void
on_document_destroy (GtkWidget* document, GladePlugin *plugin)
{
	GladeProject *project;

	DEBUG_PRINT ("%s", "Destroying Document");

	if (plugin->priv->destroying)
	{
		return;
	}

	project = glade_design_view_get_project(GLADE_DESIGN_VIEW(document));
	if (!project)
	{
		return;
	}

	glade_do_close (plugin, project);

	plugin->priv->file_count--;
	if (plugin->priv->file_count <= 0)
		anjuta_plugin_deactivate (ANJUTA_PLUGIN (plugin));
	else
		on_pointer_mode_changed (project, NULL, plugin);
}

static void
on_document_mapped (GtkWidget* doc, GladePlugin* plugin)
{
	GladeProject* project = glade_design_view_get_project (GLADE_DESIGN_VIEW (doc));
	GladeEditor* editor = GLADE_EDITOR (plugin->priv->editor);
	GList* glade_obj_node;
	GList* list = g_list_copy ((GList*)glade_project_get_objects (project));


	gboolean first = TRUE;

	/* Select the all windows in the project, select the first */
	for (glade_obj_node = list;
	     glade_obj_node != NULL;
	     glade_obj_node = g_list_next (glade_obj_node))
	{
		GObject *glade_obj = G_OBJECT (glade_obj_node->data);
		GladeWidget* glade_widget = glade_widget_get_from_gobject (glade_obj);
		if (glade_widget == glade_widget_get_toplevel (glade_widget))
		{
			glade_project_widget_visibility_changed (project, glade_widget, TRUE);
			glade_editor_load_widget (editor, glade_widget);

			if (first)
			{
				glade_project_selection_set (project, glade_obj, TRUE);
				first = FALSE;
			}
		}
	}
	g_list_free (list);

	/* Only do this on first map */
	g_signal_handlers_disconnect_by_func (doc, G_CALLBACK (on_document_mapped),
	                                      project);
}

static void
on_shell_destroy (AnjutaShell* shell, GladePlugin *glade_plugin)
{
	glade_plugin->priv->destroying = TRUE;
}

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON_FULL ("anjuta-glade-plugin",
	                    "glade-plugin-icon");
	REGISTER_ICON_FULL ("anjuta-glade-widgets",
	                    "glade-plugin-widgets");
	REGISTER_ICON_FULL ("anjuta-glade-palette",
	                    "glade-plugin-palette");
	END_REGISTER_ICON;
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
                 AnjutaSession *session, GladePlugin *plugin)
{
	GList *files, *docwids, *node;
	/*	GtkTreeModel *model;
	 GtkTreeIter iter;
	 */
	IAnjutaDocumentManager *docman;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		files = anjuta_session_get_string_list (session, "File Loader", "Files");
		if (files)
			files = g_list_reverse (files);
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				GFile* file;
				file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				if (file != NULL)
				{
					files = g_list_prepend (files, anjuta_session_get_relative_uri_from_file (session, file, NULL));
					g_object_unref (file);
					/* uri is not freed here */
				}
			}
		}
		g_list_free (docwids);
		if (files)
		{
			files = g_list_reverse (files);
			anjuta_session_set_string_list (session, "File Loader", "Files", files);
			g_list_foreach (files, (GFunc)g_free, NULL);
			g_list_free (files);
		}
	}
}

static void
glade_plugin_selection_changed (GladeProject *project,
                                GladePlugin *plugin)
{
	GladeWidget  *glade_widget = NULL;

	if (glade_project_get_has_selection (project))
	{
		GList *list;
		GList *node;

		list = glade_project_selection_get (project);

		for (node = list; node != NULL; node = g_list_next (node))
		{
			glade_widget = glade_widget_get_from_gobject (G_OBJECT (node->data));
			glade_widget_show (glade_widget);
		}
		glade_editor_load_widget (GLADE_EDITOR (plugin->priv->editor), glade_widget);
	}
	else
		glade_editor_load_widget (GLADE_EDITOR (plugin->priv->editor), NULL);
}

static void
glade_plugin_add_project (GladePlugin *glade_plugin, GladeProject *project)
{
	GtkWidget *view;
	GladePluginPriv *priv;
	IAnjutaDocumentManager* docman =
		anjuta_shell_get_interface(ANJUTA_PLUGIN(glade_plugin)->shell,
		                           IAnjutaDocumentManager, NULL);

	g_return_if_fail (GLADE_IS_PROJECT (project));

	priv = glade_plugin->priv;

	/* Create document */
	view = anjuta_design_document_new(glade_plugin, project);
	g_signal_connect (view, "destroy",
	                  G_CALLBACK (on_document_destroy), glade_plugin);
	g_signal_connect (view, "map", G_CALLBACK (on_document_mapped), glade_plugin);
	gtk_widget_show (view);
	g_object_set_data (G_OBJECT (project), "design_view", view);

	/* Change view components */
	glade_palette_set_project (GLADE_PALETTE (priv->palette), project);

	/* Connect signal */
	g_signal_connect (project, "selection-changed",
	                  G_CALLBACK (glade_plugin_selection_changed),
	                  glade_plugin);

	priv->file_count++;

	ianjuta_document_manager_add_document (docman, IANJUTA_DOCUMENT (view), NULL);
}

static void
add_glade_member (GladeWidget		 *widget,
				  AnjutaPlugin       *plugin)
{
	IAnjutaEditor* current_editor;
	IAnjutaDocumentManager *docman;
	GladeProject *project = glade_widget_get_project (widget);
	gchar *path = glade_project_get_path (project);
	gchar *widget_name = glade_widget_get_name (widget);
	gchar *widget_typename = G_OBJECT_TYPE_NAME (glade_widget_get_object(widget));
	IAnjutaDocument *doc;

	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaDocumentManager, NULL);
	if (!docman)
		return;

	doc = ianjuta_document_manager_get_current_document (docman, NULL);
	if(!doc)
		return;

	if (!IANJUTA_IS_EDITOR (doc))
		return;

	current_editor = IANJUTA_EDITOR (doc);

	if (!current_editor)
		return;

	g_signal_emit_by_name (G_OBJECT (current_editor), "glade-member-add",
													  widget_typename, widget_name, path);
}

static void
inspector_item_activated_cb (GladeInspector     *inspector,
                             AnjutaPlugin       *plugin)
{
	GList *items = glade_inspector_get_selected_items (inspector);
	GList *item;
	g_assert (GLADE_IS_WIDGET (items->data) && (items->next == NULL));

	/* switch to this widget in the workspace */
	for (item = items; item != NULL; item = g_list_next (item))
	{
		glade_widget_hide (GLADE_WIDGET (item->data));
		glade_widget_show (GLADE_WIDGET (item->data));
		add_glade_member (item->data, plugin);
	}

	g_list_free (item);
}

static void
on_selector_button_toggled (GtkToggleToolButton * button, GladePlugin *plugin)
{
	GladeProject *active_project = glade_inspector_get_project(GLADE_INSPECTOR (plugin->priv->inspector));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plugin->priv->selector_toggle)))
	{
		glade_project_set_add_item (active_project, NULL);
		glade_project_set_pointer_mode (active_project, GLADE_POINTER_SELECT);
	}
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->priv->selector_toggle), TRUE);
}

static void
on_drag_resize_button_toggled (GtkToggleToolButton *button,
                               GladePlugin         *plugin)
{
	GladeProject *active_project = glade_inspector_get_project(GLADE_INSPECTOR (plugin->priv->inspector));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plugin->priv->resize_toggle)))
		glade_project_set_pointer_mode (active_project, GLADE_POINTER_DRAG_RESIZE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->priv->resize_toggle), TRUE);

}

static void
on_pointer_mode_changed (GladeProject *project,
                         GParamSpec   *pspec,
                         GladePlugin  *plugin)
{
	GladeProject *active_project = glade_inspector_get_project(GLADE_INSPECTOR (plugin->priv->inspector));

	if (!active_project)
	{
		gtk_widget_set_sensitive (plugin->priv->selector_toggle, FALSE);
		gtk_widget_set_sensitive (plugin->priv->resize_toggle, FALSE);
		return;
	}
	else if (active_project != project)
		return;

	gtk_widget_set_sensitive (plugin->priv->selector_toggle, TRUE);
	gtk_widget_set_sensitive (plugin->priv->resize_toggle, TRUE);

	g_signal_handlers_block_by_func (plugin->priv->selector_toggle,
	                                 on_selector_button_toggled, plugin);
	g_signal_handlers_block_by_func (plugin->priv->resize_toggle,
	                                 on_drag_resize_button_toggled, plugin);

	if (glade_project_get_pointer_mode (project) == GLADE_POINTER_SELECT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->priv->selector_toggle), TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->priv->resize_toggle), FALSE);
	}
	else if (glade_project_get_pointer_mode (project) == GLADE_POINTER_DRAG_RESIZE)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->priv->resize_toggle), TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->priv->selector_toggle), FALSE);
	                              }
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->priv->resize_toggle), FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->priv->selector_toggle), FALSE);
	}

	g_signal_handlers_unblock_by_func (plugin->priv->selector_toggle,
	                                   on_selector_button_toggled, plugin);
	g_signal_handlers_unblock_by_func (plugin->priv->resize_toggle,
	                                   on_drag_resize_button_toggled, plugin);
}

/* Progress callbacks */
static void
glade_plugin_parse_began (GladeProject *project,
                           GladePlugin *plugin)
{
	AnjutaStatus *status = anjuta_shell_get_status (ANJUTA_PLUGIN(plugin)->shell,
	                                                NULL);
	anjuta_status_busy_push (status);
	plugin->priv->add_ticks = TRUE;
}

static void
glade_plugin_parse_finished (GladeProject *project,
                             AnjutaPlugin *plugin)
{
	AnjutaStatus *status = anjuta_shell_get_status (ANJUTA_PLUGIN(plugin)->shell,
	                                                NULL);
	GladePlugin* gplugin = ANJUTA_PLUGIN_GLADE (plugin);
	anjuta_status_busy_pop (status);

	glade_inspector_set_project (GLADE_INSPECTOR (gplugin->priv->inspector), project);
}

static void
glade_plugin_load_progress (GladeProject *project,
                             gint total_ticks,
                             gint current_ticks,
                             AnjutaPlugin *plugin)
{
	GladePlugin *glade_plugin = ANJUTA_PLUGIN_GLADE (plugin);
	AnjutaStatus *status = anjuta_shell_get_status (plugin->shell,
	                                                NULL);
	gchar *text;
	gchar *project_name;
	static GdkPixbuf* icon = NULL;

	if (!icon)
	{
		icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
		                                 "glade-plugin-icon",
		                                 GTK_ICON_SIZE_BUTTON,
		                                 0, NULL);
	}


	if (glade_plugin->priv->add_ticks)
	{
		glade_plugin->priv->add_ticks = FALSE;
		anjuta_status_progress_add_ticks (status, total_ticks);
	}

	project_name = glade_project_get_name (project);
	text = g_strdup_printf ("Loading %s…", project_name);
	anjuta_status_progress_tick (status,
	                             icon,
	                             text);
	g_free (text);
	g_free (project_name);
}

static GtkWidget *
create_selector_tool_button ()
{
  GtkWidget *button;
  GtkWidget *image;
  gchar *image_path;

  image_path =
      g_build_filename (glade_app_get_pixmaps_dir (), "selector.png", NULL);
  image = gtk_image_new_from_file (image_path);
  g_free (image_path);

  button = gtk_toggle_button_new ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  gtk_button_set_image (GTK_BUTTON (button), image);

  gtk_widget_set_tooltip_text (button,
                               _("Select widgets in the workspace"));

  gtk_widget_show (button);
  gtk_widget_show (image);

  return button;
}

static GtkWidget *
create_drag_resize_tool_button ()
{
  GtkWidget *button;
  GtkWidget *image;
  gchar *image_path;

  image_path =
      g_build_filename (glade_app_get_pixmaps_dir (), "drag-resize.png", NULL);
  image = gtk_image_new_from_file (image_path);
  g_free (image_path);

  button = gtk_toggle_button_new ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  gtk_button_set_image (GTK_BUTTON (button), image);

  gtk_widget_set_tooltip_text (button,
                               _("Drag and resize widgets in the workspace"));

  gtk_widget_show (button);
  gtk_widget_show (image);

  return button;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GladePlugin *glade_plugin;
	GladePluginPriv *priv;
	AnjutaStatus* status;
	GtkWidget* button_box;

	DEBUG_PRINT ("%s", "GladePlugin: Activating Glade plugin…");

	glade_plugin = ANJUTA_PLUGIN_GLADE (plugin);

	status = anjuta_shell_get_status (plugin->shell, NULL);
	priv = glade_plugin->priv;

	register_stock_icons (plugin);

	anjuta_status_busy_push (status);
	anjuta_status_set (status, "%s", _("Loading Glade…"));

	priv->app = glade_app_get ();
	if (!priv->app)
	{
		priv->app = glade_app_new ();
	}

	glade_app_set_window (GTK_WIDGET (ANJUTA_PLUGIN(plugin)->shell));

	priv->inspector = glade_inspector_new ();

	g_signal_connect (priv->inspector, "item-activated",
	                  G_CALLBACK (inspector_item_activated_cb),
	                  plugin);

	priv->paned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);

	priv->editor = GTK_WIDGET(glade_editor_new());

	priv->palette = glade_palette_new();
	priv->palette_box = gtk_vbox_new (FALSE, 5);
	priv->selector_toggle = create_selector_tool_button ();
	priv->resize_toggle = create_drag_resize_tool_button ();
	button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (button_box),
	                    priv->selector_toggle,
	                    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_box),
	                    priv->resize_toggle,
	                    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->palette_box),
	                    button_box,
	                    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->palette_box),
	                    priv->palette,
	                    TRUE, TRUE, 0);


	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->selector_toggle),
	                              TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->resize_toggle),
	                              FALSE);

	g_signal_connect (G_OBJECT (priv->selector_toggle), "toggled",
                    G_CALLBACK (on_selector_button_toggled), plugin);
	g_signal_connect (G_OBJECT (priv->resize_toggle), "toggled",
                    G_CALLBACK (on_drag_resize_button_toggled), plugin);


	glade_palette_set_show_selector_button (GLADE_PALETTE (priv->palette),
	                                        FALSE);

	gtk_paned_add1 (GTK_PANED(priv->paned), priv->inspector);
	gtk_paned_add2 (GTK_PANED(priv->paned), priv->editor);

	gtk_widget_show_all (priv->paned);

	anjuta_status_busy_pop (status);

	g_signal_connect(plugin->shell, "destroy",
	                 G_CALLBACK(on_shell_destroy), plugin);

	g_signal_connect(priv->app, "doc-search",
	                 G_CALLBACK(on_api_help), plugin);

	g_signal_connect(priv->app, "signal-editor-created",
	                 G_CALLBACK(on_signal_editor_created), plugin);

	gtk_widget_show (priv->palette);
	gtk_widget_show (priv->editor);
	gtk_widget_show (priv->inspector);

	/* Add widgets */
	anjuta_shell_add_widget (anjuta_plugin_get_shell (ANJUTA_PLUGIN (plugin)),
	                         priv->paned,
	                         "AnjutaGladeTree", _("Widgets"),
	                         "glade-plugin-widgets",
	                         ANJUTA_SHELL_PLACEMENT_RIGHT, NULL);
	anjuta_shell_add_widget (anjuta_plugin_get_shell (ANJUTA_PLUGIN (plugin)),
	                         priv->palette_box,
	                         "AnjutaGladePalette", _("Palette"),
	                         "glade-plugin-palette",
	                         ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	/* Connect to save session */
	g_signal_connect (G_OBJECT (plugin->shell), "save_session",
	                  G_CALLBACK (on_session_save), plugin);

	/* Watch documents */
	glade_plugin->priv->editor_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
		                         value_added_current_editor,
		                         value_removed_current_editor, NULL);

	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	GladePluginPriv *priv;

	priv = ANJUTA_PLUGIN_GLADE (plugin)->priv;

	DEBUG_PRINT ("%s", "GladePlugin: Dectivating Glade plugin…");

	if (glade_app_get_projects ())
	{
		/* Cannot deactivate plugin if there are still files open */
		return FALSE;
	}


	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (plugin->shell,
	                                      G_CALLBACK (on_shell_destroy),
	                                      plugin);

	g_signal_handlers_disconnect_by_func (plugin->shell,
	                                      G_CALLBACK (on_session_save), plugin);

	g_signal_handlers_disconnect_by_func (priv->app,
	                                      G_CALLBACK(on_api_help), plugin);

	/* Remove widgets */
	anjuta_shell_remove_widget (anjuta_plugin_get_shell (plugin),
	                            priv->palette_box,
	                            NULL);
	anjuta_shell_remove_widget (anjuta_plugin_get_shell (plugin),
	                            priv->paned,
	                            NULL);

	priv->uiid = 0;
	priv->action_group = NULL;

	priv->app = NULL;

	return TRUE;
}

static void
glade_plugin_dispose (GObject *obj)
{
	/* GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj); */

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
glade_plugin_finalize (GObject *obj)
{
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj);
	g_free (plugin->priv);
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
glade_plugin_instance_init (GObject *obj)
{
	GladePluginPriv *priv;
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj);

	plugin->priv = (GladePluginPriv *) g_new0 (GladePluginPriv, 1);
	priv = plugin->priv;
	priv->destroying = FALSE;
	priv->file_count = 0;
	priv->add_ticks = FALSE;

	DEBUG_PRINT ("%s", "Intializing Glade plugin");
}

static void
glade_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = glade_plugin_dispose;
	klass->finalize = glade_plugin_finalize;
}

static void
ifile_open (IAnjutaFile *ifile, GFile* file, GError **err)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN (ifile);
	GladePluginPriv *priv;
	GladeProject *project;
	gchar *filename;
	IAnjutaDocumentManager* docman;
	GList* docwids, *node;

	g_return_if_fail (file != NULL);

	priv = ANJUTA_PLUGIN_GLADE (ifile)->priv;

	filename = g_file_get_path (file);
	if (!filename)
	{
		gchar* uri = g_file_get_parse_name(file);
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
		                            _("Not local file: %s"), uri);
		if (priv->file_count <= 0)
			anjuta_plugin_deactivate (ANJUTA_PLUGIN (plugin));

		g_free (uri);
		return;
	}

	docman = anjuta_shell_get_interface(ANJUTA_PLUGIN(ifile)->shell, IAnjutaDocumentManager,
	                                    NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				GFile* cur_file;
				cur_file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				if (cur_file)
				{
					if (g_file_equal (file, cur_file))
					{
						ianjuta_document_manager_set_current_document (docman,
						                                               IANJUTA_DOCUMENT (node->data), NULL);
						g_object_unref (file);
						return;
					}
					g_object_unref (file);
				}
			}
		}
		g_list_free (docwids);
	}

	project = glade_project_new ();
	g_signal_connect (project, "parse-began",
	                  G_CALLBACK (glade_plugin_parse_began), plugin);
	g_signal_connect (project, "parse-finished",
	                  G_CALLBACK (glade_plugin_parse_finished), plugin);
	g_signal_connect (project, "load-progress",
	                  G_CALLBACK (glade_plugin_load_progress), plugin);
	if (!glade_project_load_from_file (project, filename))
	{
		gchar* name = g_file_get_parse_name (file);
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
		                            _("Could not open %s"), name);
		if (priv->file_count <= 0)
			anjuta_plugin_deactivate (ANJUTA_PLUGIN (plugin));
		g_free (name);
		g_free (filename);
		return;
	}
	g_free (filename);

	glade_plugin_add_project (ANJUTA_PLUGIN_GLADE (ifile), project);

	anjuta_shell_present_widget (ANJUTA_PLUGIN (ifile)->shell, priv->paned, NULL);
}

static GFile*
ifile_get_file (IAnjutaFile* ifile, GError** e)
{
	GladePlugin* plugin = (GladePlugin*) ifile;
	const gchar* path =
		glade_project_get_path(glade_inspector_get_project(GLADE_INSPECTOR (plugin->priv->inspector)));
	GFile* file = g_file_new_for_path (path);
	return file;
}

static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

static void
iwizard_activate (IAnjutaWizard *iwizard, GError **err)
{
	GladePluginPriv *priv;
	GladeProject *project;

	priv = ANJUTA_PLUGIN_GLADE (iwizard)->priv;

	project = glade_project_new ();
	if (!project)
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (iwizard)->shell),
		                            _("Could not create a new glade project."));
		return;
	}
	glade_plugin_add_project (ANJUTA_PLUGIN_GLADE (iwizard), project);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (iwizard)->shell,
	                            priv->palette_box, NULL);
}

static void
iwizard_iface_init(IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (GladePlugin, glade_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE (iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GladePlugin, glade_plugin);
