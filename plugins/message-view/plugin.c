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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

#include "plugin.h"
#include "anjuta-msgman.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-message-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-message-manager-plugin.glade"
#define ICON_FILE "preferences-messages.png"

/* Pixmaps */
#define ANJUTA_PIXMAP_MESSAGES                "messages.xpm"
#define ANJUTA_PIXMAP_PREV_MESSAGE            "error-prev.png"
#define ANJUTA_PIXMAP_NEXT_MESSAGE            "error-next.png"
#define ANJUTA_PIXMAP_PREV_MESSAGE_16         "error-prev-16.png"
#define ANJUTA_PIXMAP_NEXT_MESSAGE_16            "error-next-16.png"

/* Stock icons */
#define ANJUTA_STOCK_MESSAGES                 "anjuta-messages"
#define ANJUTA_STOCK_PREV_MESSAGE             "anjuta-prev-message"
#define ANJUTA_STOCK_NEXT_MESSAGE             "anjuta-next-message"

static void on_next_message(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	g_message("Next Message");
}

static void on_prev_message(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	g_message("Prev Message");
}


static GtkActionEntry actions_goto[] = {
  /* { "ActionMenuGoto", N_("_Goto"), NULL, NULL, NULL, NULL}, */
  { "ActionMessageNext", ANJUTA_STOCK_NEXT_MESSAGE,
    N_("_Next message"), NULL,
	N_("Next message"),
    G_CALLBACK (on_next_message)},
  { "ActionMessagePrev", ANJUTA_STOCK_PREV_MESSAGE,
    N_("_Previous message"), NULL,
	N_("Previous message"),
    G_CALLBACK (on_prev_message)}
};

gpointer parent_class;

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

#define ADD_ICON(icon) \
	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"icon, NULL); \
	gtk_icon_source_set_pixbuf (source, pixbuf); \
	gtk_icon_set_add_source (icon_set, source); \
	g_object_unref (pixbuf);

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	GtkIconSource *source;
	
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	
	source = gtk_icon_source_new ();
	gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
	
	REGISTER_ICON (ICON_FILE, "message-manager-plugin-icon");
	REGISTER_ICON (ANJUTA_PIXMAP_MESSAGES, ANJUTA_STOCK_MESSAGES);
	REGISTER_ICON (ANJUTA_PIXMAP_NEXT_MESSAGE, ANJUTA_STOCK_NEXT_MESSAGE);
	ADD_ICON (ANJUTA_PIXMAP_NEXT_MESSAGE_16);
	REGISTER_ICON (ANJUTA_PIXMAP_PREV_MESSAGE, ANJUTA_STOCK_PREV_MESSAGE);
	ADD_ICON (ANJUTA_PIXMAP_PREV_MESSAGE_16);
	
	gtk_icon_source_free (source);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	MessageViewPlugin *mv_plugin;
	GtkWidget* msgman;
	GladeXML *gxml;
	
	g_message ("MessageViewPlugin: Activating MessageView plugin ...");
	mv_plugin = (MessageViewPlugin*) plugin;
	
	register_stock_icons (plugin);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	msgman = anjuta_msgman_new(prefs);
	mv_plugin->msgman = msgman;
	
	anjuta_ui_add_action_group_entries (ui, "ActionGroupGotoMessages",
										_("Next/Prev Message"),
										actions_goto,
										G_N_ELEMENTS (actions_goto), plugin);

	/* Create the messages preferences page */
	gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog_messages", NULL);
	anjuta_preferences_add_page (prefs, gxml,
								"Messages", ICON_FILE);
	g_object_unref (gxml);
	
	mv_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, msgman,
							 "AnjutaMessageView", _("Messages"),
							 "message-manager-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	g_message ("MessageViewPlugin: Dectivating message view plugin ...");
	anjuta_shell_remove_widget (plugin->shell,
								((MessageViewPlugin*)plugin)->msgman, NULL);
	anjuta_ui_unmerge (ui, ((MessageViewPlugin*)plugin)->uiid);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	// MessageViewPlugin *plugin = (MessageViewPlugin*)obj;
}

static void
message_view_plugin_instance_init (GObject *obj)
{
	MessageViewPlugin *plugin = (MessageViewPlugin*)obj;
	plugin->uiid = 0;
}

static void
message_view_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
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
	GtkWidget *msgman = ((MessageViewPlugin*)plugin)->msgman;
	message_view = anjuta_msgman_add_view (ANJUTA_MSGMAN (msgman), file, icon);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 msgman, NULL);
	return IANJUTA_MESSAGE_VIEW (message_view);
}

static void
ianjuta_msgman_remove_view (IAnjutaMessageManager *plugin,
							IAnjutaMessageView * view,
							GError **e)
{
	GtkWidget *msgman = ((MessageViewPlugin*)plugin)->msgman;
	anjuta_msgman_remove_view (ANJUTA_MSGMAN (msgman), MESSAGE_VIEW (view));
}

static IAnjutaMessageView*
ianjuta_msgman_get_current_view (IAnjutaMessageManager *plugin,
								 GError **e)
{
	GtkWidget *msgman = ((MessageViewPlugin*)plugin)->msgman;
	return IANJUTA_MESSAGE_VIEW (anjuta_msgman_get_current_view
				     (ANJUTA_MSGMAN (msgman)));
}

static IAnjutaMessageView*
ianjuta_msgman_get_view_by_name (IAnjutaMessageManager *plugin,
								 const gchar * name,
								 GError ** e)
{
	GtkWidget *msgman = ((MessageViewPlugin*)plugin)->msgman;
	return IANJUTA_MESSAGE_VIEW (anjuta_msgman_get_view_by_name
				     (ANJUTA_MSGMAN (msgman), name));
}

static GList *
ianjuta_msgman_get_all_views (IAnjutaMessageManager *plugin,
							  GError ** e)
{
	GtkWidget *msgman = ((MessageViewPlugin*)plugin)->msgman;
	return anjuta_msgman_get_all_views (ANJUTA_MSGMAN (msgman));
}

static void
ianjuta_msgman_set_current_view (IAnjutaMessageManager *plugin,
								 IAnjutaMessageView *message_view,
								 GError ** e)
{
	GtkWidget *msgman = ((MessageViewPlugin*)plugin)->msgman;
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 msgman, NULL);
	return anjuta_msgman_set_current_view (ANJUTA_MSGMAN (msgman),
					       MESSAGE_VIEW (message_view));
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
}

ANJUTA_PLUGIN_BEGIN (MessageViewPlugin, message_view_plugin);
ANJUTA_INTERFACE(ianjuta_msgman, IANJUTA_TYPE_MESSAGE_MANAGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (MessageViewPlugin, message_view_plugin);
