/*
 * commands.c
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
#include <string.h>

#include <gnome.h>
#include "anjuta.h"
#include "commands.h"
#include "utilities.h"

#define LABEL_MAX_WIDTH 250

enum
{
	COMPILE_INDEX,
	MAKE_INDEX,
	BUILD_INDEX,
	EXECUTE_INDEX,
	VIEW_INDEX,
	OPEN_INDEX,
	END_OF_INDEX
};

/* Do not use translation here */
static gchar*
prog_language_map[]=
{
	"C", "c",
	"C++", "cpp",
	"C#", "cs",
	"Java", "java",
	"Perl", "perl",
	"Python", "py",
	"Pascal", "pascal",
	"Ada", "ada",
	"Lua", "lua",
	"LaTex", "latex",
	NULL, NULL
};

/* Ditto */
static gchar *
term_commands[] = {
	"gnome-terminal -e \"$(anjuta.current.command)\"",
	"gnome-terminal -x sh -c \"$(anjuta.current.command)\"",
	"gnome-terminal --command=\"sh -c \\\"$(anjuta.current.command); read x\\\"\"",
	"xterm -e sh -c \"$(anjuta.current.command)\"",

	NULL
};

/* Command data to be used in command editor */
CommandData*
command_data_new(void)
{
	CommandData* cd;
	cd = g_malloc (sizeof (CommandData));
	if (!cd) return NULL;
	cd->key = NULL;
	cd->compile = NULL;
	cd->make = NULL;
	cd->build = NULL;
	cd->execute = NULL;
	return cd;
}

void
command_data_destroy (CommandData *cdata)
{
	g_return_if_fail (cdata != NULL);
	string_assign (&cdata->key, NULL);
	string_assign (&cdata->compile, NULL);
	string_assign (&cdata->make, NULL);
	string_assign (&cdata->build, NULL);
	string_assign (&cdata->execute, NULL);
	g_free (cdata);
}


/* CommandEditor */
static void create_command_editor_gui (CommandEditor *);

CommandEditor*
command_editor_new (PropsID p_global, PropsID p_user, PropsID p)
{
	CommandEditor* ce;
	ce = g_malloc (sizeof (CommandEditor));
	if (!ce) return NULL;
	ce->props = p;
	ce->props_user = p_user;
	ce->props_global = p_global;
	ce->current_command_data = NULL;
	ce->win_pos_x = 50;
	ce->win_pos_y = 50;
	ce->is_showing = FALSE;
	
	/* Don't worry dialog is unsinkable */
	ce->win_width = 0;
	ce->win_height = 0;
	
	create_command_editor_gui (ce);
	return ce;
}

void
command_editor_destroy (CommandEditor* ce)
{
	g_return_if_fail (ce != NULL);
	g_return_if_fail (GTK_IS_WIDGET(ce->widgets.window));

	gtk_widget_unref (ce->widgets.window);
	gtk_widget_unref (ce->widgets.language_combo);
	gtk_widget_unref (ce->widgets.compile_entry);
	gtk_widget_unref (ce->widgets.make_entry);
	gtk_widget_unref (ce->widgets.build_entry);
	gtk_widget_unref (ce->widgets.execute_entry);
	gtk_widget_unref (ce->widgets.pix_editor_entry);
	gtk_widget_unref (ce->widgets.image_editor_entry);
	gtk_widget_unref (ce->widgets.html_editor_entry);
	gtk_widget_unref(ce->widgets.terminal_entry);
	gtk_widget_destroy (ce->widgets.window);
	g_free (ce);
}

static gchar*
get_key_for_file_command (gint cmd_type, gchar* filetype)
{
	gchar* head;
	g_return_val_if_fail (cmd_type < END_OF_INDEX, NULL);
	g_return_val_if_fail (filetype != NULL, NULL);
	
	switch(cmd_type)
	{
		case COMPILE_INDEX:
			head = COMMAND_COMPILE_FILE;
			break;
		case MAKE_INDEX:
			head = COMMAND_MAKE_FILE;
			break;
		case BUILD_INDEX:
			head = COMMAND_BUILD_FILE;
			break;
		case EXECUTE_INDEX:
			head = COMMAND_EXECUTE_FILE;
			break;
		case VIEW_INDEX:
			head = COMMAND_VIEW_FILE;
			break;
		case OPEN_INDEX:
			head = COMMAND_OPEN_FILE;
			break;
		default:
			return NULL;
	}
	return g_strconcat (head, "$(file.patterns.", filetype, ")", NULL);
}

