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
#include "launcher.h"
#include "pixmaps.h"

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
#include "anjuta-encodings.h"

/* Debug flag */
// #define DEBUG

/* Marker 0 is defined for bookmarks */
#define TEXT_EDITOR_LINEMARKER	2

#define MARKER_PROP_END 0xaaaaaa /* Do not define any color with this value */

/* marker fore and back colors */
static glong marker_prop[] = 
{
	SC_MARK_ROUNDRECT, 0x00007f, 0xffff80,			/* Bookmark */
	SC_MARK_CIRCLE, 0x00007f, 0xff80ff,		/* Breakpoint mark */
	SC_MARK_SHORTARROW, 0x00007f, 0x00ffff,	/* Line mark */
	MARKER_PROP_END,	/* end */
};

static void check_tm_file(TextEditor *te)
{
	if (NULL == te->tm_file)
	{
		te->tm_file = tm_workspace_find_object(
		  TM_WORK_OBJECT(app->tm_workspace), te->full_filename, FALSE);
		if (NULL == te->tm_file)
		{
#ifdef DEBUG
			g_message("File %s not found in TM workspace", te->full_filename);
#endif
			te->tm_file = tm_source_file_new(te->full_filename, TRUE);
			if (NULL != te->tm_file)
				tm_workspace_add_object(te->tm_file);
		}
	}
}

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

static void text_editor_update_preferences (AnjutaPreferences *pr,
											TextEditor * te);

TextEditor *
text_editor_new (gchar * filename, TextEditor * parent, AnjutaPreferences * eo,
                 gchar *name)
{
	gchar *buff;
	TextEditor *te;
	static guint new_file_count;

	te = (TextEditor *) g_malloc0(sizeof (TextEditor));
	if (te == NULL)
		return NULL;
	te->size	= sizeof(TextEditor);
	if (name)  
		te->filename = g_strdup(name); 
	else 
		te->filename = g_strdup_printf ("Newfile#%d", ++new_file_count);
	te->full_filename = NULL;
	te->tm_file = NULL;
	
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
	te->used_by_cvs = FALSE;
	te->encoding = NULL;
	
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

	/* Editor is created here and not in create_text_editor_gui() */
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
		te->filename = g_strdup(extract_filename(filename));
		te->full_filename = tm_get_real_path(filename);
		buff = g_strdup_printf ("Anjuta: %s", te->full_filename);
		gtk_window_set_title (GTK_WINDOW (te->widgets.window), buff);
		g_free (buff);
		if (text_editor_load_file (te) == FALSE)
		{
			/* Unable to load file */
			text_editor_destroy (te);
			return NULL;
		}
	}
	te->menu = text_editor_menu_new ();
	text_editor_update_preferences (te->preferences, te);
	text_editor_update_controls (te);
	te->changed_id = g_signal_connect (G_OBJECT (te->preferences), "changed",
					  G_CALLBACK (text_editor_update_preferences), te);
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
	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (te->preferences),
									DISABLE_SYNTAX_HILIGHTING))
		te->force_hilite = TE_LEXER_NONE;
	else if (te->force_hilite == TE_LEXER_NONE)
		te->force_hilite = TE_LEXER_AUTOMATIC;
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

		if (te->widgets.window)
			gtk_widget_destroy (te->widgets.window);
		
		gtk_widget_unref (te->widgets.window);
		gtk_widget_unref (te->widgets.client_area);
		gtk_widget_unref (te->widgets.client);
#warning "G2: Strange we are missing one _unref() somewhere"
		// gtk_widget_unref (te->widgets.editor);
		gtk_widget_unref (te->widgets.line_label);
		
		gtk_widget_unref (te->buttons.novus);
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
		
		if (te->buttons.close)
			text_editor_tab_widget_destroy(te);
		
		if (te->filename)
			g_free (te->filename);
		if (te->full_filename)
			g_free (te->full_filename);
		if (te->tm_file)
			if (te->tm_file->parent == TM_WORK_OBJECT(app->tm_workspace))
				tm_workspace_remove_object(te->tm_file, TRUE);
		if (te->menu)
			text_editor_menu_destroy (te->menu);
		if (te->editor_id)
			aneditor_destroy (te->editor_id);
		if (te->used_by_cvs)
			cvs_set_editor_destroyed (app->cvs);
		if (te->changed_id)
			g_signal_handler_disconnect (G_OBJECT (te->preferences), 
										 te->changed_id);
		if (te->encoding)
			g_free (te->encoding);
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
	text_editor_tab_widget_destroy(te);
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
text_editor_find (TextEditor * te, const gchar * str, gint scope,
				  gboolean forward, gboolean regexp,
				  gboolean ignore_case, gboolean whole_word, gboolean wrap)
{
	glong ret;
	GtkWidget *editor;
	glong flags;
	int current_pos, current_anchor;
	
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
			break;
	}
	current_pos = scintilla_send_message (SCINTILLA (editor), SCI_GETCURRENTPOS, 0, 0);
	current_anchor = scintilla_send_message (SCINTILLA (editor), SCI_GETANCHOR, 0, 0);
	ret = aneditor_command (te->editor_id, ANE_FIND, flags, (long)str);
	if (scope == TEXT_EDITOR_FIND_SCOPE_CURRENT && wrap && ret < 0) {
		/* If wrap is requested, wrap it. */
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
		ret = aneditor_command (te->editor_id, ANE_FIND, flags, (long)str);
		/* If the text is still not found, restore current pos and anchor */
		if (ret < 0) {
			scintilla_send_message (SCINTILLA (editor), SCI_SETANCHOR, current_anchor, 0);
			scintilla_send_message (SCINTILLA (editor), SCI_SETCURRENTPOS, current_pos, 0);
		}
	}
	return ret;
}

