#include <gnome.h>
#include <glade/glade.h>

#include <sys/wait.h>

#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>

#include <libanjuta/anjuta-utils.h>

#include "config.h"

#include "text_editor.h"
#include "indent-util.h"
#include "indent-dialog.h"


#define INDENT_FILE_INPUT PACKAGE_DATA_DIR"/indent_test.c" 
#define INDENT_FILE_OUTPUT TMPDIR"/indent_test.c"



static IndentOption indent_option[] = {
	{"bl", FALSE, "bl_checkbutton", NULL},
	{"bli", FALSE, "bli_checkbutton", "bli_spinbutton"},
	{"br", FALSE, "br_checkbutton", NULL},
	{"ci", FALSE, "ci_checkbutton", "ci_spinbutton"},
	{"cli", FALSE, "cli_checkbutton", "cli_spinbutton"},
	{"cs", FALSE, "cs_checkbutton", NULL},
	{"pcs", TRUE, "pcs_checkbutton", NULL},
	{"saf", TRUE, "saf_checkbutton", NULL},
	{"sai", TRUE, "sai_checkbutton", NULL},
	{"saw", TRUE, "saw_checkbutton", NULL},
	{"bad", TRUE, "bad_checkbutton", NULL},
	{"bap", TRUE, "bap_checkbutton", NULL},
	{"sob", TRUE, "sob_checkbutton", NULL},
	{"cdb", TRUE, "cdb_checkbutton", NULL},
	{"bbb", FALSE, "bbb_checkbutton", NULL},	
	{"cd", FALSE, "cd_checkbutton", "cd_spinbutton"},
	{"ce", TRUE, "ce_checkbutton", NULL},
	{"c", FALSE, "c_checkbutton", "c_spinbutton"},
	{"cp", FALSE, "cp_checkbutton", "cp_spinbutton"},
	{"d", FALSE, "d_checkbutton", "d_spinbutton"},
	{"fc1", TRUE, "fc1_checkbutton", NULL},
	{"fca", TRUE, "fca_checkbutton", NULL},
	{"sc", TRUE, "sc_checkbutton", NULL},
	{"bc", TRUE, "bc_checkbutton", NULL},
	{"bls", FALSE, "bls_checkbutton", NULL},
	{"brs", FALSE, "brs_checkbutton", NULL},
	{"di", FALSE, "di_checkbutton", "di_spinbutton"},
	{"psl", TRUE, "psl_checkbutton", NULL},
	{"i", FALSE, "i_checkbutton", "i_spinbutton"},
	{"ip", FALSE, "ip_checkbutton", "ip_spinbutton"},
	{"lp", TRUE, "lp_checkbutton", NULL},
	{"ts", FALSE, "ts_checkbutton", "ts_spinbutton"},
	{"bbo", TRUE, "bbo_checkbutton", NULL},
	{"hnl", TRUE, "hnl_checkbutton", NULL},
	{"l", FALSE, "l_checkbutton", "l_spinbutton"},
	{NULL, FALSE, NULL, NULL}
};


static IndentStyle standard_indent_style[] = {
	{"GNU coding style", "-nbad -bap -bbo -nbc -bl -bli2 -bls -ncdb -nce -cp1 "
	 "-cs -di2 -nfc1 -nfca -hnl -i2 -ip5 -lp -pcs -nprs -psl -saf -sai -saw "
	 "-nsc -nsob", FALSE},
	{"Kernighan and Ritchie style", "-nbad -bap -nbc -bbo -br -brs -c33 -cd33 "
	 "-ncdb -ce -ci4 -cli0 -cp33 -cs -d0 -di1 -nfc1 -nfca -hnl -i4 -ip0 -l75 "
	 "-lp -npcs -nprs -npsl -saf -sai -saw -nsc -nsob -nss"	, FALSE},
	{"Original Berkeley style", "-nbad -nbap -bbo -bc -br -brs -c33 -cd33 -cdb "
	 "-ce -ci4 -cli0 -cp33 -di16 -fc1 -fca -hnl -i4 -ip4 -l75 -lp -npcs "
	 "-nprs -psl", FALSE},
	{"Anjuta coding style", "-l80 -lc80 -ts4 -i4 -sc -bli0 -bl0 -cbi0 -ss", FALSE},
	{"Style of Kangleipak", "-i8 -sc -bli0 -bl0 -cbi0 -ss", FALSE},	
	{"Hello World style", "-gnu -i0 -bli0 -cbi0 -cdb -sc -bl0 -ss", FALSE},	
	{NULL, NULL, FALSE}
};

