#include "notebook.h"

GtkWidget*
notebook_new ()
{
  return GTK_WIDGET ( gtk_type_new (notebook_get_type ()));
}

static void
notebook_init (Notebook *ttt)
{
}

static gint
notebook_focus (GtkContainer     *container,
		    GtkDirectionType  direction)
{
  GtkNotebook *notebook;
  gint size;
  gint cur;

  g_return_val_if_fail (container != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_NOTEBOOK (container), FALSE);

  notebook = GTK_NOTEBOOK (container);

  if (!GTK_WIDGET_DRAWABLE (notebook) ||
      !GTK_WIDGET_IS_SENSITIVE (container) ||
      !notebook->children ||
      !notebook->cur_page)
    return FALSE;

  size = g_list_length(notebook->children);
  cur = (!notebook->cur_page)?
	  -1:
	  g_list_index (notebook->children, notebook->cur_page);

//  g_print("size=%d\ncur=%d\n",size,cur);
  switch (direction)
    {
    case GTK_DIR_TAB_FORWARD:
	    if(cur < size-1)
		gtk_notebook_next_page(notebook);
	    else
		    gtk_notebook_set_page(notebook,0);
      break;
    case GTK_DIR_TAB_BACKWARD:
	    if(cur > 0)
		gtk_notebook_prev_page(notebook);
	    else
	        gtk_notebook_set_page(notebook,size-1);
      break;
    default:
      return FALSE;
    }

  return TRUE;
}


static gint
notebook_key_press(GtkWidget *widget, GdkEventKey *event)
{
  GtkNotebook *notebook;
  GtkDirectionType direction = 0;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_NOTEBOOK (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  notebook = GTK_NOTEBOOK (widget);
  if (!notebook->children || !notebook->show_tabs)
    return FALSE;

  switch (event->keyval)
    {
    case GDK_Tab:
    case GDK_ISO_Left_Tab:
      if (event->state & GDK_SHIFT_MASK){
	direction = GTK_DIR_TAB_BACKWARD;
      } else {
	direction = GTK_DIR_TAB_FORWARD;
      }
      break;
    default:
      return FALSE;
    }
  return TRUE;
}

static void
notebook_class_init (NotebookClass *class)
{
	GtkWidgetClass *widget_class;
        GtkContainerClass *container_class;

	widget_class = (GtkWidgetClass *) class;
	container_class = (GtkContainerClass*) class;

	widget_class->key_press_event = notebook_key_press;
	container_class->focus = notebook_focus;
}

guint
notebook_get_type ()
{
  static guint ttt_type = 0;

  if (!ttt_type)
  {
    GtkTypeInfo ttt_info =
    {
	   "Notebook",
	   sizeof (Notebook),
	   sizeof (NotebookClass),
	   (GtkClassInitFunc) notebook_class_init,
	   (GtkObjectInitFunc) notebook_init,
	   (GtkArgSetFunc) NULL,
       (GtkArgGetFunc) NULL
    };
    ttt_type = gtk_type_unique (gtk_notebook_get_type (), &ttt_info);
  }
  return ttt_type;
}
