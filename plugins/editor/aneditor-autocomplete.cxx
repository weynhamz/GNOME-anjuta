/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    aneditor-calltip.cxx
    Copyright (C) 2004 Naba Kumar

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "aneditor-priv.h"

#define TYPESEP '?'

static void free_word(gpointer key, gpointer value, gpointer user_data)
{
	g_free(key);
}

bool AnEditor::FindWordInRegion(char *word, int maxlength,
								SString &region, int offset) {
/*
	Tries to find a word in the region designated by 'region'
	around the position given by 'offset' in that region.
	If a word is found, it is copied (with a terminating '\0'
	character) into 'word', which must be a buffer of at least
	'maxlength' characters; the method then returns true.
	If no word is found, the buffer designated by 'word'
	remains unmodified and the method returns false.
*/
	int startword = offset;
	while (startword> 0 && wordCharacters.contains(region[startword - 1]))
		startword--;
	int endword = offset;
	while (region[endword] && wordCharacters.contains(region[endword]))
		endword++;
	if(startword == endword)
		return false;
	
	region.change(endword, '\0');
	int cplen = (maxlength < (endword-startword+1))?maxlength:(endword-startword+1);
	strncpy (word, region.c_str() + startword, cplen);
	return true;
}

bool AnEditor::GetWordAtPosition(char* buffer, int maxlength, int pos) {
/*
	Tries to find a word around position 'pos' in the text
	and writes this '\0'-terminated word to 'buffer',
	which must be a buffer of at least 'maxlength' characters.
	Returns true if a word is found, false otherwise.
*/
	const int radius = 500;
	int start = (pos >= radius ? pos - radius : 0);
	int doclen = LengthDocument();
	int end = (doclen - pos >= radius ? pos + radius : doclen);
	char *chunk = SString::StringAllocate(2 * radius);
	GetRange(start, end, chunk, false);
	chunk[2 * radius] = '\0';
	SString region;
	region.attach(chunk);
	return FindWordInRegion(buffer, maxlength, region, pos - start);
}

bool AnEditor::StartAutoComplete() {
	SString linebuf;
	GetLine(linebuf);
	int current = GetCaretInLine();

	int startword = current;
	while ((startword > 0) &&
	        (wordCharacters.contains(linebuf[startword - 1]) ||
	         autoCompleteStartCharacters.contains(linebuf[startword - 1])))
		startword--;
	linebuf.change(current, '\0');
	const char *root = linebuf.c_str() + startword;
	int rootlen = current - startword;
	const GPtrArray *tags = tm_workspace_find(root, tm_tag_max_t, NULL, TRUE);
	if (NULL != tags)
	{
		GString *words = g_string_sized_new(100);
		TMTag *tag;
		for (guint i = 0; ((i < tags->len) &&
			 (i < MAX_AUTOCOMPLETE_WORDS)); ++i)
		{
			tag = (TMTag *) tags->pdata[i];
			if (i > 0)
				g_string_append_c(words, ' ');
			g_string_append(words, tag->name);
		}
		SendEditor(SCI_AUTOCSETAUTOHIDE, 1);
		SendEditorString(SCI_AUTOCSHOW, rootlen, words->str);
		g_string_free(words, TRUE);
	}
	return true;
}

