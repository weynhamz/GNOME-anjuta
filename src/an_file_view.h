#ifndef AN_FILE_VIEW_H
#define AN_FILE_VIEW_H

#include <gtk/gtk.h>

#include "tm_tagmanager.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _AnFileView
{
	GtkWidget *win;
	GtkWidget *tree;
	TMFileEntry *file_tree;
	TMFileEntry *curr_entry;
	struct
	{
		GtkWidget *top;
		GtkWidget *open;
		GtkWidget *view;
		struct
		{
			GtkWidget *top;
			GtkWidget *update;
			GtkWidget *commit;
			GtkWidget *status;
			GtkWidget *log;
			GtkWidget *add;
			GtkWidget *remove;
			GtkWidget *diff;
		} cvs;
		GtkWidget *refresh;
		GtkWidget *docked;
	} menu;
} AnFileView;

AnFileView *fv_populate(gboolean full);
void fv_clear(void);
gboolean anjuta_fv_open_file(const char *path, gboolean use_anjuta);

#ifdef __cplusplus
}
#endif

#endif /* AN_FILE_VIEW_H */
