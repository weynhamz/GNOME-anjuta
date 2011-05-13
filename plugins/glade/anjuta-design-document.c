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
	PROP_PLUGIN
};

struct _AnjutaDesignDocumentPrivate
{
	GladePlugin* glade_plugin;
};

#define ADD_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), ANJUTA_TYPE_DESIGN_DOCUMENT, AnjutaDesignDocumentPrivate))

static void
on_project_changed (GladeProject* project, GladeCommand* command, gboolean forward, AnjutaDesignDocument* doc)
{
	g_signal_emit_by_name (doc, "update-save-ui", NULL);
	g_signal_emit_by_name (doc, "update-ui", NULL);
}

static void
anjuta_design_document_instance_init (AnjutaDesignDocument *object)
{

}

static void
anjuta_design_document_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (object));
	AnjutaDesignDocumentPrivate* priv = ADD_GET_PRIVATE(object);
	
	switch (prop_id)
	{
	case PROP_PLUGIN:
		priv->glade_plugin = g_value_get_object(value);
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
	
	
	switch (prop_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_design_document_class_init (AnjutaDesignDocumentClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = anjuta_design_document_set_property;
	object_class->get_property = anjuta_design_document_get_property;

	g_type_class_add_private (klass, sizeof(ANJUTA_TYPE_DESIGN_DOCUMENT));
	
	g_object_class_install_property (object_class,
	                                 PROP_PLUGIN,
	                                 g_param_spec_object ("plugin",
	                                                      "",
	                                                      "",
	                                                      G_TYPE_OBJECT,
	                                                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

GtkWidget*
anjuta_design_document_new (GladePlugin* glade_plugin, GladeProject* project)
{
	GObject* doc = g_object_new(ANJUTA_TYPE_DESIGN_DOCUMENT, 
	                            "plugin", glade_plugin,
	                            "project", project,
	                            NULL);
	g_signal_connect (project, "changed", G_CALLBACK (on_project_changed), doc);

	return GTK_WIDGET (doc);
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
	
	GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(self));
	
	const gchar* path = glade_project_get_path(project);
	if (path != NULL)
		return g_file_new_for_path (path);
	else
		return NULL;

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
	
	GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(self));
	
	if (glade_project_get_path(project) != NULL) 
	{
		AnjutaStatus *status;
		
		status = anjuta_shell_get_status (ANJUTA_PLUGIN(priv->glade_plugin)->shell, NULL);
		
		if (glade_project_save (project, glade_project_get_path(project),
								NULL)) 
		{
			anjuta_status_set (status, _("Glade project '%s' saved"),
							   glade_project_get_name(project));
			g_signal_emit_by_name(G_OBJECT(self), "update-save-ui", TRUE);
		} 
		else 
		{
			anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(priv->glade_plugin)->shell),
										_("Invalid Glade file name"));
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
	
	GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(self));
	
	AnjutaStatus *status = anjuta_shell_get_status (ANJUTA_PLUGIN(priv->glade_plugin)->shell, NULL);
		
	if (glade_project_save (project, g_file_get_path (file),
								NULL)) 
	{
		anjuta_status_set (status, _("Glade project '%s' saved"),
							   glade_project_get_name(project));
		g_signal_emit_by_name(G_OBJECT(self), "save_point", TRUE);
	} 
	else 
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(priv->glade_plugin)->shell),
									_("Invalid Glade file name"));
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
	
	GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(self));
	if (project == NULL)
		return FALSE;
	if (glade_project_get_modified (project))
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

static gboolean
ifile_savable_is_conflict (IAnjutaFileSavable* savable, GError** e)
{
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
	iface->is_conflict = ifile_savable_is_conflict;
}

/* Return true if editor can redo */
static gboolean idocument_can_redo(IAnjutaDocument *editor, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(editor);
	GladeCommand *redo_item;

	GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(self));
	if (!project)
	{
		redo_item = NULL;
	}
	else
	{
		redo_item = glade_project_next_redo_item(project);
	}
	return (redo_item != NULL);
}

/* Return true if editor can undo */
static gboolean idocument_can_undo(IAnjutaDocument *editor, GError **e)
{
	AnjutaDesignDocument* self = ANJUTA_DESIGN_DOCUMENT(editor);
	GladeCommand *undo_item;
	GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(self));
	if (!project)
	{
		undo_item = NULL;
	}
	else
	{
		undo_item = glade_project_next_undo_item(project);

	}
	return (undo_item != NULL);
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
	GladeDesignView* view = GLADE_DESIGN_VIEW (edit);
	glade_project_undo (glade_design_view_get_project (view));
}

static void 
idocument_redo(IAnjutaDocument* edit, GError** ee)
{
	GladeDesignView* view = GLADE_DESIGN_VIEW (edit);
	glade_project_redo (glade_design_view_get_project (view));
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
	GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(self));
	return glade_project_get_name(project);
}

static void 
idocument_cut(IAnjutaDocument* edit, GError** ee)
{
	GladeDesignView* view = GLADE_DESIGN_VIEW (edit);
	glade_project_command_cut (glade_design_view_get_project (view));
}

static void 
idocument_copy(IAnjutaDocument* edit, GError** ee)
{
	GladeDesignView* view = GLADE_DESIGN_VIEW (edit);
	glade_project_copy_selection (glade_design_view_get_project (view));
}

static void 
idocument_paste(IAnjutaDocument* edit, GError** ee)
{
	GladeDesignView* view = GLADE_DESIGN_VIEW (edit);
	glade_project_command_paste (glade_design_view_get_project (view), NULL);
}

static void 
idocument_clear(IAnjutaDocument* edit, GError** ee)
{
	GladeDesignView* view = GLADE_DESIGN_VIEW (edit);
	glade_project_command_delete (glade_design_view_get_project (view));

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

ANJUTA_TYPE_BEGIN (AnjutaDesignDocument, anjuta_design_document, GLADE_TYPE_DESIGN_VIEW);
ANJUTA_TYPE_ADD_INTERFACE(idocument, IANJUTA_TYPE_DOCUMENT);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(ifile_savable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_END;

