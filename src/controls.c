/* 
    controls.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "launcher.h"
#include "text_editor.h"
#include "debugger.h"
#include "toolbar.h"
#include "toolbar_callbacks.h"
#include "anjuta.h"
#include "utilities.h"
#include "controls.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "ScintillaWidget.h"

void update_main_menubar ();

void
debug_toolbar_update ()
{
	DebugToolbar *dt;
	gboolean A, R, Pr, Ld;
	if (app->shutdown_in_progress)
		return;
	dt = &(app->widgets.toolbar.debug_toolbar);
	A = debugger_is_active ();
	R = debugger_is_ready ();
	Pr = debugger.prog_is_running;
	Ld = TRUE;

	gtk_widget_set_sensitive (dt->go, A && R && Ld);
	gtk_widget_set_sensitive (dt->step_in, A && R && Ld);
	gtk_widget_set_sensitive (dt->run_to_cursor, A && R && Ld);
	gtk_widget_set_sensitive (dt->step_over, A && R && Pr);
	gtk_widget_set_sensitive (dt->step_out, A && R && Pr);
	gtk_widget_set_sensitive (dt->toggle_bp, A && R && Ld);
	gtk_widget_set_sensitive (dt->inspect, A && R && Ld);
	gtk_widget_set_sensitive (dt->interrupt, A && !R && Pr && Ld);
	gtk_widget_set_sensitive (dt->frame, A && R && Ld);
	gtk_widget_set_sensitive (dt->stop, A);

	update_main_menubar ();
}

void
main_toolbar_update ()
{
	MainToolbar *mt;
	gboolean F, S, UD, RD;
	TextEditor *te;

	if (app->shutdown_in_progress)
		return;
	mt = &(app->widgets.toolbar.main_toolbar);
	te = anjuta_get_current_text_editor ();
	if (te)
	{
		F = TRUE;
		RD =
			scintilla_send_message (SCINTILLA
						(te->widgets.editor),
						SCI_CANREDO, 0, 0);
		UD =
			scintilla_send_message (SCINTILLA
						(te->widgets.editor),
						SCI_CANUNDO, 0, 0);
		S = text_editor_is_saved (te);
	}
	else
		F = S = UD = RD = FALSE;
	gtk_widget_set_sensitive (mt->save, F && !S);
	gtk_widget_set_sensitive (mt->save_all,
				  (g_list_length (app->text_editor_list) >1));
	gtk_widget_set_sensitive (mt->close, F);
	gtk_widget_set_sensitive (mt->reload, F);
	gtk_widget_set_sensitive (mt->undo, UD);
	gtk_widget_set_sensitive (mt->redo, RD);
	gtk_widget_set_sensitive (mt->detach, F);
	gtk_widget_set_sensitive (mt->print, F);
	gtk_widget_set_sensitive (mt->find, F);
	gtk_widget_set_sensitive (mt->find_combo, F);
	gtk_widget_set_sensitive (mt->go_to, F);
	gtk_widget_set_sensitive (mt->line_entry, F);

	update_main_menubar ();
}

void
extended_toolbar_update ()
{
	gboolean P, F, L, A, C;
	ExtendedToolbar *et;
	TextEditor *te;

	if (app->shutdown_in_progress)
		return;
	et = &(app->widgets.toolbar.extended_toolbar);
	P = app->project_dbase->project_is_open;
	L = launcher_is_busy ();
	A = debugger_is_active ();
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
	{
		F = FALSE;
		C = FALSE;
	}
	else
	{
		F = TRUE;
		C = anjuta_get_file_property(FILE_PROP_IS_SOURCE, te->filename, FALSE);
	}
	gtk_widget_set_sensitive (et->open_project, !P);
	gtk_widget_set_sensitive (et->save_project, P);
	gtk_widget_set_sensitive (et->close_project, P);
	gtk_widget_set_sensitive (et->compile, !L && C);
	gtk_widget_set_sensitive (et->configure, P && !L);
	gtk_widget_set_sensitive (et->build, (C || P) && !L);
	gtk_widget_set_sensitive (et->build_all, P && !L);
	gtk_widget_set_sensitive (et->exec, (C || P) && !L);
	gtk_widget_set_sensitive (et->debug, !L);
	gtk_widget_set_sensitive (et->stop, !A && L);
	update_main_menubar ();
}

void
format_toolbar_update()
{
	FormatToolbar *ft;
	gboolean F, FLD, I;
	TextEditor *te;

	if (app->shutdown_in_progress)
		return;
	ft = &(app->widgets.toolbar.format_toolbar);
	te = anjuta_get_current_text_editor ();
	if (te)
	{
		F = TRUE;
		I = anjuta_get_file_property(FILE_PROP_CAN_AUTOFORMAT, te->filename, FALSE);
		FLD = anjuta_get_file_property(FILE_PROP_HAS_FOLDS, te->filename, FALSE);
	}
	else
		F = I = FLD = FALSE;
	gtk_widget_set_sensitive (ft->toggle_fold, FLD);
	gtk_widget_set_sensitive (ft->open_all_folds, FLD);
	gtk_widget_set_sensitive (ft->close_all_folds, FLD);
	gtk_widget_set_sensitive (ft->block_select, FLD);
	gtk_widget_set_sensitive (ft->indent_increase, F);
	gtk_widget_set_sensitive (ft->indent_decrease, F);
	gtk_widget_set_sensitive (ft->autoformat, I);
	gtk_widget_set_sensitive (ft->calltip, F);
	gtk_widget_set_sensitive (ft->autocomplete, F);
}

void
browser_toolbar_update()
{
	BrowserToolbar *bt;
	gboolean F, FLD;
	TextEditor *te;

	if (app->shutdown_in_progress)
		return;
	bt = &(app->widgets.toolbar.browser_toolbar);
	te = anjuta_get_current_text_editor ();
	if (te)
	{
		F = TRUE;
		FLD = anjuta_get_file_property(FILE_PROP_HAS_FOLDS, te->filename, FALSE);
	}
	else
		F = FLD = FALSE;
	gtk_widget_set_sensitive (bt->toggle_bookmark, F);
	gtk_widget_set_sensitive (bt->prev_bookmark, F);
	gtk_widget_set_sensitive (bt->next_bookmark, F);
	gtk_widget_set_sensitive (bt->first_bookmark, F);
	gtk_widget_set_sensitive (bt->last_bookmark, F);
	gtk_widget_set_sensitive (bt->block_start, FLD);
	gtk_widget_set_sensitive (bt->block_end, FLD);
	/* Goto Tag Stuff */
	gtk_widget_set_sensitive (bt->tag, F);
	gtk_widget_set_sensitive (bt->tag_combo, F);
	if (F)
	{
		const GList *tags = anjuta_get_tag_list(te, tm_tag_max_t);
		if (tags)
		{
			gtk_signal_disconnect_by_func(GTK_OBJECT(GTK_COMBO(bt->tag_combo)->list)
			  , GTK_SIGNAL_FUNC(on_toolbar_tag_clicked), NULL);
			gtk_combo_set_popdown_strings(GTK_COMBO(bt->tag_combo), (GList *) tags);
			gtk_signal_connect (GTK_OBJECT(GTK_COMBO(bt->tag_combo)->list),
			    "selection-changed", GTK_SIGNAL_FUNC (on_toolbar_tag_clicked), NULL);
			gtk_widget_show(bt->tag_combo);
			gtk_widget_show(bt->tag);
			gtk_label_set_text(GTK_LABEL(bt->tag_label), _("Tags: "));
		}
		else
		{
			gtk_widget_hide(bt->tag_combo);
			gtk_widget_hide(bt->tag);
			gtk_label_set_text(GTK_LABEL(bt->tag_label), _("No Tags"));
		}
	}
}

