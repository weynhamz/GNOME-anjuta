#ifndef _FILE_HISTORY_H
#define _FILE_HISTORY_H

#include <glib.h>

// #include "anjuta.h"
#include "anjuta-docman.h"

G_BEGIN_DECLS

typedef struct _AnHistFile AnHistFile;

struct _AnHistFile
{
	gchar *file;
	glong line;
};

AnHistFile *an_hist_file_new(const char *name, glong line);
void an_hist_file_free(AnHistFile *h_file);

void an_file_history_reset(void);
void an_file_history_push(const char *filename, glong line);
void an_file_history_back(AnjutaDocman *docman);
void an_file_history_forward(AnjutaDocman *docman);
void an_file_history_dump(void);
void an_file_history_free(void);

G_END_DECLS

#endif /* _FILE_HISTORY_H */
