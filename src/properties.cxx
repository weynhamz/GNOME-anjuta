
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1

#include "PropSet.h"
#include "properties.h"

static bool GetFullLine(const char *&fpc, int &lenData, char *s, int len) {
	bool continuation = true;
	s[0] = '\0';
	while ((len > 1) && lenData > 0) {
		char ch = *fpc;
		fpc++;
		lenData--;
		if ((ch == '\r') || (ch == '\n')) {
			if (!continuation) {
				if ((lenData > 0) && (ch == '\r') && ((*fpc) == '\n')) {
					// munch the second half of a crlf
					fpc++;
					lenData--;
				}
				*s = '\0';
				return true;
			}
		} else if ((ch == '\\') && (lenData > 0) && ((*fpc == '\r') || (*fpc == '\n'))) {
			continuation = true;
		} else {
			continuation = false;
			*s++ = ch;
			*s = '\0';
			len--;
		}
	}
	return false;
}

bool PropSetFile::ReadLine(char *linebuf, bool ifIsTrue, const char *directoryForImports) {
	if (isalpha(linebuf[0]))    // If clause ends with first non-indented line
		ifIsTrue = true;
	if (isprefix(linebuf, "if ")) {
		const char *expr = linebuf + strlen("if") + 1;
		ifIsTrue = GetInt(expr);
	} else if (isprefix(linebuf, "import ") && directoryForImports) {
		char importPath[1024];
		strcpy(importPath, directoryForImports);
		strcat(importPath, linebuf + strlen("import") + 1);
		strcat(importPath, ".properties");
		Read(importPath, directoryForImports);
	} else if (isalpha(linebuf[0])) {
		Set(linebuf);
	} else if (isspace(linebuf[0]) && ifIsTrue) {
		Set(linebuf);
	}
	return ifIsTrue;
}

void PropSetFile::ReadFromMemory(const char *data, int len, const char *directoryForImports) {
	const char *pd = data;
	char linebuf[60000];
	bool ifIsTrue = true;
	while (len > 0) {
		GetFullLine(pd, len, linebuf, sizeof(linebuf));
		ifIsTrue = ReadLine(linebuf, ifIsTrue, directoryForImports);
	}
}

void PropSetFile::Read(const char *filename, const char *directoryForImports) {
	char propsData[60000];
#ifdef __vms
	FILE *rcfile = fopen(filename, "r");
#else
	FILE *rcfile = fopen(filename, "rb");
#endif
	if (rcfile) {
		int lenFile = fread(propsData, 1, sizeof(propsData), rcfile);
		fclose(rcfile);
		ReadFromMemory(propsData, lenFile, directoryForImports);
	} else {
		//printf("Could not open <%s>\n", filename);
	}

}

// Global property bank for anjuta.
static GList *anjuta_propset;

static PropSetFile*
get_propset(PropsID pi)
{
  PropSetFile* p;
  if(pi < 0 || (guint)pi >= g_list_length(anjuta_propset))
  {
  	g_warning("Invalind PropSetFile handle");
  	return NULL;
  }
  p = (PropSetFile*)g_list_nth_data(anjuta_propset, pi);
  if (p == NULL)
  	g_warning("Trying to access already destroyed PropSetFile object");
  return p;
}

// Followings are the C++ to C interface for the PropSetFile

PropsID
prop_set_new(void)
{
   PropsID handle;
   PropSetFile *p;
   gint length;

   length = g_list_length(anjuta_propset);
   p = new PropSetFile;
   anjuta_propset = g_list_append(anjuta_propset, (gpointer)p);
   handle = g_list_length(anjuta_propset);
   if (length == handle)
   {
   	g_warning("Unable to create PropSetFile Object");
   	return -1;
   }
   return handle-1;
}

gpointer prop_get_pointer(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  return (gpointer)p;
}

void
prop_set_destroy(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  g_list_nth(anjuta_propset, handle)->data = NULL;
  delete p;
}

void
prop_set_parent(PropsID handle1, PropsID handle2)
{
  PropSetFile *p1, *p2;
  p1 = get_propset(handle1);
  p2 = get_propset(handle2);
  if(!p1 || !p2) return;
  p1->superPS = p2;
}

void
prop_set_with_key(PropsID handle, const gchar *key, const gchar *val)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->Set(key, val);
}

void
prop_set(PropsID handle, gchar *keyval)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->Set(keyval);
}

gchar*
prop_get(PropsID handle, gchar *key)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->Get(key);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

gchar*
prop_get_expanded(PropsID handle, const gchar *key)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->GetExpanded(key);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

gchar*
prop_expand(PropsID handle, const gchar *withvars)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->Expand(withvars);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

int
prop_get_int(PropsID handle, const gchar *key, gint defaultValue=0)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return defaultValue;
  return p->GetInt(key, defaultValue);
}

gchar*
prop_get_wild(PropsID handle, const gchar *keybase, const gchar *filename)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->GetWild(keybase, filename);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

gchar*
prop_get_new_expand(PropsID handle, const gchar *keybase, const gchar *filename)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->GetNewExpand(keybase, filename);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

void
prop_clear(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->Clear();
}

void
prop_read_from_memory(PropsID handle, const gchar *data, gint len,
		const gchar *directoryForImports=0)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->ReadFromMemory(data, len, directoryForImports);
}

void
prop_read(PropsID handle, const gchar *filename, const gchar *directoryForImports)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->Read( filename, directoryForImports);
}
