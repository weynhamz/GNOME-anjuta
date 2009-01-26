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

#include <stdlib.h>
#include "designer-associations.h"
#include "anjuta-glade-marshallers.h"

/* DesignerAssociations */

G_DEFINE_TYPE (DesignerAssociations, designer_associations, G_TYPE_OBJECT);

struct _DesignerAssociationsPrivate
{
	guint last_id;
	gint notification_lock;
	gboolean notification_pending;
};

#define  DESIGNER_ASSOCIATION_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), DESIGNER_TYPE_ASSOCIATIONS, DesignerAssociationsPrivate))

enum
{
	ITEM_NOTIFY,
	LAST_SIGNAL
};

static guint designer_associations_signals[LAST_SIGNAL] = {0};

static GObjectClass *parent_class = NULL;

GQuark
designer_associations_error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0)
		error_quark = g_quark_from_static_string ("designer-associations-error");

	return error_quark;
}

static void
designer_associations_init (DesignerAssociations *self)
{
	self->priv = DESIGNER_ASSOCIATION_GET_PRIVATE (self);
}

static void
designer_associations_finalize (GObject *object)
{
	DesignerAssociations *self = DESIGNER_ASSOCIATIONS (object);

	designer_associations_clear (self);

	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
designer_associations_class_init(DesignerAssociationsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);

	object_class->finalize = designer_associations_finalize;

	g_type_class_add_private (klass, sizeof (DesignerAssociationsPrivate));

	designer_associations_signals[ITEM_NOTIFY] =
		g_signal_new ("item-notify",
		               G_TYPE_FROM_CLASS (object_class),
		               G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		               G_STRUCT_OFFSET (DesignerAssociationsClass, item_notify),
		               NULL, NULL,
		               anjuta_glade_marshallers_VOID__OBJECT_INT,
		               G_TYPE_NONE,
		               2,
		               DESIGNER_TYPE_ASSOCIATIONS_ITEM,
		               DESIGNER_TYPE_ASSOCIATIONS_ACTION);
}

DesignerAssociations *designer_associations_new (void)
{
	return g_object_new (DESIGNER_TYPE_ASSOCIATIONS, NULL);
}

void
designer_associations_save_to_xml (DesignerAssociations *self,
                                   xmlDocPtr xml_doc, xmlNodePtr node,
                                   GFile *project_root)
{
	GList *item;
	guint i;
	xmlNodePtr item_node;


	item = self->associations;
	i = 0;
	while (item)
	{
		item_node = xmlNewDocNode (xml_doc, NULL, BAD_CAST ("item"), NULL);
		xmlAddChild (node, item_node);
		designer_associations_item_to_xml ((DesignerAssociationsItem *) item->data,
		                                   xml_doc, item_node, project_root);
		i ++;
		item = item->next;
	}
}

static guint
designer_associations_make_id (DesignerAssociations *self)
{
	return ++ self->priv->last_id;
}

DesignerAssociations *
designer_associations_load_from_xml (DesignerAssociations *self,
                                     xmlDocPtr xml_doc,
                                     xmlNodePtr node,
                                     GFile *project_root,
                                     GError **error)
{
	DesignerAssociationsItem *item;
	GError *nested_error = NULL;
	xmlNodePtr child_node;

	g_return_val_if_fail (error == NULL || *error == NULL, self);
	g_return_val_if_fail (xml_doc, self);
	g_return_val_if_fail (node, self);

	designer_associations_lock_notification (self);

	designer_associations_clear (self);

	child_node = node->children;
	while (child_node)
	{
		if (!xmlStrcmp (BAD_CAST (DA_XML_TAG_ITEM), child_node->name))
		{
			item = designer_associations_item_from_xml (designer_associations_item_new (),
			                                            xml_doc, child_node, project_root,
			                                            &nested_error);
			if (nested_error)
			{
				g_object_unref (G_OBJECT (item));
				g_propagate_error (error, nested_error);
				break;
			}
			g_assert (((GObject*)item)->ref_count == 1);
			designer_associations_add_item (self, item);
		}
		child_node = child_node->next;
	}
	self->associations = g_list_reverse (self->associations);

	designer_associations_unlock_notification (self);

	return self;
}

void designer_associations_clear (DesignerAssociations *self)
{
	GList *node;

	for (node = self->associations; node;
	     node = node->next)
	{
		g_object_unref (G_OBJECT (node->data));
	}
	g_list_free (self->associations);
	self->associations = NULL;
	self->priv->last_id = 0;

	designer_associations_notify_loaded (self);
}

static gboolean
designer_associations_check_lock (DesignerAssociations *self)
{
	if (self->priv->notification_lock <= 0)
		return TRUE;
	else
	{
		self->priv->notification_pending = TRUE;
		return FALSE;
	}
}