bool AnEditor::SendAutoCompleteRecordsFields(const GPtrArray *CurrentFileTags,
                                             const char *ScanType)
{
	
	if( !(ScanType && ScanType[0] != '\0') ) return false;
		
	const GPtrArray *tags = tm_workspace_find_scope_members(CurrentFileTags,
															ScanType, TRUE);
	
	if ((tags) && (tags->len > 0))
	{	
		TMTag *tag;
		GHashTable *wordhash = g_hash_table_new(g_str_hash, g_str_equal);
		GString *words = g_string_sized_new(256);
	
		for (guint i=0; ((i < tags->len)); ++i)
		{
			tag = TM_TAG(tags->pdata[i]);
			if (NULL == g_hash_table_lookup(wordhash,
											(gconstpointer) tag->name))
			{
				g_hash_table_insert(wordhash,
									g_strdup(tag->name), (gpointer) 1);
				if (words->len > 0)
				{
					g_string_append_c(words, ' ');
				}
				g_string_append(words, tag->name);
				g_string_append_c(words, TYPESEP);
				g_string_append_printf(words, "%d", tag->type);
				// g_string_append_printf(words, "%d", sv_get_node_type (tag));
			}
		}
		
		SendEditor(SCI_AUTOCSETAUTOHIDE, 0);
		//SendEditorString(SCI_AUTOCSHOW, 0, words->str);
		SendEditorString(SCI_USERLISTSHOW, 0, words->str);
	
		g_string_free(words, TRUE);
		g_hash_table_foreach(wordhash, free_word, NULL);
		g_hash_table_destroy(wordhash);	

		return true;
	}
  return false;
}

static bool IsRecordOperatorForward(char *ch)
{
	return (((ch[0] == '-') && (ch[1] == '>')) ||
	        ((ch[0] == ':') && (ch[1] == ':')));
}

static bool IsRecordOperatorBackward(char *ch)
{
	return (((ch[0] == '>') && (ch[-1] == '-')) ||
	        ((ch[0] == ':') && (ch[-1] == ':')));
}

static bool IsAlnum(char ch)
{
	return (ch == '_' || isalnum(ch));
}

static bool IsAlpha(char ch)
{
	return (ch == '_' || isalpha(ch));
}

static bool IsRecordOperator(char *ch)
{
	return ((ch[0] == '.') ||
        	IsRecordOperatorForward(ch) ||
	        IsRecordOperatorBackward(ch) );
}

