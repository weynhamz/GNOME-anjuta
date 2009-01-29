/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 *
 * anjuta is free software.
 *
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

#include "anjuta-design-document.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-document.h>

enum
{
	PROP_0,
	PROP_PLUGIN,
	PROP_DESIGN_VIEW,
	PROP_DESIGN_VIEW_PARENT
};

struct _AnjutaDesignDocumentPrivate
{
	GladePlugin* glade_plugin;
	GladeDesignView *design_view;
	GtkContainer *design_view_parent;
};

static GObjectClass *parent_class = NULL;

#define ADD_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), ANJUTA_TYPE_DESIGN_DOCUMENT, AnjutaDesignDocumentPrivate))

static void
anjuta_design_document_instance_init (AnjutaDesignDocument *object)
{

}

static void
anjuta_design_document_design_view_destroy_cb (GtkObject *object, AnjutaDesignDocument* self);

void
anjuta_design_document_set_design_view_parent (AnjutaDesignDocument* self,
                                               GtkContainer *container)
{
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	g_return_if_fail (priv->design_view != NULL);

	/* Remove the old label */
	if (container == GTK_CONTAINER (self))
	{
		GtkWidget *child = gtk_bin_get_child (GTK_BIN (self));
		if (child)
			gtk_container_remove (GTK_CONTAINER (self), child);
	}
	/* Don't set priv->design_view_parent here because it will be set
	 * in anjuta_design_document_design_view_parent_change_cb
	 */
	if (priv->design_view_parent)
	{
		gtk_container_remove (priv->design_view_parent,
		                      GTK_WIDGET (priv->design_view));
	}
	DEBUG_PRINT ("%s", container ? "Setting the new container for design view" :
	             "Unsetting the container for design view");
	if (container)
	{
		gtk_container_add (container,
		                   GTK_WIDGET (priv->design_view));
	}

	/* if the document widget is empty then add a label with an appropriate text.
	 * container may be NULL only while construction or destruction */
	if (container && gtk_bin_get_child (GTK_BIN (self)) == NULL)
	{
		GtkWidget *label;
		label = gtk_label_new ("Designer layout is detached");
		gtk_container_add (GTK_CONTAINER (self), label);
		gtk_widget_show (GTK_WIDGET (label));
	}
}

static void
anjuta_design_document_design_view_destroy_cb (GtkObject *object, AnjutaDesignDocument* self)
{
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (self));
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	if (priv->design_view)
	{
		DEBUG_PRINT ("Design view destroying");
		anjuta_design_document_set_design_view_parent (self, NULL);
	}
	else
		DEBUG_PRINT ("The design view has already been destroyed");
}/*

static void
anjuta_design_document_design_view_parent_destroy_cb (GtkObject *object, AnjutaDesignDocument* self)
{
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (self));
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);
	DEBUG_PRINT ("The design view parent destroyed");
	if (priv->design_view)
		anjuta_design_document_set_design_view_parent (self, NULL);
}*/

static void
anjuta_design_document_design_view_parent_set_cb (GtkWidget *widget,
                                                  GtkObject *old_parent,
                                                  AnjutaDesignDocument *self)
{
	GtkWidget *new_parent;
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (self));
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);
	if (priv->design_view_parent)
	{
		/*g_signal_handlers_disconnect_by_func (G_OBJECT (priv->design_view_parent),
		         G_CALLBACK(anjuta_design_document_design_view_destroy_cb), self);*/
		g_object_unref (priv->design_view_parent);
	}
	new_parent = gtk_widget_get_parent (widget);
	if (new_parent)
	{
		g_object_ref (new_parent);
		/*g_signal_connect (G_OBJECT (new_parent), "destroy",
						  G_CALLBACK(anjuta_design_document_design_view_parent_destroy_cb),
						  self);*/
	}
	priv->design_view_parent = GTK_CONTAINER (new_parent);
}

void
anjuta_design_document_set_design_view (AnjutaDesignDocument *self, GladeDesignView *value)
{
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);
	g_return_if_fail (priv->design_view == NULL);
	priv->design_view = g_object_ref (value);
	g_signal_connect (G_OBJECT(priv->design_view), "destroy",
					  G_CALLBACK(anjuta_design_document_design_view_destroy_cb),
					  self);
	g_signal_connect (G_OBJECT(priv->design_view), "parent-set",
					  G_CALLBACK(anjuta_design_document_design_view_parent_set_cb),
					  self);
}

