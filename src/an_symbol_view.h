#ifndef AN_SYMBOL_VIEW_H
#define AN_SYMBOL_VIEW_H

#include <gtk/gtk.h>

#include "tm_tagmanager.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _SymbolFileInfo
{
	char *sym_name;
	struct
	{
		char *name;
		glong line;
	} def;
	struct
	{
		char *name;
		glong line;
	} decl;
} SymbolFileInfo;

typedef struct _AnSymbolView
{
	GtkWidget *win;
	GtkWidget *tree;
	struct
	{
		GtkWidget *top;
		GtkWidget *goto_decl;
		GtkWidget *goto_def;
		GtkWidget *find;
		GtkWidget *refresh;
		GtkWidget *docked;
	} menu;
	SymbolFileInfo *sinfo;
} AnSymbolView;

AnSymbolView *sv_populate (gboolean full);
void	      sv_clear (void);

#ifdef __cplusplus
}
#endif

#endif /* AN_SYMBOL_VIEW_H */