TMTag ** AnEditor::FindTypeInLocalWords(GPtrArray *CurrentFileTags,
										const char *root, const bool type,
										bool *retptr, int *count)
{
  int posCurrentWord = SendEditor (SCI_GETCURRENTPOS);
	int Line = SendEditor(SCI_LINEFROMPOSITION , posCurrentWord);
  
  {
    TMTag *tag;
    if(NULL == (tag = TM_TAG(tm_get_current_function(CurrentFileTags, Line))))
    {
      return NULL;
    }
    Line = tag->atts.entry.line - 2;
  }
  
  size_t rootlen = strlen(root);
  posCurrentWord -= (rootlen + (type ? 1 : 2));
  int EndBlokLine = 
       GetBlockEndLine(SendEditor(SCI_LINEFROMPOSITION , posCurrentWord));  
  if(EndBlokLine < 0) return NULL;
  
  SString linebuf;
  const char *name; 
 
  /* find open block '{' */
  do {
    Line++;
    GetLine(linebuf, Line);
    name = linebuf.c_str();
    name = strchr(name, '{');
  } while(!name);
  
  int posFind = SendEditor(SCI_POSITIONFROMLINE , Line) + (name - linebuf.c_str());
  
	int doclen = LengthDocument();
	TextToFind ft = {{0, 0}, 0, {0, 0}};
	ft.lpstrText = const_cast<char*>(root);
  ft.chrg.cpMin = posFind;
  /* find close block '}' */
	ft.chrg.cpMax = SendEditor(SCI_BRACEMATCH , posFind, 0);
    
  for (;;) {	// search all function blocks
		posFind = SendEditorString(SCI_FINDTEXT,
        (SCFIND_WHOLEWORD | SCFIND_MATCHCASE), reinterpret_cast<char *>(&ft));
    
		if (posFind == -1 || posFind >= doclen || posFind >= posCurrentWord)
		{
			break;
		}
		
    Line = SendEditor(SCI_LINEFROMPOSITION , posFind);
    int current = GetBlockEndLine(Line);
    if (current < 0) break;
		if ( EndBlokLine > current )
		{
			/* not in our block */
      ft.chrg.cpMin = posFind + rootlen;
			continue;
		}

    if(GetFullLine(linebuf, Line) < 0) break;
    
    /* find word position */    
		current = 0;				
		while(1)
		{
		  current = linebuf.search(root, current);
			if(current < 0) break;
			if(!current && !IsAlnum(linebuf[rootlen]))
			{
				break;
			}	
			if(current > 0 && !IsAlpha(linebuf[current-1]) &&
				                !IsAlnum(linebuf[current + rootlen]))
			{
				break;
			}
			current += rootlen;
		}
    if(current < 1) break;
    
    bool isPointer = false;
    int startword = current - 1;
    
		{
			/* find 'pointer' capability and move to "true start of line" */
			bool findPointer = true;
			while (startword > 0)
			{
				if(linebuf[startword] == ';' || linebuf[startword] == '{' ||
					 linebuf[startword] == '}')
				{
					break;
				}
				if(findPointer && linebuf[startword] == ',')
				{
					findPointer = false;
				}
				if(findPointer && linebuf[startword] == '*')
				{
					findPointer = false;
					isPointer = true;
				}
				startword--;
			}	
		}		
		
		name = NULL;
		for (;;) {
			/* find begin of word */
			while (startword < current && !IsAlpha(linebuf[startword]))
		  {
			  startword++;
		  }
		  if (startword == current) break;
			
		  name = (linebuf.c_str() + startword);
			
			/* find end of word */
			while (startword < current && IsAlnum(linebuf[startword]))
		  {
			  startword++;
		  }
			if (startword == current) break;
				
		  char backup = linebuf[startword];
			linebuf.change(startword, '\0');
			
			if (strcmp(name, root) == 0)
			{
				break;
			}
			
			if (strcmp(name, "struct")   == 0 ||
				strcmp(name, "class" )   == 0 ||
			    strcmp(name, "union" )   == 0 ||
			    strcmp(name, "const" )   == 0 ||
			    strcmp(name, "static")   == 0 ||
			    strcmp(name, "auto"  )   == 0 ||
			    strcmp(name, "register") == 0 ||
			    strcmp(name, "extern")   == 0 ||
				strcmp(name, "restrict") == 0)
			{
				linebuf.change(startword, backup);
				continue;
			}
		  
      /* search in file type tag's entrys */
      doclen = 0;
      TMTag **match = tm_tags_find(CurrentFileTags, name, FALSE, &current);
      for(startword = 0; match && startword < current; ++startword)
      {
        if ((tm_tag_class_t | tm_tag_struct_t |tm_tag_typedef_t |
             tm_tag_union_t | tm_tag_namespace_t) & match[startword]->type)
        {
					doclen++;
        }
      }
      if (!doclen)
	  {
		  /* search in global type tag's entrys */
		  const GPtrArray *tags = tm_workspace_find(name, (tm_tag_class_t |
														   tm_tag_struct_t |
														   tm_tag_typedef_t |
														   tm_tag_union_t  |
														   tm_tag_namespace_t),
													NULL, FALSE);  
		  doclen = tags->len;
		  match = (TMTag **)tags->pdata;
      }
      
      if(retptr) *retptr = isPointer;  
      if(count) *count = doclen;
	    return match;
	
		}
    break;
  }
  return NULL;
}

static TMWorkObject * get_current_tm_file (GtkWidget *scintilla)
{
	GtkWidget *container;
	container = gtk_widget_get_parent (scintilla);
	
	g_return_val_if_fail (GTK_IS_VBOX (container), NULL);
	
	TMWorkObject * tm_file =
		TM_WORK_OBJECT (g_object_get_data (G_OBJECT (container), "tm_file"));
	return tm_file;
}

