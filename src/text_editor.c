/*
 * text_editor.c
 * Copyright (C) 2000  Kh. Naba Kumar Singh
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gnome.h>
#include <errno.h>

#include "global.h"
#include "resources.h"
#include "anjuta.h"

#include "text_editor.h"
#include "text_editor_gui.h"
#include "text_editor_cbs.h"
#include "text_editor_menu.h"
#include "utilities.h"
#include "syntax.h"
#include "launcher.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

#include "properties.h"
#include "lexer.h"
#include "aneditor.h"
#include "controls.h"

/* Marker 0 is defined for bookmarks */
#define TEXT_EDITOR_LINEMARKER	2

#define MARKER_PROP_END 0xaaaaaa /* Do not define any color with this value */

/* marker fore and back colors */
static glong marker_prop[] = 
{
	SC_MARK_ROUNDRECT, 0x00007f, 0xffff80,			/* Book mark */
	SC_MARK_CIRCLE, 0x00007f, 0xff80ff,		/* Breakpoint mark */
	SC_MARK_SHORTARROW, 0x00007f, 0x00ffff,	/* Line mark */
	MARKER_PROP_END,	/* end */
};

static void initialize_markers (TextEditor* te)
{
	gint i, marker;
	g_return_if_fail (te != NULL);
	
	marker = 0;
	for (i = 0;; i += 3)
	{
		if (marker_prop[i] == MARKER_PROP_END)
			break;
		scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_MARKERDEFINE,
			marker, marker_prop[i+0]);
		scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_MARKERSETFORE,
			marker, marker_prop[i+1]);
		scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_MARKERSETBACK,
			marker, marker_prop[i+2]);
		marker++;
	}
}

TextEditor *
text_editor_new (gchar * filename, TextEditor * parent, Preferences * eo)
{
	gchar *buff;
	TextEditor *te;
	static guint new_file_count;

	te = (TextEditor *) g_malloc (sizeof (TextEditor));
	if (te == NULL)
		return NULL;
	te->filename = g_strdup_printf ("Newfile#%d", ++new_file_count);
	te->full_filename = NULL;
	te->modified_time = time (NULL);
	te->preferences = eo;
	te->force_hilite = TE_LEXER_AUTOMATIC;
	te->freeze_count = 0;
	te->current_line = 0;
	te->menu = NULL;
	te->autosave_on = FALSE;
	te->autosave_it = 10;
	te->props_base = eo->props;
	te->first_time_expose = TRUE;

	/* geom must be set before create_widget */
	if (parent)
	{
		te->mode = parent->mode;
		te->geom.x = parent->geom.x + 10;
		te->geom.y = parent->geom.y + 20;
		te->geom.width = parent->geom.width;
		te->geom.height = parent->geom.height;
	}
	else
	{
		te->mode = TEXT_EDITOR_PAGED;
		te->geom.x = 50;
		te->geom.y = 50;
		te->geom.width = 600;
		te->geom.height = 400;
	}

	/* Editor is created here and not in create_text_editot_gui() */
	te->editor_id = aneditor_new (prop_get_pointer (te->props_base));
	/*
	aneditor_command (te->editor_id, ANE_SETACCELGROUP,
			  (glong) app->accel_group, 0);
	*/

	create_text_editor_gui (te);
	initialize_markers (te);

	if (te->mode == TEXT_EDITOR_WINDOWED)
	{
		gtk_container_add (GTK_CONTAINER (te->widgets.client_area),
				      te->widgets.client);
	}
	buff = g_strdup_printf ("Anjuta: %s", te->filename);
	gtk_window_set_title (GTK_WINDOW (te->widgets.window), buff);
	g_free (buff);
	if (filename)
	{
		new_file_count--;
		if (te->filename)
			g_free (te->filename);
		if (te->full_filename)
			g_free (te->full_filename);
		te->filename = g_strdup (extract_filename (filename));
		te->full_filename = g_strdup (filename);
		buff = g_strdup_printf ("Anjuta: %s", te->full_filename);
		gtk_window_set_title (GTK_WINDOW (te->widgets.window), buff);
		g_free (buff);
		if (text_editor_load_file (te) == FALSE)
		{
			anjuta_system_error (errno, _("Couldn't load file: %s."), te->full_filename);
			text_editor_destroy (te);
			return NULL;
		}
	}
	te->menu = text_editor_menu_new ();
	text_editor_update_preferences (te);
	text_editor_update_controls (te);
	return te;
}

