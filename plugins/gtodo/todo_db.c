#include <gtk/gtk.h>
#include <string.h>
#include "main.h"

int categorys = 0;



void read_categorys ()
{
	GTodoList *list;
	if (mw.mitems != NULL)
	{
		int i;
		for (i = 0; mw.mitems[i] != NULL; i++)
		{
			if (mw.mitems[i]->date != NULL)
				g_free (mw.mitems[i]->date);

			gtk_widget_destroy (mw.mitems[i]->item);
			g_free (mw.mitems[i]);
		}
		categorys = 0;
		mw.mitems = g_realloc (mw.mitems, (categorys + 1) * sizeof (int *));
		mw.mitems[0] = NULL;
	}

	list = gtodo_client_get_category_list (cl);
	if(list != NULL) 
	{
		do
		{
			mw.mitems = g_realloc (mw.mitems, (categorys + 2) * sizeof (int *));
			mw.mitems[categorys + 1] = NULL;
			mw.mitems[categorys] = g_malloc (sizeof (catitems));
			mw.mitems[categorys]->item =
				gtk_menu_item_new_with_label (gtodo_client_get_category_from_list
						(list));
			mw.mitems[categorys]->date =
				g_strdup (gtodo_client_get_category_from_list (list));
			gtk_menu_shell_append (GTK_MENU_SHELL (mw.menu),
					mw.mitems[categorys]->item);
			categorys++;
		}
		while (gtodo_client_get_list_next (list));

		gtodo_client_free_category_list (cl, list);
	}

	if(!gtodo_client_get_read_only(cl))
	{                             	
		mw.mitems = g_realloc (mw.mitems, (categorys + 3) * sizeof (catitems));
		mw.mitems[categorys + 2] = NULL;


		mw.mitems[categorys] = g_malloc (sizeof (catitems));
		mw.mitems[categorys]->item = gtk_separator_menu_item_new ();
		mw.mitems[categorys]->date = g_strdup ("");
		gtk_menu_shell_append (GTK_MENU_SHELL (mw.menu),
				mw.mitems[categorys]->item);



		mw.mitems[categorys + 1] = g_malloc (sizeof (catitems));

		mw.mitems[categorys + 1]->item =
			gtk_menu_item_new_with_label (_("Edit Categories"));
		gtk_menu_item_new_with_label (_("Edit Categories"));	
		mw.mitems[categorys + 1]->date = g_strdup (("Edit"));


		gtk_menu_shell_append (GTK_MENU_SHELL (mw.menu),
				mw.mitems[categorys + 1]->item);
	}

	gtk_widget_show_all (mw.menu);
	if (categorys > 0)
		gtk_option_menu_set_history (GTK_OPTION_MENU (mw.option), 0);
}



	void