void
designer_associations_notify_added (DesignerAssociations *self,
                                    DesignerAssociationsItem *item)
{
	if (designer_associations_check_lock (self))
		g_signal_emit (self, designer_associations_signals[ITEM_NOTIFY],
		               g_quark_from_static_string (DESIGNER_ASSOCIATIONS_DETAIL_ADDED),
		               item, DESIGNER_ASSOCIATIONS_ADDED);
}

void
designer_associations_notify_changed (DesignerAssociations *self,
                                      DesignerAssociationsItem *item)
{
	if (designer_associations_check_lock (self))
		g_signal_emit (self, designer_associations_signals[ITEM_NOTIFY],
		               g_quark_from_static_string (DESIGNER_ASSOCIATIONS_DETAIL_CHANGED),
		               item, DESIGNER_ASSOCIATIONS_CHANGED);

}

void
designer_associations_notify_removed (DesignerAssociations *self,
                                      DesignerAssociationsItem *item)
{
	if (designer_associations_check_lock (self))
		g_signal_emit (self, designer_associations_signals[ITEM_NOTIFY],
		               g_quark_from_static_string (DESIGNER_ASSOCIATIONS_DETAIL_REMOVED),
		               item, DESIGNER_ASSOCIATIONS_REMOVED);

}

void
designer_associations_notify_loaded (DesignerAssociations *self)
{
	if (designer_associations_check_lock (self))
		g_signal_emit (self, designer_associations_signals[ITEM_NOTIFY],
		               g_quark_from_static_string (DESIGNER_ASSOCIATIONS_DETAIL_LOADED),
		               NULL, DESIGNER_ASSOCIATIONS_LOADED);

}

gint
designer_associations_lock_notification (DesignerAssociations *self)
{
	return self->priv->notification_lock ++;
}

gint
designer_associations_unlock_notification (DesignerAssociations *self)
{
	self->priv->notification_lock --;
	if (self->priv->notification_lock < 0)
		g_critical ("Unbalanced lock stack detected in %s\n",
		             G_GNUC_PRETTY_FUNCTION);
	if (self->priv->notification_lock == 0 && self->priv->notification_pending)
		g_signal_emit (self, designer_associations_signals[ITEM_NOTIFY],
		               g_quark_from_static_string (DESIGNER_ASSOCIATIONS_DETAIL_LOADED),
		               NULL, DESIGNER_ASSOCIATIONS_LOADED);
	return self->priv->notification_lock;
}

guint
designer_associations_add_item (DesignerAssociations *self,
                                DesignerAssociationsItem *item)
{
	g_return_val_if_fail (item, 0);

	g_object_ref (G_OBJECT (item));
	item->id = designer_associations_make_id (self);
	self->associations = g_list_prepend (self->associations, item);
	designer_associations_notify_added (self, item);
	return item->id;
}

void
designer_associations_remove_item_by_id (DesignerAssociations *self, guint id)
{
	GList *node;
	GList *del_node = NULL;
	DesignerAssociationsItem *item;

	node = self->associations;

	while (node)
	{
		item = node->data;

		if (item->id == id)
			del_node = node;

		node = node->next;

		if (del_node)
		{
			self->associations = g_list_delete_link (self->associations, del_node);
			designer_associations_notify_removed (self, item);
			g_object_unref (G_OBJECT (item));
			del_node = NULL;
		}
	}
}

DesignerAssociationsItem *
designer_associations_search_item (DesignerAssociations *self, GFile *editor,
                                   GFile *designer)
{
	GList *node;
	DesignerAssociationsItem *item;

	for (node = self->associations;
	     node; node = node->next)
	{
		item = node->data;

		if (g_file_equal (editor, item->editor) &&
		    g_file_equal (designer, item->designer))
		{
			return item;
		}
	}

	return NULL;
}

/* DesignerAssociationsAction */

GType
designer_associations_action_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		static GEnumValue values[] = {
			{ DESIGNER_ASSOCIATIONS_ADDED,  "DESIGNER_ASSOCIATIONS_ADDED",
				"designer-associations-added" },
			{ DESIGNER_ASSOCIATIONS_CHANGED,     "DESIGNER_ASSOCIATIONS_CHANGED",
				"designer-associations-changed" },
			{ DESIGNER_ASSOCIATIONS_REMOVED, "DESIGNER_ASSOCIATIONS_REMOVED",
				"designer-associations-removed" },
			{ DESIGNER_ASSOCIATIONS_LOADED, "DESIGNER_ASSOCIATIONS_LOADED",
				"designer-associations-loaded" },
			{ 0, NULL, NULL }
		};

		etype = g_enum_register_static ("DesignerAssociationsAction", values);
	}
	return etype;
}

