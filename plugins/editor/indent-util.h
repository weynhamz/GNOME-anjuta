#ifndef _INDENT_UTIL_H_
#define _INDENT_UTIL_H_

#include <gconf/gconf-client.h>
#include <libanjuta/anjuta-preferences.h>


typedef struct _IndentData
{
	GladeXML *xml;
	GtkWidget *dialog;
	GHashTable *option_hash;
	GHashTable *check_hash;
	GHashTable *spin_hash;
	GList *style_list;
	gint style_active;
	GtkWidget *pref_indent_combo;
	GtkWidget *pref_indent_options;
	gboolean checkbutton_blocked;
	AnjutaPreferences *prefs;
} IndentData;

typedef struct _IndentStyle
{
	gchar *name;
	gchar *options;
	gboolean modifiable;
} IndentStyle;


typedef struct _IndentOption
{
	gchar *option;
	gboolean not_option;
	gchar *checkbutton;
	gchar *spinbutton;
} IndentOption;


typedef struct _OptionData
{
	gboolean not_option;
	gchar *checkbutton;
	gchar *spinbutton;
} OptionData;

typedef struct _CheckData
{
	gchar *option;
	gboolean not_option;
	gchar *spinbutton;
} CheckData;

IndentData *indent_init(AnjutaPreferences *prefs);


void indent_free_data(IndentData *idt);
void indent_init_load_style(IndentData *idt);

gchar *indent_find_style(gchar *style_name, IndentData *idt);
void indent_save_active_style(gchar *style_name, gchar *options, IndentData *idt);
gchar * indent_delete_option(gchar *line, gchar *short_option, gboolean num);
gchar * indent_insert_option(gchar *line, gchar *option);
void indent_anal_line_option(gchar *line, IndentData *idt);
gint indent_execute(gchar *line_option, IndentData *idt);
gchar *indent_get_buffer(void);
gboolean indent_add_style(gchar *style_name, IndentData *idt);
void indent_save_all_style(IndentData *idt);
gint indent_find_index(gchar *style_name, IndentData *idt);
gboolean indent_update_style(gchar *style_name, gchar *options, IndentData *idt); 
void indent_save_style(gchar *style_name, gchar *options, IndentData *idt);
gboolean indent_remove_style(gchar *style_name, IndentData *idt);

void indent_free(IndentData *idt);

#endif
