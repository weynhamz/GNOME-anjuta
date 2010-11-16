/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "plugin.h"
#include "anjuta-msgman.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-message-manager.xml"
#define PREFS_BUILDER PACKAGE_DATA_DIR"/glade/anjuta-message-manager-plugin.ui"
#define PREFERENCES_SCHEMA "org.gnome.anjuta.message-manager"

/* Pixmaps */
#define ANJUTA_PIXMAP_MESSAGES                "anjuta-messages-plugin-48.png"
#define ANJUTA_PIXMAP_PREV_MESSAGE            "anjuta-go-message-prev"
#define ANJUTA_PIXMAP_NEXT_MESSAGE            "anjuta-go-message-next"

/* Stock icons */
#define ANJUTA_STOCK_MESSAGES                 "anjuta-messages"
#define ANJUTA_STOCK_PREV_MESSAGE             "anjuta-prev-message"
#define ANJUTA_STOCK_NEXT_MESSAGE             "anjuta-next-message"

static void on_next_message(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	AnjutaMsgman* msgman = ANJUTA_MSGMAN(plugin->msgman);
	MessageView* view = anjuta_msgman_get_current_view(msgman);
	if (view != NULL)
		message_view_next(view);
}

static void on_prev_message(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	AnjutaMsgman* msgman = ANJUTA_MSGMAN(plugin->msgman);
	MessageView* view = anjuta_msgman_get_current_view(msgman);
	if (view != NULL)
		message_view_previous(view);
}

static void on_copy_message(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	AnjutaMsgman* msgman = ANJUTA_MSGMAN(plugin->msgman);
	MessageView* view = anjuta_msgman_get_current_view(msgman);
	if (view != NULL)
		message_view_copy(view);
}

static void on_save_message(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	AnjutaMsgman* msgman = ANJUTA_MSGMAN(plugin->msgman);
	MessageView* view = anjuta_msgman_get_current_view(msgman);
	if (view != NULL)
		message_view_save(view);
}

static GtkActionEntry actions_goto[] = {
  { "ActionMenuGoto", NULL, N_("_Go to"), NULL, NULL, NULL},
  { "ActionMessageCopy", GTK_STOCK_COPY,
    N_("_Copy Message"), NULL,
	N_("Copy message"),
    G_CALLBACK (on_copy_message)},
  { "ActionMessageNext", ANJUTA_STOCK_NEXT_MESSAGE,
    N_("_Next Message"), "<control><alt>n",
	N_("Next message"),
    G_CALLBACK (on_next_message)},
  { "ActionMessagePrev", ANJUTA_STOCK_PREV_MESSAGE,
    N_("_Previous Message"), "<control><alt>p",
	N_("Previous message"),
    G_CALLBACK (on_prev_message)},
  { "ActionMessageSave", NULL,
    N_("_Save Message"), NULL,
	N_("Save message"),
    G_CALLBACK (on_save_message)}
};

static void on_view_changed(AnjutaMsgman* msgman, GtkWidget* page,
                            gint page_num, MessageViewPlugin* plugin)
{
	MessageView* view = anjuta_msgman_get_current_view (msgman);
	AnjutaUI* ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(plugin)->shell, NULL);
	GtkAction* action_next = anjuta_ui_get_action (ui, "ActionGroupGotoMessages",
								   "ActionMessageNext");
	GtkAction* action_prev = anjuta_ui_get_action (ui, "ActionGroupGotoMessages",
								   "ActionMessagePrev");
	GtkAction* action_copy = anjuta_ui_get_action (ui, "ActionGroupGotoMessages",
								   "ActionMessageCopy");
	gboolean sensitive = (anjuta_msgman_get_current_view(msgman) != NULL);
	if (sensitive)
		anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 GTK_WIDGET(msgman), NULL);
	g_object_set (G_OBJECT (action_next), "sensitive", sensitive, NULL);
	g_object_set (G_OBJECT (action_prev), "sensitive", sensitive, NULL);
	g_object_set (G_OBJECT (action_copy), "sensitive", sensitive, NULL);

	/* Toggle buttons */
	gtk_widget_set_sensitive (plugin->normal, view != NULL);	
	gtk_widget_set_sensitive (plugin->info, view != NULL);
	gtk_widget_set_sensitive (plugin->warn, view != NULL);
	gtk_widget_set_sensitive (plugin->error, view != NULL);
	if (view)
	{
		MessageViewFlags flags =
			message_view_get_flags (view);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(plugin->normal), flags & MESSAGE_VIEW_SHOW_NORMAL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(plugin->info), flags & MESSAGE_VIEW_SHOW_INFO);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(plugin->warn), flags & MESSAGE_VIEW_SHOW_WARNING);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(plugin->error), flags & MESSAGE_VIEW_SHOW_ERROR);
	}	
}

