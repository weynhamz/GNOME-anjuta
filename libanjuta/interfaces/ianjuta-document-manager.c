#include <config.h>

#include <string.h>
#include <gobject/gvaluecollector.h>
#include <libanjuta/anjuta-marshal.h>
#include "ianjuta-document-manager.h"

GQuark
ianjuta_document_manager_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("ianjuta-document-manager-quark");
	}
	
	return quark;
}

void
ianjuta_document_manager_goto_file_line (IAnjutaDocumentManager *docman,
										 const gchar *file, gint lineno,
										 GError **e)
{
	g_return_if_fail (docman != NULL);
	g_return_if_fail (file != NULL);
	
	g_return_if_fail (IANJUTA_IS_DOCUMENT_MANAGER (docman));

	IANJUTA_DOCUMENT_MANAGER_GET_IFACE (docman)->goto_file_line (docman,
																 file, lineno,
																 e);
}

IAnjutaEditor*
ianjuta_document_manager_get_current_editor (IAnjutaDocumentManager *docman,
											 GError **e)
{
	g_return_val_if_fail (docman != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_DOCUMENT_MANAGER (docman), NULL);

	return IANJUTA_DOCUMENT_MANAGER_GET_IFACE (docman)->get_current_editor (docman,
																			e);
}

void
ianjuta_document_manager_set_current_editor (IAnjutaDocumentManager *docman,
											 IAnjutaEditor *editor,
											 GError **e)
{
	g_return_if_fail (docman != NULL);
	g_return_if_fail (editor != NULL);
	
	g_return_if_fail (IANJUTA_IS_DOCUMENT_MANAGER (docman));
	g_return_if_fail (IANJUTA_IS_EDITOR (editor));

	IANJUTA_DOCUMENT_MANAGER_GET_IFACE (docman)->set_current_editor (docman,
																	 editor,
																	 e);
}

GList*
ianjuta_document_manager_get_editors (IAnjutaDocumentManager *docman,
									  GError **e)
{
	g_return_val_if_fail (docman != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_DOCUMENT_MANAGER (docman), NULL);

	return IANJUTA_DOCUMENT_MANAGER_GET_IFACE (docman)->get_editors (docman, e);
}

static void
ianjuta_document_manager_base_init (gpointer gclass)
{
/*
	static gboolean initialized = FALSE;
	
	if (!initialized) {
		g_signal_new ("save-point",
			      IANJUTA_TYPE_EDITOR,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IAnjutaDocumentManagerIface, save_point),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	}
*/
}

GType
ianjuta_document_manager_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (IAnjutaDocumentManagerIface),
			ianjuta_document_manager_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "IAnjutaDocumentManager", 
					       &info,
					       0);
		
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;			
}
