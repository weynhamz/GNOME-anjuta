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
	TMProject *project;
	GtkWidget *win;
	GtkWidget *tree;
} AnFileView;

AnFileView *fv_populate(TMProject *tm_proj);
void fv_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* AN_FILE_VIEW_H */
