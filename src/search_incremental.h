/***************************************************************************
 *            search_incremental.h
 *
 *  Mon Jan  5 21:17:10 2004
 *  Copyright  2004  Jean-Noel Guiheneuf
 *  jnoel@lotuscompounds.com
 ****************************************************************************/

#ifndef _SEARCH_INCREMENTAL_H
#define _SEARCH_INCREMENTAL_H

#ifdef __cplusplus
extern "C"
{
#endif

	
gboolean
on_toolbar_find_incremental_start (GtkEntry *entry, GdkEvent* e, 
	                               gpointer user_data);
gboolean
on_toolbar_find_incremental_end (GtkEntry *entry, GdkEvent*e, 
	                             gpointer user_data);
void
on_toolbar_find_incremental (GtkEntry *entry, gpointer user_data);

void
on_toolbar_find_clicked_cb (GtkButton *button, gpointer user_data);
	
void
enter_selection_as_search_target(void);	
	
	
#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_INCREMENTAL_H */