void
text_editor_replace_selection (TextEditor * te, const gchar* r_str)
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

guint
text_editor_get_line_from_position (TextEditor * te, glong pos)
{
	guint count;
	
	g_return_val_if_fail (te != NULL, 0);

	count =	scintilla_send_message (SCINTILLA (te->widgets.editor),
					SCI_LINEFROMPOSITION, pos, 0);
	return linenum_scintilla_to_text_editor(count);
}

glong
text_editor_get_current_position (TextEditor * te)
{
	guint count;
	
	g_return_val_if_fail (te != NULL, 0);

	count =	scintilla_send_message (SCINTILLA (te->widgets.editor),
					SCI_GETCURRENTPOS, 0, 0);
	return count;
}

gboolean
text_editor_goto_point (TextEditor * te, glong point)
{
	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail(IS_SCINTILLA (te->widgets.editor) == TRUE, FALSE);

	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_GOTOPOS,
				point, 0);
	return TRUE;
}

gboolean
text_editor_goto_line (TextEditor * te, glong line,
					   gboolean mark, gboolean ensure_visible)
{
	gint selpos;
	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail(IS_SCINTILLA (te->widgets.editor) == TRUE, FALSE);
	g_return_val_if_fail(line >= 0, FALSE);

	te->current_line = line;
	if (mark) text_editor_set_line_marker (te, line);
	if (ensure_visible)
		scintilla_send_message (SCINTILLA (te->widgets.editor),
								SCI_ENSUREVISIBLE,
								linenum_text_editor_to_scintilla (line), 0);
	selpos = scintilla_send_message(SCINTILLA (te->widgets.editor),
									SCI_POSITIONFROMLINE,
								linenum_text_editor_to_scintilla (line), 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
							SCI_SETSELECTIONSTART, selpos, 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
							SCI_SETSELECTIONEND, selpos, 0);
	
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
	if (line >= 0) text_editor_goto_line (te, line, TRUE, TRUE);
	else gdk_beep();
	return line;
}

gint
text_editor_goto_block_end (TextEditor* te)
{
	gint line;
	line = aneditor_command (te->editor_id, ANE_GETBLOCKENDLINE, 0, 0);
	if (line >= 0) text_editor_goto_line (te, line, TRUE, TRUE);
	else gdk_beep();
	return line;
}

gint
text_editor_set_marker (TextEditor *te, glong line, gint marker)
{
	g_return_val_if_fail (te != NULL, -1);
	g_return_val_if_fail(IS_SCINTILLA (te->widgets.editor) == TRUE, -1);

	/* Scintilla interprets line+1 rather than line */
	/* A bug perhaps */
	/* So counterbalance it with line-1 */
	/* Using the macros linenum_* */
	return scintilla_send_message (SCINTILLA (te->widgets.editor),
								   SCI_MARKERADD,
								   linenum_text_editor_to_scintilla (line),
								   marker);
}

