
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1

#include "PropSet.h"
#include "properties_cxx.h"
#include "properties.h"

bool PropSetFile::caseSensitiveFilenames = false;

PropSetFile::PropSetFile(bool lowerKeys_) : lowerKeys(lowerKeys_) {}

PropSetFile::~PropSetFile() {}

/**
 * Get a line of input. If end of line escaped with '\\' then continue reading.
 */
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
			if ((lenData > 1) && ((*fpc == '\r') && (*(fpc+1) == '\r') || (*fpc == '\n') && (*(fpc+1) == '\n')))
				continuation = false;
			else if ((lenData > 2) && ((*fpc == '\r') && (*(fpc+1) == '\n') && (*(fpc+2) == '\n' || *(fpc+2) == '\r')))
				continuation = false;
		} else {
			continuation = false;
			*s++ = ch;
			*s = '\0';
			len--;
		}
	}
	return false;
}

static bool IsSpaceOrTab(char ch) {
	return (ch == ' ') || (ch == '\t');
}

static bool IsCommentLine(const char *line) {
	while (IsSpaceOrTab(*line)) ++line;
	return (*line == '#');
}

bool PropSetFile::ReadLine(const char *lineBuffer, bool ifIsTrue, const char *directoryForImports) {
	if (!IsSpaceOrTab(lineBuffer[0]))    // If clause ends with first non-indented line
		ifIsTrue = true;
	if (isprefix(lineBuffer, "if ")) {
		const char *expr = lineBuffer + strlen("if") + 1;
		ifIsTrue = GetInt(expr) != 0;
	} else if (isprefix(lineBuffer, "import ") && directoryForImports) {
		char importPath[1024];
		strcpy(importPath, directoryForImports);
		strcat(importPath, lineBuffer + strlen("import") + 1);
		strcat(importPath, ".properties");
		Read(importPath, directoryForImports);
	} else if (ifIsTrue && !IsCommentLine(lineBuffer)) {
		Set(lineBuffer);
	}
	return ifIsTrue;
}

void PropSetFile::ReadFromMemory(const char *data, int len, const char *directoryForImports) {
	const char *pd = data;
	char lineBuffer[60000];
	bool ifIsTrue = true;
	while (len > 0) {
		GetFullLine(pd, len, lineBuffer, sizeof(lineBuffer));
		if (lowerKeys) {
			for (int i=0; lineBuffer[i] && (lineBuffer[i] != '='); i++) {
				if ((lineBuffer[i] >= 'A') && (lineBuffer[i] <= 'Z')) {
					lineBuffer[i] = static_cast<char>(lineBuffer[i] - 'A' + 'a');
				}
			}
		}
		ifIsTrue = ReadLine(lineBuffer, ifIsTrue, directoryForImports);
	}
}

bool PropSetFile::Read(const char *filename, const char *directoryForImports) {
#ifdef __vms
	FILE *rcfile = fopen(filename, "r");
#else
	FILE *rcfile = fopen(filename, "rb");
#endif
	if (rcfile) {
		char propsData[60000];
		int lenFile = fread(propsData, 1, sizeof(propsData), rcfile);
		fclose(rcfile);
		ReadFromMemory(propsData, lenFile, directoryForImports);
		return true;

	} else {
		//printf("Could not open <%s>\n", filename);
		return false;
	}
}

static inline char MakeUpperCase(char ch) {
	if (ch < 'a' || ch > 'z')
		return ch;
	else
		return static_cast<char>(ch - 'a' + 'A');
}

static bool StringEqual(const char *a, const char *b, size_t len, bool caseSensitive) {
	if (caseSensitive) {
		for (size_t i = 0; i < len; i++) {
			if (a[i] != b[i])
				return false;
		}
	} else {
		for (size_t i = 0; i < len; i++) {
			if (MakeUpperCase(a[i]) != MakeUpperCase(b[i]))
				return false;
		}
	}
	return true;
}

// Match file names to patterns allowing for a '*' suffix or prefix.
static bool MatchWild(const char *pattern, size_t lenPattern, const char *fileName, bool caseSensitive) {
	size_t lenFileName = strlen(fileName);
	if (lenFileName == lenPattern) {
		if (StringEqual(pattern, fileName, lenFileName, caseSensitive)) {
			return true;
		}
	}
	if (lenFileName >= lenPattern-1) {
		if (pattern[0] == '*') {
			// Matching suffixes
			return StringEqual(pattern+1, fileName + lenFileName - (lenPattern-1), lenPattern-1, caseSensitive);
		} else if (pattern[lenPattern-1] == '*') {
			// Matching prefixes
			return StringEqual(pattern, fileName, lenPattern-1, caseSensitive);
		}
	}
	return false;
}