void
text_editor_freeze (TextEditor * te)
{
	te->freeze_count++;
}

void
text_editor_thaw (TextEditor * te)
{
	te->freeze_count--;
	if (te->freeze_count < 0)
		te->freeze_count = 0;
}

void
text_editor_set_hilite_type (TextEditor * te)
{
	aneditor_command (te->editor_id, ANE_SETLANGUAGE, te->force_hilite,
			  0);
	aneditor_command (te->editor_id, ANE_SETHILITE,
			  (glong) extract_filename (te->full_filename), 0);
}

void
text_editor_set_zoom_factor (TextEditor * te, gint zfac)
{
	aneditor_command (te->editor_id, ANE_SETZOOM, zfac,  0);
}

void
text_editor_destroy (TextEditor * te)
{
	if (te)
	{
		gtk_widget_hide (te->widgets.window);
		gtk_widget_hide (te->widgets.client);
		if (te->autosave_on)
			gtk_timeout_remove (te->autosave_id);
		gtk_widget_unref (te->widgets.window);
		gtk_widget_unref (te->widgets.client_area);
		gtk_widget_unref (te->widgets.client);
		gtk_widget_unref (te->widgets.line_label);
		gtk_widget_unref (te->widgets.editor);
		gtk_widget_unref (te->buttons.new);
		gtk_widget_unref (te->buttons.new);
		gtk_widget_unref (te->buttons.open);
		gtk_widget_unref (te->buttons.save);
		gtk_widget_unref (te->buttons.reload);
		gtk_widget_unref (te->buttons.cut);
		gtk_widget_unref (te->buttons.copy);
		gtk_widget_unref (te->buttons.paste);
		gtk_widget_unref (te->buttons.find);
		gtk_widget_unref (te->buttons.replace);
		gtk_widget_unref (te->buttons.compile);
		gtk_widget_unref (te->buttons.build);
		gtk_widget_unref (te->buttons.print);
		gtk_widget_unref (te->buttons.attach);

		if (te->widgets.window)
			gtk_widget_destroy (te->widgets.window);
		if (te->filename)
			g_free (te->filename);
		if (te->full_filename)
			g_free (te->full_filename);
		if (te->menu)
			text_editor_menu_destroy (te->menu);
		if (te->editor_id)
			aneditor_destroy (te->editor_id);
		g_free (te);
		te = NULL;
	}
}

void
text_editor_undock (TextEditor * te, GtkWidget * container)
{
	if (!te)
		return;
	if (te->mode == TEXT_EDITOR_WINDOWED)
		return;
	te->mode = TEXT_EDITOR_WINDOWED;
	if (GTK_IS_CONTAINER (container) == FALSE)
		return;
	gtk_container_remove (GTK_CONTAINER (container), te->widgets.client);
	gtk_container_add (GTK_CONTAINER (te->widgets.client_area),
			   te->widgets.client);
	gtk_widget_show (te->widgets.client);
	gtk_widget_show (te->widgets.window);
}

void
text_editor_dock (TextEditor * te, GtkWidget * container)
{
	if (!te)
		return;
	if (te->mode == TEXT_EDITOR_PAGED)
		return;
	te->mode = TEXT_EDITOR_PAGED;
	gtk_widget_hide (te->widgets.window);
	gtk_container_remove (GTK_CONTAINER (te->widgets.client_area),
			      te->widgets.client);
	gtk_container_add (GTK_CONTAINER (container), te->widgets.client);
	gtk_widget_show (te->widgets.client);
}

