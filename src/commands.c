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
#include "anjuta-tools.h"

#define LABEL_MAX_WIDTH 250

typedef struct _CommandEditorWidgets CommandEditorWidgets;
typedef struct _CommandData CommandData;

struct _CommandData
{
	gchar* key;

	gchar* compile;
	gchar* make;
	gchar* build;
	gchar* execute;
};

struct _CommandEditorWidgets
{
	GtkWidget *window;

	GtkWidget *pix_editor_entry;
	GtkWidget *image_editor_entry;
	GtkWidget *html_editor_entry;
	GtkWidget *terminal_entry;
	GtkWidget *language_om;
	GtkWidget *compile_entry;
	GtkWidget *build_entry;
	GtkWidget *execute_entry;
	GtkWidget *make_entry;
};

struct _CommandEditorPriv {
	CommandEditorWidgets widgets;
	CommandData *current_command_data;
	
	PropsID props;
	PropsID props_user;
	PropsID props_global;
};

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
prog_language_map_buildin[]=
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

static gchar **prog_language_map;

/* Ditto */
static gchar *
term_commands[] = {
	"gnome-terminal -e \"$(anjuta.current.command)\"",
	"gnome-terminal -x sh -c \"$(anjuta.current.command)\"",
	"gnome-terminal --command=\"sh -c \\\"$(anjuta.current.command); read x\\\"\"",
	"xterm -e sh -c \"$(anjuta.current.command)\"",

	NULL
};

static gboolean on_delete_event (GtkWidget *w, GdkEvent *event,
								 gpointer user_data);
static void on_response (GtkDialog *dialog, gint response,
							 gpointer user_data);
static void on_language_menu_changed (GtkOptionMenu *optionmenu,
									  gpointer user_data);
static void on_load_global_clicked (GtkButton *button, gpointer user_data);
static void on_load_user_clicked (GtkButton *button, gpointer user_data);

/* Command data to be used in command editor */
CommandData *command_data_new(void);
void command_data_destroy (CommandData *cdata);

/* Get the string version of the command */
gchar* command_data_get (CommandData *cdata, gchar *cmd);

/* String data should use these functions */
void command_data_set (CommandData *cdata, gchar* cmd, gchar *cmd_str);


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

CommandEditor*
command_editor_new (PropsID p_global, PropsID p_user, PropsID p)
{
	CommandEditor* ce;
	GList *command_lang_info;
	
	ce = g_new0 (CommandEditor, 1);
	ce->priv = g_new0 (CommandEditorPriv, 1);

	ce->priv->props = p;
	ce->priv->props_user = p_user;
	ce->priv->props_global = p_global;
	ce->priv->current_command_data = NULL;
	ce->priv->widgets.window = NULL;

	command_lang_info = glist_from_data (ce->priv->props_user, COMMAND_LANGUAGES);	
	if (!command_lang_info)
		prog_language_map = prog_language_map_buildin;
	else {
		GList *node;
		gint length, count;
		
		node = command_lang_info;
		length = g_list_length(node);
		prog_language_map = (gchar**) g_new (gchar*, length+3);
		count = 0;
		
		while (node) {
			if (node->data) {
				prog_language_map[count++] = g_strdup((gchar*)node->data);
			}
			node = g_list_next (node);
		}
		prog_language_map[count++] = NULL;
		prog_language_map[count++] = NULL;
		glist_strings_free (command_lang_info);
	}

	return ce;
}

void
command_editor_destroy (CommandEditor* ce)
{
	g_return_if_fail (ce != NULL);
	g_free (ce->priv);
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

	for (i = 0; prog_language_map[i] != NULL; i += 2) {
		CommandData* cdata;
		
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
		gtk_object_set_data_full (GTK_OBJECT (ce->priv->widgets.window),
			cdata->key, cdata, (GtkDestroyNotify) command_data_destroy);
	}
	ce->priv->current_command_data = NULL;
	g_signal_emit_by_name (ce->priv->widgets.language_om, "changed");

	key = get_key_for_file_command (OPEN_INDEX, "icon");
	str = prop_get (pr, key);
	g_free (key);
	gtk_entry_set_text (GTK_ENTRY(ce->priv->widgets.pix_editor_entry), str);
	g_free (str);

	key = get_key_for_file_command (OPEN_INDEX, "image");
	str = prop_get (pr, key);
	g_free (key);
	gtk_entry_set_text (GTK_ENTRY(ce->priv->widgets.image_editor_entry), str);
	g_free (str);

	key = get_key_for_file_command (VIEW_INDEX, "html");
	str = prop_get (pr, key);
	g_free (key);
	gtk_entry_set_text (GTK_ENTRY(ce->priv->widgets.html_editor_entry), str);
	g_free (str);

	if (NULL != (str = prop_get(pr, COMMAND_TERMINAL)))
	{
		gtk_entry_set_text(GTK_ENTRY(ce->priv->widgets.terminal_entry), str);
		g_free(str);
	}
}

