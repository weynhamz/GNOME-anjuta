/*
** Gnome-Print support
** Author: Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
** Original Author: Chema Celorio <chema@celorio.com>
** Licence: GPL
*/

#ifndef AN_PRINTING_PRINT_DOC_H
#define AN_PRINTING_PRINT_DOC_H

#include "print.h"

#ifdef __cplusplus
extern "C"
{
#endif

void  anjuta_print_document(PrintJobInfo *pji);
guint anjuta_print_document_determine_lines(PrintJobInfo *pji, gboolean real);

#ifdef __cplusplus
}
#endif

#endif /* AN_PRINTING_PRINT_DOC_H */