/******************************************************************************/
void indent_init_hash(IndentData *idt);

void  indent_destroy_hash_data(gpointer key, gpointer data, gpointer user_data);
void indent_free_data(IndentData *idt);
void indent_free_style(IndentData *idt);

void indent_init_indent_style(IndentData *idt);
gint indent_load_all_style(IndentData *idt);

gchar *indent_alpha_string(gchar *option);
gboolean indent_option_is_numeric(gchar *option);
void indent_anal_option(gchar *option, IndentData *idt);
gint indent_compare_options(gchar *opt1, gchar *opt2);
gchar *indent_sort_options(gchar *line);
gint indent_compare_style(IndentStyle *style, gchar *name_style);

void indent_save_list_style(GList *list, IndentData *idt);

/*****************************************************************************/

IndentData *
indent_init(AnjutaPreferences *prefs)
{
	IndentData *idt;
	
	idt = g_new(IndentData, 1);
	indent_init_hash(idt);
	idt->dialog = NULL;
	idt->style_list = NULL;
	idt->checkbutton_blocked = FALSE;
	return idt;
}

void
indent_init_hash(IndentData *idt)
{
	int i;
	OptionData *ptroption;
	CheckData *ptrcheck;
	
	idt->option_hash = g_hash_table_new_full ((GHashFunc) g_str_hash,
	                                          (GEqualFunc) g_str_equal,
	                                          (GDestroyNotify) g_free, 
	                                          (GDestroyNotify) g_free);
	idt->check_hash = g_hash_table_new_full ((GHashFunc) g_str_hash, 
	                                         (GEqualFunc) g_str_equal,
	                                         (GDestroyNotify) g_free, 
	                                         (GDestroyNotify) g_free);
	idt->spin_hash = g_hash_table_new_full ((GHashFunc) g_str_hash, 
	                                        (GEqualFunc) g_str_equal,
	                                        (GDestroyNotify) g_free, 
	                                        (GDestroyNotify) g_free);
	for (i=0; indent_option[i].option != NULL; i++)
	{
		ptroption = g_new(OptionData, 1);
		ptroption->not_option = indent_option[i].not_option;
		ptroption->checkbutton = indent_option[i].checkbutton;
		ptroption->spinbutton = indent_option[i].spinbutton;
		g_hash_table_insert (idt->option_hash, indent_option[i].option, ptroption);
		ptrcheck = g_new(CheckData, 1);
		ptrcheck->option = indent_option[i].option;
		ptrcheck->not_option = indent_option[i].not_option;
		ptrcheck->spinbutton = indent_option[i].spinbutton;
		g_hash_table_insert (idt->check_hash, indent_option[i].checkbutton, ptrcheck);
		if (indent_option[i].spinbutton)
			g_hash_table_insert (idt->spin_hash, indent_option[i].spinbutton, 
		                         indent_option[i].option);
	}
}



void 
indent_destroy_hash_data(gpointer key, gpointer data, gpointer user_data)
{
	g_free(data);
}

void
indent_free_data(IndentData *idt)
{
	g_hash_table_foreach(idt->option_hash, indent_destroy_hash_data, NULL);	
	g_hash_table_foreach(idt->check_hash, indent_destroy_hash_data, NULL);
}

void
indent_free_style(IndentData *idt)
{
	GList *list;
	
	list = idt->style_list;
	while (list)
	{
		g_free(list->data);
		list = g_list_next(list);
	}
	g_list_free(idt->style_list);
}

void
indent_free(IndentData *idt)
{
	indent_free_style(idt);
	indent_free_data(idt);
}

void
indent_init_indent_style(IndentData *idt)
{
	IndentStyle *indent_style;
	gint i;
	
	for (i=0; standard_indent_style[i].name != NULL; i++)
	{
		indent_style = g_new(IndentStyle, 1);
		indent_style->name = standard_indent_style[i].name;
		indent_style->options = standard_indent_style[i].options;
		indent_style->modifiable = standard_indent_style[i].modifiable;
		idt->style_list =g_list_append(idt->style_list, indent_style);
	}
}

