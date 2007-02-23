/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-profile.h
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

#ifndef _ANJUTA_PROFILE_H_
#define _ANJUTA_PROFILE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-plugin-description.h>
#include <libanjuta/anjuta-plugin-manager.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_PROFILE             (anjuta_profile_get_type ())
#define ANJUTA_PROFILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PROFILE, AnjutaProfile))
#define ANJUTA_PROFILE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_PROFILE, AnjutaProfileClass))
#define ANJUTA_IS_PROFILE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_PROFILE))
#define ANJUTA_IS_PROFILE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_PROFILE))
#define ANJUTA_PROFILE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_PROFILE, AnjutaProfileClass))
#define ANJUTA_PROFILE_ERROR            (anjuta_profile_error_quark())

typedef enum
{
	ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
	ANJUTA_PROFILE_ERROR_URI_WRITE_FAILED
} AnjutaProfileError;

typedef struct _AnjutaProfileClass AnjutaProfileClass;
typedef struct _AnjutaProfile AnjutaProfile;
typedef struct _AnjutaProfilePriv AnjutaProfilePriv;

struct _AnjutaProfileClass
{
	GObjectClass parent_class;

	/* Signals */
	void(* plugin_added) (AnjutaProfile *self,
						  AnjutaPluginDescription *plugin);
	void(* plugin_removed) (AnjutaProfile *self,
							AnjutaPluginDescription *plugin);
	void(* changed) (AnjutaProfile *self, GList *plugins);
};

struct _AnjutaProfile
{
	GObject parent_instance;
	AnjutaProfilePriv *priv;
};

GQuark anjuta_profile_error_quark (void);
GType anjuta_profile_get_type (void) G_GNUC_CONST;

AnjutaProfile* anjuta_profile_new (const gchar *profile_name,
								   AnjutaPluginManager *plugin_manager);
const gchar *anjuta_profile_get_name (AnjutaProfile *profile);
void anjuta_profile_add_plugin (AnjutaProfile *profile,
								AnjutaPluginDescription *plugin);
void anjuta_profile_remove_plugin (AnjutaProfile *profile,
								   AnjutaPluginDescription *plugin);
gboolean anjuta_profile_add_plugins_from_xml (AnjutaProfile *profile,
											  const gchar* profile_xml_uri,
											  gboolean exclude_from_sync,
											  GError **error);
gboolean anjuta_profile_has_plugin (AnjutaProfile *profile,
									AnjutaPluginDescription *plugin);
GList* anjuta_profile_get_plugins (AnjutaProfile *profile);

gchar* anjuta_profile_to_xml (AnjutaProfile *profile);
void anjuta_profile_set_sync_uri (AnjutaProfile *profile,
								  const gchar *sync_uri);
gboolean anjuta_profile_sync (AnjutaProfile *profile, GError **error);

G_END_DECLS

#endif /* _ANJUTA_PROFILE_H_ */