/* Sync from a perticular prop set database */
static void
sync_from_props (CommandEditor *ce, PropsID pr)
{
	gint i;
	gchar *str, *key;

	for (i = 0;; i+=2)
	{
		CommandData* cdata;
		
		if (prog_language_map[i] == NULL) break;
		cdata = command_data_new();

		key = get_key_for_file_command (COMPILE_INDEX, prog_language_map[i+1]);
		str = prop_get (pr, key);
		g_free (key);
		string_assign (&cdata->compile, str);
		g_free (str);
		
		key = get_key_for_file_command (MAKE_INDEX, prog_language_map[i+1]);
		str = prop_get (pr, key);
		g_free (key);
		string_assign (&cdata->make, str);
		g_free (str);

		key = get_key_for_file_command (BUILD_INDEX, prog_language_map[i+1]);
		str = prop_get (pr, key);
		g_free (key);
		string_assign (&cdata->build, str);
		g_free (str);
		
		key = get_key_for_file_command (EXECUTE_INDEX, prog_language_map[i+1]);
		str = prop_get (pr, key);
		g_free (key);
		string_assign (&cdata->execute, str);
		g_free (str);
		
		/* Yes, prog_language_map[i] and not [i+1] */
		string_assign (&cdata->key, prog_language_map[i]);
		gtk_object_set_data_full (GTK_OBJECT (ce->widgets.window),
			cdata->key, cdata, (GtkDestroyNotify) command_data_destroy);
	}
	ce->current_command_data = NULL;
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_COMBO(ce->widgets.language_combo)->entry), "changed");

	key = get_key_for_file_command (OPEN_INDEX, "icon");
	str = prop_get (pr, key);
	g_free (key);
	gtk_entry_set_text (GTK_ENTRY(ce->widgets.pix_editor_entry), str);
	g_free (str);

	key = get_key_for_file_command (OPEN_INDEX, "image");
	str = prop_get (pr, key);
	g_free (key);
	gtk_entry_set_text (GTK_ENTRY(ce->widgets.image_editor_entry), str);
	g_free (str);

	key = get_key_for_file_command (VIEW_INDEX, "html");
	str = prop_get (pr, key);
	g_free (key);
	gtk_entry_set_text (GTK_ENTRY(ce->widgets.html_editor_entry), str);
	g_free (str);

	if (NULL != (str = prop_get(pr, COMMAND_TERMINAL)))
	{
		gtk_entry_set_text(GTK_ENTRY(ce->widgets.terminal_entry), str);
		g_free(str);
	}
}

/* ----- */
void
command_editor_hide (CommandEditor *ce)
{
	g_return_if_fail (ce != NULL);
	g_return_if_fail (ce->is_showing != FALSE);

	gdk_window_get_root_origin (ce->widgets.window->window, &ce->win_pos_x,
			      &ce->win_pos_y);
	ce->is_showing = FALSE;
}

void
command_editor_show (CommandEditor *ce)
{
	g_return_if_fail (ce != NULL);
	if (ce->is_showing)
	{
		gdk_window_raise (ce->widgets.window->window);
		return;
	}
	sync_from_props (ce, ce->props);
	gtk_widget_set_uposition (ce->widgets.window, ce->win_pos_x, ce->win_pos_y);
	gtk_widget_show (ce->widgets.window);
	ce->is_showing = TRUE;
}