void
indent_init_load_style(IndentData *idt)
{
	indent_init_indent_style(idt);
	idt->style_active = indent_load_all_style(idt);
}


gint
indent_load_all_style(IndentData *idt)
{
	GSList *list2 = NULL;
	gchar *key;
	gchar *style_name = NULL;
	gchar *options = NULL;
	IndentStyle *indent_style;
	
	list2 = anjuta_preferences_get_list (idt->prefs, AUTOFORMAT_LIST_STYLE,
                                         GCONF_VALUE_STRING);
	if (list2 == NULL) 
		return 0;
	while (list2)
	{
		indent_style = g_new(IndentStyle, 1);
		indent_style->name = g_strdup((gchar*)list2->data);
		key = g_strdup((gchar*)list2->data);
		key = g_strconcat(AUTOFORMAT_OPTS, "/", g_strdelimit(key, " ", '_'), NULL); 
		options = anjuta_preferences_get (idt->prefs, key);
		indent_style->options = g_strdup(options);
		indent_style->modifiable = TRUE;
		idt->style_list =g_list_append(idt->style_list, indent_style);
		g_free(key);
		g_free(options);
		list2 = g_slist_next(list2);
	}
	g_slist_free(list2);
	if (!anjuta_preferences_get_pair (idt->prefs, AUTOFORMAT_STYLE,
                                 GCONF_VALUE_STRING, GCONF_VALUE_STRING,
                                 &style_name, &options))	
		return 0;
	else
		return indent_find_index(style_name, idt);
}

gchar *
indent_alpha_string(gchar *option)
{
	gchar *ptr = option;
	
	while(g_ascii_isalpha(*(ptr)) )
		ptr++;
	if (ptr == option) 
		return NULL;
	return g_strndup(option, ptr - option);
}

gboolean
indent_option_is_numeric(gchar *option)
{
	gboolean numeric = FALSE; 
	while (*option)
	{
		if (! g_ascii_isdigit(*(option++)) )
			return FALSE;
		else
			numeric = TRUE;
	}
	return numeric;
}

void
indent_anal_option(gchar *option, IndentData *idt)
{
	gboolean flag_n = FALSE;
	gchar *alpha_option;
	gchar *num;
	OptionData *ptrdata;
	
	if (*(option++) != '-') return;
	if (*option == 'n')
	{
		option++;
		flag_n = TRUE;
	}
	if (*option == 0) return;	
	
	if ((ptrdata = g_hash_table_lookup(idt->option_hash, option)) == NULL
		|| ptrdata->spinbutton)
	{
		if ( (alpha_option = indent_alpha_string(option)) == NULL)
			return;
		if ((ptrdata = g_hash_table_lookup(idt->option_hash, alpha_option)) == NULL)
			return;
		if ( flag_n && ptrdata->not_option)
		{
			g_free(alpha_option);
			return;
		}
		if ( !flag_n && (ptrdata->spinbutton != NULL))
		{			
			num = g_strdup(option + strlen(alpha_option));	
			if (!indent_option_is_numeric(num)) 
			{
				g_free(num);
				return;
			}
			indent_toggle_button_set_active(ptrdata->checkbutton, !flag_n, idt);
			indent_widget_set_sensitive(ptrdata->spinbutton, TRUE, idt);
			indent_spinbutton_set_value(ptrdata->spinbutton, num, idt);
			g_free(num);
		}
		g_free(alpha_option);
	}
	else
	{			
		if (!flag_n)
			indent_toggle_button_set_active(ptrdata->checkbutton, TRUE, idt);
		else
			if (ptrdata->not_option)
				indent_toggle_button_set_active(ptrdata->checkbutton, FALSE, idt);
	}
}


void
indent_anal_line_option(gchar *line, IndentData *idt)
{
	gchar **split;
	gint i = 0;
	
	split = g_strsplit(line, " ", -1);
	while (split[i])
	{
		if (strlen(split[i]) > 0)
			indent_anal_option(split[i], idt);
		i++;
	}
	g_strfreev(split);
}


