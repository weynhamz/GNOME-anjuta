/****************************************************************************
 * anjuta-vim.c
 *
 * Copyright  2006  Naba Kumar  <naba@gnome.org>
 ****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#include "config.h"
#include "anjuta-vim.h"
#include "gtkvim.h"
#include "plugin.h"

#define FORWARD 	0
#define BACKWARD 	1

struct _AnjutaVimPrivate
{
	gchar *uri;
	gchar *filename;
};

static void anjuta_vim_class_init(AnjutaVimClass *klass);
static void anjuta_vim_instance_init(AnjutaVim *av);
static void anjuta_vim_finalize(GObject *object);

static GObjectClass *parent_class = NULL;

/* Callbacks */

/* Update Monitor on load/save */

static void 
anjuta_vim_instance_init(AnjutaVim* av)
{
	av->priv = g_new0(AnjutaVimPrivate, 1);
}

static void
anjuta_vim_class_init(AnjutaVimClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = anjuta_vim_finalize;
}

static void
anjuta_vim_finalize(GObject *object)
{
	AnjutaVim *cobj;
	cobj = ANJUTA_VIM(object);
	
	/* Free private members, etc. */
	g_free (cobj->priv->uri);
	g_free (cobj->priv->filename);
	g_free(cobj->priv);
	
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* Create a new anjuta_vim instance. If uri is valid,
the file will be loaded in the buffer */

AnjutaVim *
anjuta_vim_new (const gchar* uri, const gchar* filename, AnjutaPlugin* plugin)
{
	AnjutaShell* shell;
	
	AnjutaVim *av = ANJUTA_VIM (g_object_new (ANJUTA_TYPE_VIM, NULL));

	if (uri != NULL && strlen(uri) > 0)
	{
		ianjuta_file_open (IANJUTA_FILE(av), uri, NULL);
	}
	else if (filename != NULL && strlen(filename) > 0)
		av->priv->filename = g_strdup(filename);
	
	return av;
}

/* IAnjutaFile interface */

/* Open uri in Editor */
static void 
ifile_open (IAnjutaFile* file, const gchar *uri, GError** e)
{
	AnjutaVim* av = ANJUTA_VIM(file);
	g_free (av->priv->uri);
	g_free (av->priv->filename);
	av->priv->uri = g_strdup (uri);
	av->priv->filename = g_path_get_basename (uri);
	gtk_vim_edit (GTK_VIM (av), uri, NULL);
}

/* Return the currently loaded uri */

static gchar* 
ifile_get_uri (IAnjutaFile* file, GError** e)
{
	AnjutaVim* av = ANJUTA_VIM(file);
	return g_strdup (av->priv->uri);
}

/* IAnjutaFileSavable interface */

/* Save file */
static void 
ifile_savable_save (IAnjutaFileSavable* file, GError** e)
{
	AnjutaVim* av = ANJUTA_VIM(file);
}

/* Save file as */
static void 
ifile_savable_save_as (IAnjutaFileSavable* file, const gchar *uri, GError** e)
{
	AnjutaVim* av = ANJUTA_VIM(file);
}

static void 
ifile_savable_set_dirty (IAnjutaFileSavable* file, gboolean dirty, GError** e)
{
	AnjutaVim* av = ANJUTA_VIM(file);
}

static gboolean 
ifile_savable_is_dirty (IAnjutaFileSavable* file, GError** e)
{
	AnjutaVim* av = ANJUTA_VIM(file);
	return FALSE;
}

static void
isavable_iface_init (IAnjutaFileSavableIface *iface)
{
	iface->save = ifile_savable_save;
	iface->save_as = ifile_savable_save_as;
	iface->set_dirty = ifile_savable_set_dirty;
	iface->is_dirty = ifile_savable_is_dirty;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}

/* IAnjutaEditor interface */

/* Scroll to line */
static void ieditor_goto_line(IAnjutaEditor *editor, gint line, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
}

/* Scroll to position */
static void ieditor_goto_position(IAnjutaEditor *editor, gint position, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
}

/* Return a newly allocated pointer containing the whole text */
static gchar* ieditor_get_text(IAnjutaEditor* editor, gint start,
							   gint end, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return NULL;
}

/* TODO! No documentation, so we do nothing */
static gchar* ieditor_get_attributes(IAnjutaEditor* editor, 
									 gint start, gint end, GError **e)
{
	return NULL;
}

/* Get cursor position */
static gint ieditor_get_position(IAnjutaEditor* editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return 0;
}


/* Return line of cursor */
static gint ieditor_get_lineno(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return 0;
}

/* Return the length of the text in the buffer */
static gint ieditor_get_length(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return 0;
}

/* Return word on cursor position */
static gchar* ieditor_get_current_word(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return NULL;
}

/* Insert text at position */
static void ieditor_insert(IAnjutaEditor *editor, gint position,
							   const gchar* text, gint length, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
}

/* Append text to buffer */
static void ieditor_append(IAnjutaEditor *editor, const gchar* text,
							   gint length, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
}

static void ieditor_erase_all(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
}

/* Return true if editor can redo */
static gboolean ieditor_can_redo(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return FALSE;
}

/* Return true if editor can undo */
static gboolean ieditor_can_undo(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return FALSE;
}

/* Return column of cursor */
static gint ieditor_get_column(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return 0;
}

/* Return TRUE if editor is in overwrite mode */
static gboolean ieditor_get_overwrite(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return FALSE;
}


/* Set the editor popup menu */
static void ieditor_set_popup_menu(IAnjutaEditor *editor, 
								   GtkWidget* menu, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
}

/* Return the opened filename */
static const gchar* ieditor_get_filename(IAnjutaEditor *editor, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return av->priv->filename;
}

/* Convert from position to line */
static gint ieditor_get_line_from_position(IAnjutaEditor *editor, 
										   gint position, GError **e)
{
	AnjutaVim* av = ANJUTA_VIM(editor);
	return 0;
}

static void 
ieditor_undo(IAnjutaEditor* edit, GError** ee)
{
	AnjutaVim* av = ANJUTA_VIM(edit);
}

static void 
ieditor_redo(IAnjutaEditor* edit, GError** ee)
{
	AnjutaVim* av = ANJUTA_VIM(edit);
}

static void
ieditor_iface_init (IAnjutaEditorIface *iface)
{
	iface->goto_line = ieditor_goto_line;
	iface->goto_position = ieditor_goto_position;
	iface->get_text = ieditor_get_text;
	iface->get_attributes = ieditor_get_attributes;
	iface->get_position = ieditor_get_position;
	iface->get_lineno = ieditor_get_lineno;
	iface->get_length = ieditor_get_length;
	iface->get_current_word = ieditor_get_current_word;
	iface->insert = ieditor_insert;
	iface->append = ieditor_append;
	iface->erase_all = ieditor_erase_all;
	iface->get_filename = ieditor_get_filename;
	iface->can_undo = ieditor_can_undo;
	iface->can_redo = ieditor_can_redo;
	iface->get_column = ieditor_get_column;
	iface->get_overwrite = ieditor_get_overwrite;
	iface->set_popup_menu = ieditor_set_popup_menu;
	iface->get_line_from_position = ieditor_get_line_from_position;
	iface->undo = ieditor_undo;
	iface->redo = ieditor_redo;
}

ANJUTA_TYPE_BEGIN(AnjutaVim, anjuta_vim, GTK_TYPE_VIM);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(isavable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_ADD_INTERFACE(ieditor, IANJUTA_TYPE_EDITOR);
ANJUTA_TYPE_END;
