/* gbf-project.h
 *
 * Copyright (C) 2002 Jeroen Zwartepoorte
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _GBF_PROJECT_H_
#define _GBF_PROJECT_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GBF_TYPE_PROJECT		(gbf_project_get_type ())
#define GBF_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GBF_TYPE_PROJECT, GbfProject))
#define GBF_PROJECT_CLASS(obj)		(G_TYPE_CHECK_CLASS_CAST ((klass), GBF_TYPE_PROJECT, GbfProjectClass))
#define GBF_IS_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GBF_TYPE_PROJECT))
#define GBF_IS_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GBF_TYPE_PROJECT))
#define GBF_PROJECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GBF_TYPE_PROJECT, GbfProjectClass))

#define GBF_PROJECT_ERROR		(gbf_project_error_quark ())

#define GBF_TYPE_PROJECT_GROUP          (gbf_project_group_get_type ())
#define GBF_TYPE_PROJECT_TARGET         (gbf_project_target_get_type ())
#define GBF_TYPE_PROJECT_TARGET_SOURCE  (gbf_project_target_source_get_type ())

#define GBF_BUILD_ID_DEFAULT		"DEFAULT"

typedef struct _GbfProject		GbfProject;
typedef struct _GbfProjectClass		GbfProjectClass;
typedef struct _GbfProjectGroup		GbfProjectGroup;
typedef struct _GbfProjectTarget	GbfProjectTarget;
typedef struct _GbfProjectTargetSource	GbfProjectTargetSource;
typedef enum   _GbfProjectError		GbfProjectError;
typedef enum   _GbfProjectCapabilities GbfProjectCapabilities;

struct _GbfProjectGroup {
	gchar   *id;
	gchar   *parent_id;

	gchar   *name;

	GList   *groups;
	GList   *targets;
};

struct _GbfProjectTarget {
	gchar   *id;
	gchar   *group_id;

	gchar   *name;
	gchar   *type;

	GList   *sources;
};

struct _GbfProjectTargetSource {
	gchar   *id;
	gchar   *target_id;

	gchar   *source_uri;
};

/* FIXME: extend this list */
enum _GbfProjectError {
	GBF_PROJECT_ERROR_SUCCESS = 0,
	GBF_PROJECT_ERROR_DOESNT_EXIST,
	GBF_PROJECT_ERROR_ALREADY_EXISTS,
	GBF_PROJECT_ERROR_VALIDATION_FAILED,
	GBF_PROJECT_ERROR_PROJECT_MALFORMED,
	GBF_PROJECT_ERROR_GENERAL_FAILURE,
};

enum _GbfProjectCapabilities {
	GBF_PROJECT_CAN_ADD_NONE              = 0,
	GBF_PROJECT_CAN_ADD_GROUP             = 1 << 0,
	GBF_PROJECT_CAN_ADD_TARGET            = 1 << 1,
	GBF_PROJECT_CAN_ADD_SOURCE            = 1 << 2,
	GBF_PROJECT_CAN_PACKAGES							= 1 << 3
};

struct _GbfProject {
	GObject parent;
};

struct _GbfProjectClass {
	GObjectClass parent_class;

	void                     (* project_updated)       (GbfProject *project);

	/* Virtual Table */
	/* Project. */
	void                     (* load)                  (GbfProject  *project,
							    const gchar *path,
							    GError     **error);
	gboolean                 (* probe)                 (GbfProject  *project,
							    const gchar *path,
							    GError     **error);
	void                     (* refresh)               (GbfProject  *project,
							    GError     **error);
	GbfProjectCapabilities   (* get_capabilities)      (GbfProject  *project,
							    GError     **error);