glong
text_editor_find (TextEditor * te, gchar * str, gint scope, gboolean forward,
		gboolean regexp, gboolean ignore_case, gboolean whole_word)
{
	glong ret;
	GtkWidget *editor;
	glong flags;

	if (!te) return -1;
	editor = te->widgets.editor;
	
	flags = (ignore_case ? 0 : SCFIND_MATCHCASE)
		| (regexp ? SCFIND_REGEXP : 0)
		| (whole_word ? SCFIND_WHOLEWORD : 0)
		| (forward ? 0 : ANEFIND_REVERSE_FLAG);
		
	switch (scope)
	{
		case TEXT_EDITOR_FIND_SCOPE_WHOLE:
				if (forward)
				{
					scintilla_send_message (SCINTILLA (editor), SCI_SETANCHOR, 0, 0);
					scintilla_send_message (SCINTILLA (editor), SCI_SETCURRENTPOS, 0, 0);
				}
				else
				{
					glong length;
					length = scintilla_send_message (SCINTILLA (editor), SCI_GETTEXTLENGTH, 0, 0);
					scintilla_send_message (SCINTILLA (editor), SCI_SETCURRENTPOS, length-1, 0);
					scintilla_send_message (SCINTILLA (editor), SCI_SETANCHOR, length-1, 0);
				}
			break;
		default:
	}
	ret = aneditor_command (te->editor_id, ANE_FIND, flags, (long)str);
	return ret;
}

void
text_editor_replace_selection (TextEditor * te, gchar* r_str)
{
	if (!te) return;
	scintilla_send_message (SCINTILLA(te->widgets.editor), SCI_REPLACESEL, 0, (long)r_str);
}

guint
text_editor_get_total_lines (TextEditor * te)
{
	guint i;
	guint count = 0;
	if (te == NULL)
		return 0;
	if (IS_SCINTILLA (te->widgets.editor) == FALSE)
		return 0;
	for (i = 0;
	     i < scintilla_send_message (SCINTILLA (te->widgets.editor),
					 SCI_GETLENGTH, 0, 0); i++)
	{
		if (scintilla_send_message
		    (SCINTILLA (te->widgets.editor), SCI_GETCHARAT, i,
		     0) == '\n')
			count++;
	}
	return count;
}

guint
text_editor_get_current_lineno (TextEditor * te)
{
	guint count;
	
	g_return_val_if_fail (te != NULL, 0);

	count =	scintilla_send_message (SCINTILLA (te->widgets.editor),
					SCI_GETCURRENTPOS, 0, 0);
	count =	scintilla_send_message (SCINTILLA (te->widgets.editor),
					SCI_LINEFROMPOSITION, count, 0);
	return linenum_scintilla_to_text_editor(count);
}

gboolean
text_editor_goto_point (TextEditor * te, guint point)
{
	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail(IS_SCINTILLA (te->widgets.editor) == TRUE, FALSE);

	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_GOTOPOS,
				point, 0);
	return TRUE;
}

gboolean
text_editor_goto_line (TextEditor * te, guint line, gboolean mark)
{
	gint selpos;
	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail(IS_SCINTILLA (te->widgets.editor) == TRUE, FALSE);
	g_return_val_if_fail(line >= 0, FALSE);

	te->current_line = line;
	if (mark) text_editor_set_line_marker (te, line);
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_ENSUREVISIBLE,
		linenum_text_editor_to_scintilla (line), 0);
	selpos = scintilla_send_message(SCINTILLA (te->widgets.editor), SCI_POSITIONFROMLINE,
		linenum_text_editor_to_scintilla (line), 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_SETSELECTIONSTART, selpos, 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_SETSELECTIONEND, selpos, 0);
	
	/* This ensures that we have arround 5 lines visible below the mark */
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_GOTOLINE, 
		linenum_text_editor_to_scintilla (line)+5, 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_GOTOLINE, 
		linenum_text_editor_to_scintilla (line), 0);
	return TRUE;
}

