
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "general.h"
#include "entry.h"
#include "parse.h"
#include "read.h"
#define LIBCTAGS_DEFINED
#include "tm_work_object.h"

#include "tm_source_file.h"
#include "tm_tag.h"

#define TM_DEBUG

guint source_file_class_id = 0;
static TMSourceFile *current_source_file = NULL;


int tm_source_file_tags(const tagEntryInfo *tag)
{
	if (NULL == current_source_file)
		return 0;
	if (NULL == current_source_file->work_object.tags_array)
		current_source_file->work_object.tags_array = g_ptr_array_new();
	g_ptr_array_add(current_source_file->work_object.tags_array,
	  tm_tag_new(current_source_file, tag));
	  
//	g_message ("adding new tag: %s", tag->name);

	return TRUE;
}


gboolean tm_buffer_parse(TMSourceFile *source_file, unsigned char* buf, int buf_size)
{
	const char *file_name;
	gboolean status = TRUE;

	if ((NULL == source_file) || (NULL == source_file->work_object.file_name))
	{
		g_warning("Attempt to parse NULL file");
		return FALSE;
	}

#ifdef TM_DEBUG
	g_message("Parsing %s", source_file->work_object.file_name);
#endif
	file_name = source_file->work_object.file_name;
	if (NULL == LanguageTable)
	{
		initializeParsing();
		installLanguageMapDefaults();
		if (NULL == TagEntryFunction)
			TagEntryFunction = tm_source_file_tags;
	}
	current_source_file = source_file;
	if (LANG_AUTO == source_file->lang)
		source_file->lang = getFileLanguage (file_name);
	if (source_file->lang == LANG_IGNORE)
	{
#ifdef TM_DEBUG
		g_warning("ignoring %s (unknown language)\n", file_name);
#endif
	}
	else if (! LanguageTable [source_file->lang]->enabled)
	{
#ifdef TM_DEBUG
		g_warning("ignoring %s (language disabled)\n", file_name);
#endif
	}
	else
	{
		int passCount = 0;
		while ((TRUE == status) && (passCount < 3))
		{
			if (source_file->work_object.tags_array)
				tm_tags_array_free(source_file->work_object.tags_array, FALSE);
			if (bufferOpen (buf, buf_size, file_name, source_file->lang))
			{
				if (LanguageTable [source_file->lang]->parser != NULL) {
					LanguageTable [source_file->lang]->parser ();
				}
				else if (LanguageTable [source_file->lang]->parser2 != NULL) {
					status = LanguageTable [source_file->lang]->parser2 (passCount);
				}
				g_message ("closing buffer");
				bufferClose ();
			}
			else
			{
				g_warning("Unable to open %s", file_name);
				return FALSE;
			}
			++ passCount;
		}
		return TRUE;
	}
	return status;
}



int main (int argc, char** argv) {

	TMWorkObject *source_file_wo;
	FILE *fp;
	int file_size, i;
	unsigned char* buf;
	
	if (argc < 2) {
		g_message ("usage: %s <file.c>", argv[0]);
		exit(-1);
	}
	
	source_file_wo = tm_source_file_new (argv[1], FALSE);
	
	if ( (fp = fopen (argv[1], "rb")) == NULL) {
		g_message ("cannot open file for buffering...");
		exit (-1);
	}
	
	fseek (fp, 0, SEEK_END);
	file_size =	(int)ftell (fp);
	
	fseek (fp, 0, SEEK_SET);
	
	buf = (unsigned char*)calloc (file_size + 1, sizeof(unsigned char));
	
	memset( buf, 0, file_size +1 );
	
	fread (buf, file_size, sizeof(unsigned char), fp);

	fclose(fp);
	
	g_message ("file size is %d, buf %s", file_size, buf);	
	tm_buffer_parse (TM_SOURCE_FILE(source_file_wo), buf, file_size);

	g_message ("done");	
	if (source_file_wo->tags_array == NULL) {
		g_message ("no tags identified");
		return 0;
	}
	g_message ("total tags discovered: %d", source_file_wo->tags_array->len);

	/* gonna list the tags, just a test */
	for (i=0; i < source_file_wo->tags_array->len; i++) {
		TMTag* tag;
		
		tag = (TMTag*)g_ptr_array_index(source_file_wo->tags_array, i);
		g_message ("tag %d is: %s", i, tag->name);
	}
	return 0;
}