static gpointer parent_class;

static void
register_stock_icons (AnjutaPlugin *plugin)
{	
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON (ANJUTA_PIXMAP_MESSAGES, "message-manager-plugin-icon");
	REGISTER_ICON (ANJUTA_PIXMAP_MESSAGES, ANJUTA_STOCK_MESSAGES);
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_NEXT_MESSAGE, ANJUTA_STOCK_NEXT_MESSAGE);
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_PREV_MESSAGE, ANJUTA_STOCK_PREV_MESSAGE);
	END_REGISTER_ICON;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkWidget *popup;
	MessageViewPlugin *mv_plugin;
	static gboolean initialized = FALSE;
	
	DEBUG_PRINT ("%s", "MessageViewPlugin: Activating MessageView plugin ...");
	mv_plugin = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin);
	
	if (!initialized)
	{
		register_stock_icons (plugin);
	}
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	mv_plugin->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupGotoMessages",
											_("Next/Previous Message"),
											actions_goto,
											G_N_ELEMENTS (actions_goto),
											GETTEXT_PACKAGE, TRUE, plugin);
	mv_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupMessageView");
	mv_plugin->msgman = 
		anjuta_msgman_new(popup);
	g_signal_connect(mv_plugin->msgman, "switch-page", 
					 G_CALLBACK(on_view_changed), mv_plugin);
	GtkAction* action_next = anjuta_ui_get_action (ui, "ActionGroupGotoMessages",
								   "ActionMessageNext");
	GtkAction* action_prev = anjuta_ui_get_action (ui, "ActionGroupGotoMessages",
								   "ActionMessagePrev");
	GtkAction* action_copy = anjuta_ui_get_action (ui, "ActionGroupGotoMessages",
								   "ActionMessageCopy");
	g_object_set (G_OBJECT (action_next), "sensitive", FALSE, NULL);
	g_object_set (G_OBJECT (action_prev), "sensitive", FALSE, NULL);
	g_object_set (G_OBJECT (action_copy), "sensitive", FALSE, NULL);
	
	initialized = TRUE;
	mv_plugin->widget_shown = FALSE;
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	MessageViewPlugin *mplugin;
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	DEBUG_PRINT ("%s", "MessageViewPlugin: Dectivating message view plugin ...");
	
	mplugin = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin);

	/* Widget is removed as soon as it is destroyed */
	if (mplugin->widget_shown)
		gtk_widget_destroy (mplugin->msgman);
	anjuta_ui_unmerge (ui, mplugin->uiid);
	anjuta_ui_remove_action_group (ui, mplugin->action_group);
	
	mplugin->action_group = NULL;
	mplugin->msgman = NULL;
	mplugin->uiid = 0;
	
	return TRUE;
}

static void
message_view_plugin_dispose (GObject *obj)
{
	MessageViewPlugin *plugin = ANJUTA_PLUGIN_MESSAGE_VIEW (obj);
	g_object_unref (plugin->settings);
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
message_view_plugin_finalize (GObject *obj)
{
	// MessageViewPlugin *plugin = ANJUTA_PLUGIN_MESSAGE_VIEW (obj);
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
message_view_plugin_instance_init (GObject *obj)
{
	MessageViewPlugin *plugin = ANJUTA_PLUGIN_MESSAGE_VIEW (obj);
	plugin->action_group = NULL;
	plugin->msgman = NULL;
	plugin->uiid = 0;
	plugin->settings = g_settings_new (PREFERENCES_SCHEMA);
}

static void
message_view_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = message_view_plugin_dispose;
	klass->finalize = message_view_plugin_finalize;
}

static gboolean
on_filter_button_tooltip (GtkWidget* widget, 
                          gint        x,
                          gint        y,
                          gboolean    keyboard_mode,
                          GtkTooltip *tooltip,
                          MessageViewPlugin* plugin)
{
	gchar* temp = NULL;
	MessageView* view = anjuta_msgman_get_current_view (ANJUTA_MSGMAN(plugin->msgman));
	if (!view)
		return FALSE;
	if (widget == plugin->normal)
	{
		temp = g_strdup_printf(ngettext ("%d Message", "%d Messages", 
		                                 message_view_get_count (view,
		                                                         MESSAGE_VIEW_SHOW_NORMAL)),
		                       message_view_get_count (view,
		                                               MESSAGE_VIEW_SHOW_NORMAL));
		gtk_tooltip_set_text (tooltip, temp);
	}
	else if (widget == plugin->info)
	{
		temp = g_strdup_printf(ngettext ("%d Info", "%d Infos", 
		                                 message_view_get_count (view,
		                                                         MESSAGE_VIEW_SHOW_INFO)),
		                       message_view_get_count (view,
		                                               MESSAGE_VIEW_SHOW_INFO));
		gtk_tooltip_set_text (tooltip, temp);
	}
	else if (widget == plugin->warn)
	{
		temp = g_strdup_printf(ngettext ("%d Warning", "%d Warnings", 
		                                 message_view_get_count (view,
		                                                         MESSAGE_VIEW_SHOW_WARNING)),
		                       message_view_get_count (view,
		                                               MESSAGE_VIEW_SHOW_WARNING));
		gtk_tooltip_set_text (tooltip, temp);
	}
	else if (widget == plugin->error)
	{
		temp = g_strdup_printf(ngettext ("%d Error", "%d Errors", 
		                                 message_view_get_count (view,
		                                                         MESSAGE_VIEW_SHOW_ERROR)),
		                       message_view_get_count (view,
		                                               MESSAGE_VIEW_SHOW_ERROR));
		gtk_tooltip_set_text (tooltip, temp);
	}
	else
		g_assert_not_reached ();

	return TRUE;
}

static void
on_filter_buttons_toggled (GtkWidget* button, MessageViewPlugin *plugin)
{
	MessageViewFlags flags = 0;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(plugin->normal)))
		flags |= MESSAGE_VIEW_SHOW_NORMAL;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(plugin->info)))
		flags |= MESSAGE_VIEW_SHOW_INFO;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(plugin->warn)))
		flags |= MESSAGE_VIEW_SHOW_WARNING;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(plugin->error)))
		flags |= MESSAGE_VIEW_SHOW_ERROR;
	message_view_set_flags (anjuta_msgman_get_current_view (ANJUTA_MSGMAN(plugin->msgman)),
	                        flags);
}

