#ifndef _IANJUTA_MESSAGE_VIEW_H_
#define _IANJUTA_MESSAGE_VIEW_H_

#include <glib-object.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define IANJUTA_TYPE_MESSAGE_VIEW            (ianjuta_message_view_get_type ())
#define IANJUTA_MESSAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IANJUTA_TYPE_MESSAGE_VIEW, IAnjutaMessageView))
#define IANJUTA_IS_MESSAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IANJUTA_TYPE_MESSAGE_VIEW))
#define IANJUTA_MESSAGE_VIEW_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IANJUTA_TYPE_MESSAGE_VIEW, IAnjutaMessageViewIface))

#define IANJUTA_MESSAGE_VIEW_ERROR ianjuta_message_view_error_quark()

typedef struct _IAnjutaMessageView       IAnjutaMessageView;
typedef struct _IAnjutaMessageViewIface  IAnjutaMessageViewIface;

struct _IAnjutaMessageViewIface {
	GTypeInterface g_iface;
	
	/* Signals */
	void (*message_clicked) (IAnjutaMessageView *message_view,
							 const gchar *message);

	/* Virtual Table */
	void (*append) (IAnjutaMessageView *message_view,
					const gchar *message, GError  **e);
	void (*clear) (IAnjutaMessageView *message_view, GError **e);
	void (*select_next) (IAnjutaMessageView *message_view, GError **e);
	void (*select_previous) (IAnjutaMessageView *message_view, GError **e);
	gint (*get_line) (IAnjutaMessageView *message_view, GError **e);
	gchar* (*get_message) (IAnjutaMessageView *message_view, GError **e);
	GList* (*get_all_messages) (IAnjutaMessageView *message_view, GError **e);
};

enum IAnjutaMessageViewError {
	IANJUTA_MESSAGE_VIEW_ERROR_DOESNT_EXIST,
};

GQuark ianjuta_message_view_error_quark (void);
GType  ianjuta_message_view_get_type (void);

void ianjuta_message_view_append (IAnjutaMessageView *message_view,
								  const gchar* message, GError **e);
void ianjuta_message_view_clear (IAnjutaMessageView *message_view,
								 GError **e);
void ianjuta_message_view_select_next (IAnjutaMessageView *message_view,
									   GError **e);
void ianjuta_message_view_select_previous (IAnjutaMessageView *message_view,
										   GError **e);
gint ianjuta_message_view_get_line (IAnjutaMessageView *message_view,
									GError **e);
gchar* ianjuta_message_view_get_message (IAnjutaMessageView *message_view,
										GError **e);
GList* ianjuta_message_view_get_all_messages (IAnjutaMessageView *message_view,
											  GError **e);

G_END_DECLS

#endif
