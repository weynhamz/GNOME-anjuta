#ifndef TM_SYMBOL_H
#define TM_SYMBOL_H

/*! \file
 The TMSymbol structure and related routines are used by TMProject to maintain a symbol
 hierarchy. The top level TMSymbol maintains a pretty simple hierarchy, consisting of
 compounds (classes and structs) and their children (member variables and functions).
*/

#include <glib.h>

#include "tm_tag.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _TMSymbol
{
	TMTag *tag;
	struct _TMSymbol *parent; /*!< Parent class/struct for functions/variables */
	GSList *children; /*!< List of member functions/variables for classes/structs */
} TMSymbol;

#define TM_SYMBOL(S) ((TMSymbol *) (S))

TMSymbol *tm_symbol_tree_new(GPtrArray *tags);
void tm_symbol_tree_free(gpointer root);
TMSymbol *tm_symbol_tree_update(TMSymbol *root, GPtrArray *tags);
gint tm_symbol_compare(gconstpointer p1, gconstpointer p2);

#ifdef __cplusplus
}
#endif

#endif /* TM_SYMBOL_H */