static char * get_current_function_scope (GtkWidget *scintilla, int line)
{
	TMWorkObject * tm_file = get_current_tm_file (scintilla);
	
	if((tm_file) && (tm_file->tags_array))
	{
		TMTag *tag;
		g_message ("TMFile for editor found");
		if(NULL != (tag = TM_TAG(tm_get_current_function(tm_file->tags_array,
														 line))))
		{
			return tag->atts.entry.scope;
		}
	}
	return NULL;
};

static char * FindTypeInFunctionArgs(GPtrArray *CurrentFileTags,
                          const char *root, bool *retptr, const int line)
{
  /* find type in function arguments */
  if(CurrentFileTags)
  {
    char *name;
    size_t rootlen;
    int end, start;
    TMTag *tag;
    bool isPointer;
    if(NULL == (tag = TM_TAG(tm_get_current_function(CurrentFileTags, line))))
    {
      return NULL;
    }
    if(NULL == (name = tag->atts.entry.arglist))
    {
      return NULL;
    }
          
    rootlen = strlen(root);
    while(name)
    {
      if(NULL == (name = strstr(name, root)))
      {
        return NULL;
      }
      end = name - tag->atts.entry.arglist;
      if (!end || IsAlpha(name[-1]) || IsAlnum(name[rootlen]))
      {
        name++;
        continue;
      }
      name = tag->atts.entry.arglist;
      break;
    }
    isPointer = false;
    while(end > 0)
    {
      while(end > 0 && !IsAlnum(name[end - 1]))
      {
        if(!isPointer && name[end - 1] == '*')
        {
          isPointer = true;
        }
        end--;
      }
      start = end;
            
      while(start > 0 && IsAlnum(name[start - 1]))
      {
        start--;
      }
            
      char backup = name[end];
      name[end] = '\0';
      name += start;
      if( (0 == strcmp("const", name)) ||
          (0 == strcmp("restrict", name)))
      {
        name = tag->atts.entry.arglist;
        name[end] = backup;
        end = start;
        continue;
      }
      name = g_strdup(name);
      tag->atts.entry.arglist[end] = backup;
      break;
    }
    if(retptr) *retptr = isPointer;
    return name;
  }
  
  return NULL;
}