gint
text_editor_goto_block_start (TextEditor* te)
{
	gint line;
	line = aneditor_command (te->editor_id, ANE_GETBLOCKSTARTLINE, 0, 0);
	if (line >= 0) text_editor_goto_line (te, line, TRUE);
	else gdk_beep();
	return line;
}

gint
text_editor_goto_block_end (TextEditor* te)
{
	gint line;
	line = aneditor_command (te->editor_id, ANE_GETBLOCKENDLINE, 0, 0);
	if (line >= 0) text_editor_goto_line (te, line, TRUE);
	else gdk_beep();
	return line;
}

gint
text_editor_set_marker (TextEditor *te, glong line, gint marker)
{
	g_return_val_if_fail (te != NULL, -1);
	g_return_val_if_fail(IS_SCINTILLA (te->widgets.editor) == TRUE, -1);

	/* Scintilla interpretes line+1 rather than line */
	/* A bug perhaps */
	/* So counter balance it with line-1 */
	/* Using the macros linenum_* */
	return scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_MARKERADD,
		linenum_text_editor_to_scintilla (line), marker);
}

void
text_editor_set_line_marker (TextEditor *te, glong line)
{
	g_return_if_fail (te != NULL);
	g_return_if_fail(IS_SCINTILLA (te->widgets.editor) == TRUE);

	anjuta_delete_all_marker (TEXT_EDITOR_LINEMARKER);
	text_editor_set_marker (te, line, TEXT_EDITOR_LINEMARKER);
}

static gboolean
load_from_file (TextEditor * te, gchar * fn)
{
	FILE *fp;
	gchar *buffer;
	gint nchars;
	struct stat st;
	size_t size;

	if (stat (fn, &st) != 0)
	{
		g_warning ("Could not stat the file");
		return FALSE;
	}
	size = st.st_size;
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_CLEARALL,
				0, 0);
	buffer = g_malloc (size);
	if (buffer == NULL && size != 0)
	{
		/* This is funny in unix, but never hurts */
		g_warning ("This file is too big. Unable to allocate memory");
		return FALSE;
	}
	fp = fopen (fn, "rb");
	if (!fp)
	{
		g_free (buffer);
		return FALSE;
	}
	/* Crude way of loading, but faster */
	nchars = fread (buffer, 1, size, fp);
	if (size != nchars)
		g_warning ("File size and loaded size not matching");
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_ADDTEXT,
				nchars, (long) buffer);
	g_free (buffer);
	if (ferror (fp))
	{
		fclose (fp);
		return FALSE;
	}
	fclose (fp);
	return TRUE;
}

#if 0

static gint
strip_trailing_spaces (char *data, int ds, gboolean lastBlock)
{
	gint lastRead;
	gchar *w;
	gint i;

	w = data;
	lastRead = 0;

	for (i = 0; i < ds; i++)
	{
		gchar ch;

		ch = data[i];
		if ((ch == ' ') || (ch == '\t'))
		{
			/* Swallow those spaces */
		}
		else if ((ch == '\r') || (ch == '\n'))
		{
			*w++ = ch;
			lastRead = i + 1;
		}
		else
		{
			while (lastRead < i)
			{
				*w++ = data[lastRead++];
			}
			*w++ = ch;
			lastRead = i + 1;
		}
	}
	/* If a non-final block, then preserve whitespace
	 * at end of block as it may be significant.
	 */
	if (!lastBlock)
	{
		while (lastRead < ds)
		{
			*w++ = data[lastRead++];
		}
	}
	return w - data;
}

#endif

static gboolean
save_to_file (TextEditor * te, gchar * fn)
{
	FILE *fp;
	guint nchars;
	gint strip;
	gchar *data;

	fp = fopen (fn, "wb");
	if (!fp)
		return FALSE;
	strip = prop_get_int (te->props_base, "strip.trailing.spaces", 1);
	nchars =
		scintilla_send_message (SCINTILLA (te->widgets.editor),
					SCI_GETLENGTH, 0, 0);
	data =
		(gchar *) aneditor_command (te->editor_id, ANE_GETTEXTRANGE,
					    0, nchars);
	if (data)
	{
		size_t size;

		size = strlen (data);
		if (size != nchars)
			g_warning
				("Text length and no. of bytes saved is not equal");
		size = fwrite (data, size, 1, fp);
		g_free (data);
	}
	if (ferror (fp))
	{
		fclose (fp);
		return FALSE;
	}
	fclose (fp);
	return TRUE;
}