gchar *
indent_delete_option(gchar *line, gchar *short_option, gboolean num)
{
	gchar **split;
	gint i=0;
	gchar *result; 
	gchar *ptr_start = NULL;
	gchar *ptr_end = NULL;
	gchar *opt;
	
	result = g_strdup("");
	split = g_strsplit(line, " ", -1);
	while (split[i])
	{
		if (strlen(split[i]) != 0)
		{
			ptr_start = split[i];
			if (*(ptr_start++) == '-')
			{
				if (*(ptr_start) == 'n')
					ptr_start++;
				ptr_end = ptr_start;
				if (num)
				{
					while(g_ascii_isalpha(*ptr_end))
						ptr_end++;
					opt = g_strndup(ptr_start, ptr_end - ptr_start);
				}
				else
				{
					while(g_ascii_isalnum(*ptr_end))
						ptr_end++;
					opt = g_strndup(ptr_start, ptr_end - ptr_start);
				}
				if (strlen(opt) > 0 && g_ascii_strcasecmp(opt, short_option) != 0)
					result= g_strconcat(result, split[i], " ", NULL);
				g_free(opt);
			}
		}
		i++;
	}
	g_strfreev(split);
	return result;
}

gint
indent_compare_options(gchar *opt1, gchar *opt2)
{
	if (*(opt1) == '-')
	{
		opt1++;
		if (*(opt1) == 'n') opt1++;
	}
	if (*(opt2) == '-')
	{
		opt2++;
		if (*(opt2) == 'n') opt2++;
	}
	return g_ascii_strcasecmp(opt1, opt2);
}

gchar *
indent_sort_options(gchar *line)
{
	gchar **split;
	gint i=0, j=0;
	gchar *tmp;
	gboolean exch = TRUE;
	gchar *result = "";
	
	split = g_strsplit(line, " ", -1);
	/* Remove empty items or not beginning by '-' */ 
	while (split[i])
	{
		if ((strlen(split[i]) != 0) && (*(split[i]) == '-'))
			split[j++] = split[i];
		i++;
	}	/*  j = number of items  */

	/* Sort split[] */ 
	while (exch)
	{
		exch = FALSE;
		for (i=0; i<j-1; i++)
		{
			if (indent_compare_options(split[i], split[i+1]) > 0)
			{
				tmp = split[i]; split[i] = split[i+1]; split[i+1] = tmp;
				exch = TRUE;
			}	
		}
	}
	
	for (i=0; i<j; i++)
		result = g_strconcat(result, split[i], " ", NULL);
	
	g_strfreev(split);
	return result;
}

gchar *
indent_insert_option(gchar *line, gchar *option)
{
	line = g_strconcat(option, " ", line, NULL);
	line = indent_sort_options(line);
	return line;
}

gint
indent_execute(gchar *line_option, IndentData *idt)
{
	gchar *cmd;
	gchar *options;
	pid_t pid;
	int status;

	options = g_strconcat(line_option, " ",INDENT_FILE_INPUT, NULL);
	cmd = g_strconcat ("indent ", options, " -o ", INDENT_FILE_OUTPUT, NULL);	
	g_free(options);

	pid = anjuta_util_execute_shell (PACKAGE_DATA_DIR, cmd);	
	
	waitpid (pid, &status, 0);
	g_free (cmd);
	return status;
}

gchar *
indent_get_buffer(void)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	GnomeVFSFileInfo info;
	gchar *read_buf = NULL;
	gchar *text_uri;

	text_uri = gnome_vfs_get_uri_from_local_path(INDENT_FILE_OUTPUT);
	result = gnome_vfs_get_file_info(text_uri, &info,
	                                 GNOME_VFS_FILE_INFO_DEFAULT |
	                                 GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);
	if(result != GNOME_VFS_OK )
	{
		g_warning("Cannot get info: %s\n", text_uri);
		return NULL;
	}
	if((result = gnome_vfs_open (&handle, text_uri,
								 GNOME_VFS_OPEN_READ)) != GNOME_VFS_OK)
	{
		g_warning("Cannot open: %s\n", text_uri);
		return NULL;
	}
	read_buf = g_new0(char, info.size + 1);
	
	result = gnome_vfs_read (handle, read_buf, info.size, NULL);
	if(!(result == GNOME_VFS_OK || result == GNOME_VFS_ERROR_EOF))
	{
		g_free (read_buf);
		g_warning("No file: %s\n", text_uri);	
		return NULL;
	}
	gnome_vfs_close (handle);
	return read_buf;
}


