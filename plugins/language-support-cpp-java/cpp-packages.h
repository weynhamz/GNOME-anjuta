/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2011 <jhs@Obelix>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CPP_PACKAGES_H_
#define _CPP_PACKAGES_H_

#include <libanjuta/anjuta-command-queue.h>
#include <libanjuta/anjuta-plugin.h>

G_BEGIN_DECLS

#define CPP_TYPE_PACKAGES             (cpp_packages_get_type ())
#define CPP_PACKAGES(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPP_TYPE_PACKAGES, CppPackages))
#define CPP_PACKAGES_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CPP_TYPE_PACKAGES, CppPackagesClass))
#define CPP_IS_PACKAGES(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPP_TYPE_PACKAGES))
#define CPP_IS_PACKAGES_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CPP_TYPE_PACKAGES))
#define CPP_PACKAGES_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CPP_TYPE_PACKAGES, CppPackagesClass))

typedef struct _CppPackagesClass CppPackagesClass;
typedef struct _CppPackages CppPackages;

struct _CppPackagesClass
{
	GObjectClass parent_class;
};

struct _CppPackages
{
	GObject parent_instance;

	AnjutaPlugin* plugin;
	AnjutaCommandQueue* queue;
	gboolean loading;
	guint idle_id;
};

GType cpp_packages_get_type (void) G_GNUC_CONST;
CppPackages* cpp_packages_new (AnjutaPlugin* plugin);
void cpp_packages_load (CppPackages* packages, gboolean force);

G_END_DECLS

#endif /* _CPP_PACKAGES_H_ */
