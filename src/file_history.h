#ifndef _FILE_HISTORY_H
#define _FILE_HISTORY_H

#include <glib.h>

#include "anjuta.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _FileLineInfo AnHistFile;
AnHistFile *an_hist_file_new(const char *name, glong line);
void an_hist_file_free(AnHistFile *h_file);

void an_file_history_reset(void);
void an_file_history_push(const char *filename, glong line);
void an_file_history_back(void);
void an_file_history_forward(void);
void an_file_history_dump(void);
void an_file_history_free(void);

#ifdef __cplusplus
}
#endif

#endif /* _FILE_HISTORY_H */
