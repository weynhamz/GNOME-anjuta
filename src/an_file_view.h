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
	GtkWidget *menu;
	char *file;
} AnFileView;

AnFileView *fv_populate(void);
void fv_clear(void);
gboolean anjuta_fv_open_file(const char *path, gboolean use_anjuta);

#ifdef __cplusplus
}
#endif

#endif /* AN_FILE_VIEW_H */
