 /*
  * mainmenu_callbacks.c
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

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <sys/wait.h>
#include <errno.h>

#include <gnome.h>

#include <libgnomeui/gnome-window-icon.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/plugins.h>

#include "anjuta-app.h"
#include "help.h"
#include "about.h"
#include "action-callbacks.h"
#include "anjuta.h"

void
on_exit1_activate (GtkAction * action, GObject *app)
{
	if (on_anjuta_delete_event (GTK_WIDGET (app), NULL, app) == FALSE)
		on_anjuta_destroy (GTK_WIDGET (app), app);
}

void
on_set_preferences1_activate (GtkAction * action, AnjutaApp *app)
{
	gtk_widget_show (GTK_WIDGET (app->preferences));
}

void
on_set_default_preferences1_activate (GtkAction * action,
				      AnjutaApp *app)
{
	anjuta_preferences_reset_defaults (ANJUTA_PREFERENCES (app->preferences));
}

void
on_customize_shortcuts_activate(GtkAction *action, AnjutaApp *app)
{
	GtkWidget *win, *accel_editor;
	
	accel_editor = anjuta_ui_get_accel_editor (ANJUTA_UI (app->ui));
	win = gtk_dialog_new_with_buttons (_("Anjuta Plugins"), GTK_WINDOW (app),
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_STOCK_CLOSE, GTK_STOCK_CANCEL, NULL);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG(win)->vbox), accel_editor);
	gtk_window_set_default_size (GTK_WINDOW (win), 500, 400);
	gtk_dialog_run (GTK_DIALOG (win));
	gtk_widget_destroy (win);
}

void
on_layout_manager_activate(GtkAction *action, AnjutaApp *app)
{
	egg_dock_layout_run_manager (app->layout_manager);
}

void
on_show_plugins_activate (GtkAction *action, AnjutaApp *app)
{
	GtkWidget *win, *plg;
	
	plg = anjuta_plugins_get_installed_dialog (ANJUTA_SHELL (app));
	win = gtk_dialog_new_with_buttons (_("Anjuta Plugins"), GTK_WINDOW (app),
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_STOCK_CLOSE, GTK_STOCK_CANCEL, NULL);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG(win)->vbox), plg);
	gtk_widget_set_usize (win, 300, 400);
	gtk_dialog_run (GTK_DIALOG (win));
	gtk_widget_destroy (win);
}

void
on_help_activate (GtkAction *action, gpointer data)
{
	if (gnome_help_display ((const gchar*)data, NULL, NULL) == FALSE)
	{
	  anjuta_util_dialog_error (NULL, _("Unable to display help. Please make sure Anjuta documentation package is install. It can be downloaded from http://anjuta.org"));	
	}
}

void
on_gnome_pages1_activate (GtkAction *action, AnjutaApp *app)
{
	if (anjuta_util_prog_is_installed ("devhelp", TRUE))
	{
		anjuta_res_help_search (NULL);
	}
}

void
on_context_help_activate (GtkAction * action, AnjutaApp *app)
{
  /*	TextEditor* te;
	gboolean ret;
	gchar buffer[1000];
	
	te = anjuta_get_current_text_editor();
	if(!te) return;
	ret = aneditor_command (te->editor_id, ANE_GETCURRENTWORD, (long)buffer, (long)sizeof(buffer));
	if (ret == FALSE) return;
	anjuta_help_search(app->help_system, buffer);
  */
}

void
on_search_a_topic1_activate (GtkAction * action, AnjutaApp *app)
{
  //anjuta_help_show (app->help_system);
}

void
on_url_man_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("man:man");
}

void
on_url_info_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("info:info");
}

void
on_url_home_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("http://anjuta.org");
}

void
on_url_libs_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("http://lidn.sourceforge.net");
}

void
on_url_bugs_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("http://sourceforge.net/tracker/?atid=114222&group_id=14222&func=browse");
}

void
on_url_features_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("http://sourceforge.net/tracker/?atid=364222&group_id=14222&func=browse");
}

void
on_url_patches_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("http://sourceforge.net/tracker/?atid=314222&group_id=14222&func=browse");
}

void
on_url_faqs_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("mailto:anjuta-list@lists.sourceforge.net");
}

void
on_about1_activate (GtkAction * action, gpointer user_data)
{
	GtkWidget *about_dlg = about_box_new ();
	gtk_widget_show (about_dlg);
}