gint
text_editor_set_indicator (TextEditor *te, glong line, gint indicator)
{
	glong start, end;
	gchar ch;
	glong indic_mask[] = {INDIC0_MASK, INDIC1_MASK, INDIC2_MASK};
	
	g_return_val_if_fail (te != NULL, -1);
	g_return_val_if_fail (IS_SCINTILLA (te->widgets.editor) == TRUE, -1);

	if (line > 0) {
		line = linenum_text_editor_to_scintilla (line);
		start = scintilla_send_message (SCINTILLA (te->widgets.editor),
										SCI_POSITIONFROMLINE, line, 0);
		end = scintilla_send_message (SCINTILLA (te->widgets.editor),
									  SCI_POSITIONFROMLINE, line+1, 0) - 1;
	
		if (end <= start)
			return -1;

		do
		{
			ch = scintilla_send_message (SCINTILLA (te->widgets.editor),
										 SCI_GETCHARAT, start, 0);
			start++;
		} while (isspace(ch));
		start--;
		
		do {
			ch = scintilla_send_message (SCINTILLA (te->widgets.editor),
										 SCI_GETCHARAT, end, 0);
			end--;
		} while (isspace(ch));
		end++;
		if (end < start) return -1;
		
		if (indicator >= 0 && indicator < 3) {
			char current_mask;
			current_mask =
				scintilla_send_message (SCINTILLA (te->widgets.editor),
										SCI_GETSTYLEAT, start, 0);
			current_mask &= INDICS_MASK;
			current_mask |= indic_mask[indicator];
			scintilla_send_message (SCINTILLA (te->widgets.editor),
									SCI_STARTSTYLING, start, INDICS_MASK);
			scintilla_send_message (SCINTILLA (te->widgets.editor),
									SCI_SETSTYLING, end-start+1, current_mask);
		} else {
			scintilla_send_message (SCINTILLA (te->widgets.editor),
									SCI_STARTSTYLING, start, INDICS_MASK);
			scintilla_send_message (SCINTILLA (te->widgets.editor),
									SCI_SETSTYLING, end-start+1, 0);
		}
	} else {
		if (indicator < 0) {
			glong last;
			last = scintilla_send_message (SCINTILLA (te->widgets.editor),
										   SCI_GETTEXTLENGTH, 0, 0);
			if (last > 0) {
				scintilla_send_message (SCINTILLA (te->widgets.editor),
										SCI_STARTSTYLING, 0, INDICS_MASK);
				scintilla_send_message (SCINTILLA (te->widgets.editor),
										SCI_SETSTYLING, last, 0);
			}
		}
	}
	return 0;
}

void
text_editor_set_line_marker (TextEditor *te, glong line)
{
	g_return_if_fail (te != NULL);
	g_return_if_fail(IS_SCINTILLA (te->widgets.editor) == TRUE);

	anjuta_delete_all_marker (TEXT_EDITOR_LINEMARKER);
	text_editor_set_marker (te, line, TEXT_EDITOR_LINEMARKER);
}

/* Support for DOS-Files
 *
 * On load, Anjuta will detect DOS-files by finding <CR><LF>. 
 * Anjuta will translate some chars >= 128 to native charset.
 * On save the DOS_EOL_CHECK(preferences->editor) will be checked
 * and chars >=128 will be replaced by DOS-codes, if any translation 
 * match(see struct tr_dos in this file) and <CR><LF> will used 
 * instead of <LF>.
 * The DOS_EOL_CHECK-checkbox will be set on loading a DOS-file.
 *
 *  23.Sep.2001  Denis Boehme <boehme at syncio dot de>
 */

/*
 * this is a translation table from unix->dos 
 * this table will be used by filter_chars and save filtered.
 */
static struct {
	unsigned char c; /* unix-char */
	unsigned char b; /* dos-char */
} tr_dos[]= {
	{ 'ä', 0x84 },
	{ 'Ä', 0x8e },
	{ 'ß', 0xe1 },
	{ 'ü', 0x81 },
	{ 'Ü', 0x9a },
	{ 'ö', 0x94 },
	{ 'Ö', 0x99 },
	{ 'é', 0x82 },
	{ 'É', 0x90 },
	{ 'è', 0x9a },
	{ 'È', 0xd4 },
	{ 'ê', 0x88 },
	{ 'Ê', 0xd2 },
	{ 'á', 0xa0 },
	{ 'Á', 0xb5 },
	{ 'à', 0x85 },
	{ 'À', 0xb7 },
	{ 'â', 0x83 },
	{ 'Â', 0xb6 },
	{ 'ú', 0xa3 },
	{ 'Ú', 0xe9 },
	{ 'ù', 0x97 },
	{ 'Ù', 0xeb },
	{ 'û', 0x96 },
	{ 'Û', 0xea }
};

/*
 * filter chars in buffer, by using tr_dos-table. 
 *
 */
static size_t
filter_chars_in_dos_mode(gchar *data_, size_t size )
{
	int k;
	size_t i;
	unsigned char * data = data_;
	unsigned char * tr_map;

	tr_map = (unsigned char *)malloc( 256 );
	memset( tr_map, 0, 256 );
	for ( k = 0; k < sizeof(tr_dos )/2 ; k++ )
		tr_map[tr_dos[k].b] = tr_dos[k].c;

	for ( i = 0; i < size; i++ )
	{
      		if ( (data[i] >= 128) && ( tr_map[data[i]] != 0) )
			data[i] = tr_map[data[i]];;
	}

	if ( tr_map )
		free( tr_map );

	return size;
}

/*
 * save buffer. filter chars and set dos-like CR/LF if dos_text is set.
 */
