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

#include "anjuta-docman.h"
	
void search_incremental_init (AnjutaDocman *dm);
	
void toolbar_search_incremental_start (void);
	
void toolbar_search_incremental_end (void);
	
void toolbar_search_incremental (void);

void toolbar_search_clicked (void);
	
void enter_selection_as_search_target(void);	
	
	
#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_INCREMENTAL_H */