gboolean
command_editor_save (CommandEditor *ce, FILE* s)
{
	gint i;
	gchar *str, *key;
	PropsID pr;

	g_return_val_if_fail (ce != NULL, FALSE);
	g_return_val_if_fail (s != NULL, FALSE);
	
	pr = ce->props;
	fprintf (s, "\n");
	for (i = 0;; i+=2)
	{
		if (prog_language_map[i] == NULL) break;
		key = get_key_for_file_command (COMPILE_INDEX, prog_language_map[i+1]);
		str = prop_get (pr, key);
		if (str)
		{
			if (fprintf (s, "%s=%s\n", key, str) < 2) return FALSE;
			g_free (str);
		}
		else
		{
			if (fprintf (s, "%s=\n", key) < 1) return FALSE;
		}
		g_free (key);

		key = get_key_for_file_command (MAKE_INDEX, prog_language_map[i+1]);
		str = prop_get (pr, key);
		if (str)
		{
			if (fprintf (s, "%s=%s\n", key, str) < 2) return FALSE;
			g_free (str);
		}
		else
		{
			if (fprintf (s, "%s=\n", key) < 1) return FALSE;
		}
		g_free (key);

		key = get_key_for_file_command (BUILD_INDEX, prog_language_map[i+1]);
		str = prop_get (pr, key);
		if (str)
		{
			if (fprintf (s, "%s=%s\n", key, str) < 2) return FALSE;
			g_free (str);
		}
		else
		{
			if (fprintf (s, "%s=\n", key) < 1) return FALSE;
		}
		g_free (key);
		
		key = get_key_for_file_command (EXECUTE_INDEX, prog_language_map[i+1]);
		str = prop_get (pr, key);
		if (str)
		{
			if (fprintf (s, "%s=%s\n", key, str) < 2) return FALSE;
			g_free (str);
		}
		else
		{
			if (fprintf (s, "%s=\n", key) < 1) return FALSE;
		}
		g_free (key);
	}

	key = get_key_for_file_command (OPEN_INDEX, "icon");
	str = prop_get (pr, key);
	if (str)
	{
		if (fprintf (s, "%s=%s\n", key, str) < 2) return FALSE;
		g_free (str);
	}
	else
	{
		if (fprintf (s, "%s=\n", key) < 1) return FALSE;
	}
	g_free (key);

	key = get_key_for_file_command (OPEN_INDEX, "image");
	str = prop_get (pr, key);
	if (str)
	{
		if (fprintf (s, "%s=%s\n", key, str) < 2) return FALSE;
		g_free (str);
	}
	else
	{
		if (fprintf (s, "%s=\n", key) < 1) return FALSE;
	}
	g_free (key);

	key = get_key_for_file_command (VIEW_INDEX, "html");
	str = prop_get (pr, key);
	if (str)
	{
		if (fprintf (s, "%s=%s\n", key, str) < 2) return FALSE;
		g_free (str);
	}
	else
	{
		if (fprintf (s, "%s=\n", key) < 1) return FALSE;
	}
	g_free (key);

	if (NULL != (str = prop_get(pr, COMMAND_TERMINAL)))
	{
		fprintf(s, "%s=%s\n", COMMAND_TERMINAL, str);
		g_free(str);
	}
	return TRUE;
}

gchar*
command_editor_get_command_file (CommandEditor* ce, gchar* cmd_key, gchar* fname)
{
	g_return_val_if_fail (ce != NULL, NULL);
	g_return_val_if_fail (fname != NULL, NULL);
	g_return_val_if_fail (strlen (fname) != 0, NULL);

	return prop_get_new_expand (ce->props, cmd_key, fname);
}

gchar*
command_editor_get_command (CommandEditor* ce, gchar* cmd_key)
{
	g_return_val_if_fail (ce != NULL, NULL);

	return prop_get_expanded (ce->props, cmd_key);
}