static size_t
save_filtered_in_dos_mode(FILE * f, gchar *data_, size_t size)
{
	size_t i, j;
	unsigned char *data;
	unsigned char *tr_map;
	int k;

	/* build the translation table */
	tr_map = malloc( 256 );
	memset( tr_map, 0, 256 );
	for ( k = 0; k < sizeof(tr_dos)/2; k++)
	  tr_map[tr_dos[k].c] = tr_dos[k].b;

	data = data_;
	i = 0; j = 0;
	while ( i < size )
	{
		if (data[i]>=128) {
			/* convert dos-text */
			if ( tr_map[data[i]] != 0 )
				j += fwrite( &tr_map[data[i]], 1, 1, f );
			else
				/* char not found, skip transform */
				j += fwrite( &data[i], 1, 1, f );
			i++;
		} else {
			/* write normal chars */
			j += fwrite( &data[i], 1, 1, f );
			i++;
		}
	}

	if ( tr_map )
		free(tr_map);

//	printf( "size: %d, written: %d\n", size, j );
	return size;
}

static gint
determine_editor_mode(gchar* buffer, glong size)
{
	gint i;
	guint cr, lf, crlf, max_mode;
	gint mode;
	
	cr = lf = crlf = 0;
	
	for ( i = 0; i < size ; i++ )
	{
		if ( buffer[i] == 0x0a ){
			// LF
			// mode = SC_EOF_LF;
			lf++;
		} else if ( buffer[i] == 0x0d ) {
			if (i >= (size-1)) {
				// Last char
				// CR
				// mode = SC_EOL_CR;
				cr++;
			} else {
				if (buffer[i+1] != 0x0a) {
					// CR
					// mode = SC_EOL_CR;
					cr++;
				} else {
					// CRLF
					// mode = SC_EOL_CRLF;
					crlf++;
				}
				i++;
			}
		}
	}

	/* Vote for the maximum */
	mode = SC_EOL_LF;
	max_mode = lf;
	if (crlf > max_mode) {
		mode = SC_EOL_CRLF;
		max_mode = crlf;
	}
	if (cr > max_mode) {
		mode = SC_EOL_CR;
		max_mode = cr;
	}
#ifdef DEBUG
	g_message("EOL chars: LR = %d, CR = %d, CRLF = %d", lf, cr, crlf);
	g_message("Autodetected Editor mode [%d]", mode);
#endif
	return mode;
}

static gchar *
convert_to_utf8_from_charset (const gchar *content,
							  gsize len,
							  const gchar *charset)
{
	gchar *utf8_content = NULL;
	GError *conv_error = NULL;
	gchar* converted_contents = NULL;
	gsize bytes_written;

	g_return_val_if_fail (content != NULL, NULL);

#ifdef DEBUG
	g_message ("Trying to convert from %s to UTF-8", charset);
#endif
	converted_contents = g_convert (content, len, "UTF-8",
									charset, NULL, &bytes_written,
									&conv_error); 
						
	if ((conv_error != NULL) || 
	    !g_utf8_validate (converted_contents, bytes_written, NULL))		
	{
#ifdef DEBUG
		g_message ("Couldn't convert from %s to UTF-8.", charset);
#endif
		if (converted_contents != NULL)
			g_free (converted_contents);

		if (conv_error != NULL)
		{
			g_error_free (conv_error);
			conv_error = NULL;
		}

		utf8_content = NULL;
	} else {
#ifdef DEBUG
		g_message ("Converted from %s to UTF-8.", charset);
#endif
		utf8_content = converted_contents;
	}
	return utf8_content;
}

static gchar *
convert_to_utf8 (const gchar *content, gsize len,
			     gchar **encoding_used)
{
	GList *encodings = NULL;
	GList *start;
	const gchar *locale_charset;
	GList *encoding_strings;
	
	g_return_val_if_fail (!g_utf8_validate (content, len, NULL), 
			g_strndup (content, len < 0 ? strlen (content) : len));

	encoding_strings = glist_from_data (app->preferences->props,
										SUPPORTED_ENCODINGS);
	encodings = anjuta_encoding_get_encodings (encoding_strings);
	glist_strings_free (encoding_strings);
	
	if (g_get_charset (&locale_charset) == FALSE) 
	{
		const AnjutaEncoding *locale_encoding;

		/* not using a UTF-8 locale, so try converting
		 * from that first */
		if (locale_charset != NULL)
		{
			locale_encoding = anjuta_encoding_get_from_charset (locale_charset);
			encodings = g_list_prepend (encodings,
						(gpointer) locale_encoding);
#ifdef DEBUG
			g_message ("Current charset = %s", locale_charset);
			/* g_message ("Current encoding = %s", locale_encoding); */
#endif
		}
	}

	start = encodings;

	while (encodings != NULL) 
	{
		AnjutaEncoding *enc;
		const gchar *charset;
		gchar *utf8_content;

		enc = (AnjutaEncoding *) encodings->data;

		charset = anjuta_encoding_get_charset (enc);
#ifdef DEBUG
		g_message ("Trying to convert %d bytes of data into UTF-8.", len);
#endif
		fflush (stdout);
		utf8_content = convert_to_utf8_from_charset (content, len, charset);

		if (utf8_content != NULL) {
			if (encoding_used != NULL)
			{
				if (*encoding_used != NULL)
					g_free (*encoding_used);
				
				*encoding_used = g_strdup (charset);
			}

			return utf8_content;
		}

		encodings = encodings->next;
	}

	g_list_free (start);
	
	return NULL;
}