void
update_main_menubar ()
{
	FileSubMenu *fm;
	EditSubMenu *em;
	ViewSubMenu *vm;
	ProjectSubMenu *pm;
	FormatSubMenu *ftm;
	BuildSubMenu *bm;
	BookmarkSubMenu *mm;
	WindowsSubMenu *wm;
	DebugSubMenu *dm;
	UtilitiesSubMenu *um;
	HelpSubMenu *hm;
	gboolean F, P, SF, L, G, A, R, Pr, UD, RD, Ld, C, I, FLD, UT;
	TextEditor *te;

	if (app->shutdown_in_progress)
		return;
	fm = &(app->widgets.menubar.file);
	em = &(app->widgets.menubar.edit);
	vm = &(app->widgets.menubar.view);
	pm = &(app->widgets.menubar.project);
	ftm = &(app->widgets.menubar.format);
	bm = &(app->widgets.menubar.build);
	mm = &(app->widgets.menubar.bookmark);
	wm = &(app->widgets.menubar.windows);
	dm = &(app->widgets.menubar.debug);
	um = &(app->widgets.menubar.utilities);
	hm = &(app->widgets.menubar.help);

	P = app->project_dbase->project_is_open;
	if (P)
		G = TRUE;
	else
		G = FALSE;
	L = launcher_is_busy ();

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
	{
		I = FLD = UD = RD = FALSE;
		C = SF = F = FALSE;
		UT = P;
	}
	else
	{
		F = TRUE;
		C = anjuta_get_file_property(FILE_PROP_IS_SOURCE, te->filename, FALSE);
		I = anjuta_get_file_property(FILE_PROP_CAN_AUTOFORMAT, te->filename, FALSE);
		FLD = anjuta_get_file_property(FILE_PROP_HAS_FOLDS, te->filename, FALSE);
		UT = anjuta_get_file_property(FILE_PROP_HAS_TAGS, te->filename, FALSE);
		RD =
			scintilla_send_message (SCINTILLA
						(te->widgets.editor),
						SCI_CANREDO, 0, 0);
		UD =
			scintilla_send_message (SCINTILLA
						(te->widgets.editor),
						SCI_CANUNDO, 0, 0);
		SF = text_editor_is_saved (te);
	}
	A = debugger_is_active ();
	R = debugger_is_ready ();
	Pr = debugger.prog_is_running;
	Ld = TRUE;

	gtk_widget_set_sensitive (fm->save_file, F && !SF);
	gtk_widget_set_sensitive (fm->save_as_file, F);
	gtk_widget_set_sensitive (fm->save_all_file,
				  (g_list_length (app->text_editor_list) > 1));
	gtk_widget_set_sensitive (fm->close_file, F);
	gtk_widget_set_sensitive (fm->close_all_file, F);
	gtk_widget_set_sensitive (fm->reload_file, F);
	gtk_widget_set_sensitive (fm->new_project, !P);
	gtk_widget_set_sensitive (fm->import_project, !P);
	gtk_widget_set_sensitive (fm->open_project, !P);
	gtk_widget_set_sensitive (fm->save_project, P);
	gtk_widget_set_sensitive (fm->close_project, P);

	gtk_widget_set_sensitive (fm->rename, F);
	gtk_widget_set_sensitive (fm->print, F);

	gtk_widget_set_sensitive (em->undo, UD);
	gtk_widget_set_sensitive (em->redo, RD);
	gtk_widget_set_sensitive (em->cut, F);
	gtk_widget_set_sensitive (em->copy, F);
	gtk_widget_set_sensitive (em->paste, F);
	gtk_widget_set_sensitive (em->clear, F);
	
	gtk_widget_set_sensitive (em->uppercase, F);
	gtk_widget_set_sensitive (em->lowercase, F);
	gtk_widget_set_sensitive (em->convert_crlf, F);
	gtk_widget_set_sensitive (em->convert_lf, F);
	gtk_widget_set_sensitive (em->convert_cr, F);
	gtk_widget_set_sensitive (em->convert_auto, F);
	
	gtk_widget_set_sensitive (em->insert_c_gpl, F);
	gtk_widget_set_sensitive (em->insert_cpp_gpl, F);
	gtk_widget_set_sensitive (em->insert_py_gpl, F);
	gtk_widget_set_sensitive (em->insert_username, F);
   	gtk_widget_set_sensitive (em->insert_datetime, F);
	gtk_widget_set_sensitive (em->insert_header_template, F);
	gtk_widget_set_sensitive (em->select_all, F);
	gtk_widget_set_sensitive (em->select_brace, F);
	gtk_widget_set_sensitive (em->select_block, FLD);
	gtk_widget_set_sensitive (em->calltip, F);
	gtk_widget_set_sensitive (em->autocomplete, F);
	gtk_widget_set_sensitive (em->find, F);
	gtk_widget_set_sensitive (em->find_in_files, !L);
	gtk_widget_set_sensitive (em->find_replace, F);
	gtk_widget_set_sensitive (em->goto_line, F);
	gtk_widget_set_sensitive (em->goto_brace, F);
	gtk_widget_set_sensitive (em->goto_block_start, F);
	gtk_widget_set_sensitive (em->goto_block_end, F);
	
	gtk_widget_set_sensitive (em->edit_app_gui, P && G);
	gtk_widget_set_sensitive (vm->show_hide_locals, P && G);

	gtk_widget_set_sensitive (pm->add_src, P);
	gtk_widget_set_sensitive (pm->add_pix, P);
	gtk_widget_set_sensitive (pm->add_doc, P);
	gtk_widget_set_sensitive (pm->add_data, P);
	gtk_widget_set_sensitive (pm->add_po, P);
	gtk_widget_set_sensitive (pm->add_inc, P);
	gtk_widget_set_sensitive (pm->add_hlp, P);
	gtk_widget_set_sensitive (pm->remove, P);
	gtk_widget_set_sensitive (pm->readme, P);
	gtk_widget_set_sensitive (pm->todo, P);
	gtk_widget_set_sensitive (pm->changelog, P);
	gtk_widget_set_sensitive (pm->news, P);
	gtk_widget_set_sensitive (pm->configure, P);
	gtk_widget_set_sensitive (pm->info, P);

	gtk_widget_set_sensitive (ftm->force_hilite, F);
	gtk_widget_set_sensitive (ftm->indent, I);
	gtk_widget_set_sensitive (ftm->update_tags, UT);
	gtk_widget_set_sensitive (ftm->close_folds, FLD);
	gtk_widget_set_sensitive (ftm->open_folds, FLD);
	gtk_widget_set_sensitive (ftm->toggle_fold, FLD);
	gtk_widget_set_sensitive (ftm->detach, F);
	gtk_widget_set_sensitive (ftm->indent_inc, F);
	gtk_widget_set_sensitive (ftm->indent_dcr, F);

	gtk_widget_set_sensitive (bm->compile, C && !L);
	gtk_widget_set_sensitive (bm->make, C && !L);
	gtk_widget_set_sensitive (bm->build, (C || P) && !L);
	gtk_widget_set_sensitive (bm->build_all, P && !L);
	gtk_widget_set_sensitive (bm->install, P && !L);
	gtk_widget_set_sensitive (bm->build_dist, P && !L);
	gtk_widget_set_sensitive (bm->configure, P && !L);
	gtk_widget_set_sensitive (bm->autogen, P && !L);
	gtk_widget_set_sensitive (bm->clean, P && !L);
	gtk_widget_set_sensitive (bm->clean_all, P && !L);
	gtk_widget_set_sensitive (bm->stop_build, !A && L);
	gtk_widget_set_sensitive (bm->execute, (C || P) && !L);
	gtk_widget_set_sensitive (bm->execute_params, (C || P));

	gtk_widget_set_sensitive (mm->toggle, F);
	gtk_widget_set_sensitive (mm->prev, F);
	gtk_widget_set_sensitive (mm->next, F);
	gtk_widget_set_sensitive (mm->last, F);
	gtk_widget_set_sensitive (mm->first, F);
	gtk_widget_set_sensitive (mm->clear, F);

	gtk_widget_set_sensitive (wm->close, F);

	gtk_widget_set_sensitive (dm->start_debug, !L);
	gtk_widget_set_sensitive (dm->open_exec, A && R);
	gtk_widget_set_sensitive (dm->load_core, A && R);
	gtk_widget_set_sensitive (dm->attach, A && R);
	gtk_widget_set_sensitive (dm->restart, A && R && Pr
				  && !debugger.prog_is_attached);
	gtk_widget_set_sensitive (dm->stop_prog, A && Pr && R
				  && !debugger.prog_is_attached);
	gtk_widget_set_sensitive (dm->detach, A && R && Pr
				  && debugger.prog_is_attached);
	gtk_widget_set_sensitive (dm->cont, A && R && Ld);
	gtk_widget_set_sensitive (dm->run_to_cursor, A && R && Ld);
	gtk_widget_set_sensitive (dm->step_in, A && R && Ld);
	gtk_widget_set_sensitive (dm->step_over, A && R && Pr);
	gtk_widget_set_sensitive (dm->step_out, A && R && Pr);
	gtk_widget_set_sensitive (dm->tog_break, A && R && Ld);
	gtk_widget_set_sensitive (dm->set_break, A && R && Ld);
	gtk_widget_set_sensitive (dm->show_breakpoints, A && R && Ld);
	gtk_widget_set_sensitive (dm->disable_all_breakpoints, A && R && Ld);
	gtk_widget_set_sensitive (dm->clear_all_breakpoints, A && R && Ld);
	gtk_widget_set_sensitive (dm->interrupt, A && !R && Pr);
	gtk_widget_set_sensitive (dm->send_signal, A && Pr);
	gtk_widget_set_sensitive (dm->inspect, A && R);
	gtk_widget_set_sensitive (dm->add_watch, A && R);
	gtk_widget_set_sensitive (dm->stop, A);

	gtk_widget_set_sensitive (dm->info_targets, A && R);
	gtk_widget_set_sensitive (dm->info_program, A && R);
	gtk_widget_set_sensitive (dm->info_udot, A && R);
	gtk_widget_set_sensitive (dm->info_threads, A && R);
	gtk_widget_set_sensitive (dm->info_variables, A && R);
	gtk_widget_set_sensitive (dm->info_locals, A && R);
	gtk_widget_set_sensitive (dm->info_frame, A && R);
	gtk_widget_set_sensitive (dm->info_args, A && R);

	update_led_animator ();
}

void
update_led_animator ()
{
	GnomeAnimator *led;
	gboolean A, L, R;

	if (app->shutdown_in_progress)
		return;
	A = debugger_is_active ();
	R = debugger_is_ready ();
	L = launcher_is_busy ();

	led = GNOME_ANIMATOR (app->widgets.toolbar.main_toolbar.led);
	if (L && !A)
		gnome_animator_start (led);
	else if (!L && !A)
	{
		gnome_animator_stop (led);
		gnome_animator_goto_frame (led, 0);
	}
	else
	{
		if (!R)
			gnome_animator_start (led);
		else
		{
			gnome_animator_stop (led);
			gnome_animator_goto_frame (led, 0);
		}
	}
}