SString PropSetFile::GetWildUsingStart(const PropSet &psStart, const char *keybase, const char *filename) {

	for (int root = 0; root < hashRoots; root++) {
		for (Property *p = props[root]; p; p = p->next) {
			if (isprefix(p->key, keybase)) {
				char *orgkeyfile = p->key + strlen(keybase);
				char *keyfile = NULL;

				if (strncmp(orgkeyfile, "$(", 2) == 0) {
					const char *cpendvar = strchr(orgkeyfile, ')');
					if (cpendvar) {
						SString var(orgkeyfile, 2, cpendvar-orgkeyfile);
						SString s = psStart.GetExpanded(var.c_str());
						keyfile = StringDup(s.c_str());
					}
				}
				char *keyptr = keyfile;

				if (keyfile == NULL)
					keyfile = orgkeyfile;

				for (;;) {
					char *del = strchr(keyfile, ';');
					if (del == NULL)
						del = keyfile + strlen(keyfile);
					if (MatchWild(keyfile, del - keyfile, filename, caseSensitiveFilenames)) {
						delete []keyptr;
						return p->val;
					}
					if (*del == '\0')
						break;
					keyfile = del + 1;
				}
				delete []keyptr;

				if (0 == strcmp(p->key, keybase)) {
					return p->val;
				}
			}
		}
	}
	if (superPS) {
		// Failed here, so try in super property set
		return static_cast<PropSetFile *>(superPS)->GetWildUsingStart(psStart, keybase, filename);
	} else {
		return "";
	}
}

SString PropSetFile::GetWild(const char *keybase, const char *filename) {
	return GetWildUsingStart(*this, keybase, filename);
}

// GetNewExpand does not use Expand as it has to use GetWild with the filename for each
// variable reference found.
SString PropSetFile::GetNewExpand(const char *keybase, const char *filename) {
	char *base = StringDup(GetWild(keybase, filename).c_str());
	char *cpvar = strstr(base, "$(");
	int maxExpands = 1000;	// Avoid infinite expansion of recursive definitions
	while (cpvar && (maxExpands > 0)) {
		const char *cpendvar = strchr(cpvar, ')');
		if (cpendvar) {
			int lenvar = cpendvar - cpvar - 2;  	// Subtract the $()
			char *var = StringDup(cpvar + 2, lenvar);
			SString val = GetWild(var, filename);
			if (0 == strcmp(var, keybase))
				val.clear(); // Self-references evaluate to empty string
			size_t newlenbase = strlen(base) + val.length() - lenvar;
			char *newbase = new char[newlenbase];
			strncpy(newbase, base, cpvar - base);
			strcpy(newbase + (cpvar - base), val.c_str());
			strcpy(newbase + (cpvar - base) + val.length(), cpendvar + 1);
			delete []var;
			delete []base;
			base = newbase;
		}
		cpvar = strstr(base, "$(");
		maxExpands--;
	}
	SString sret = base;
	delete []base;
	return sret;
}

/**
 * Initiate enumeration.
 */
bool PropSetFile::GetFirst(char **key, char **val) {
	for (int i = 0; i < hashRoots; i++) {
		for (Property *p = props[i]; p; p = p->next) {
			if (p) {
				*key = p->key;
				*val = p->val;
				enumnext = p->next; // GetNext will begin here ...
				enumhash = i;		  // ... in this block
				return true;
			}
		}
	}
	return false;
}

/**
 * Continue enumeration.
 */
bool PropSetFile::GetNext(char ** key, char ** val) {
	bool firstloop = true;

	// search begins where we left it : in enumhash block
	for (int i = enumhash; i < hashRoots; i++) {
		if (!firstloop)
			enumnext = props[i]; // Begin with first property in block
		// else : begin where we left
		firstloop = false;

		for (Property *p = enumnext; p; p = p->next) {
			if (p) {
				*key = p->key;
				*val = p->val;
				enumnext = p->next; // for GetNext
				enumhash = i;
				return true;
			}
		}
	}
	return false;
}



