#ifndef _SEARCH_REPLACE_H
#define _SEARCH_REPLACE_H

#ifdef __cplusplus
extern "C"
{
#endif

void anjuta_search_replace_activate(void);
void function_select(TextEditor *te);
gboolean on_search_replace_delete_event(GtkWidget *window, GdkEvent *event
  , gboolean user_data);
void on_search_match_whole_word_toggled (GtkToggleButton *togglebutton, 
                                         gpointer user_data);
void on_search_match_whole_line_toggled (GtkToggleButton *togglebutton, 
                                         gpointer user_data);
void on_search_match_word_start_toggled (GtkToggleButton *togglebutton, 
                                         gpointer user_data);
void on_replace_regex_toggled (GtkToggleButton *togglebutton,
                               gpointer user_data);
void on_search_action_changed(GtkEditable *editable, gpointer user_data);
void on_search_target_changed(GtkEditable *editable, gpointer user_data);
void on_actions_no_limit_clicked(GtkButton *button, gpointer user_data);
void on_search_button_close_clicked(GtkButton *button, gpointer user_data);
void on_search_button_help_clicked(GtkButton *button, gpointer user_data);
void on_search_button_next_clicked(GtkButton *button, gpointer user_data);
void on_search_expression_activate(GtkEditable *edit, gpointer user_data);
void on_search_button_save_clicked(GtkButton *button, gpointer user_data);

#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_REPLACE_H */