/* ----- */
void
command_editor_show (CommandEditor *ce)
{
	GladeXML *gxml;
	GtkWidget *menu;
	GList *list;
	int i;

	g_return_if_fail (ce != NULL);
	if (ce->priv->widgets.window != NULL)
		return;

	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "commands_dialog", NULL);
	glade_xml_signal_autoconnect (gxml);
	ce->priv->widgets.window = glade_xml_get_widget (gxml, "commands_dialog");
	gtk_widget_show (ce->priv->widgets.window);
	gtk_window_set_transient_for (GTK_WINDOW (ce->priv->widgets.window),
								  GTK_WINDOW (app->widgets.window));
	ce->priv->widgets.pix_editor_entry = glade_xml_get_widget (gxml, "commands_pixmap_editor_entry");
	ce->priv->widgets.image_editor_entry = glade_xml_get_widget (gxml, "commands_image_editor_entry");
	ce->priv->widgets.html_editor_entry = glade_xml_get_widget (gxml, "commands_html_viewer_entry");
	ce->priv->widgets.terminal_entry = glade_xml_get_widget (gxml, "commands_terminal_entry");
	ce->priv->widgets.language_om = glade_xml_get_widget (gxml, "commands_language_om");
	ce->priv->widgets.compile_entry = glade_xml_get_widget (gxml, "commands_compile_entry");
	ce->priv->widgets.build_entry = glade_xml_get_widget (gxml, "commands_build_entry");
	ce->priv->widgets.execute_entry = glade_xml_get_widget (gxml, "commands_execute_entry");
	ce->priv->widgets.make_entry = glade_xml_get_widget (gxml, "commands_make_entry");

	/* Filling some terminal commands */
	list = NULL;

	for (i = 0; term_commands[i] != NULL; i++)
		list = g_list_append (list, term_commands[i]);

	gtk_combo_set_popdown_strings (GTK_COMBO (glade_xml_get_widget (gxml,
				"commands_terminal_combo")), list);

	g_list_free (list);

	/* Filling the different languages available */
	menu = gtk_menu_new ();
	gtk_widget_show (menu);

	for (i = 0; prog_language_map[i] != NULL; i += 2) {
		GtkWidget *item;

		item = gtk_menu_item_new_with_label (prog_language_map[i]);
		gtk_widget_show (item);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (ce->priv->widgets.language_om),
				  menu);

	g_signal_connect (ce->priv->widgets.window, "delete_event",
			  G_CALLBACK (on_delete_event), ce);
	g_signal_connect (ce->priv->widgets.window, "response",
			  G_CALLBACK (on_response), ce);
	g_signal_connect (ce->priv->widgets.language_om, "changed",
			  G_CALLBACK (on_language_menu_changed), ce);
	
	g_signal_connect (glade_xml_get_widget (gxml,
											"commands_global_defaults_button"),
					  "clicked",
					  G_CALLBACK (on_load_global_clicked), ce);
	g_signal_connect (glade_xml_get_widget (gxml,
											"commands_user_defaults_button"),
					 "clicked",
			  		 G_CALLBACK (on_load_user_clicked), ce);

	g_object_unref (gxml);
	sync_from_props (ce, ce->priv->props);
}

void
command_editor_hide (CommandEditor *ce)
{
	g_return_if_fail (ce);
	g_return_if_fail (ce->priv->widgets.window);
	gtk_widget_destroy (ce->priv->widgets.window);
	ce->priv->widgets.window = NULL;
}