bool AnEditor::StartAutoCompleteRecordsFields (char ch)
{
	/* TagManager autocompletion - only for C/C++/Java */	  
	if (SCLEX_CPP != lexLanguage ||
		((ch != '.') &&	(ch != '>') && (ch != ':')))
		return false;
	
	SString linebuf;
	const char *root;
	
	int current = GetFullLine(linebuf);
  
	if (current < 0) return false;
	if ((ch == '>') && linebuf[current - 2] != '-') return false;
	if ((ch == ':') && linebuf[current - 2] != ':') return false;
	
	char tmp_chr;
	bool was_long_decl = false, long_declaration = true;
	int endword, startword = current;
	TMTag *ScanType;
	GPtrArray *CurrentFileTags;
	const GPtrArray *tags;
	
	/* scan entire record's chain */
	while(long_declaration)
	{
		endword = startword;
		
		while (endword > 1 && IsRecordOperator((char *)(linebuf.c_str() +
														(endword - 1))) )
		{
			endword--;
		}
		/* detect function parameter list or othgers () */
		if(linebuf[endword - 1] == ')')
		{
			int count = 0;
			do {
				endword--;
				if(linebuf[endword] == ')') count++;
				if(linebuf[endword] == '(') count--;
			}	while (endword > 0 && count);
		}
		else if(linebuf[endword - 1] == '>')
		{
			int count = 0;
			do {
				endword--;
				if(linebuf[endword] == '>') count++;
				if(linebuf[endword] == '<') count--;
			}	while (endword > 0 && count);
		}
		else
		{
			while(linebuf[endword - 1] == ']')
			{
				int count = 0;
				do {
					endword--;
					if(linebuf[endword] == ']') count++;
					if(linebuf[endword] == '[') count--;
				}	while (endword > 0 && count);
			}
		}
				
		while (endword > 1 && isspace(linebuf[endword - 1]))
		{
			endword--;
		}
		
		startword = endword;
		while (startword > 1 && IsAlnum(linebuf[startword - 1]))
		{
			startword--;
		}
		
		long_declaration = startword > 1 && 
			IsRecordOperator((char *)(linebuf.c_str() + (startword - 1)));
		if(!was_long_decl && long_declaration) was_long_decl = true;
	}
	
	tmp_chr = linebuf[endword];
	linebuf.change(endword, '\0');
	root = linebuf.c_str() + startword;
	
	bool isPointer = false;	
	char *name;
	char *new_name = NULL;
	int types = (ch == ':' ? tm_tag_class_t : (tm_tag_max_t & ~tm_tag_class_t));
	{
		TMWorkObject  *tm_file = get_current_tm_file (GTK_WIDGET (GetID()));
		CurrentFileTags = tm_file ? tm_file->tags_array : NULL;
	}
	if(0 == strcmp(root, "this"))
	{
		name = get_current_function_scope(GTK_WIDGET (GetID()),
										  GetCurrentLineNumber());
		if(!name || name[0] == '\0') return false;
			isPointer = true;
		}
		else
		{
			TMTagAttrType attrs[] = {
				tm_tag_attr_name_t,
				tm_tag_attr_type_t,
				tm_tag_attr_none_t
			};
			TMTag **match = NULL;
			int count = 0;
			
			tm_tags_sort(CurrentFileTags, attrs, FALSE);
			match = tm_tags_find(CurrentFileTags, root, FALSE, &count);
			if(count && ch == ':')
			{
				int real_count = 0;
				for(int i = 0; i < count; ++i)
				{
					if(types & match[i]->type)
					{
						real_count++;
					}
				}
				count = real_count;
			}
			if (!count || !match || !(*match))
			{
				/* search in global variables and functions tag's entrys */
				tags = tm_workspace_find(root, types, attrs, FALSE);  
				count = tags->len;
				match = NULL;
			}
			ScanType = NULL;
			if( count == 1 )
			{
				ScanType = (match ? match[0] : TM_TAG(tags->pdata[0]));
				if(!(!ScanType->atts.entry.scope ||
					((name =
						get_current_function_scope(GTK_WIDGET (GetID()),
												   GetCurrentLineNumber())) &&
					0 == strcmp(name, ScanType->atts.entry.scope))) )
				{
					ScanType = NULL;
				}
			}
			else
			{
				if((count > 0) &&
					(name =
						get_current_function_scope(GTK_WIDGET (GetID()),
												   GetCurrentLineNumber())))
				{
					int iter;
					for( iter =0; iter < count; ++iter)
					{
						ScanType =
							(match ? match[iter] : TM_TAG(tags->pdata[iter]));
						if(ch != ':' &&
						   0 == strcmp(name, ScanType->atts.entry.scope))
						{
							break;
						}
						if(ch == ':' && ScanType->type == tm_tag_class_t)
						{
							break;
						}
					}
					if(iter == count) ScanType = NULL;
				}
			}
			if(ScanType)
			{
				if(ScanType->type == tm_tag_class_t)
				{
					/* show nothing when use normal acces operators (./->) */
					name = ScanType->name;
					isPointer = (ch == '.');
				}
				else
				{
					name = ScanType->atts.entry.var_type;
					isPointer = ScanType->atts.entry.isPointer;
				}
			}
			else
			{
				new_name = FindTypeInFunctionArgs(CurrentFileTags, root,
												  &isPointer,
												  GetCurrentLineNumber());
				if(new_name)
				{
					name = new_name;
				}
				else
				{
				count = 0;
				TMTag **match = FindTypeInLocalWords(CurrentFileTags,
													 root, (tmp_chr == '.'),
													 &isPointer, &count);
				if ((match) && (count == 1) && (ScanType = match[0]))
				{
					name = ScanType->name;
				}
				else
				{
					return false;
				}
			}
		}
	}
	
	linebuf.change(endword, tmp_chr);
	
	while(was_long_decl)
	{
		tags = tm_workspace_find_scope_members(CurrentFileTags, name, TRUE);
		if (!((tags) && (tags->len > 0)))
		{
			if(new_name) g_free(new_name);
			return false;
		}

		while (endword < current && isspace(linebuf[endword]))
		{
			endword++;
		}
		if(linebuf[endword] == '(')
		{
			int count = 0;
			do {
				if(linebuf[endword] == ')') count--;
				if(linebuf[endword] == '(') count++;
				endword++;
			}
			while (count && endword < current);
		}
		else
		{
			while(linebuf[endword] == '[')
			{
				int count = 0;
				do {
					if(linebuf[endword] == ']') count--;
					if(linebuf[endword] == '[') count++;
					endword++;
				}
				while (count && endword < current);
			}
		}
		while (endword < current &&
			   IsRecordOperator((char *)(linebuf.c_str() + endword)))
		{
			endword++;
		}
		if(!IsAlnum(linebuf[endword]))
			break;
		
		startword = endword;
		while (endword < current && IsAlnum(linebuf[endword]))
		{
			endword++;
		}
	
		tmp_chr = linebuf[endword];
		linebuf.change(endword, '\0');
		root = linebuf.c_str() + startword;
		
		unsigned int i;
		for (i=0; (i < tags->len); ++i)
		{
			ScanType = TM_TAG(tags->pdata[i]);
			if(ScanType && 0 == strcmp(ScanType->name, root))
			{
				break;
			}
		}
		if (i >= tags->len)
		{
			if(new_name) g_free(new_name);
				return false;
		}
		linebuf.change(endword, tmp_chr);
	
		CurrentFileTags = ScanType->atts.entry.file->work_object.tags_array;
		name = ScanType->atts.entry.var_type;
		isPointer = ScanType->atts.entry.isPointer;
	}
	if (ch == ':' && ScanType->type != tm_tag_class_t)
	{
		if(new_name) g_free(new_name);
			//anjuta_status (_("Wrong acces operator ... "));
			return false;
	}
	if (ch == '>' && !isPointer)
	{
		if(new_name) g_free(new_name);
			//anjuta_status (_("Wrong acces operator... Please use \"->\""));
			return false;
	}
	if (ch == '.' && isPointer)
	{
		if(new_name) g_free(new_name);
			//anjuta_status (_("Wrong acces operator... Please use \".\""));
			return false;
	}	
	bool retval = SendAutoCompleteRecordsFields(CurrentFileTags, name);
	if(new_name) g_free(new_name);
		return retval;	
}

