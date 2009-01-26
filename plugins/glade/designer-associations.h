/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _DESIGNER_ASSOCIATIONS_H_
#define _DESIGNER_ASSOCIATIONS_H_

#include <glib.h>
#include <glib-object.h>
#include <libanjuta/anjuta-shell.h>
#include <libxml/tree.h>

#include "designer-associations-item.h"

G_BEGIN_DECLS

#define DESIGNER_TYPE_ASSOCIATIONS   (designer_associations_get_type ())
#define DESIGNER_ASSOCIATIONS(obj)   (G_TYPE_CHECK_INSTANCE_CAST ((obj), DESIGNER_TYPE_ASSOCIATIONS, DesignerAssociations))
#define DESIGNER_ASSOCIATIONS_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DESIGNER_TYPE_ASSOCIATIONS, DesignerAssociationsClass))
#define DESIGNER_IS_ASSOCIATIONS(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DESIGNER_TYPE_ASSOCIATIONS))
#define DESIGNER_IS_ASSOCIATIONS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), DESIGNER_TYPE_ASSOCIATIONS))
#define DESIGNER_ASSOCIATIONS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DESIGNER_TYPE_ASSOCIATIONS, DesignerAssociationsClass))

typedef struct _DesignerAssociationsClass DesignerAssociationsClass;
typedef struct _DesignerAssociations DesignerAssociations;
typedef struct _DesignerAssociationsPrivate DesignerAssociationsPrivate;

#define DESIGNER_ASSOCIATIONS_ERROR designer_associations_error_quark ()

typedef enum
{
  DESIGNER_ASSOCIATIONS_ERROR_LOADING
} DesignerAssociationsError;

#define DESIGNER_ASSOCIATIONS_DETAIL_ADDED "added"
#define DESIGNER_ASSOCIATIONS_DETAIL_CHANGED "changed"
#define DESIGNER_ASSOCIATIONS_DETAIL_REMOVED "removed"
#define DESIGNER_ASSOCIATIONS_DETAIL_LOADED "loaded"

typedef enum {
	DESIGNER_ASSOCIATIONS_ADDED,
	DESIGNER_ASSOCIATIONS_CHANGED,
	DESIGNER_ASSOCIATIONS_REMOVED,
	DESIGNER_ASSOCIATIONS_LOADED
} DesignerAssociationsAction;

#define DESIGNER_TYPE_ASSOCIATIONS_ACTION (designer_associations_action_get_type())
GType designer_associations_action_get_type (void) G_GNUC_CONST;

struct _DesignerAssociationsClass
{
	GObjectClass parent_class;

	void (*item_notify) (DesignerAssociations *self, DesignerAssociationsItem *item,
                                 DesignerAssociationsAction action);
};

struct _DesignerAssociations
{
	GObject parent_instance;
	GList *associations;
	DesignerAssociationsPrivate *priv;
};

/* DesignerAssociations */

GQuark designer_associations_error_quark (void) G_GNUC_CONST;

GType designer_associations_get_type (void) G_GNUC_CONST;

DesignerAssociations *designer_associations_new (void);

void
designer_associations_save_to_xml (DesignerAssociations *self,
                                   xmlDocPtr xml_doc, xmlNodePtr node,
                                   GFile *project_root);
DesignerAssociations *
designer_associations_load_from_xml (DesignerAssociations *self,
                                     xmlDocPtr xml_doc,
                                     xmlNodePtr node,
                                     GFile *project_root,
                                     GError **error);
void
designer_associations_clear (DesignerAssociations *self);

guint
designer_associations_add_item (DesignerAssociations *self,
                                DesignerAssociationsItem *item);
void
designer_associations_remove_item_by_id (DesignerAssociations *self, guint id);

void
designer_associations_notify_added (DesignerAssociations *self,
                                    DesignerAssociationsItem *item);
void
designer_associations_notify_changed (DesignerAssociations *self,
                                      DesignerAssociationsItem *item);
void
designer_associations_notify_removed (DesignerAssociations *self,
                                      DesignerAssociationsItem *item);
void
designer_associations_notify_loaded (DesignerAssociations *self);

gint
designer_associations_lock_notification (DesignerAssociations *self);

gint
designer_associations_unlock_notification (DesignerAssociations *self);

DesignerAssociationsItem *
designer_associations_search_item (DesignerAssociations *self, GFile *editor,
                                   GFile *designer);

G_END_DECLS

#endif /* _DESIGNER_ASSOCIATIONS_H_ */