gboolean
text_editor_load_file (TextEditor * te)
{
	gint tags_update;

	if (te == NULL || te->filename == NULL)
		return FALSE;
	if (IS_SCINTILLA (te->widgets.editor) == FALSE)
		return FALSE;
	anjuta_status (_("Loading File ..."));
	text_editor_freeze (te);
	anjuta_set_busy ();
	te->modified_time = time (NULL);
	if (load_from_file (te, te->full_filename) == FALSE)
	{
		anjuta_system_error (errno, _("Could not load file: %s"), te->full_filename);
		anjuta_set_active ();
		text_editor_thaw (te);
		return FALSE;
	}
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_GOTOPOS,
				0, 0);
	tags_update = preferences_get_int (te->preferences, AUTOMATIC_TAGS_UPDATE);
	if (app->project_dbase->project_is_open == FALSE && tags_update)
	{
		tags_manager_update (app->tags_manager, te->full_filename);
	}
	else if (project_dbase_is_file_in_module
		    (app->project_dbase, MODULE_SOURCE, te->full_filename) && tags_update)
	{
		tags_manager_update (app->tags_manager, te->full_filename);
	}
	anjuta_set_active ();
	text_editor_thaw (te);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
				SCI_SETSAVEPOINT, 0, 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
				SCI_EMPTYUNDOBUFFER, 0, 0);
	text_editor_set_hilite_type (te);
	if (preferences_get_int (te->preferences, FOLD_ON_OPEN))
	{
		aneditor_command (te->editor_id, ANE_CLOSE_FOLDALL, 0, 0);
	}
	anjuta_status (_("File Loaded Successfully"));
	return TRUE;
}

gboolean
text_editor_save_file (TextEditor * te)
{
	gint tags_update;

	if (te == NULL)
		return FALSE;
	if (IS_SCINTILLA (te->widgets.editor) == FALSE)
		return FALSE;
	anjuta_set_busy ();
	text_editor_freeze (te);
	anjuta_status (_("Saving File ..."));
	if (save_to_file (te, te->full_filename) == FALSE)
	{
		text_editor_thaw (te);
		anjuta_set_active ();
		anjuta_system_error (errno, _("Couldn't save file: %s."), te->full_filename);
		return FALSE;
	}
	else
	{
		te->modified_time = time (NULL);
		tags_update =
			preferences_get_int (te->preferences,
					     AUTOMATIC_TAGS_UPDATE);
		if (app->project_dbase->project_is_open == FALSE
		    && tags_update)
			tags_manager_update (app->tags_manager,
					     te->full_filename);
		else
			if (project_dbase_is_file_in_module
			    (app->project_dbase, MODULE_SOURCE, te->full_filename)
			    && tags_update)
			tags_manager_update (app->tags_manager,
					     te->full_filename);
		/* we need to update UI with the call to scintilla */
		text_editor_thaw (te);
		scintilla_send_message (SCINTILLA (te->widgets.editor),
					SCI_SETSAVEPOINT, 0, 0);
		anjuta_set_active ();
		anjuta_status (_("File Saved successfully"));
		return TRUE;
	}
}

gboolean
text_editor_save_yourself (TextEditor * te, FILE * stream)
{
	return TRUE;
}

gboolean
text_editor_recover_yourself (TextEditor * te, FILE * stream)
{
	return TRUE;
}

