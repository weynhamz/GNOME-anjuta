#ifndef _SEARCH_REPLACE_H
#define _SEARCH_REPLACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pcre.h>

void search_and_replace (void);
	
void search_replace_next(void);
	
void search_replace_previous(void);
	
void anjuta_search_replace_activate (gboolean replace, gboolean project);
	
	
#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_REPLACE_H */