static gboolean
load_from_file (TextEditor *te, gchar *fn, gchar **err)
{
	FILE *fp;
	gchar *buffer;
	gint nchars, dos_filter, editor_mode;
	struct stat st;
	size_t size;

	if (stat (fn, &st) != 0)
	{
		g_warning ("Could not stat the file %s", fn);
		*err = g_strdup (g_strerror (errno));
		return FALSE;
	}
	size = st.st_size;
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_CLEARALL,
							0, 0);
	buffer = g_malloc (size);
	if (buffer == NULL && size != 0)
	{
		/* This is funny in linux, but never hurts */
		g_warning ("This file is too big. Unable to allocate memory.");
		*err = g_strdup (_("This file is too big. Unable to allocate memory."));
		return FALSE;
	}
	fp = fopen (fn, "rb");
	if (!fp)
	{
		g_free (buffer);
		*err = g_strdup (g_strerror (errno));
		return FALSE;
	}
	/* Crude way of loading, but faster */
	nchars = fread (buffer, 1, size, fp);
	
	if (size != nchars) g_warning ("File size and loaded size not matching");
	dos_filter = 
		anjuta_preferences_get_int (ANJUTA_PREFERENCES (te->preferences),
									DOS_EOL_CHECK);
	
	/* Set editor mode */
	editor_mode =  determine_editor_mode (buffer, nchars);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
							SCI_SETEOLMODE, editor_mode, 0);
#ifdef DEBUG
	g_message("Loaded in editor mode [%d]", editor_mode);
#endif

	/* Determine character encoding and convert to utf-8*/
	if (nchars > 0)
	{
		if (g_utf8_validate (buffer, nchars, NULL))
		{
			if (te->encoding)
				g_free (te->encoding);
			te->encoding = NULL;
		}
		else
		{
			gchar *converted_text;
	
			converted_text = convert_to_utf8 (buffer, nchars, &te->encoding);

			if (converted_text == NULL)
			{
				/* bail out */
				g_free (buffer);
				*err = g_strdup (_("The file does not look like a text file or the file encoding is not supported."
								   " Please check if the encoding of file is in the supported encodings list."
								   " If not, add it from the preferences."));
				fclose (fp);
				return FALSE;
			}
			g_free (buffer);
			buffer = converted_text;
			nchars = strlen (converted_text);
		}
	}
	if (dos_filter && editor_mode == SC_EOL_CRLF){
#ifdef DEBUG
		g_message("Filtering Extrageneous DOS characters in dos mode [Dos => Unix]");
#endif
		nchars = filter_chars_in_dos_mode( buffer, nchars );
	}
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_ADDTEXT,
							nchars, (long) buffer);
	
	g_free (buffer);
	if (ferror (fp))
	{
		fclose (fp);
		*err = g_strdup (g_strerror (errno));
		return FALSE;
	}
	fclose (fp);
	return TRUE;
}

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
	nchars = scintilla_send_message (SCINTILLA (te->widgets.editor),
									 SCI_GETLENGTH, 0, 0);
	data =	(gchar *) aneditor_command (te->editor_id,
										ANE_GETTEXTRANGE, 0, nchars);
	if (data)
	{
		size_t size;
		gint dos_filter, editor_mode;
		
		size = strlen (data);
		
		/* Save according to the correct encoding */
		if (anjuta_preferences_get_int (app->preferences,
										SAVE_ENCODING_CURRENT_LOCALE))
		{
			/* Save in current locate */
			GError *conv_error = NULL;
			gchar* converted_file_contents = NULL;
#ifdef DEBUG
			g_message ("Using current locale's encoding");
#endif
			converted_file_contents = g_locale_from_utf8 (data, -1, NULL,
														  NULL, &conv_error);
	
			if (conv_error != NULL)
			{
				/* Conversion error */
				g_error_free (conv_error);
			}
			else
			{
				g_free (data);
				data = converted_file_contents;
				size = strlen (converted_file_contents);
			}
		}
		else
		{
			/* Save in original encoding */
			if ((te->encoding != NULL) &&
				anjuta_preferences_get_int (te->preferences,
											 SAVE_ENCODING_ORIGINAL))
			{
				GError *conv_error = NULL;
				gchar* converted_file_contents = NULL;
	
#ifdef DEBUG
				g_message ("Using encoding %s", te->encoding);
#endif	
				/* Try to convert it from UTF-8 to original encoding */
				converted_file_contents = g_convert (data, -1, 
													 te->encoding,
													 "UTF-8", NULL, NULL,
													 &conv_error); 
	
				if (conv_error != NULL)
				{
					/* Conversion error */
					g_error_free (conv_error);
				}
				else
				{
					g_free (data);
					data = converted_file_contents;
					size = strlen (converted_file_contents);
				}
			}
			else
			{
				/* Save in utf-8 */
#ifdef DEBUG
				g_message ("Using utf-8 encoding");
#endif
			}				
		}
		
		/* Strip trailing spaces */
		strip = prop_get_int (te->props_base, STRIP_TRAILING_SPACES, 0);
		if (strip)
		{
			while (size > 0 && isspace(data[size - 1]))
				-- size;
		}
		if ((size > 1) && ('\n' != data[size-1]))
		{
			data[size] = '\n';
			++ size;
		}
		dos_filter = anjuta_preferences_get_int (te->preferences,
												 DOS_EOL_CHECK);
		editor_mode =  scintilla_send_message (SCINTILLA (te->widgets.editor),
											   SCI_GETEOLMODE, 0, 0);
#ifdef DEBUG
		g_message("Saving in editor mode [%d]", editor_mode);
#endif
		nchars = size;
		if (editor_mode == SC_EOL_CRLF && dos_filter)
		{
#ifdef DEBUG
			g_message("Filtering Extrageneous DOS characters in dos mode [Unix => Dos]");
#endif
			size = save_filtered_in_dos_mode( fp, data, size);
		}
		else
		{
			size = fwrite (data, size, 1, fp);
		}
		g_free (data);
		/* FIXME: Find a nice way to check that all the bytes have been written */
		/* if (size != nchars)
			g_warning("Text length and number of bytes saved is not equal [%d != %d]", nchars, size);
		*/
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
	gchar *err = NULL;
	
	if (te == NULL || te->filename == NULL)
		return FALSE;
	if (IS_SCINTILLA (te->widgets.editor) == FALSE)
		return FALSE;
	anjuta_status (_("Loading file ..."));
	text_editor_freeze (te);
	anjuta_set_busy ();
	te->modified_time = time (NULL);
	if (load_from_file (te, te->full_filename, &err) == FALSE)
	{
		anjuta_error (_("Could not load file: %s\n\nDetails: %s"),
					  te->filename, err);
		g_free (err);
		anjuta_set_active ();
		text_editor_thaw (te);
		return FALSE;
	}
	scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_GOTOPOS,
							0, 0);
	check_tm_file(te);
	anjuta_set_active ();
	text_editor_thaw (te);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
							SCI_SETSAVEPOINT, 0, 0);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
							SCI_EMPTYUNDOBUFFER, 0, 0);
	text_editor_set_hilite_type (te);
	if (anjuta_preferences_get_int (te->preferences, FOLD_ON_OPEN))
	{
		aneditor_command (te->editor_id, ANE_CLOSE_FOLDALL, 0, 0);
	}
	anjuta_status (_("File loaded successfully"));
	return TRUE;
}