static void
on_language_entry_changed (GtkEditable     *editable, gpointer         user_data)
{
	CommandData *cdata;
	CommandEditor *ce;
	gchar *str;
	
	ce = user_data;
	g_return_if_fail (ce != NULL);
	if (ce->current_command_data)
	{
		str = gtk_entry_get_text (GTK_ENTRY (ce->widgets.compile_entry));
		string_assign (&ce->current_command_data->compile, str);

		str = gtk_entry_get_text (GTK_ENTRY (ce->widgets.make_entry));
		string_assign (&ce->current_command_data->make, str);

		str = gtk_entry_get_text (GTK_ENTRY (ce->widgets.build_entry));
		string_assign (&ce->current_command_data->build, str);

		str = gtk_entry_get_text (GTK_ENTRY (ce->widgets.execute_entry));
		string_assign (&ce->current_command_data->execute, str);
	}
	str = gtk_entry_get_text (GTK_ENTRY (editable));
	cdata = gtk_object_get_data (GTK_OBJECT (ce->widgets.window), str);
	g_return_if_fail (cdata != NULL);
	if (cdata->compile)
		gtk_entry_set_text (GTK_ENTRY (ce->widgets.compile_entry), cdata->compile);
	else
		gtk_entry_set_text (GTK_ENTRY (ce->widgets.compile_entry), "");

	if (cdata->make)
		gtk_entry_set_text (GTK_ENTRY (ce->widgets.make_entry), cdata->make);
	else
		gtk_entry_set_text (GTK_ENTRY (ce->widgets.make_entry), "");

	if (cdata->build)
		gtk_entry_set_text (GTK_ENTRY (ce->widgets.build_entry), cdata->build);
	else
		gtk_entry_set_text (GTK_ENTRY (ce->widgets.build_entry), "");

	if (cdata->execute)
		gtk_entry_set_text (GTK_ENTRY (ce->widgets.execute_entry), cdata->execute);
	else
		gtk_entry_set_text (GTK_ENTRY (ce->widgets.execute_entry), "");

	ce->current_command_data = cdata;
}

static void
on_load_global_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
	CommandEditor *ce;
	
	ce = user_data;
	g_return_if_fail (ce != NULL);
	sync_from_props (ce, ce->props_global);
}


static void
on_load_user_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
	CommandEditor *ce;
	
	ce = user_data;
	g_return_if_fail (ce != NULL);
	sync_from_props (ce, ce->props_user);
}

static void
on_help_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
}

static void
on_apply_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
	CommandEditor *ce;
	gchar* str, *key;
	gint i;

	ce = user_data;

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_COMBO(ce->widgets.language_combo)->entry), "changed");
	for (i = 0;; i+=2)
	{
		CommandData* cdata;

		if (prog_language_map [i] == NULL) break;
		
		/* Yes, prog_language_map[i] and not [i+1] */
		cdata = gtk_object_get_data (GTK_OBJECT (ce->widgets.window), prog_language_map[i]);

		g_return_if_fail (cdata != NULL);
		
		key = get_key_for_file_command (COMPILE_INDEX, prog_language_map[i+1]);
		prop_set_with_key (ce->props, key, cdata->compile);
		g_free (key);

		key = get_key_for_file_command (MAKE_INDEX, prog_language_map[i+1]);
		prop_set_with_key (ce->props, key, cdata->make);
		g_free (key);

		key = get_key_for_file_command (BUILD_INDEX, prog_language_map[i+1]);
		prop_set_with_key (ce->props, key, cdata->build);
		g_free (key);

		key = get_key_for_file_command (EXECUTE_INDEX, prog_language_map[i+1]);
		prop_set_with_key (ce->props, key, cdata->execute);
		g_free (key);
	}
	str = gtk_entry_get_text (GTK_ENTRY(ce->widgets.pix_editor_entry));
	key = get_key_for_file_command (OPEN_INDEX, "icon");
	prop_set_with_key (ce->props, key, str);
	g_free (key);

	str = gtk_entry_get_text (GTK_ENTRY(ce->widgets.image_editor_entry));
	key = get_key_for_file_command (OPEN_INDEX, "image");
	prop_set_with_key (ce->props, key, str);
	g_free (key);

	str = gtk_entry_get_text (GTK_ENTRY(ce->widgets.html_editor_entry));
	key = get_key_for_file_command (VIEW_INDEX, "html");
	prop_set_with_key (ce->props, key, str);
	g_free (key);

	if (NULL != (str = gtk_entry_get_text(GTK_ENTRY(ce->widgets.terminal_entry))))
		prop_set_with_key(ce->props, COMMAND_TERMINAL, str);

	anjuta_save_settings ();
}

