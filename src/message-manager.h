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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <gtk/gtk.h>
#include <gnome.h>

#include "preferences.h"

#ifndef _MESSAGE_MANAGER
#define _MESSAGE_MANAGER

#ifdef __cplusplus
extern "C" 
{
#endif
	
typedef struct _AnjutaMessageManager AnjutaMessageManager;
typedef struct _AnjutaMessageManagerClass AnjutaMessageManagerClass;
	
#define ANJUTA_MESSAGE_MANAGER_TYPE        (anjuta_message_manager_get_type ())
#define ANJUTA_MESSAGE_MANAGER(o)          (GTK_CHECK_CAST ((o), ANJUTA_MESSAGE_MANAGER_TYPE, AnjutaMessageManager))
#define ANJUTA_MESSAGE_MANAGER_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), ANJUTA_MESSAGE_MANAGER_TYPE, AnjutaMessageManagerClass))
#define ANJUTA_IS_MESSAGE_MANAGER(o)       (GTK_CHECK_TYPE ((o), ANJUTA_MESSAGE_MANAGER_TYPE))
#define ANJUTA_IS_MESSAGE_MANAGER_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), ANJUTA_MESSAGE_MANAGER_TYPE))

typedef struct _AnjutaMessageManagerPrivate AnjutaMessageManagerPrivate;

struct _AnjutaMessageManager
{
	GtkFrame parent;
	
	AnjutaMessageManagerPrivate* intern;
};

struct _AnjutaMessageManagerClass
{
	GtkFrameClass parent_class;
	
	// Signals
	void (*message_clicked) (AnjutaMessageManager *amm, gchar* message);
};

// Public functions
GtkWidget* anjuta_message_manager_new();
GtkType anjuta_message_manager_get_type();

gboolean anjuta_message_manager_add_type(AnjutaMessageManager* amm, const gint type_name, const gchar* pixmap);
gboolean anjuta_message_manager_append(AnjutaMessageManager* amm, const gchar* msg_string, gint type_name);

void anjuta_message_manager_clear(AnjutaMessageManager* amm, gint type_name);
void anjuta_message_manager_show(AnjutaMessageManager* amm, gint type_name);

void anjuta_message_manager_next(AnjutaMessageManager* amm);
void anjuta_message_manager_previous(AnjutaMessageManager* amm);

void anjuta_message_manager_info_locals (AnjutaMessageManager* amm, GList * lines, gpointer data);
void anjuta_message_manager_update(AnjutaMessageManager* amm);

void anjuta_message_manager_dock(AnjutaMessageManager* amm);
void anjuta_message_manager_undock(AnjutaMessageManager* amm);

gboolean anjuta_message_manager_save_yourself (AnjutaMessageManager * amm, FILE * stream);
gboolean anjuta_message_manager_load_yourself (AnjutaMessageManager * amm, PropsID props);

gboolean anjuta_message_manager_save_build (AnjutaMessageManager* amm, FILE * stream);

gboolean anjuta_message_manager_is_shown(AnjutaMessageManager* amm);
gboolean anjuta_message_manager_build_is_empty(AnjutaMessageManager* amm);

void create_default_types(AnjutaMessageManager* amm);

#ifdef __cplusplus
};
#endif

// Sync with label in message-manager.cc
typedef enum _AnMessageType
{ 
	MESSAGE_BUILD = 0,
	MESSAGE_DEBUG,
	MESSAGE_FIND,
	MESSAGE_CVS,
	MESSAGE_LOCALS,
	MESSAGE_TERMINAL,
	MESSAGE_STDOUT,
	MESSAGE_STDERR,
	MESSAGE_NONE = -1,
} AnMessageType;

#endif