bool AnEditor::StartAutoCompleteWord(int autoShowCount) {
	SString linebuf;
	int nwords = 0;
	int minWordLength = 0;
	int wordlen = 0;
	
	GHashTable *wordhash = g_hash_table_new(g_str_hash, g_str_equal);
	GString *words = g_string_sized_new(256);
	
	GetLine(linebuf);
	int current = GetCaretInLine();

	bool isNum = true;
	int startword = current;
	while (startword > 0 && wordCharacters.contains(linebuf[startword - 1])) {
		if (isNum && !isdigit(linebuf[startword - 1]))
			isNum = false;
		startword--;
	}
	
	if (startword == current)
		return true;
	
	// Don't show autocomple for numbers (very annoying).
	if (isNum)
		return true;
	
	linebuf.change(current, '\0');
	const char *root = linebuf.c_str() + startword;
	int rootlen = current - startword;
	
	/* TagManager autocompletion - only for C/C++/Java */
	if (SCLEX_CPP == lexLanguage)
	{
		const GPtrArray *tags = tm_workspace_find(root, tm_tag_max_t,
												  NULL, TRUE);
		if ((tags) && (tags->len > 0))
		{
			TMTag *tag;
			for (guint i=0; ((i < tags->len) &&
				 (i < MAX_AUTOCOMPLETE_WORDS)); ++i)
			{
				tag = (TMTag *) tags->pdata[i];
				if (NULL == g_hash_table_lookup(wordhash
				  , (gconstpointer)	tag->name))
				{
					g_hash_table_insert(wordhash, g_strdup(tag->name),
										(gpointer) 1);
					if (words->len > 0)
						g_string_append_c(words, ' ');
					g_string_append(words, tag->name);
					g_string_append_c(words, TYPESEP);
					g_string_append_printf(words, "%d", tag->type);
					// g_string_append_printf(words, "%d",
					//					   sv_get_node_type (tag));
					
					wordlen = strlen(tag->name);
					if (minWordLength < wordlen)
						minWordLength = wordlen;

					nwords++;
					if (autoShowCount > 0 && nwords > autoShowCount) {
						g_string_free(words, TRUE);
						g_hash_table_foreach(wordhash, free_word, NULL);
						g_hash_table_destroy(wordhash);
						return true;
					}
				}
			}
		}
	}

	/* Word completion based on words in current buffer */
	int doclen = LengthDocument();
	TextToFind ft = {{0, 0}, 0, {0, 0}};
	ft.lpstrText = const_cast<char*>(root);
	ft.chrg.cpMin = 0;
	ft.chrgText.cpMin = 0;
	ft.chrgText.cpMax = 0;
	int flags = SCFIND_WORDSTART | (autoCompleteIgnoreCase ? 0 : SCFIND_MATCHCASE);
	int posCurrentWord = SendEditor (SCI_GETCURRENTPOS) - rootlen;

	for (;;) {	// search all the document
		ft.chrg.cpMax = doclen;
		int posFind = SendEditorString(SCI_FINDTEXT, flags,
									   reinterpret_cast<char *>(&ft));
		if (posFind == -1 || posFind >= doclen)
			break;
		if (posFind == posCurrentWord) {
			ft.chrg.cpMin = posFind + rootlen;
			continue;
		}
		// Grab the word and put spaces around it
		const int wordMaxSize = 80;
		char wordstart[wordMaxSize];
		GetRange(wEditor, posFind, Platform::Minimum(posFind + wordMaxSize - 3,
													 doclen), wordstart);
		char *wordend = wordstart + rootlen;
		while (iswordcharforsel(*wordend))
			wordend++;
		*wordend = '\0';
		wordlen = wordend - wordstart;
		if (wordlen > rootlen) {
			/* Check if the word is already there - if not, insert it */
			if (NULL == g_hash_table_lookup(wordhash, wordstart))
			{
				g_hash_table_insert(wordhash, g_strdup(wordstart),
									(gpointer) 1);
				if (0 < words->len)
					g_string_append_c(words, ' ');
				g_string_append(words, wordstart);
				
				if (minWordLength < wordlen)
					minWordLength = wordlen;

				nwords++;
				if (autoShowCount > 0 && nwords > autoShowCount) {
					g_string_free(words, TRUE);
					g_hash_table_foreach(wordhash, free_word, NULL);
					g_hash_table_destroy(wordhash);
					return true;
				}
			}
		}
		ft.chrg.cpMin = posFind + wordlen;
	}
	size_t length = (words->str) ? strlen (words->str) : 0;
	if ((length > 2) && (autoShowCount <= 0 || (minWordLength > rootlen))) {
		SendEditor(SCI_AUTOCSETAUTOHIDE, 1);
		SendEditorString(SCI_AUTOCSHOW, rootlen, words->str);
	} else {
		SendEditor(SCI_AUTOCCANCEL);
	}

	g_string_free(words, TRUE);
	g_hash_table_foreach(wordhash, free_word, NULL);
	g_hash_table_destroy(wordhash);
	return true;
}