static void
on_cancel_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
	CommandEditor *ce = user_data;
	if (NULL != ce)
		gnome_dialog_close(GNOME_DIALOG(ce->widgets.window));
}

static void
on_ok_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
	on_apply_clicked (NULL, user_data);
	on_cancel_clicked(NULL, user_data);
}

static gboolean
on_close (GtkWidget *w, gpointer *user_data) {
	CommandEditor *ce = (CommandEditor *)user_data;
	g_return_val_if_fail ((ce != NULL), FALSE);
	command_editor_hide(ce);
	return FALSE;
}


static gint
get_label_max_width(GtkWidget *widget, gint size)
{
	GtkRequisition requisition;

	gtk_widget_size_request(widget, &requisition);
	return MAX(size, requisition.width);
}

static void
create_command_editor_gui (CommandEditor *ce)
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *frame1;
	GtkWidget *table1;
	GtkWidget *label1;
	GtkWidget *combo1;
	GtkWidget *combo_entry1;
	GtkWidget *frame2;
	GtkWidget *table3;
	GtkWidget *label2;
	GtkWidget *label2a;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkWidget *entry1;
	GtkWidget *entry1a;
	GtkWidget *entry2;
	GtkWidget *entry3;
	GtkWidget *frame3;
	GtkWidget *table4;
	GtkWidget *label5;
	GtkWidget *label6;
	GtkWidget *label7;
	GtkWidget *label8;
	GtkWidget *terminal_combo;
	GtkWidget *entry4;
	GtkWidget *entry5;
	GtkWidget *entry6;
	GtkWidget *entry7;
	GtkWidget *button4;
	GtkWidget *button5;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button6;
	GList* list = NULL;
	gint i;
	gint label_max_width;

	dialog1 = gnome_dialog_new (_("Commands"), NULL);
	gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(app->widgets.window));
	gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog1), "commands", "Anjuta");
	gnome_dialog_close_hides (GNOME_DIALOG (dialog1), TRUE);
	
	dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox1);
	gtk_widget_set_usize (dialog_vbox1, 681, -2);
	
	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame1, TRUE, TRUE, 0);
	
	table1 = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame1), table1);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 5);
	
	label1 = gtk_label_new (_("Select programming language:"));
	gtk_widget_show (label1);
	gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_padding (GTK_MISC (label1), 5, 0);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, -1);
	
	combo1 = gtk_combo_new ();
	gtk_widget_show (combo1);
	gtk_table_attach (GTK_TABLE (table1), combo1, 1, 2, 0, 1,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (combo1), 5);
	gtk_entry_set_editable (GTK_ENTRY(GTK_COMBO(combo1)->entry), FALSE);

	for (i = 0;; i+=2)
	{
		if (prog_language_map[i] == NULL) break;
			list = g_list_append (list, prog_language_map[i]);
	}
	gtk_combo_set_popdown_strings (GTK_COMBO (combo1), list);
	g_list_free (list);
	combo_entry1 = GTK_COMBO (combo1)->entry;
	gtk_widget_show (combo_entry1);
	
	frame2 = gtk_frame_new (_(" Language specific commands "));
	gtk_widget_show (frame2);
	gtk_table_attach (GTK_TABLE (table1), frame2, 0, 2, 1, 2,
		    (GtkAttachOptions) (GTK_FILL),
		    (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);
	
	table3 = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table3);
	gtk_container_add (GTK_CONTAINER (frame2), table3);
	gtk_container_set_border_width (GTK_CONTAINER (table3), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table3), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table3), 5);
	
	label2 = gtk_label_new (_("Compile File:"));
	gtk_widget_show (label2);
	gtk_table_attach (GTK_TABLE (table3), label2, 0, 1, 0, 1,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	label_max_width = get_label_max_width(label2, 0);
	gtk_misc_set_alignment (GTK_MISC (label2), 0, -1);

	label2a = gtk_label_new (_("Make File:"));
	gtk_widget_show (label2a);
	gtk_table_attach (GTK_TABLE (table3), label2a, 0, 1, 3, 4,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	label_max_width = get_label_max_width(label2a, label_max_width);
	gtk_misc_set_alignment (GTK_MISC (label2a), 0, -1);

	label3 = gtk_label_new (_("Build File:"));
	gtk_widget_show (label3);
	gtk_table_attach (GTK_TABLE (table3), label3, 0, 1, 1, 2,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	label_max_width = get_label_max_width(label3, label_max_width);
	gtk_misc_set_alignment (GTK_MISC (label3), 0, -1);
	
	label4 = gtk_label_new (_("Execute File:"));
	gtk_widget_show (label4);
	gtk_table_attach (GTK_TABLE (table3), label4, 0, 1, 2, 3,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	label_max_width = get_label_max_width(label4, label_max_width);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, -1);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_table_attach (GTK_TABLE (table3), entry1, 1, 2, 0, 1,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);
	
	entry1a = gtk_entry_new ();
	gtk_widget_show (entry1a);
	gtk_table_attach (GTK_TABLE (table3), entry1a, 1, 2, 3, 4,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);
	
	entry2 = gtk_entry_new ();
	gtk_widget_show (entry2);
	gtk_table_attach (GTK_TABLE (table3), entry2, 1, 2, 1, 2,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);
	
	entry3 = gtk_entry_new ();
	gtk_widget_show (entry3);
	gtk_table_attach (GTK_TABLE (table3), entry3, 1, 2, 2, 3,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);
	
	frame3 = gtk_frame_new (_(" General commands "));
	gtk_widget_show (frame3);
	gtk_table_attach (GTK_TABLE (table1), frame3, 0, 2, 2, 3,
		    (GtkAttachOptions) (GTK_FILL),
		    (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame3), 5);
	
	table4 = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table4);
	gtk_container_add (GTK_CONTAINER (frame3), table4);
	gtk_container_set_border_width (GTK_CONTAINER (table4), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table4), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table4), 5);
	
	label5 = gtk_label_new (_("Pixmap editor:"));
	gtk_widget_show (label5);
	gtk_table_attach (GTK_TABLE (table4), label5, 0, 1, 0, 1,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	label_max_width = get_label_max_width(label5, label_max_width);
	gtk_misc_set_alignment (GTK_MISC (label5), 0, -1);
	
	label6 = gtk_label_new (_("Image editor:"));
	gtk_widget_show (label6);
	gtk_table_attach (GTK_TABLE (table4), label6, 0, 1, 1, 2,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	label_max_width = get_label_max_width(label6, label_max_width);
	gtk_misc_set_alignment (GTK_MISC (label6), 0, -1);

	label7 = gtk_label_new (_("HTML viewer:"));
	gtk_widget_show (label7);
	gtk_table_attach (GTK_TABLE (table4), label7, 0, 1, 2, 3,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	label_max_width = get_label_max_width(label7, label_max_width);
	gtk_misc_set_alignment (GTK_MISC (label7), 0, -1);

	label8 = gtk_label_new (_("Terminal Launcher:"));
	gtk_widget_show (label8);
	gtk_table_attach (GTK_TABLE (table4), label8, 0, 1, 3, 4,
		    (GtkAttachOptions) (0),
		    (GtkAttachOptions) (0), 0, 0);
	label_max_width = get_label_max_width(label8, label_max_width);
	gtk_misc_set_alignment (GTK_MISC (label8), 0, -1);

	label_max_width = (label_max_width > LABEL_MAX_WIDTH) ? LABEL_MAX_WIDTH : label_max_width;
	gtk_widget_set_usize (label2, label_max_width, -2);
	gtk_widget_set_usize (label2a, label_max_width, -2);
	gtk_widget_set_usize (label3, label_max_width, -2);
	gtk_widget_set_usize (label4, label_max_width, -2);
	gtk_widget_set_usize (label5, label_max_width, -2);
	gtk_widget_set_usize (label6, label_max_width, -2);
	gtk_widget_set_usize (label7, label_max_width, -2);
	gtk_widget_set_usize (label8, label_max_width, -2);

	entry4 = gtk_entry_new ();
	gtk_widget_show (entry4);
	gtk_table_attach (GTK_TABLE (table4), entry4, 1, 2, 0, 1,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);

	entry5 = gtk_entry_new ();
	gtk_widget_show (entry5);
	gtk_table_attach (GTK_TABLE (table4), entry5, 1, 2, 1, 2,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);
	
	entry6 = gtk_entry_new ();
	gtk_widget_show (entry6);
	gtk_table_attach (GTK_TABLE (table4), entry6, 1, 2, 2, 3,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);

	/* Terminal combo */
	terminal_combo = gtk_combo_new ();

	list = NULL;

	for (i = 0; term_commands[i] != NULL ; i++)
		list = g_list_append (list, term_commands[i]);

	gtk_combo_set_popdown_strings (GTK_COMBO (terminal_combo), list);
	g_list_free (list);

	gtk_widget_show (terminal_combo);
	gtk_table_attach (GTK_TABLE (table4), terminal_combo, 1, 2, 3, 4,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);

	entry7 = GTK_COMBO (terminal_combo)->entry;
	
	button4 = gtk_button_new_with_label (_("Load global defaults"));
	gtk_widget_show (button4);
	gtk_table_attach (GTK_TABLE (table1), button4, 0, 1, 3, 4,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button4), 5);

	button5 = gtk_button_new_with_label (_("Load user defaults"));
	gtk_widget_show (button5);
	gtk_table_attach (GTK_TABLE (table1), button5, 1, 2, 3, 4,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button5), 5);

	dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_HELP);
	button6 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button6);
	GTK_WIDGET_SET_FLAGS (button6, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
	button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_APPLY);
	button2 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button2);
	GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CANCEL);
	button3 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button3);
	GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

	gtk_signal_connect (GTK_OBJECT (dialog1), "close",
		      GTK_SIGNAL_FUNC (on_close),
		      ce);
	gtk_signal_connect (GTK_OBJECT (combo_entry1), "changed",
		      GTK_SIGNAL_FUNC (on_language_entry_changed),
		      ce);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
		      GTK_SIGNAL_FUNC (on_load_global_clicked),
		      ce);
	gtk_signal_connect (GTK_OBJECT (button5), "clicked",
		      GTK_SIGNAL_FUNC (on_load_user_clicked),
		      ce);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
		      GTK_SIGNAL_FUNC (on_ok_clicked),
		      ce);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
		      GTK_SIGNAL_FUNC (on_apply_clicked),
		      ce);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
		      GTK_SIGNAL_FUNC (on_cancel_clicked),
		      ce);
	gtk_signal_connect (GTK_OBJECT (button6), "clicked",
		      GTK_SIGNAL_FUNC (on_help_clicked),
		      ce);

	ce->widgets.window = dialog1;
	ce->widgets.language_combo = combo1;
	ce->widgets.compile_entry = entry1;
	ce->widgets.make_entry = entry1a;
	ce->widgets.build_entry = entry2;
	ce->widgets.execute_entry = entry3;
	ce->widgets.pix_editor_entry = entry4;
	ce->widgets.image_editor_entry = entry5;
	ce->widgets.html_editor_entry = entry6;
	ce->widgets.terminal_entry = entry7;

	gtk_widget_ref (dialog1);
	gtk_widget_ref (combo1);
	gtk_widget_ref (entry1);
	gtk_widget_ref (entry1a);
	gtk_widget_ref (entry2);
	gtk_widget_ref (entry3);
	gtk_widget_ref (entry4);
	gtk_widget_ref (entry5);
	gtk_widget_ref (entry6);
	gtk_widget_ref (entry7);
}

/* Save and Load */
gboolean
command_editor_save_yourself (CommandEditor *ce, FILE* stream)
{
	return TRUE;
}

gboolean
command_editor_load_yourself (CommandEditor *ce, PropsID pr)
{
	return TRUE;
}

