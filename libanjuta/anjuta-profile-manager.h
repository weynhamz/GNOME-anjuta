/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-profile-manager.h
 * Copyright (C) Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _ANJUTA_PROFILE_MANAGER_H_
#define _ANJUTA_PROFILE_MANAGER_H_

#include <glib-object.h>
#include <libanjuta/anjuta-profile.h>
#include <libanjuta/anjuta-plugin-manager.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_PROFILE_MANAGER             (anjuta_profile_manager_get_type ())
#define ANJUTA_PROFILE_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PROFILE_MANAGER, AnjutaProfileManager))
#define ANJUTA_PROFILE_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_PROFILE_MANAGER, AnjutaProfileManagerClass))
#define ANJUTA_IS_PROFILE_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_PROFILE_MANAGER))
#define ANJUTA_IS_PROFILE_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_PROFILE_MANAGER))
#define ANJUTA_PROFILE_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_PROFILE_MANAGER, AnjutaProfileManagerClass))

typedef struct _AnjutaProfileManagerClass AnjutaProfileManagerClass;
typedef struct _AnjutaProfileManagerPriv AnjutaProfileManagerPriv;
typedef struct _AnjutaProfileManager AnjutaProfileManager;

struct _AnjutaProfileManagerClass
{
	GObjectClass parent_class;
	void(* profile_pushed) (AnjutaProfileManager *self,
							AnjutaProfile* profile);
	void(* profile_popped) (AnjutaProfileManager *self,
							AnjutaProfile* profile);
};

struct _AnjutaProfileManager
{
	GObject parent_instance;
	AnjutaProfileManagerPriv *priv;
};

GType anjuta_profile_manager_get_type (void) G_GNUC_CONST;
AnjutaProfileManager *anjuta_profile_manager_new (AnjutaPluginManager *plugin_manager);

/* Plugin profiles */
gboolean anjuta_profile_manager_push (AnjutaProfileManager *profile_manager,
									  AnjutaProfile *profile, GError **error);
gboolean anjuta_profile_manager_pop (AnjutaProfileManager *profile_manager,
									 const gchar *profile_name, GError **error);

void anjuta_profile_manager_freeze (AnjutaProfileManager *profile_manager);
gboolean anjuta_profile_manager_thaw (AnjutaProfileManager *plugin_manager,
									 GError **error);

G_END_DECLS

#endif /* _ANJUTA_PROFILE_MANAGER_H_ */