void
text_editor_update_preferences (TextEditor * te)
{
	Preferences *pr;
	gboolean auto_save;
	guint auto_save_timer;

	auto_save = preferences_get_int (te->preferences, SAVE_AUTOMATIC);
	auto_save_timer =
		preferences_get_int (te->preferences, AUTOSAVE_TIMER);

	g_return_if_fail (te != NULL);
	pr = te->preferences;

	if (auto_save)
	{
		if (te->autosave_on == TRUE)
		{
			if (auto_save_timer != te->autosave_it)
			{
				gtk_timeout_remove (te->autosave_id);
				te->autosave_id =
					gtk_timeout_add (auto_save_timer *
							 60000,
							 on_text_editor_auto_save,
							 te);
			}
		}
		else
		{
			te->autosave_id =
				gtk_timeout_add (auto_save_timer * 60000,
						 on_text_editor_auto_save,
						 te);
		}
		te->autosave_it = auto_save_timer;
		te->autosave_on = TRUE;
	}
	else
	{
		if (te->autosave_on == TRUE)
			gtk_timeout_remove (te->autosave_id);
		te->autosave_on = FALSE;
	}
	text_editor_set_hilite_type (te);
	text_editor_set_zoom_factor(te, preferences_get_int (te->preferences, "text.zoom.factor"));
}

gboolean
text_editor_check_disk_status (TextEditor * te)
{
	struct stat status;
	gchar *buff;
	if (!te)
		return FALSE;
	if (stat (te->full_filename, &status))
		return TRUE;
	if (te->modified_time < status.st_mtime)
	{
		buff =
			g_strdup_printf (_
					 ("WARNING: The file \"%s\" in the disk is more recent "
					  "than\nthe current buffer.\nDo you want to reload it."),
te->filename);
		messagebox2 (GNOME_MESSAGE_BOX_WARNING, buff,
			     GNOME_STOCK_BUTTON_YES,
			     GNOME_STOCK_BUTTON_NO,
			     GTK_SIGNAL_FUNC
			     (on_text_editor_check_yes_clicked),
			     GTK_SIGNAL_FUNC
			     (on_text_editor_check_no_clicked), te);
		return FALSE;
	}
	return TRUE;
}

void
text_editor_undo (TextEditor * te)
{
	scintilla_send_message (SCINTILLA (te->widgets.editor),
				SCI_UNDO, 0, 0);
}

void
text_editor_redo (TextEditor * te)
{
	scintilla_send_message (SCINTILLA (te->widgets.editor),
				SCI_REDO, 0, 0);
}

void
text_editor_update_controls (TextEditor * te)
{
	gboolean F, P, L, C, S;

	if (te == NULL)
		return;
	S = text_editor_is_saved (te);
	L = launcher_is_busy ();
	P = app->project_dbase->project_is_open;

	switch (get_file_ext_type (te->filename))
	{
	case FILE_TYPE_C:
	case FILE_TYPE_CPP:
	case FILE_TYPE_ASM:
		F = TRUE;
		break;
	default:
		F = FALSE;
	}
	switch (get_file_ext_type (te->filename))
	{
	case FILE_TYPE_C:
	case FILE_TYPE_CPP:
	case FILE_TYPE_HEADER:
		C = TRUE;
		break;
	default:
		C = FALSE;
	}
	gtk_widget_set_sensitive (te->buttons.save, !S);
	gtk_widget_set_sensitive (te->buttons.reload,
				  (te->full_filename != NULL));
	gtk_widget_set_sensitive (te->buttons.compile, F && !L);
	gtk_widget_set_sensitive (te->buttons.build, (F || P) && !L);
}

gboolean
text_editor_is_saved (TextEditor * te)
{
	return !(scintilla_send_message (SCINTILLA (te->widgets.editor),
					 SCI_GETMODIFY, 0, 0));
}

gboolean
text_editor_has_selection (TextEditor * te)
{
	gint from, to;
	from = scintilla_send_message (SCINTILLA (te->widgets.editor),
				       SCI_GETSELECTIONSTART, 0, 0);
	to = scintilla_send_message (SCINTILLA (te->widgets.editor),
				     SCI_GETSELECTIONEND, 0, 0);
	return (from == to) ? FALSE : TRUE;
}