// Global property bank for anjuta.
static GList *anjuta_propset;

static PropSetFile*
get_propset(PropsID pi)
{
  PropSetFile* p;
  if(pi < 0 || (guint)pi >= g_list_length(anjuta_propset))
  {
  	DEBUG_PRINT("%s", "Invalid PropSetFile handle");
  	return NULL;
  }
  p = (PropSetFile*)g_list_nth_data(anjuta_propset, pi);
  if (p == NULL)
  	DEBUG_PRINT("%s", "Trying to access already destroyed PropSetFile object");
  return p;
}

// Followings are the C++ to C interface for the PropSetFile

PropsID
sci_prop_set_new(void)
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
   	DEBUG_PRINT("%s", "Unable to create PropSetFile Object");
   	return -1;
   }
   return handle-1;
}

gpointer
sci_prop_get_pointer(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  return (gpointer)p;
}

void
sci_prop_set_destroy(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  g_list_nth(anjuta_propset, handle)->data = NULL;
  delete p;
}

void
sci_prop_set_parent(PropsID handle1, PropsID handle2)
{
  PropSetFile *p1, *p2;
  p1 = get_propset(handle1);
  p2 = get_propset(handle2);
  if(!p1 || !p2) return;
  p1->superPS = p2;
}

void
sci_prop_set_with_key(PropsID handle, const gchar *key, const gchar *val)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  if(val)
	  p->Set(key, val);
  else
	  p->Set(key, "");
}

void 
sci_prop_set_int_with_key (PropsID p, const gchar *key, int value)
{
  gchar *str;
  str = g_strdup_printf ("%d", value);
  sci_prop_set_with_key (p, key, str);
  g_free (str);  
}

void
sci_prop_set(PropsID handle, const gchar *keyval)
{
  PropSetFile* p;
  p = get_propset (handle);
  if(!p) return;
  p->Set(keyval);
}

gchar*
sci_prop_get(PropsID handle, const gchar *key)
{
  PropSetFile* p;
  SString s;
  if (!key) return NULL;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->Get(key);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

gchar*
sci_prop_get_expanded(PropsID handle, const gchar *key)
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
sci_prop_expand(PropsID handle, const gchar *withvars)
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
sci_prop_get_int(PropsID handle, const gchar *key, gint defaultValue=0)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return defaultValue;
  return p->GetInt(key, defaultValue);
}

gchar*
sci_prop_get_wild(PropsID handle, const gchar *keybase, const gchar *filename)
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
sci_prop_get_new_expand(PropsID handle, const gchar *keybase, const gchar *filename)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->GetNewExpand(keybase, filename);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

/* GList of strings operations */
static GList *
sci_prop_glist_from_string (const gchar *string)
{
	gchar *str, *temp, buff[256];
	GList *list;
	gchar *word_start, *word_end;
	gboolean the_end;

	list = NULL;
	the_end = FALSE;
	temp = g_strdup (string);
	str = temp;
	if (!str)
		return NULL;

	while (1)
	{
		gint i;
		gchar *ptr;

		/* Remove leading spaces */
		while (isspace (*str) && *str != '\0')
			str++;
		if (*str == '\0')
			break;

		/* Find start and end of word */
		word_start = str;
		while (!isspace (*str) && *str != '\0')
			str++;
		word_end = str;

		/* Copy the word into the buffer */
		for (ptr = word_start, i = 0; ptr < word_end; ptr++, i++)
			buff[i] = *ptr;
		buff[i] = '\0';
		if (strlen (buff))
			list = g_list_append (list, g_strdup (buff));
		if (*str == '\0')
			break;
	}
	if (temp)
		g_free (temp);
	return list;
}

/* Get the list of strings as GList from a property value.
   Strings are splitted from white spaces */
GList *
sci_prop_glist_from_data (guint props, const gchar *id)
{
	gchar *str;
	GList *list;

	str = sci_prop_get (props, id);
	list = sci_prop_glist_from_string (str);
	g_free(str);
	return list;
}

void
sci_prop_clear(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->Clear();
}

void
sci_prop_read_from_memory(PropsID handle, const gchar *data, gint len,
		const gchar *directoryForImports=0)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->ReadFromMemory(data, len, directoryForImports);
}

void
sci_prop_read(PropsID handle, const gchar *filename, const gchar *directoryForImports)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->Read( filename, directoryForImports);
}
