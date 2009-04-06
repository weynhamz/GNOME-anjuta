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

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-message-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-message-manager-plugin.glade"

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
  { "ActionMenuGoto", NULL, N_("_Goto"), NULL, NULL, NULL},
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

static void on_view_changed(AnjutaMsgman* msgman, MessageViewPlugin* plugin)
{
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

#if 0 /* Disable session saving/loading until a way is found to avoid
       * number of message panes infinitely growing */
static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, MessageViewPlugin *plugin)
{
	gboolean success;
	const gchar *dir;
	gchar *messages_file;
	AnjutaSerializer *serializer;
		
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	dir = anjuta_session_get_session_directory (session);
	messages_file = g_build_filename (dir, "messages.txt", NULL);
	serializer = anjuta_serializer_new (messages_file,
										ANJUTA_SERIALIZER_WRITE);
	if (!serializer)
	{
		g_free (messages_file);
		return;
	}
	success = anjuta_msgman_serialize (ANJUTA_MSGMAN (plugin->msgman),
									   serializer);
	g_object_unref (serializer);
	if (!success)
	{
		g_warning ("Serialization failed: deleting %s", messages_file);
		unlink (messages_file);
	}
	g_free (messages_file);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, MessageViewPlugin *plugin)
{
	const gchar *dir;
	gchar *messages_file;
	AnjutaSerializer *serializer;
	
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	dir = anjuta_session_get_session_directory (session);
	messages_file = g_build_filename (dir, "messages.txt", NULL);
	serializer = anjuta_serializer_new (messages_file,
										ANJUTA_SERIALIZER_READ);
	if (!serializer)
	{
		g_free (messages_file);
		return;
	}
	anjuta_msgman_remove_all_views (ANJUTA_MSGMAN (plugin->msgman));
	anjuta_msgman_deserialize (ANJUTA_MSGMAN (plugin->msgman), serializer);
	g_object_unref (serializer);
	g_free (messages_file);
}
#endif

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
	g_signal_connect(G_OBJECT(mv_plugin->msgman), "view_changed", 
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
	
#if 0
	/* Connect to save and load session */
	g_signal_connect (G_OBJECT (plugin->shell), "save-session",
					  G_CALLBACK (on_session_save), plugin);
	g_signal_connect (G_OBJECT (plugin->shell), "load-session",
					  G_CALLBACK (on_session_load), plugin);
#endif
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
#if 0
	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_save),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_load),
										  plugin);
#endif
	/* Widget is destroyed as soon as it is removed */
	if (mplugin->widget_shown)
		anjuta_shell_remove_widget (plugin->shell, mplugin->msgman, NULL);
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
	// MessageViewPlugin *plugin = ANJUTA_PLUGIN_MESSAGE_VIEW (obj);
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
		anjuta_shell_add_widget (shell, msgman,
							 "AnjutaMessageView", _("Messages"),
							 "message-manager-plugin-icon",
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

static guint notify_id;

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
		GladeXML *gxml;
		MessageViewPlugin* plugin = ANJUTA_PLUGIN_MESSAGE_VIEW (ipref);
		/* Create the messages preferences page */
		gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog_messages", NULL);
		anjuta_preferences_add_page (prefs, gxml,
									"Messages", _("Messages"),
									 ANJUTA_PIXMAP_MESSAGES);
		notify_id = anjuta_preferences_notify_add_string (prefs, MESSAGES_TABS_POS, 
		                                                on_notify_message_pref, plugin->msgman, NULL);
		
		g_object_unref (gxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	anjuta_preferences_notify_remove(prefs, notify_id);
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
