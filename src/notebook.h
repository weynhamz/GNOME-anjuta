#ifndef __NOTEBOOK_H__
#define __NOTEBOOK_H__

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtknotebook.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NOTEBOOK(obj)          GTK_CHECK_CAST (obj, notebook_get_type (), Notebook)
#define NOTEBOOK_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, notebook_get_type (), NotebookClass)
#define IS_NOTEBOOK(obj)       GTK_CHECK_TYPE (obj, notebook_get_type ())

typedef struct _Notebook       Notebook;
typedef struct _NotebookClass  NotebookClass;

struct _Notebook
{
   GtkNotebook notebook;
};

struct _NotebookClass
{
   GtkNotebookClass parent_class;
};

guint          notebook_get_type        (void);
GtkWidget*     notebook_new             (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NOTEBOOK_H__ */