GladeDesignView *
anjuta_design_document_get_design_view (AnjutaDesignDocument *self)
{
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	return priv->design_view;
}

static void
anjuta_design_document_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (object));
	AnjutaDesignDocument *self = ANJUTA_DESIGN_DOCUMENT (object);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	switch (prop_id)
	{
	case PROP_PLUGIN:
		priv->glade_plugin = g_value_get_object(value);
		break;
	case PROP_DESIGN_VIEW:
		anjuta_design_document_set_design_view (self, g_value_get_object(value));
		break;
	case PROP_DESIGN_VIEW_PARENT:
		g_return_if_fail (priv->design_view != NULL);
		anjuta_design_document_set_design_view_parent (self, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_design_document_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (object));
	AnjutaDesignDocument *self = ANJUTA_DESIGN_DOCUMENT (object);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	switch (prop_id)
	{
	case PROP_DESIGN_VIEW:
		g_value_set_object(value, priv->design_view);
		break;
	case PROP_DESIGN_VIEW_PARENT:
		g_value_set_object(value, priv->design_view_parent);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_design_document_parent_set_cb (GtkWidget *widget,
                                      GtkObject *old_parent,
                                      AnjutaDesignDocument *self)
{
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (self));
	AnjutaDesignDocumentPrivate *priv = ADD_GET_PRIVATE (self);
	if (gtk_widget_get_parent (GTK_WIDGET (self)) == NULL && priv->design_view)
		anjuta_design_document_set_design_view_parent (self, NULL);
}

static void
anjuta_design_document_constructed (GObject *object)
{
	AnjutaDesignDocument *self = ANJUTA_DESIGN_DOCUMENT (object);
	AnjutaDesignDocumentPrivate *priv = ADD_GET_PRIVATE (self);
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (object));

	if (!priv->design_view_parent)
	{
		anjuta_design_document_set_design_view_parent (self, GTK_CONTAINER (self));
	}

	/* document manager doesn't destroy the document, but unparents it,
	 * so we do destroy the document when parent is NULL
	 */
	g_signal_connect (object, "parent-set",
	                  G_CALLBACK (anjuta_design_document_parent_set_cb), self);

	if (parent_class->constructed)
		parent_class->constructed (object);
}

static void
anjuta_design_document_dispose (GObject *object)
{
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (object));
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(object);

	DEBUG_PRINT ("Disposing anjuta_design_document");
	anjuta_design_document_set_design_view_parent (ANJUTA_DESIGN_DOCUMENT(object), NULL);
	if (priv->design_view)
	{
		GladeDesignView *design_view = priv->design_view;
		/* Mark design view as destroying */
		priv->design_view = NULL;
		gtk_widget_destroy (GTK_WIDGET (design_view));
		g_object_unref (design_view);
	}

	parent_class->dispose (object);
}