load_category ()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GTodoList *list;
	gboolean all = FALSE;
	gchar *category;
	if (gtk_option_menu_get_history (GTK_OPTION_MENU (mw.option)) == 0)
		all = TRUE;
	if (!all)
		category =
			mw.mitems[gtk_option_menu_get_history (GTK_OPTION_MENU (mw.option)) -
			2]->date;
	else
		category = NULL;

	list = gtodo_client_get_todo_item_list (cl, category);
	if (list != NULL)
	{
		do
		{
			GTodoItem *item = gtodo_client_get_todo_item_from_list (list);
			if (item == NULL)
				break;
			if (((settings.hide_done && gtodo_todo_item_get_done (item)) ||
						(settings.hide_due
						 && MAX (0, gtodo_todo_item_check_due (item)) && gtodo_todo_item_check_due(item) != GTODO_NO_DUE_DATE)
						|| (settings.hide_nodate
							&& (gtodo_todo_item_check_due (item) ==
								GTODO_NO_DUE_DATE))) == FALSE)
			{
				gchar *priostr = NULL;
				gchar *datestr = NULL;
				gboolean due = FALSE;
				gchar *color = NULL;
				int i = 0;
				gchar *summary = NULL;

				/* create a priority string */
				if (gtodo_todo_item_get_priority (item) == GTODO_PRIORITY_LOW)		priostr= g_strdup (_("Low"));
				else if (gtodo_todo_item_get_priority (item) == GTODO_PRIORITY_MEDIUM) priostr = g_strdup (_("Medium"));
				else priostr = g_strdup (_("High"));

				/* check if due */
				due = gtodo_todo_item_check_due (item);
				if (due == GTODO_NO_DUE_DATE)
				{
					due = FALSE;
				}
				else if (due == 0 && settings.hl_today && gtodo_todo_item_check_due_time_minutes_left (item) == 0)
				{
					due = TRUE;
					color = settings.due_color;
				}

				else if (due == 0 && settings.hl_today)
				{
					due = TRUE;
					color = settings.due_today_color;
				}
				else if (due > 0 && settings.hl_due)
				{
					due = TRUE;
					color = settings.due_color;
				}
				else if (due > (-settings.due_days) && due < 0  && settings.hl_indays)
				{
					due = TRUE;
					color = settings.due_in_days_color;
				}
				else	due = FALSE;

				/* create a date string */
				datestr = gtodo_todo_item_get_due_date_as_string (item);
				if (datestr == NULL) datestr = g_strdup (_("No Date"));
				/* brought everything back in one big list store set..  in the hope to fix my sorting problems.. doesnt seems to work shamefully */

				/* escape the item's, this is required by pango markup */

				if(strlen(gtodo_todo_item_get_comment(item)) > 0)
				{
					gchar *sum = g_markup_escape_text(gtodo_todo_item_get_summary(item),-1);
					gchar *com = g_markup_escape_text(gtodo_todo_item_get_comment(item),-1);
					summary = g_strdup_printf("<b>%s</b>\n<i>%s</i>",
							sum,
							com);
					g_free(sum);
					g_free(com);
				}
				else
				{
					gchar *sum = g_markup_escape_text(gtodo_todo_item_get_summary(item),-1);
					summary = g_strdup_printf("<b>%s</b>", sum);
					g_free(sum);
				}


				gtk_list_store_append ((mw.list), &iter);
				gtk_list_store_set ((mw.list), &iter, 
						EDITABLE, TRUE, 
						CATEGORY, gtodo_todo_item_get_category (item),
						COMMENT, gtodo_todo_item_get_comment (item),
						SUMMARY, summary,
						ID, (guint64) gtodo_todo_item_get_id (item),
						DONE, gtodo_todo_item_get_done (item),
						START_DATE, (guint64) gtodo_todo_item_get_start_date_as_julian (item), 
						COMPLETED_DATE, (guint64) gtodo_todo_item_get_stop_date_as_julian(item), 
						END_DATE, (guint64) gtodo_todo_item_get_due_date_as_julian(item),
						PRIORITY,  gtodo_todo_item_get_priority (item), 
						PRIOSTR, priostr, 
						F_DATE, datestr,
						COLOR, color,
						DUE, due,-1);

				g_free(datestr);
				g_free (priostr);
				g_free(summary);

			}			/* show done */

		}
		while (gtodo_client_get_list_next (list));
		gtodo_client_free_todo_item_list (cl, list);
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mw.treeview));
		if (gtk_tree_model_get_iter_first (mw.sortmodel, &iter))
			gtk_tree_selection_select_iter (selection, &iter);
		gtk_widget_set_sensitive (mw.tbeditbut, TRUE);
		if(!gtodo_client_get_read_only(cl))
		{
			gtk_widget_set_sensitive (mw.tbdelbut, TRUE);
			// FIXME: gtk_widget_set_sensitive (gtk_item_factory_get_widget
			//		(GTK_ITEM_FACTORY (mw.item_factory),
			//		 N_("/Item/Edit")), TRUE);
			// FIXME: gtk_widget_set_sensitive (gtk_item_factory_get_widget
			//		(GTK_ITEM_FACTORY (mw.item_factory),
			//		 N_("/Item/Remove")), TRUE);
		}
	}
	else
	{
		gtk_widget_set_sensitive (mw.tbeditbut, FALSE);
		if(!gtodo_client_get_read_only(cl))
		{
			gtk_widget_set_sensitive (mw.tbdelbut, FALSE);
			// FIXME: gtk_widget_set_sensitive (gtk_item_factory_get_widget
			// 		(GTK_ITEM_FACTORY (mw.item_factory),
			//		 N_("/Item/Edit")), FALSE);
			// FIXME: gtk_widget_set_sensitive (gtk_item_factory_get_widget
			//		(GTK_ITEM_FACTORY (mw.item_factory),
			//		 N_("/Item/Remove")), FALSE);
		}

	}
}

	int
get_all_past_purge ()
{
	GTodoList *list;
	GTodoItem *item;
	guint today = 0;
	GDate *date = g_date_new ();
	g_date_set_time (date, time (NULL));
	if (g_date_valid (date) == FALSE)
	{
		g_date_free (date);
		return FALSE;
	}
	today = g_date_get_julian (date);
	g_date_free (date);
	if (!g_date_valid_julian (today))
		return FALSE;
	if (cl == NULL)
	{
		return FALSE;
	}
	list = gtodo_client_get_todo_item_list (cl, NULL);
	if (list == NULL)
	{
		return FALSE;
	}
	do
	{
		item = gtodo_client_get_todo_item_from_list (list);
		if (gtodo_todo_item_get_done (item))
		{
			guint32 i = gtodo_todo_item_get_stop_date_as_julian (item);
			if (i != 1)
			{
				if (i < (today - settings.purge_days))
				{
					g_print ("auto-purge delete %i\n",
							gtodo_todo_item_get_id (item));
					gtodo_client_delete_todo_by_id (cl,
							gtodo_todo_item_get_id
							(item));
				}
			}
		}

	}
	while (gtodo_client_get_list_next (list));
	gtodo_client_free_todo_item_list (cl, list);
	return TRUE;
}
