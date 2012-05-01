/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    build.h
    Copyright (C) 2000 Naba Kumar

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

#ifndef __BUILD_H__
#define __BUILD_H__

#include <glib.h>


#include <libanjuta/interfaces/ianjuta-builder.h>

#include "plugin.h"


GFile * build_file_from_file (BasicAutotoolsPlugin *plugin, GFile *file, gchar **target);
GFile * build_object_from_file (BasicAutotoolsPlugin *plugin, GFile *file);
GFile * normalize_project_file (GFile *file, GFile *root);
gboolean directory_has_makefile (GFile *dir);
gboolean directory_has_makefile_am (BasicAutotoolsPlugin *bb_plugin,  GFile *dir);

/* Build function type */
typedef BuildContext* (*BuildFunc) (BasicAutotoolsPlugin *plugin, GFile *file,
                                    IAnjutaBuilderCallback callback, gpointer user_data,
                                    GError **err);

BuildContext* build_build_file_or_dir (BasicAutotoolsPlugin *plugin,
                                       GFile *file,
                                       IAnjutaBuilderCallback callback,
                                       gpointer user_data,
                                       GError **err);

BuildContext* build_is_file_built (BasicAutotoolsPlugin *plugin,
                                   GFile *file,
                                   IAnjutaBuilderCallback callback,
                                   gpointer user_data,
                                   GError **err);

BuildContext* build_install_dir (BasicAutotoolsPlugin *plugin,
                                 GFile *dir,
                                 IAnjutaBuilderCallback callback,
                                 gpointer user_data,
                                 GError **err);

BuildContext* build_clean_dir (BasicAutotoolsPlugin *plugin,
                               GFile *file,
                               GError **err);

BuildContext* build_check_dir (BasicAutotoolsPlugin *plugin,
                               GFile *dir,
                               IAnjutaBuilderCallback callback,
                               gpointer user_data,
                               GError **err);

BuildContext* build_distclean (BasicAutotoolsPlugin *plugin);

BuildContext* build_tarball (BasicAutotoolsPlugin *plugin);

BuildContext* build_compile_file (BasicAutotoolsPlugin *plugin,
                                  GFile *file);

BuildContext* build_configure_dir (BasicAutotoolsPlugin *plugin,
                                   GFile *dir,
                                   const gchar *args,
                                   BuildFunc func,
                                   GFile *file,
                                   IAnjutaBuilderCallback callback,
                                   gpointer user_data,
                                   GError **error);

BuildContext* build_generate_dir (BasicAutotoolsPlugin *plugin,
                                  GFile *dir,
                                  const gchar *args,
                                  BuildFunc func,
                                  GFile *file,
                                  IAnjutaBuilderCallback callback,
                                  gpointer user_data,
                                  GError **error);

void build_project_configured (GObject *sender,
                               IAnjutaBuilderHandle handle,
                               GError *error,
                               gpointer user_data);

BuildContext* build_configure_and_build (BasicAutotoolsPlugin *plugin,
                                BuildFunc func,
                                GFile *file,
                                IAnjutaBuilderCallback callback,
                                gpointer user_data,
                                GError **err);

BuildContext* build_configure_dialog (BasicAutotoolsPlugin *plugin,
                             BuildFunc func, GFile *file,
                             IAnjutaBuilderCallback callback,
                             gpointer user_data,
                             GError **error);


GList* build_list_configuration (BasicAutotoolsPlugin *plugin);
const gchar* build_get_uri_configuration (BasicAutotoolsPlugin *plugin, const gchar *uri);







#endif