static void
anjuta_design_document_class_init (AnjutaDesignDocumentClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = anjuta_design_document_set_property;
	object_class->get_property = anjuta_design_document_get_property;
	object_class->dispose = anjuta_design_document_dispose;
	object_class->constructed = anjuta_design_document_constructed;

	parent_class = g_type_class_peek (g_type_parent (G_TYPE_FROM_CLASS (klass)));

	g_type_class_add_private (klass, sizeof(AnjutaDesignDocumentPrivate));

	g_object_class_install_property (object_class,
	                                 PROP_PLUGIN,
	                                 g_param_spec_object ("plugin",
	                                                      "",
	                                                      "",
	                                                      G_TYPE_OBJECT,
	                                                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
	                                 PROP_DESIGN_VIEW,
	                                 g_param_spec_object ("design-view",
	                                                      "",
	                                                      "",
	                                                      GLADE_TYPE_DESIGN_VIEW,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
	                                 PROP_DESIGN_VIEW_PARENT,
	                                 g_param_spec_object ("design-view-parent",
	                                                      "",
	                                                      "",
	                                                      GTK_TYPE_CONTAINER,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

GtkWidget*
anjuta_design_document_new (GladePlugin* glade_plugin,
                            GladeDesignView *design_view,
                            GtkContainer *design_view_parent)
{
	/* "design-view" property should be before "design-view-parent" one */
	return GTK_WIDGET(g_object_new(ANJUTA_TYPE_DESIGN_DOCUMENT,
								   "plugin", glade_plugin,
								   "design-view", design_view,
								   "design-view-parent", design_view_parent,
								   NULL));
}

static void ifile_open(IAnjutaFile* ifile, GFile* file, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(ifile);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	ianjuta_file_open(IANJUTA_FILE(priv->glade_plugin), file, e);
}

static GFile* ifile_get_file(IAnjutaFile* ifile, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(ifile);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	GladeProject* project = glade_design_view_get_project(priv->design_view);

#if (GLADEUI_VERSION >= 330)
	const gchar* path = glade_project_get_path(project);
	if (path != NULL)
		return g_file_new_for_path (path);
	else
		return NULL;
#else
	if (project && project->path)
		return g_file_new_for_path (project->path);
	else
		return NULL;
#endif
}


static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

static void ifile_savable_save (IAnjutaFileSavable* file, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(file);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);
	
	GladeProject* project = glade_design_view_get_project(priv->design_view);
	
#if (GLADEUI_VERSION >= 330)
	if (glade_project_get_path(project) != NULL) 
	{
#else
		if (project && project->path != NULL) 
		{
#endif
			AnjutaStatus *status;
			
			status = anjuta_shell_get_status (ANJUTA_PLUGIN(priv->glade_plugin)->shell, NULL);
			
#if (GLADEUI_VERSION >= 330)
			if (glade_project_save (project, glade_project_get_path(project),
									NULL)) 
			{
				anjuta_status_set (status, _("Glade project '%s' saved"),
								   glade_project_get_name(project));
#else
				if (glade_project_save (project, project->path, NULL)) 
				{
					GFile* file = g_file_new_for_path(project->path);
					anjuta_status_set (status, _("Glade project '%s' saved"),
									   project->name);
#endif
					g_signal_emit_by_name(G_OBJECT(self), "save_point", TRUE);
					g_signal_emit_by_name(G_OBJECT(self), "saved", file);
				}
				else
				{
					anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(priv->glade_plugin)->shell),
												_("Invalid glade file name"));
					g_signal_emit_by_name(G_OBJECT(self), "saved", NULL);
				}
				return;
			}
			DEBUG_PRINT("%s", "Invalid use of ifile_savable_save!");
}

static void ifile_savable_save_as(IAnjutaFileSavable* ifile, GFile* file, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(ifile);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	GladeProject* project = glade_design_view_get_project(priv->design_view);

	AnjutaStatus *status = anjuta_shell_get_status (ANJUTA_PLUGIN(priv->glade_plugin)->shell, NULL);

#if (GLADEUI_VERSION >= 330)
	if (glade_project_save (project, g_file_get_path (file),
								NULL))
	{
		anjuta_status_set (status, _("Glade project '%s' saved"),
							   glade_project_get_name(project));
#else
	if (glade_project_save (project, g_file_get_path (file), NULL))
	{
		anjuta_status_set (status, _("Glade project '%s' saved"),
							   project->name);
#endif
	g_signal_emit_by_name(G_OBJECT(self), "save_point", TRUE);
	}
	else
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(priv->glade_plugin)->shell),
									_("Invalid glade file name"));
	}
	return;
}

static void ifile_savable_set_dirty(IAnjutaFileSavable* file, gboolean dirty, GError **e)
{
	/* FIXME */
}

static gboolean ifile_savable_is_dirty(IAnjutaFileSavable* file, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(file);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);

	GladeProject* project = glade_design_view_get_project(priv->design_view);
	if (project == NULL)
		return FALSE;
#if (GLADEUI_VERSION >= 330)
#  if (GLADEUI_VERSION >= 331)
	if (glade_project_get_modified (project))
#  else
 	if (glade_project_get_has_unsaved_changes (project))
#  endif
#else
	if (project->changed)
#endif
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static gboolean
ifile_savable_is_read_only (IAnjutaFileSavable* savable, GError** e)
{
	// FIXME
	return FALSE;
}

static void
ifile_savable_iface_init(IAnjutaFileSavableIface *iface)
{
	iface->save = ifile_savable_save;
	iface->save_as = ifile_savable_save_as;
	iface->set_dirty = ifile_savable_set_dirty;
	iface->is_dirty = ifile_savable_is_dirty;
	iface->is_read_only = ifile_savable_is_read_only;
}

/* Return true if editor can redo */
static gboolean idocument_can_redo(IAnjutaDocument *editor, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(editor);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);
#if (GLADEUI_VERSION >= 330)
	GladeCommand *redo_item;
#else
	GList *prev_redo_item;
	GList *redo_item;
#endif
	const gchar *redo_description = NULL;

	GladeProject* project = glade_design_view_get_project(priv->design_view);
	if (!project)
	{
		redo_item = NULL;
	}
	else
	{
#if (GLADEUI_VERSION >= 330)
		redo_item = glade_project_next_redo_item(project);
		if (redo_item)
			redo_description = redo_item->description;
#else
		prev_redo_item = project->prev_redo_item;
		redo_item = (prev_redo_item == NULL) ? project->undo_stack : prev_redo_item->next;
		if (redo_item && redo_item->data)
			redo_description = GLADE_COMMAND (redo_item->data)->description;
#endif
	}
	return (redo_description != NULL);
}

/* Return true if editor can undo */
static gboolean idocument_can_undo(IAnjutaDocument *editor, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(editor);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);
#if (GLADEUI_VERSION >= 330)
	GladeCommand *undo_item;
#else
	GList *prev_redo_item;
	GList *undo_item;
#endif
	const gchar *undo_description = NULL;
	GladeProject* project = glade_design_view_get_project(priv->design_view);
	if (!project)
	{
		undo_item = NULL;
	}
	else
	{
#if (GLADEUI_VERSION >= 330)
		undo_item = glade_project_next_undo_item(project);
		if (undo_item)
			undo_description = undo_item->description;
#else
		undo_item = prev_redo_item = project->prev_redo_item;
		if (undo_item && undo_item->data)
			undo_description = GLADE_COMMAND (undo_item->data)->description;
#endif
	}
	return (undo_description != NULL);
}