static GtkWidget*
create_mini_button (MessageViewPlugin* plugin, const gchar* stock_id)
{
	GtkWidget* button, *image;
	gint h,w;
	image = gtk_image_new_from_stock (stock_id, 
	                                  GTK_ICON_SIZE_MENU);
	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
	button = gtk_toggle_button_new ();
	g_signal_connect (G_OBJECT (button), "toggled",
					  G_CALLBACK (on_filter_buttons_toggled), plugin);
	gtk_container_add (GTK_CONTAINER (button), image);

	g_object_set (button, "has-tooltip", TRUE, NULL);
	g_signal_connect (button, "query-tooltip", 
	                  G_CALLBACK (on_filter_button_tooltip), plugin);
	
	return button;
}

static void
create_toggle_buttons (MessageViewPlugin* plugin,
                       GtkWidget* hbox)
{
	GtkWidget* filter_buttons_box = gtk_hbox_new (FALSE, 0);
	
	plugin->normal = create_mini_button (plugin, "message-manager-plugin-icon");
	plugin->info = create_mini_button (plugin, GTK_STOCK_INFO);
	plugin->warn = create_mini_button (plugin, GTK_STOCK_DIALOG_WARNING);
	plugin->error = create_mini_button (plugin, GTK_STOCK_DIALOG_ERROR);
	
	gtk_box_pack_start (GTK_BOX (filter_buttons_box), GTK_WIDGET (plugin->normal),
						FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_buttons_box), GTK_WIDGET (plugin->info),
						FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_buttons_box), GTK_WIDGET (plugin->warn),
						FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_buttons_box), GTK_WIDGET (plugin->error),
	                    FALSE, FALSE, 0);
	
	gtk_widget_show_all (filter_buttons_box);
	gtk_box_pack_start (GTK_BOX(hbox), filter_buttons_box, FALSE, FALSE, 0);
}

/*
 * IAnjutaMessagerManager interface implementation 
 */
static IAnjutaMessageView*
ianjuta_msgman_add_view (IAnjutaMessageManager *plugin,
						 const gchar *file, const gchar *icon,
						 GError **e)
{
	MessageView* message_view;
	AnjutaShell* shell = ANJUTA_PLUGIN(plugin)->shell;
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	if (ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->widget_shown == FALSE)
	{
		GtkWidget* hbox = gtk_hbox_new (FALSE, 0);
		GtkWidget* label = gtk_label_new (_("Messages"));
		GtkWidget* image = gtk_image_new_from_stock ("message-manager-plugin-icon",
		                                            GTK_ICON_SIZE_MENU);
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX(hbox), anjuta_msgman_get_tabber (ANJUTA_MSGMAN(msgman)),
		                    TRUE, TRUE, 5);
	
		gtk_widget_show_all (hbox);

		create_toggle_buttons (ANJUTA_PLUGIN_MESSAGE_VIEW(plugin), hbox);
		
		anjuta_shell_add_widget_custom (shell, msgman,
							 "AnjutaMessageView", _("Messages"),
							 "message-manager-plugin-icon", hbox,
							 ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);
		ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->widget_shown = TRUE;
	}
	anjuta_shell_present_widget(shell, msgman, NULL);
	message_view = anjuta_msgman_add_view (ANJUTA_MSGMAN (msgman), file, icon);
	return IANJUTA_MESSAGE_VIEW (message_view);
}