void
indent_save_list_style(GList *list, IndentData *idt)
{
	GSList *list2 = NULL;
	IndentStyle *idtst;

	while (list)
	{
		idtst = list->data;	
		if (idtst->modifiable)
			list2 = g_slist_append(list2, idtst->name);
		list = g_list_next(list);
	}
	anjuta_preferences_set_list (idt->prefs, AUTOFORMAT_LIST_STYLE,
					             GCONF_VALUE_STRING, list2);
	g_slist_free(list2);
}

void
indent_save_style(gchar *style_name, gchar *options, IndentData *idt)
{
	gchar *key;
	
	if (!anjuta_preferences_dir_exists (idt->prefs, AUTOFORMAT_OPTS))
		anjuta_preferences_add_dir (idt->prefs, AUTOFORMAT_OPTS, 
		                            GCONF_CLIENT_PRELOAD_NONE);
	
	key = g_strdup(style_name);
	key = g_strconcat(AUTOFORMAT_OPTS, "/", g_strdelimit(key, " ", '_'), NULL); 
	
	anjuta_preferences_set(idt->prefs, key, options); //**//
	g_free(key);
}

void
indent_save_all_style(IndentData *idt)
{
	GList *list1;
	IndentStyle *idtst;
	
	if (anjuta_preferences_dir_exists (idt->prefs, AUTOFORMAT_OPTS) )
		anjuta_preferences_remove_dir (idt->prefs, AUTOFORMAT_OPTS);
	anjuta_preferences_add_dir (idt->prefs, AUTOFORMAT_OPTS, 
	                            GCONF_CLIENT_PRELOAD_NONE);
	
	list1 = idt->style_list;
	indent_save_list_style(list1, idt);
	
	list1 = idt->style_list;
	while (list1)
	{
		idtst = list1->data;	
		if (idtst->modifiable)
			indent_save_style(idtst->name, idtst->options, idt);
		list1 = g_list_next(list1);
	}
}

gint
indent_compare_style(IndentStyle *style, gchar *name_style)
{
	return g_ascii_strcasecmp(style->name, name_style);
}

gchar *
indent_find_style(gchar *style_name, IndentData *idt)
{
	GList *list;
	
	list = g_list_find_custom(idt->style_list, style_name,
	                          (GCompareFunc) indent_compare_style);
	if (list)
		return ((IndentStyle*)list->data)->options;
	else
		return NULL;
}

gint
indent_find_index(gchar *style_name, IndentData *idt)
{
	gint index;
	GList *list;
	
	if (style_name == NULL)
		return 0;
	list = g_list_find_custom(idt->style_list, style_name,
	                          (GCompareFunc) indent_compare_style);	
	index = g_list_index(idt->style_list, list->data);
	return index;
}

gboolean
indent_remove_style(gchar *style_name, IndentData *idt)
{
	GList *list;
	
	list = g_list_find_custom(idt->style_list, style_name,
	                          (GCompareFunc) indent_compare_style);
	if ( list && ((IndentStyle*)list->data)->modifiable)
	{
		g_list_remove(list, list->data);
		return TRUE;
	}
	return FALSE;
}

gboolean
indent_update_style(gchar *style_name, gchar *options, IndentData *idt)
{
	GList *list;
	
	list = g_list_find_custom(idt->style_list, style_name,
	                          (GCompareFunc) indent_compare_style);
	if ( list && ((IndentStyle*)list->data)->modifiable)
	{
		((IndentStyle*)list->data)->name = style_name;
		((IndentStyle*)list->data)->options = options;
		
		return TRUE;
	}
	return FALSE;
}

void
indent_save_active_style(gchar *style_name, gchar *options, IndentData *idt)
{
	anjuta_preferences_set_pair (idt->prefs, AUTOFORMAT_STYLE,
					         GCONF_VALUE_STRING, GCONF_VALUE_STRING,
                             &style_name, &options);
}

gboolean
indent_add_style(gchar *style_name, IndentData *idt)
{
	GList *list;
	IndentStyle *indent_style;
	
	list = g_list_find_custom(idt->style_list, style_name,
	                          (GCompareFunc) indent_compare_style);
	if (list)
		return FALSE;
	else
	{
		indent_style = g_new(IndentStyle, 1);
		indent_style->name = style_name;
		indent_style->options = standard_indent_style[0].options;
		indent_style->modifiable = TRUE;
		idt->style_list =g_list_append(idt->style_list, indent_style);
		return TRUE;
	}
}