/* Return true if editor can undo */
static void idocument_begin_undo_action (IAnjutaDocument *editor, GError **e)
{
	/* FIXME */
}

/* Return true if editor can undo */
static void idocument_end_undo_action (IAnjutaDocument *editor, GError **e)
{
	/* FIXME */
}


static void
idocument_undo(IAnjutaDocument* edit, GError** ee)
{
	glade_app_command_undo();
}

static void
idocument_redo(IAnjutaDocument* edit, GError** ee)
{
	glade_app_command_redo();
}

/* Grab focus */
static void idocument_grab_focus (IAnjutaDocument *editor, GError **e)
{
	gtk_widget_grab_focus (GTK_WIDGET(editor));
}

/* Return the opened filename */
static const gchar* idocument_get_filename(IAnjutaDocument *editor, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(editor);
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(self);
	GladeProject* project = glade_design_view_get_project(priv->design_view);
	#if (GLADEUI_VERSION >= 330)
		return glade_project_get_name(project);
	#else
		if (project)
			return project->name;
		else
			return "Glade";
	#endif
}

static void
idocument_cut(IAnjutaDocument* edit, GError** ee)
{
	glade_app_command_cut();
}

static void
idocument_copy(IAnjutaDocument* edit, GError** ee)
{
 	glade_app_command_copy();
}

static void
idocument_paste(IAnjutaDocument* edit, GError** ee)
{
	glade_app_command_paste(NULL);
}

static void
idocument_clear(IAnjutaDocument* edit, GError** ee)
{
	glade_app_command_delete();
}


static void
idocument_iface_init (IAnjutaDocumentIface *iface)
{
	iface->grab_focus = idocument_grab_focus;
	iface->get_filename = idocument_get_filename;
	iface->can_undo = idocument_can_undo;
	iface->can_redo = idocument_can_redo;
	iface->begin_undo_action = idocument_begin_undo_action;
	iface->end_undo_action = idocument_end_undo_action;
	iface->undo = idocument_undo;
	iface->redo = idocument_redo;
	iface->cut = idocument_cut;
	iface->copy = idocument_copy;
	iface->paste = idocument_paste;
	iface->clear = idocument_clear;
}

ANJUTA_TYPE_BEGIN (AnjutaDesignDocument, anjuta_design_document, GTK_TYPE_ALIGNMENT);
ANJUTA_TYPE_ADD_INTERFACE(idocument, IANJUTA_TYPE_DOCUMENT);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(ifile_savable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_END;
