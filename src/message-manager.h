/*
    message-manager.h
    Copyright (C) 2000, 2001  Kh. Naba Kumar Singh, Johannes Schmid

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
    Foundation, Inc., 59 Temple Place, Suiate 330, Boston, MA  02111-1307  USA
*/

#include <gtk/gtk.h>

#include "preferences.h"

#ifndef _MESSAGE_MANAGER
#define _MESSAGE_MANAGER

#ifdef __cplusplus
extern "C" 
{
#endif

#define MESSAGE_INDICATOR_ERROR   2
#define MESSAGE_INDICATOR_WARNING 1
#define MESSAGE_INDICATOR_OTHERS  0

typedef struct _AnMessageManager AnMessageManager;
typedef struct _AnMessageManagerClass AnMessageManagerClass;
	
#define AN_MESSAGE_MANAGER_TYPE        (an_message_manager_get_type ())
#define AN_MESSAGE_MANAGER(o)          (GTK_CHECK_CAST ((o), AN_MESSAGE_MANAGER_TYPE, AnMessageManager))
#define AN_MESSAGE_MANAGER_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), AN_MESSAGE_MANAGER_TYPE, AnMessageManagerClass))
#define AN_IS_MESSAGE_MANAGER(o)       (GTK_CHECK_TYPE ((o), AN_MESSAGE_MANAGER_TYPE))
#define AN_IS_MESSAGE_MANAGER_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), AN_MESSAGE_MANAGER_TYPE))

/* Message info passed to indicate signal */
typedef struct
{
	gint type;
	gchar *filename;
	gint line;
	gint message_type;

} MessageIndicatorInfo;

typedef struct _AnMessageManagerPrivate AnMessageManagerPrivate;

struct _AnMessageManager
{
	GtkFrame parent;
	
	AnMessageManagerPrivate* intern;
};

struct _AnMessageManagerClass
{
	GtkFrameClass parent_class;
};

// Public functions
GtkWidget* an_message_manager_new();
guint an_message_manager_get_type();

gboolean an_message_manager_add_type(AnMessageManager* amm, const gint type_name, const gchar* pixmap);
gboolean an_message_manager_append(AnMessageManager* amm, const gchar* msg_string, gint type_name);

gboolean an_message_manager_show_pane (AnMessageManager* amm, gint type_name);

void an_message_manager_clear(AnMessageManager* amm, gint type_name);
void an_message_manager_show(AnMessageManager* amm, gint type_name);

void an_message_manager_next(AnMessageManager* amm);
void an_message_manager_previous(AnMessageManager* amm);

void an_message_manager_info_locals (AnMessageManager* amm, GList * lines, gpointer data);
void an_message_manager_update(AnMessageManager* amm);

void an_message_manager_dock(AnMessageManager* amm);
void an_message_manager_undock(AnMessageManager* amm);

gboolean an_message_manager_save_yourself (AnMessageManager * amm, FILE * stream);
gboolean an_message_manager_load_yourself (AnMessageManager * amm, PropsID props);

gboolean an_message_manager_save_build (AnMessageManager* amm, FILE * stream);

gboolean an_message_manager_is_shown(AnMessageManager* amm);
gboolean an_message_manager_build_is_empty(AnMessageManager* amm);

void an_message_manager_indicate_error (AnMessageManager * amm, gint type_name, gchar* file, glong line);
void an_message_manager_indicate_warning (AnMessageManager * amm, gint type_name, gchar* file, glong line);
void an_message_manager_indicate_others (AnMessageManager * amm, gint type_name, gchar* file, glong line);

void create_default_types(AnMessageManager* amm);
void an_message_manager_set_widget(AnMessageManager* amm, gint type_name, GtkWidget* widget);

#ifdef __cplusplus
};
#endif

// Sync with label in message-manager.cc
typedef enum _AnMessageType
{ 
	MESSAGE_BUILD = 0,
	MESSAGE_FIND,
	MESSAGE_CVS,
	MESSAGE_TERMINAL,
	MESSAGE_STDOUT,
	MESSAGE_STDERR,
	MESSAGE_DEBUG,
	MESSAGE_LOCALS,
	MESSAGE_WATCHES,
	MESSAGE_STACK,
	MESSAGE_MAX = MESSAGE_STACK,
	MESSAGE_NONE = -1,
} AnMessageType;

/* Defined in anjuta.h */
void on_message_indicate(AnMessageManager* amm, MessageIndicatorInfo *info);
void on_message_clicked(AnMessageManager* amm, const char* message);	

#endif
