#include <config.h>

#include <string.h>
#include <gobject/gvaluecollector.h>
#include <libanjuta/anjuta-marshal.h>
#include "ianjuta-message-view.h"

GQuark
ianjuta_message_view_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("ianjuta-message-view-quark");
	}
	
	return quark;
}

void
ianjuta_message_view_append (IAnjutaMessageView *message_view,
							 const gchar *message, GError **error)
{
	g_return_if_fail (message_view != NULL);
	g_return_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view));

	IANJUTA_MESSAGE_VIEW_GET_IFACE (message_view)->append (message_view,
														   message,
														   error);
}

void
ianjuta_message_view_clear (IAnjutaMessageView *message_view, GError **error)
{
	g_return_if_fail (message_view != NULL);
	g_return_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view));

	IANJUTA_MESSAGE_VIEW_GET_IFACE (message_view)->clear (message_view,
														  error);
}

void
ianjuta_message_view_select_next (IAnjutaMessageView *message_view,
								  GError **error)
{
	g_return_if_fail (message_view != NULL);
	g_return_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view));

	IANJUTA_MESSAGE_VIEW_GET_IFACE (message_view)->select_next (message_view,
																error);
}

void
ianjuta_message_view_select_previous (IAnjutaMessageView *message_view,
									  GError **error)
{
	g_return_if_fail (message_view != NULL);
	g_return_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view));

	IANJUTA_MESSAGE_VIEW_GET_IFACE (message_view)->select_previous (message_view,
																	error);
}

gint
ianjuta_message_view_get_line (IAnjutaMessageView *message_view,
							   GError **error)
{
	g_return_val_if_fail (message_view != NULL, -1);
	g_return_val_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view), -1);

	return IANJUTA_MESSAGE_VIEW_GET_IFACE (message_view)->get_line (message_view,
																	error);
}

gchar*
ianjuta_message_view_get_message (IAnjutaMessageView *message_view,
								  GError **error)
{
	g_return_val_if_fail (message_view != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view), NULL);

	return IANJUTA_MESSAGE_VIEW_GET_IFACE (message_view)->get_message (message_view,
																	   error);
}

GList*
ianjuta_message_view_get_all_messages (IAnjutaMessageView *message_view,
									   GError **error)
{
	g_return_val_if_fail (message_view != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view), NULL);

	return IANJUTA_MESSAGE_VIEW_GET_IFACE (message_view)->get_all_messages (message_view,
																		    error);
}

static void
ianjuta_message_view_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	
	if (!initialized) {
		g_signal_new ("save-clicked",
			      IANJUTA_TYPE_MESSAGE_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IAnjutaMessageViewIface, message_clicked),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	}
}

GType
ianjuta_message_view_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (IAnjutaMessageViewIface),
			ianjuta_message_view_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "IAnjutaMessageView", 
					       &info,
					       0);
		
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;			
}