static void
ianjuta_msgman_remove_view (IAnjutaMessageManager *plugin,
							IAnjutaMessageView * view,
							GError **e)
{
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	anjuta_msgman_remove_view (ANJUTA_MSGMAN (msgman), MESSAGE_VIEW (view));
}

static IAnjutaMessageView*
ianjuta_msgman_get_current_view (IAnjutaMessageManager *plugin,
								 GError **e)
{
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	return IANJUTA_MESSAGE_VIEW (anjuta_msgman_get_current_view
				     (ANJUTA_MSGMAN (msgman)));
}

static IAnjutaMessageView*
ianjuta_msgman_get_view_by_name (IAnjutaMessageManager *plugin,
								 const gchar * name,
								 GError ** e)
{
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	return IANJUTA_MESSAGE_VIEW (anjuta_msgman_get_view_by_name
				     (ANJUTA_MSGMAN (msgman), name));
}

static GList *
ianjuta_msgman_get_all_views (IAnjutaMessageManager *plugin,
							  GError ** e)
{
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	return anjuta_msgman_get_all_views (ANJUTA_MSGMAN (msgman));
}

static void
ianjuta_msgman_set_current_view (IAnjutaMessageManager *plugin,
								 IAnjutaMessageView *message_view,
								 GError ** e)
{
	AnjutaShell* shell;
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	anjuta_msgman_set_current_view (ANJUTA_MSGMAN (msgman),
					       MESSAGE_VIEW (message_view));
	
	/* Ensure the message-view is visible! */
	g_object_get(G_OBJECT(plugin), "shell", &shell, NULL);
}

static void
ianjuta_msgman_set_view_title (IAnjutaMessageManager *plugin,
							   IAnjutaMessageView *message_view,
							   const gchar *title, GError ** e)
{
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	anjuta_msgman_set_view_title (ANJUTA_MSGMAN (msgman),
								  MESSAGE_VIEW (message_view), title);
}

static void
ianjuta_msgman_set_view_icon_from_stock (IAnjutaMessageManager *plugin,
							  IAnjutaMessageView *message_view,
							  const gchar *icon, GError ** e)
{
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	anjuta_msgman_set_view_icon_from_stock (ANJUTA_MSGMAN (msgman),
								  MESSAGE_VIEW (message_view), icon);
}

static void
ianjuta_msgman_set_view_icon (IAnjutaMessageManager *plugin,
							  IAnjutaMessageView *message_view,
							  GdkPixbufAnimation *icon, GError ** e)
{
	GtkWidget *msgman = ANJUTA_PLUGIN_MESSAGE_VIEW (plugin)->msgman;
	anjuta_msgman_set_view_icon (ANJUTA_MSGMAN (msgman),
								 MESSAGE_VIEW (message_view), icon);
}

static void
ianjuta_msgman_iface_init (IAnjutaMessageManagerIface *iface)
{
	iface->add_view = ianjuta_msgman_add_view;
	iface->remove_view = ianjuta_msgman_remove_view;
	iface->get_view_by_name = ianjuta_msgman_get_view_by_name;
	iface->get_current_view = ianjuta_msgman_get_current_view;
	iface->set_current_view = ianjuta_msgman_set_current_view;
	iface->get_all_views = ianjuta_msgman_get_all_views;
	iface->set_view_title = ianjuta_msgman_set_view_title;
	iface->set_view_icon = ianjuta_msgman_set_view_icon;
	iface->set_view_icon_from_stock = ianjuta_msgman_set_view_icon_from_stock;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkBuilder *bxml = gtk_builder_new ();
	GError* error = NULL;
	MessageViewPlugin* plugin = ANJUTA_PLUGIN_MESSAGE_VIEW (ipref);
	/* Create the messages preferences page */

	if (!gtk_builder_add_from_file (bxml, PREFS_BUILDER, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
		return;
	}
	anjuta_preferences_add_from_builder (prefs, bxml, plugin->settings,
									"Messages", _("Messages"),
									 ANJUTA_PIXMAP_MESSAGES);
	
	g_signal_connect (plugin->settings, "changed::messages-tab-position",
	                  G_CALLBACK (on_notify_message_pref), plugin->msgman);		
	g_object_unref (bxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	anjuta_preferences_remove_page(prefs, _("Messages"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (MessageViewPlugin, message_view_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ianjuta_msgman, IANJUTA_TYPE_MESSAGE_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (MessageViewPlugin, message_view_plugin);
