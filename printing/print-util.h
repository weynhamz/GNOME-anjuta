/*
** Gnome-Print support
** Author: Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
** Original Author: Chema Celorio <chema@celorio.com>
** Licence: GPL
*/

#ifndef AN_PRINTING_PRINT_UTIL_H
#define AN_PRINTING_PRINT_UTIL_H

#include "print.h"

#ifdef __cplusplus
extern "C"
{
#endif

int anjuta_print_show(GnomePrintContext *pc, char const *text);
gboolean anjuta_print_verify_fonts(void);
void anjuta_print_progress_start(PrintJobInfo *pji);
void anjuta_print_progress_tick(PrintJobInfo *pji, gint page);
void anjuta_print_progress_end(PrintJobInfo *pji);

#ifdef __cplusplus
}
#endif

#endif /* AN_PRINTING_PRINT_UTIL_H */
