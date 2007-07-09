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

#include "anjuta-glade-notebook.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-document.h>

enum
{
	PROP_0,
	PROP_PLUGIN
};

struct _AnjutaGladeNotebookPrivate
{
	GladePlugin* glade_plugin;
};

#define AGN_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), ANJUTA_TYPE_GLADE_NOTEBOOK, AnjutaGladeNotebookPrivate))

static void
anjuta_glade_notebook_instance_init (AnjutaGladeNotebook *object)
{
	
}

static void
anjuta_glade_notebook_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (ANJUTA_IS_GLADE_NOTEBOOK (object));
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(object);
	
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
anjuta_glade_notebook_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (ANJUTA_IS_GLADE_NOTEBOOK (object));
	
	
	switch (prop_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_glade_notebook_class_init (AnjutaGladeNotebookClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkNotebookClass* parent_class = GTK_NOTEBOOK_CLASS (klass);

	object_class->set_property = anjuta_glade_notebook_set_property;
	object_class->get_property = anjuta_glade_notebook_get_property;

	g_type_class_add_private (klass, sizeof(ANJUTA_TYPE_GLADE_NOTEBOOK));
	
	g_object_class_install_property (object_class,
	                                 PROP_PLUGIN,
	                                 g_param_spec_object ("plugin",
	                                                      "",
	                                                      "",
	                                                      G_TYPE_OBJECT,
	                                                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


GtkWidget*
anjuta_glade_notebook_new (GladePlugin* glade_plugin)
{
	return GTK_WIDGET(g_object_new(ANJUTA_TYPE_GLADE_NOTEBOOK, 
								   "plugin", glade_plugin,
								   "show-tabs", FALSE,
								   "show-border", FALSE,
								   NULL));
}

static void ifile_open(IAnjutaFile* file, const gchar* uri, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(file);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	
	ianjuta_file_open(IANJUTA_FILE(priv->glade_plugin), uri, e);
}

static gchar* ifile_get_uri(IAnjutaFile* file, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(file);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	
	return ianjuta_file_get_uri(IANJUTA_FILE(priv->glade_plugin), e);
}


static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}

static void ifile_savable_save (IAnjutaFileSavable* file, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(file);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	
	ianjuta_file_savable_save(IANJUTA_FILE_SAVABLE(priv->glade_plugin), e);
}

static void ifile_savable_save_as(IAnjutaFileSavable* file, const gchar* uri, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(file);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	
	ianjuta_file_savable_save_as(IANJUTA_FILE_SAVABLE(priv->glade_plugin), uri, e);
}

static void ifile_savable_set_dirty(IAnjutaFileSavable* file, gboolean dirty, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(file);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	
	ianjuta_file_savable_set_dirty(IANJUTA_FILE_SAVABLE(priv->glade_plugin), dirty, e);
}

static gboolean ifile_savable_is_dirty(IAnjutaFileSavable* file, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(file);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	
	return ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(priv->glade_plugin), e);
}

static void
ifile_savable_iface_init(IAnjutaFileSavableIface *iface)
{
	iface->save = ifile_savable_save;
	iface->save_as = ifile_savable_save_as;
	iface->set_dirty = ifile_savable_set_dirty;
	iface->is_dirty = ifile_savable_is_dirty;
}

/* Return true if editor can redo */
static gboolean idocument_can_redo(IAnjutaDocument *editor, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(editor);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	DEBUG_PRINT("Glade can redo: %d", glade_can_redo(priv->glade_plugin));
	return glade_can_redo(priv->glade_plugin);
}

/* Return true if editor can undo */
static gboolean idocument_can_undo(IAnjutaDocument *editor, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(editor);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	DEBUG_PRINT("Glade can undo: %d", glade_can_undo(priv->glade_plugin));
	return glade_can_undo(priv->glade_plugin);
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
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(edit);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	if (idocument_can_undo(edit, NULL))	
		on_undo_activated(NULL, priv->glade_plugin);
	g_signal_emit_by_name(G_OBJECT(self), "update_ui"); 
}

static void 
idocument_redo(IAnjutaDocument* edit, GError** ee)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(edit);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	if (idocument_can_undo(edit, NULL))	
		on_redo_activated(NULL, priv->glade_plugin);
	g_signal_emit_by_name(G_OBJECT(self), "update_ui"); 
}

/* Grab focus */
static void idocument_grab_focus (IAnjutaDocument *editor, GError **e)
{
	gtk_widget_grab_focus (GTK_WIDGET(editor));
}

/* Return the opened filename */
static const gchar* idocument_get_filename(IAnjutaDocument *editor, GError **e)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(editor);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	gchar* filename = glade_get_filename(priv->glade_plugin);
	if (!filename || !strlen(filename))
	{
		return "Glade";
	}
	else
		return filename;
}

static void 
idocument_cut(IAnjutaDocument* edit, GError** ee)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(edit);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	on_cut_activated(NULL, priv->glade_plugin);
}

static void 
idocument_copy(IAnjutaDocument* edit, GError** ee)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(edit);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	on_copy_activated(NULL, priv->glade_plugin);
}

static void 
idocument_paste(IAnjutaDocument* edit, GError** ee)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(edit);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	on_paste_activated(NULL, priv->glade_plugin);
}

static void 
idocument_clear(IAnjutaDocument* edit, GError** ee)
{
	AnjutaGladeNotebook* self = ANJUTA_GLADE_NOTEBOOK(edit);
	AnjutaGladeNotebookPrivate* priv = AGN_GET_PRIVATE(self);
	on_delete_activated(NULL, priv->glade_plugin);
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

ANJUTA_TYPE_BEGIN (AnjutaGladeNotebook, anjuta_glade_notebook, GTK_TYPE_NOTEBOOK);
ANJUTA_TYPE_ADD_INTERFACE(idocument, IANJUTA_TYPE_DOCUMENT);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(ifile_savable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_END;