	/* Groups. */
	gchar *                  (* add_group)             (GbfProject  *project,
							    const gchar *parent_id,
							    const gchar *name,
							    GError     **error);
	void                     (* remove_group)          (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GbfProjectGroup *        (* get_group)             (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GList *                  (* get_all_groups)        (GbfProject  *project,
							    GError     **error);
	GtkWidget *              (* configure_group)       (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GtkWidget *              (* configure_new_group)   (GbfProject  *project,
							    GError     **error);

	/* Targets. */
	gchar *                  (* add_target)            (GbfProject  *project,
							    const gchar *group_id,
							    const gchar *name,
							    const gchar *type,
							    GError     **error);
	void                     (* remove_target)         (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GbfProjectTarget *       (* get_target)            (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GList *                  (* get_all_targets)       (GbfProject  *project,
							    GError     **error);
	GtkWidget *              (* configure_target)      (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GtkWidget *              (* configure_new_target)  (GbfProject *project,
							    GError     **error);

	/* Sources. */
	gchar *                  (* add_source)            (GbfProject  *project,
							    const gchar *target_id,
							    const gchar *uri,
							    GError     **error);
	void                     (* remove_source)         (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GbfProjectTargetSource * (* get_source)            (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GList *                  (* get_all_sources)       (GbfProject  *project,
							    GError     **error);
	GtkWidget *              (* configure_source)      (GbfProject  *project,
							    const gchar *id,
							    GError     **error);
	GtkWidget *              (* configure_new_source)  (GbfProject  *project,
							    GError     **error);
	GtkWidget *              (* configure)             (GbfProject  *project,
							    GError     **error);

	GList *									 (* get_config_modules)		 (GbfProject  *project,
																											GError **error);
	GList *									 (* get_config_packages)	 (GbfProject  *project,
																											const gchar* module,
																											GError **error);
	
	
	/* Types. */
	const gchar *            (* name_for_type)         (GbfProject  *project,
							    const gchar *type);
	const gchar *            (* mimetype_for_type)     (GbfProject  *project,
							    const gchar *type);
	gchar **                 (* get_types)             (GbfProject  *project);
};

GQuark                  gbf_project_error_quark          (void);
GType                   gbf_project_get_type             (void);
GType                   gbf_project_group_get_type       (void);
GType                   gbf_project_target_get_type      (void);
GType                   gbf_project_target_source_get_type (void);
void                    gbf_project_load                 (GbfProject    *project,
							  const gchar   *path,
							  GError       **error);
gboolean                gbf_project_probe                (GbfProject    *project,
							  const gchar   *path,
							  GError       **error);
void                    gbf_project_refresh              (GbfProject    *project,
							  GError       **error);
GbfProjectCapabilities  gbf_project_get_capabilities     (GbfProject *project,
							  GError    **error);

/* Groups. */
gchar                  *gbf_project_add_group            (GbfProject    *project,
							  const gchar   *parent_id,
							  const gchar   *name,
							  GError       **error);
void                    gbf_project_remove_group         (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GbfProjectGroup        *gbf_project_get_group            (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GList                  *gbf_project_get_all_groups       (GbfProject    *project,
							  GError       **error);
GtkWidget              *gbf_project_configure_group      (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GtkWidget              *gbf_project_configure_new_group  (GbfProject    *project,
							  GError       **error);


/* Targets. */
gchar                  *gbf_project_add_target           (GbfProject    *project,
							  const gchar   *group_id,
							  const gchar   *name,
							  const gchar   *type,
							  GError       **error);
void                    gbf_project_remove_target        (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GbfProjectTarget       *gbf_project_get_target           (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GList                  *gbf_project_get_all_targets      (GbfProject    *project,
							  GError       **error);
GtkWidget              *gbf_project_configure_target     (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GtkWidget              *gbf_project_configure_new_target (GbfProject    *project,
							  GError       **error);


/* Sources. */
gchar                  *gbf_project_add_source           (GbfProject    *project,
							  const gchar   *target_id,
							  const gchar   *uri,
							  GError       **error);
void                    gbf_project_remove_source        (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GbfProjectTargetSource *gbf_project_get_source           (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GList                  *gbf_project_get_all_sources      (GbfProject    *project,
							  GError       **error);
GtkWidget              *gbf_project_configure_source     (GbfProject    *project,
							  const gchar   *id,
							  GError       **error);
GtkWidget              *gbf_project_configure_new_source (GbfProject    *project,
							  GError       **error);
/* Project */

GtkWidget              *gbf_project_configure            (GbfProject *project,
							  GError **error);

/* Packages */
GList									 *gbf_project_get_config_modules   (GbfProject *project,
																													GError** error);

GList									 *gbf_project_get_config_packages  (GbfProject *project,
																													const gchar* module,
																													GError** error);


/* Types. */
const gchar            *gbf_project_name_for_type        (GbfProject    *project,
							  const gchar   *type);
const gchar            *gbf_project_mimetype_for_type    (GbfProject    *project,
							  const gchar   *type);

gchar                 **gbf_project_get_types            (GbfProject    *project);

/* functions for copying/freeing data structures */

GbfProjectGroup        *gbf_project_group_copy         (GbfProjectGroup        *group);
void                    gbf_project_group_free         (GbfProjectGroup        *group);

GbfProjectTarget       *gbf_project_target_copy        (GbfProjectTarget       *target);
void                    gbf_project_target_free        (GbfProjectTarget       *target);

GbfProjectTargetSource *gbf_project_target_source_copy (GbfProjectTargetSource *source);
void                    gbf_project_target_source_free (GbfProjectTargetSource *source);




#define GBF_BACKEND_BOILERPLATE(class_name, prefix)					\
GType											\
prefix##_get_type (GTypeModule *module)							\
{											\
	static GType type = 0;								\
	if (!type) {									\
		static const GTypeInfo type_info = {					\
			sizeof (class_name##Class),					\
			NULL,								\
			NULL,								\
			(GClassInitFunc)prefix##_class_init,				\
			NULL,								\
			NULL,								\
			sizeof (class_name),						\
			0,								\
			(GInstanceInitFunc)prefix##_instance_init			\
		};									\
		if (module == NULL) {							\
			type = g_type_register_static (GBF_TYPE_PROJECT,		\
						       #class_name,			\
						       &type_info, 0);			\
		} else {								\
			type = g_type_module_register_type (module,	\
							    GBF_TYPE_PROJECT,		\
							    #class_name,		\
							    &type_info, 0);		\
		}									\
	}										\
	return type;									\
}

G_END_DECLS

#endif /* _GBF_PROJECT_H_ */