gboolean
text_editor_save_file (TextEditor * te, gboolean update)
{
	gint tags_update;

	if (te == NULL)
		return FALSE;
	if (IS_SCINTILLA (te->widgets.editor) == FALSE)
		return FALSE;
	anjuta_set_busy ();
	text_editor_freeze (te);
	anjuta_status (_("Saving file ..."));
	if (save_to_file (te, te->full_filename) == FALSE)
	{
		text_editor_thaw (te);
		anjuta_set_active ();
		anjuta_system_error (errno, _("Could not save file: %s."), te->full_filename);
		return FALSE;
	}
	else
	{
		te->modified_time = time (NULL);
		tags_update =
			anjuta_preferences_get_int (te->preferences,
					     AUTOMATIC_TAGS_UPDATE);
		if (tags_update)
		{
			if ((app->project_dbase->project_is_open == FALSE)
			  || (project_dbase_is_file_in_module
			    (app->project_dbase, MODULE_SOURCE, te->full_filename))
			  || (project_dbase_is_file_in_module
			    (app->project_dbase, MODULE_INCLUDE, te->full_filename)))
			{
				check_tm_file(te);
				if (te->tm_file)
				{
					GList *tmp;
					TextEditor *te1;
					tm_source_file_update(TM_WORK_OBJECT(te->tm_file)
					  , FALSE, FALSE, TRUE);
					if(update)
					{
						for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
						{
							te1 = (TextEditor *) tmp->data;
							text_editor_set_hilite_type(te1);
						}
					}
				}
			}
		}
		/* we need to update UI with the call to scintilla */
		text_editor_thaw (te);
		scintilla_send_message (SCINTILLA (te->widgets.editor),
					SCI_SETSAVEPOINT, 0, 0);
		anjuta_set_active ();
		anjuta_status (_("File saved successfully"));
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
text_editor_update_preferences (AnjutaPreferences *pr, TextEditor * te)
{
	gboolean auto_save;
	guint auto_save_timer;

	auto_save = anjuta_preferences_get_int (pr, SAVE_AUTOMATIC);
	auto_save_timer = anjuta_preferences_get_int (pr, AUTOSAVE_TIMER);

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
	text_editor_set_zoom_factor(te, anjuta_preferences_get_int (pr,
														 TEXT_ZOOM_FACTOR));
}

gboolean
text_editor_check_disk_status (TextEditor * te, const gboolean bForce )
{
	struct stat status;
	time_t t;
	
	gchar *buff;
	if (!te)
		return FALSE;
	if (stat (te->full_filename, &status))
		return TRUE;
	t = time(NULL);
	if (te->modified_time > t || status.st_mtime > t)
	{
		/*
		 * Something is worng with the time stamp. They are refering to the
		 * future or the system clock is wrong.
		 * FIXME: Prompt the user about this inconsistency.
		 */
		return TRUE;
	}
	if (te->modified_time < status.st_mtime)
	{
		if( bForce )
		{
			text_editor_load_file (te);
		}
		else
		{
			GtkWidget *dlg;
			GtkWidget *parent;
			
			buff =
				g_strdup_printf (_
						 ("The file '%s' on the disk is more recent than\n"
						  "the current buffer.\nDo you want to reload it?"),
				te->filename);
			
			if (te->mode == TEXT_EDITOR_PAGED)
				parent = app->widgets.window;
			else
				parent = te->widgets.window;
			
			dlg = gtk_message_dialog_new (GTK_WINDOW (parent),
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_MESSAGE_WARNING,
										  GTK_BUTTONS_NONE, buff);
			gtk_dialog_add_button (GTK_DIALOG (dlg),
								   GTK_STOCK_NO,
								   GTK_RESPONSE_NO);
			anjuta_dialog_add_button (GTK_DIALOG (dlg),
									  _("_Reload"),
									  GTK_STOCK_REFRESH,
									  GTK_RESPONSE_YES);
			g_free (buff);
			if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_YES)
					text_editor_load_file (te);
			else
					te->modified_time = time (NULL);
			gtk_widget_destroy (dlg);
			return FALSE;
		}
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
	L = anjuta_launcher_is_busy (app->launcher);
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

glong text_editor_get_selection_start (TextEditor * te)
{
	return scintilla_send_message (SCINTILLA (te->widgets.editor),
				       SCI_GETSELECTIONSTART, 0, 0);
}
	
glong text_editor_get_selection_end (TextEditor * te)
{
	return scintilla_send_message (SCINTILLA (te->widgets.editor),
				     SCI_GETSELECTIONEND, 0, 0);
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

static gchar *
get_indent_style(AnjutaPreferences *pr, const gchar *name_style)
{
	typedef struct _IndentStyle{
		gchar *name;
		gchar *style;
	}IndentStyle;
	
	static struct _IndentStyle indentstyle[] = {
		{"GNU coding style", " -gnu"},
		{"Kernighan and Ritchie style", " -kr"},
		{"Original Berkeley style", " -orig"},
		{"Style of Kangleipak", " -i8 -sc -bli0 -bl0 -cbi0 -ss"},
		{"Hello World style", " -gnu -i0 -bli0 -cbi0 -cdb -sc -bl0 -ss"},
		{"Crazy boy style", " "}
	};
	
	gint n;
	if (g_strcasecmp (name_style, "Custom style") == 0)
		return anjuta_preferences_get (pr, AUTOFORMAT_CUSTOM_STYLE);
	for (n=0; n < (sizeof(indentstyle)/sizeof(IndentStyle)); n++)
	{		
		if (g_strcasecmp (name_style, _(indentstyle[n].name)) == 0)
			return g_strdup(indentstyle[n].style);
	}
	return NULL;
}

void
text_editor_autoformat (TextEditor * te)
{
	gchar *cmd, *file, *fopts, *shell;
	pid_t pid;
	int status;
	gchar *err;

	if (anjuta_is_installed ("indent", TRUE) == FALSE)
		return;
	if (anjuta_preferences_get_int (app->preferences, AUTOFORMAT_DISABLE))
	{
		anjuta_warning (_("Auto format is currently disabled. Change the setting in Preferences."));
		return;
	}
	if (te == NULL)
		return;

	anjuta_status (_("Auto formatting ..."));
	anjuta_set_busy ();
	text_editor_freeze (te);
	file = get_a_tmp_file ();
	if (save_to_file (te, file) == FALSE)
	{
		remove (file);
		g_free (file);
		text_editor_thaw (te);
		anjuta_set_active ();
		anjuta_warning (_("Error in auto formatting ..."));
		return;
	}

	fopts = get_indent_style(te->preferences, 
							 anjuta_preferences_get (te->preferences,
													 AUTOFORMAT_STYLE));
	if (!fopts)
	{
		gchar *msg;
		
		remove (file);
		g_free (file);
		text_editor_thaw (te);
		anjuta_set_active ();
		msg = g_strdup_printf (_("Anjuta does not know %s!"),
			anjuta_preferences_get (te->preferences, AUTOFORMAT_STYLE));
		anjuta_warning (msg);
		g_free(msg);
		return;
	}
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
	if (load_from_file (te, file, &err) == FALSE)
	{
		anjuta_warning (_("Error in auto formatting ...\nDetails: %s"), err);
		g_free (err);
	}
	else
	{
		text_editor_set_hilite_type (te);
		gtk_widget_queue_draw (te->widgets.editor);
		anjuta_status (_("Auto formatting completed"));
	}
	remove (file);
	g_free (file);
	anjuta_set_active ();
	text_editor_thaw (te);
	scintilla_send_message (SCINTILLA (te->widgets.editor),
				SCI_ENDUNDOACTION, 0, 0);
	main_toolbar_update ();
	anjuta_update_page_label(te);
	anjuta_refresh_breakpoints(te);
}

gboolean
text_editor_is_marker_set (TextEditor* te, glong line, gint marker)
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
text_editor_delete_marker (TextEditor* te, glong line, gint marker)
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

gint text_editor_get_bookmark_line( TextEditor* te, const glong nLineStart )
{
	return aneditor_command (te->editor_id, ANE_GETBOOKMARK_POS, nLineStart, 0 );
}

gint text_editor_get_num_bookmarks(TextEditor* te)
{
	gint	nLineNo = -1 ;
	gint	nMarkers = 0 ;
	
	g_return_val_if_fail (te != NULL, 0 );

	while( ( nLineNo = text_editor_get_bookmark_line( te, nLineNo ) ) >= 0 )
	{
		//printf( "Line %d\n", nLineNo );
		nMarkers++;
	}
	//printf( "out Line %d\n", nLineNo );
	return nMarkers ;
}

GtkWidget* text_editor_tab_widget_new(TextEditor* te)
{
	GtkWidget *button15;
	GtkWidget *close_pixmap;
	GtkWidget *tmp_toolbar_icon;
	GtkWidget *label;
	GtkWidget *box;
	int h, w;
	GdkColor color;
	
	g_return_val_if_fail(te != NULL, NULL);
	
	if (te->buttons.close)
		text_editor_tab_widget_destroy(te);
	
	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

	tmp_toolbar_icon = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_show(tmp_toolbar_icon);
	
	button15 = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button15), tmp_toolbar_icon);
	gtk_button_set_relief(GTK_BUTTON(button15), GTK_RELIEF_NONE);
	gtk_widget_set_size_request (button15, w, h);
	
	close_pixmap = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_set_size_request(close_pixmap, w,h);
	gtk_widget_set_sensitive(close_pixmap, FALSE);
	
	label = gtk_label_new (te->filename);
	gtk_widget_show (label);
	
	color.red = 0;
	color.green = 0;
	color.blue = 0;
	
	gtk_widget_modify_fg (button15, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_fg (button15, GTK_STATE_INSENSITIVE, &color);
	gtk_widget_modify_fg (button15, GTK_STATE_ACTIVE, &color);
	gtk_widget_modify_fg (button15, GTK_STATE_PRELIGHT, &color);
	gtk_widget_modify_fg (button15, GTK_STATE_SELECTED, &color);
	gtk_widget_show(button15);
	
	box = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), button15, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), close_pixmap, FALSE, FALSE, 0);
	
	gtk_widget_show(box);

	gtk_signal_connect (GTK_OBJECT (button15), "clicked",
				GTK_SIGNAL_FUNC(on_text_editor_notebook_close_page),
				te);

	te->widgets.close_pixmap = close_pixmap;
	te->buttons.close = button15;
	te->widgets.tab_label = label;
	
	gtk_widget_ref (te->buttons.close);
	gtk_widget_ref (te->widgets.close_pixmap);
	gtk_widget_ref (te->widgets.tab_label);
	
	return box;
}

void
text_editor_tab_widget_destroy(TextEditor* te)
{
	g_return_if_fail(te != NULL);
	g_return_if_fail(te->buttons.close != NULL);

	gtk_widget_unref (te->buttons.close);
	gtk_widget_unref (te->widgets.close_pixmap);
	gtk_widget_unref (te->widgets.tab_label);
	
	te->widgets.close_pixmap = NULL;
	te->buttons.close = NULL;
	te->widgets.tab_label = NULL;
}

/* Get the current selection. If there is no selection, or if the selection
** is all blanks, get the word under teh cursor.
*/
gchar *text_editor_get_current_word(TextEditor *te)
{
	char *buf = text_editor_get_selection(te);
	if (buf)
	{
		g_strstrip(buf);
		if ('\0' == *buf)
		{
			g_free(buf);
			buf = NULL;
		}
	}
	if (NULL == buf)
	{
		int ret;
		buf = g_new(char, 256);
		ret = aneditor_command (te->editor_id, ANE_GETCURRENTWORD, (long)buf, 255L);
		if (!ret)
		{
			g_free(buf);
			buf = NULL;
		}
	}
#ifdef DEBUG
	if (buf)
	{
		g_message("Current word is '%s'", buf);
	}
#endif
	return buf;
}