gchar*
text_editor_get_selection (TextEditor * te)
{
	guint from, to;
	struct TextRange tr;

	from = scintilla_send_message (SCINTILLA (te->widgets.editor),
				       SCI_GETSELECTIONSTART, 0, 0);
	to = scintilla_send_message (SCINTILLA (te->widgets.editor),
				     SCI_GETSELECTIONEND, 0, 0);
	if (from == to)
		return NULL;
	tr.chrg.cpMin = MIN(from, to);
	tr.chrg.cpMax = MAX(from, to);
	tr.lpstrText = g_malloc (sizeof(gchar)*(tr.chrg.cpMax-tr.chrg.cpMin)+5);
	scintilla_send_message (SCINTILLA(te->widgets.editor), SCI_GETTEXTRANGE, 0, (long)(&tr));
	return tr.lpstrText;
}

void
text_editor_autoformat (TextEditor * te)
{
	gchar *cmd, *file, *fopts, *shell;
	pid_t pid;
	int status;

	if (anjuta_is_installed ("indent", TRUE) == FALSE)
		return;
	if (preferences_get_int (app->preferences, AUTOFORMAT_DISABLE))
	{
		anjuta_warning (_("Autoformat is disablde. If you want, enable in Preferences."));
		return;
	}
	if (te == NULL)
		return;

	anjuta_status (_("Auto formating ..."));
	anjuta_set_busy ();
	text_editor_freeze (te);
	file = get_a_tmp_file ();
	if (save_to_file (te, file) == FALSE)
	{
		remove (file);
		g_free (file);
		text_editor_thaw (te);
		anjuta_set_active ();
		anjuta_warning (_("Error in auto formating ..."));
		return;
	}

	fopts = preferences_get_format_opts (te->preferences);
	cmd = g_strconcat ("indent ", fopts, " ", file, NULL);
	g_free (fopts);

	/* This does not work, because SIGCHILD is not emitted.
	 * pid = gnome_execute_shell(app->dirs->tmp, cmd);
	 * So using fork instead.
	 */
	shell = gnome_util_user_shell ();
	if ((pid = fork ()) == 0)
	{
		execlp (shell, shell, "-c", cmd, NULL);
		g_error (_("Cannot execute command shell"));
	}
	g_free (cmd);

	waitpid (pid, &status, 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
				SCI_GETCURRENTPOS, 0, 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
				SCI_BEGINUNDOACTION, 0, 0);
	if (load_from_file (te, file) == FALSE)
	{
		anjuta_warning (_("Error in auto formating ..."));
	}
	else
	{
		text_editor_set_hilite_type (te);
		gtk_widget_queue_draw (te->widgets.editor);
		anjuta_status (_("Auto formating completed"));
	}
	remove (file);
	g_free (file);
	anjuta_set_active ();
	text_editor_thaw (te);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
				SCI_ENDUNDOACTION, 0, 0);
	main_toolbar_update ();
	update_main_menubar ();
	anjuta_refresh_breakpoints(te);
}

gboolean
text_editor_is_marker_set (TextEditor* te, gint line, gint marker)
{
	gint state;

	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail (line >= 0, FALSE);
	g_return_val_if_fail (marker < 32, FALSE);
	
	state = scintilla_send_message (SCINTILLA(te->widgets.editor),
		SCI_MARKERGET, linenum_text_editor_to_scintilla (line), 0);
	return ((state & (1 << marker)));
}

void
text_editor_delete_marker (TextEditor* te, gint line, gint marker)
{
	g_return_if_fail (te != NULL);
	g_return_if_fail (line >= 0);
	g_return_if_fail (marker < 32);
	
	scintilla_send_message (SCINTILLA(te->widgets.editor),
		SCI_MARKERDELETE, linenum_text_editor_to_scintilla (line), marker);
}
gint
text_editor_line_from_handle (TextEditor* te, gint marker_handle)
{
	gint line;
	
	g_return_val_if_fail (te != NULL, -1);
	
	line = scintilla_send_message (SCINTILLA(te->widgets.editor),
		SCI_MARKERLINEFROMHANDLE, marker_handle, 0);
	
	return linenum_scintilla_to_text_editor (line);
}

