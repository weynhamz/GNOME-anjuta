#ifndef AN_SYMBOL_VIEW_H
#define AN_SYMBOL_VIEW_H

#include <gtk/gtk.h>

#include "tm_tagmanager.h"

#ifdef __cplusplus
extern "C"
{
#endif
typedef struct _AnSymbolView
{
	TMProject *project;
	GtkWidget *win;
	GtkWidget *tree;
} AnSymbolView;

AnSymbolView *sv_populate(TMProject *tm_proj);
void sv_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* AN_SYMBOL_VIEW_H */
