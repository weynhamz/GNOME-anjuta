/* Scroll menu: a scrolling menu (if it's beyond screen size)
 * (C) 2000  Eazel, Inc.
 *
 * Authors:  George Lebl
 */
#ifndef SCROLL_MENU_H
#define SCROLL_MENU_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_SCROLL_MENU          (scroll_menu_get_type ())
#define SCROLL_MENU(obj)          GTK_CHECK_CAST ((obj), scroll_menu_get_type (), ScrollMenu)
#define SCROLL_MENU_CLASS(klass)  GTK_CHECK_CLASS_CAST ((klass), scroll_menu_get_type (), ScrollMenuClass)
#define IS_SCROLL_MENU(obj)       GTK_CHECK_TYPE ((obj), scroll_menu_get_type ())

typedef struct _ScrollMenu        ScrollMenu;
typedef struct _ScrollMenuClass   ScrollMenuClass;

struct _ScrollMenu
{
	GtkMenu			menu;

	/*< private >*/
	int			offset;
	int			max_offset;

	gboolean		scroll;

	GtkWidget		*up_scroll /* GtkButton */;
	GtkWidget		*down_scroll /* GtkButton */;
	gboolean		in_up;
	gboolean		in_down;

	int			scroll_by;
	guint			scroll_timeout;
};

struct _ScrollMenuClass
{
	GtkMenuClass parent_class;
};

GtkType		scroll_menu_get_type		(void) G_GNUC_CONST;
GtkWidget *	scroll_menu_new			(void);
void scroll_menu_map             (GtkWidget        *widget);
void scroll_menu_unmap           (GtkWidget        *widget);
void scroll_menu_insert_sorted (GtkMenu *menu,
				GtkWidget     *child);

typedef struct _GtkMenuShell2	   GtkMenuShell2;

struct _GtkMenuShell2
{
  GtkContainer container;
  
  GList *children;
  GtkWidget *active_menu_item;
  GtkWidget *parent_menu_shell;
  
  guint active : 1;
  guint have_grab : 1;
  guint have_xgrab : 1;
  guint button : 2;
  guint ignore_leave : 1;
  guint menu_flag : 1;
  guint ignore_enter : 1;
  
  guint32 activate_time;
};

#ifdef __cplusplus
}
#endif

#endif /* SCROLL_MENU_H */