gboolean
command_editor_save (CommandEditor *ce, FILE* s)
{
	gint i;
	gchar *str, *key;
	PropsID pr;

	g_return_val_if_fail (ce != NULL, FALSE);
	g_return_val_if_fail (s != NULL, FALSE);
	
	pr = ce->priv->props;
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
command_editor_get_command_file (CommandEditor* ce,
								 gchar* cmd_key, gchar* fname)
{
	g_return_val_if_fail (ce != NULL, NULL);
	g_return_val_if_fail (fname != NULL, NULL);
	g_return_val_if_fail (strlen (fname) != 0, NULL);

	return prop_get_new_expand (ce->priv->props, cmd_key, fname);
}

gchar*
command_editor_get_command (CommandEditor* ce, gchar* cmd_key)
{
	g_return_val_if_fail (ce != NULL, NULL);

	return prop_get_expanded (ce->priv->props, cmd_key);
}

static void
on_language_menu_changed (GtkOptionMenu *optionmenu,
			  gpointer       user_data)
{
	CommandData *cdata;
	CommandEditor *ce;
	const gchar *str;
	gint idx;

	ce = user_data;
	g_return_if_fail (ce != NULL);

	if (ce->priv->current_command_data) {
		str = gtk_entry_get_text (GTK_ENTRY (ce->priv->widgets.compile_entry));
		string_assign (&ce->priv->current_command_data->compile, str);

		str = gtk_entry_get_text (GTK_ENTRY (ce->priv->widgets.make_entry));
		string_assign (&ce->priv->current_command_data->make, str);

		str = gtk_entry_get_text (GTK_ENTRY (ce->priv->widgets.build_entry));
		string_assign (&ce->priv->current_command_data->build, str);

		str = gtk_entry_get_text (GTK_ENTRY (ce->priv->widgets.execute_entry));
		string_assign (&ce->priv->current_command_data->execute, str);
	}

	idx = gtk_option_menu_get_history (GTK_OPTION_MENU (optionmenu));
	str = prog_language_map[idx * 2];
	cdata = (CommandData *) gtk_object_get_data (GTK_OBJECT (ce->priv->widgets.window), str);
	g_return_if_fail (cdata != NULL);

	if (cdata->compile)
		gtk_entry_set_text (GTK_ENTRY (ce->priv->widgets.compile_entry), cdata->compile);
	else
		gtk_entry_set_text (GTK_ENTRY (ce->priv->widgets.compile_entry), "");

	if (cdata->make)
		gtk_entry_set_text (GTK_ENTRY (ce->priv->widgets.make_entry), cdata->make);
	else
		gtk_entry_set_text (GTK_ENTRY (ce->priv->widgets.make_entry), "");

	if (cdata->build)
		gtk_entry_set_text (GTK_ENTRY (ce->priv->widgets.build_entry), cdata->build);
	else
		gtk_entry_set_text (GTK_ENTRY (ce->priv->widgets.build_entry), "");

	if (cdata->execute)
		gtk_entry_set_text (GTK_ENTRY (ce->priv->widgets.execute_entry), cdata->execute);
	else
		gtk_entry_set_text (GTK_ENTRY (ce->priv->widgets.execute_entry), "");

	ce->priv->current_command_data = cdata;
}

static void
on_load_global_clicked (GtkButton *button,
			gpointer   user_data)
{
	CommandEditor *ce;
	
	ce = user_data;
	g_return_if_fail (ce != NULL);
	sync_from_props (ce, ce->priv->props_global);
}

static void
on_load_user_clicked (GtkButton *button,
		      gpointer   user_data)
{
	CommandEditor *ce;
	
	ce = user_data;
	g_return_if_fail (ce != NULL);
	sync_from_props (ce, ce->priv->props_user);
}

static void
sync_to_props (CommandEditor* ce)
{
	const gchar *str;
	gchar *key;
	gint i;

	g_signal_emit_by_name (ce->priv->widgets.language_om, "changed");
	for (i = 0;; i+=2)
	{
		CommandData* cdata;

		if (prog_language_map [i] == NULL) break;
		
		/* Yes, prog_language_map[i] and not [i+1] */
		cdata = (CommandData *) gtk_object_get_data (GTK_OBJECT (ce->priv->widgets.window), prog_language_map[i]);

		g_return_if_fail (cdata != NULL);
		
		key = get_key_for_file_command (COMPILE_INDEX, prog_language_map[i+1]);
		prop_set_with_key (ce->priv->props, key, cdata->compile);
		g_free (key);

		key = get_key_for_file_command (MAKE_INDEX, prog_language_map[i+1]);
		prop_set_with_key (ce->priv->props, key, cdata->make);
		g_free (key);

		key = get_key_for_file_command (BUILD_INDEX, prog_language_map[i+1]);
		prop_set_with_key (ce->priv->props, key, cdata->build);
		g_free (key);

		key = get_key_for_file_command (EXECUTE_INDEX, prog_language_map[i+1]);
		prop_set_with_key (ce->priv->props, key, cdata->execute);
		g_free (key);
	}
	str = gtk_entry_get_text (GTK_ENTRY(ce->priv->widgets.pix_editor_entry));
	key = get_key_for_file_command (OPEN_INDEX, "icon");
	prop_set_with_key (ce->priv->props, key, str);
	g_free (key);

	str = gtk_entry_get_text (GTK_ENTRY(ce->priv->widgets.image_editor_entry));
	key = get_key_for_file_command (OPEN_INDEX, "image");
	prop_set_with_key (ce->priv->props, key, str);
	g_free (key);

	str = gtk_entry_get_text (GTK_ENTRY(ce->priv->widgets.html_editor_entry));
	key = get_key_for_file_command (VIEW_INDEX, "html");
	prop_set_with_key (ce->priv->props, key, str);
	g_free (key);

	if (NULL != (str = gtk_entry_get_text(GTK_ENTRY(ce->priv->widgets.terminal_entry))))
		prop_set_with_key(ce->priv->props, COMMAND_TERMINAL, str);

	anjuta_save_settings ();
}

static void
on_response (GtkDialog *dialog, gint response, gpointer user_data)
{
	CommandEditor *ce = user_data;
	g_return_if_fail (ce);

	switch (response)
	{
	case GTK_RESPONSE_APPLY:
		sync_to_props (ce);
		return;
	case GTK_RESPONSE_OK:
		sync_to_props (ce);
	case GTK_RESPONSE_CANCEL:
		command_editor_hide (ce);
		return;
	case GTK_RESPONSE_HELP:
		return;
	}
}

static gboolean
on_delete_event (GtkWidget *w, GdkEvent *event,
				 gpointer   user_data)
{
	CommandEditor *ce = (CommandEditor *)user_data;
	command_editor_hide (ce);
	return FALSE;
}

static gint
get_label_max_width(GtkWidget *widget, gint size)
{
	GtkRequisition requisition;

	gtk_widget_size_request(widget, &requisition);
	return MAX(size, requisition.width);
}
