/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "vgstrpool.h"
#include "vgrule.h"
#include "vgio.h"


static const char *vg_caller_types[] = { "fun", "obj", NULL };


const char *
vg_caller_type_to_name (vgcaller_t type)
{
	return vg_caller_types[type];
}


vgcaller_t
vg_caller_type_from_name (const char *name)
{
	int i;
	
	for (i = 0; i < VG_CALLER_LAST; i++) {
		if (!strcmp (vg_caller_types[i], name))
			break;
	}
	
	return i;
}


VgCaller *
vg_caller_new (vgcaller_t type, const char *name)
{
	VgCaller *caller;
	
	caller = g_new (VgCaller, 1);
	caller->next = NULL;
	caller->type = type;
	caller->name = g_strdup (name);
	
	return caller;
}


void
vg_caller_free (VgCaller *caller)
{
	if (caller == NULL)
		return;
	
	g_free (caller->name);
	g_free (caller);
}


static const char *vg_rule_types[] = {
	"Addr1", "Addr2", "Addr4", "Addr8", "Cond", "Free", "Leak",
	"Param", "PThread", "Value1", "Value2", "Value4", "Value8", NULL
};


const char *
vg_rule_type_to_name (vgrule_t type)
{
	return vg_rule_types[type];
}


vgrule_t
vg_rule_type_from_name (const char *name)
{
	int i;
	
	for (i = 0; i < VG_RULE_LAST; i++) {
		if (!strcmp (vg_rule_types[i], name))
			break;
	}
	
	return i;
}


int
vg_rule_type_from_report (const char *report, vgrule_t *type, char **syscall)
{
	const char *inptr, *call;
	unsigned int size;
	char *end;
	
	if (syscall)
		*syscall = NULL;
	
	/* FIXME: How can we auto-detect PThread errors? */
	if (!strncmp (report, "Conditional ", 12)) {
		*type = VG_RULE_COND;
		return TRUE;
	} else if (!strncmp (report, "Syscall param ", 14)) {
		*type = VG_RULE_PARAM;
		
		if (syscall) {
			call = report + 14;
			if ((inptr = strchr (call, ' ')) != NULL)
				*syscall = g_strndup (call, (inptr - call));
		}
		
		return TRUE;
	} else if (!strcmp (report, "Invalid free() / delete / delete[]")) {
		*type = VG_RULE_FREE;
		return TRUE;
	} else if ((inptr = strstr (report, " are still reachable in loss record "))) {
		*type = VG_RULE_LEAK;
		return TRUE;
	} else if (!strncmp (report, "Invalid read of size ", 21)) {
		inptr = report + 21;
		size = strtoul (inptr, &end, 10);
		switch (size) {
		case 1:
			*type = VG_RULE_ADDR1;
			break;
		case 2:
			*type = VG_RULE_ADDR2;
			break;
		case 4:
			*type = VG_RULE_ADDR4;
			break;
		case 8:
			*type = VG_RULE_ADDR8;
			break;
		default:
			return FALSE;
		}
		
		return TRUE;
	} else if ((inptr = strstr (report, "value of size "))) {
		/* only reason we strstr here instead of strncmp'ing
		 * against "Use of uninitialised value of size " is
		 * that someone may eventually convince the valgrind
		 * authors to use the Americanised spelling and then
		 * our strncmp would fail */
		inptr += 14;
		size = strtoul (inptr, &end, 10);
		switch (size) {
		case 1:
			*type = VG_RULE_VALUE1;
			break;
		case 2:
			*type = VG_RULE_VALUE2;
			break;
		case 4:
			*type = VG_RULE_VALUE4;
			break;
		case 8:
			*type = VG_RULE_VALUE8;
			break;
		default:
			return FALSE;
		}
		
		return TRUE;
	}
	
	return FALSE;
}


VgRule *
vg_rule_new (vgrule_t type, const char *name)
{
	VgRule *rule;
	
	rule = g_new (VgRule, 1);
	rule->name = g_strdup (name);
	rule->tools = NULL;
	rule->type = type;
	rule->syscall = NULL;
	rule->callers = NULL;
	
	return rule;
}


static VgTool *
parse_tool_list (unsigned char *tool_list)
{
	VgTool *tail, *n, *list = NULL;
	register unsigned char *inptr;
	unsigned char *start;
	
	tail = (VgTool *) &list;
	
	start = inptr = tool_list;
	while (*inptr) {
		while (*inptr && *inptr != ',')
			inptr++;
		
		if (*inptr == ',')
			*inptr++ = '\0';
		
		n = g_new (VgTool, 1);
		n->next = NULL;
		n->name = vg_strdup ((char *)start);
		
		tail->next = n;
		tail = n;
		
		start = inptr;
	}
	
	return list;
}


void
vg_rule_add_tool (VgRule *rule, const char *tool)
{
	VgTool *tail, *n;
	
	n = g_new (VgTool, 1);
	n->next = NULL;
	n->name = vg_strdup (tool);
	
	tail = (VgTool *) &rule->tools;
	while (tail->next != NULL)
		tail = tail->next;
	
	tail->next = n;
}


void
vg_rule_free (VgRule *rule)
{
	VgCaller *n, *nn;
	VgTool *s, *sn;
	
	if (rule == NULL)
		return;
	
	g_free (rule->name);
	g_free (rule->syscall);
	
	s = rule->tools;
	while (s != NULL) {
		sn = s->next;
		vg_strfree (s->name);
		g_free (s);
		s = sn;
	}
	
	n = rule->callers;
	while (n != NULL) {
		nn = n->next;
		vg_caller_free (n);
		n = nn;
	}
	
	g_free (rule);
}


enum {
	VG_RULE_PARSER_STATE_INIT         = 0,
	VG_RULE_PARSER_STATE_COMMENT      = (1 << 0),
	VG_RULE_PARSER_STATE_RULE_NAME    = (1 << 1),
	VG_RULE_PARSER_STATE_RULE_TYPE    = (1 << 2),
	VG_RULE_PARSER_STATE_RULE_SYSCALL = (1 << 3),
	VG_RULE_PARSER_STATE_RULE_CALLER  = (1 << 4),
};

VgRuleParser *
vg_rule_parser_new (int fd, VgRuleParserRuleCallback rule_cb, void *user_data)
{
	VgRuleParser *parser;
	
	parser = g_new (VgRuleParser, 1);
	parser_init ((Parser *) parser, fd);
	
	parser->linebuf = g_malloc (128);
	parser->lineptr = parser->linebuf;
	parser->lineleft = 128;
	
	parser->current = NULL;
	parser->tail = NULL;
	
	parser->rule_cb = rule_cb;
	parser->user_data = user_data;
	
	parser->state = VG_RULE_PARSER_STATE_INIT;
	
	return parser;
}

void
vg_rule_parser_free (VgRuleParser *parser)
{
	if (parser == NULL)
		return;
	
	g_free (parser->linebuf);
	g_free (parser);
}


#define backup_linebuf(parser, start, len) G_STMT_START {                 \
	if (parser->lineleft <= len) {                                    \
		unsigned int llen, loff;                                  \
		                                                          \
		llen = loff = parser->lineptr - parser->linebuf;          \
		llen = llen ? llen : 1;                                   \
		                                                          \
		while (llen < (loff + len + 1))                           \
			llen <<= 1;                                       \
		                                                          \
		parser->linebuf = g_realloc (parser->linebuf, llen);      \
		parser->lineptr = parser->linebuf + loff;                 \
		parser->lineleft = llen - loff;                           \
	}                                                                 \
	                                                                  \
	memcpy (parser->lineptr, start, len);                             \
	parser->lineptr += len;                                           \
	parser->lineleft -= len;                                          \
} G_STMT_END

static void
vg_parser_parse_caller (VgRuleParser *parser)
{
	register unsigned char *inptr;
	VgCaller *caller;
	vgcaller_t type;
	
	*parser->lineptr = ':';
	inptr = parser->linebuf;
	while (*inptr != ':')
		inptr++;
	
	if (inptr == parser->lineptr) {
		*parser->lineptr = '\0';
		fprintf (stderr, "Invalid caller field encountered: '%s'\n", parser->linebuf);
		return;
	}
	
	*inptr++ = '\0';
	if ((type = vg_caller_type_from_name ((char *)parser->linebuf)) == VG_CALLER_LAST) {
		fprintf (stderr, "Invalid caller type encountered: '%s'\n", parser->linebuf);
		return;
	}
	
	*parser->lineptr = '\0';
	caller = vg_caller_new (type, (char *)inptr);
	
	parser->tail->next = caller;
	parser->tail = caller;
}

int
vg_rule_parser_step (VgRuleParser *parser)
{
	register unsigned char *inptr;
	unsigned char *start;
	Parser *priv;
	int ret;
	
	priv = (Parser *) parser;
	
	if ((ret = parser_fill (priv)) == 0) {
		return 0;
	} else if (ret == -1) {
		return -1;
	}
	
	start = inptr = priv->inptr;
	*priv->inend = '\n';
	
	while (inptr < priv->inend) {
		if (parser->state == VG_RULE_PARSER_STATE_INIT) {
			if (*inptr == '#') {
				parser->state = VG_RULE_PARSER_STATE_COMMENT;
				inptr++;
			} else if (*inptr == '{') {
				parser->state = VG_RULE_PARSER_STATE_RULE_NAME;
				parser->current = vg_rule_new (0, NULL);
				parser->tail = (VgCaller *) &parser->current->callers;
				inptr++;
			} else {
				/* eat any whitespace?? */
				while (*inptr == ' ' || *inptr == '\t')
					inptr++;
				
				if (*inptr == '#' || *inptr == '{')
					continue;
				
				/* unknown, skip this line??? */
				*priv->inend = '\n';
				while (*inptr != '\n')
					inptr++;
				
				if (inptr == priv->inend)
					break;
				
				inptr++;
				
				continue;
			}
		}
		
	comment:
		if (parser->state & VG_RULE_PARSER_STATE_COMMENT) {
			/* eat this line... in the future we may want to save comment lines? */
			*priv->inend = '\n';
			while (*inptr != '\n')
				inptr++;
			
			if (inptr == priv->inend)
				break;
			
			inptr++;
			
			parser->state &= ~VG_RULE_PARSER_STATE_COMMENT;
			
			if (parser->state == VG_RULE_PARSER_STATE_INIT)
				continue;
		}
		
		if (parser->lineptr == parser->linebuf) {
			/* haven't come to the start of the actual rule data line yet */
			
			/* eat lwsp */
			*priv->inend = '\0';
			while (isspace ((int) *inptr))
				inptr++;
			
			if (inptr == priv->inend)
				break;
			
			if (*inptr == '#') {
				/* comment within the rule brackets */
				parser->state |= VG_RULE_PARSER_STATE_COMMENT;
				inptr++;
				
				goto comment;
			}
		}
		
		if (*inptr == '}') {
			inptr++;
			
			if (parser->state == VG_RULE_PARSER_STATE_RULE_CALLER) {
				parser->rule_cb (parser, parser->current, parser->user_data);
			} else {
				fprintf (stderr, "Encountered unexpected '}'\n");
				vg_rule_free (parser->current);
			}
			
			parser->state = VG_RULE_PARSER_STATE_INIT;
			
			parser->current = NULL;
			parser->tail = NULL;
			
			parser->lineleft += (parser->lineptr - parser->linebuf);
			parser->lineptr = parser->linebuf;
			
			continue;
		}
		
		*priv->inend = '\n';
		start = inptr;
		while (*inptr != '\n')
			inptr++;
		
		backup_linebuf (parser, start, (inptr - start));
		
		if (inptr == priv->inend)
			break;
		
		switch (parser->state) {
		case VG_RULE_PARSER_STATE_RULE_NAME:
			*parser->lineptr = '\0';
			parser->current->name = g_strdup ((char *)parser->linebuf);
			parser->state = VG_RULE_PARSER_STATE_RULE_TYPE;
			break;
		case VG_RULE_PARSER_STATE_RULE_TYPE:
			/* The newer valgrind (1.9.5) suppression format contains
			 * a list of tool names preceeding the suppression type:
			 *
			 * Memcheck,Addrcheck:Param
			 *
			 * The older format (1.0.x) contained just the type name.
			 */
			
			/* trim tailing whitespc */
			start = parser->lineptr - 1;
			while (start > parser->linebuf && isspace ((int) *start))
				start--;
			
			start[1] = '\0';
			
			/* does this suppression rule contain a list
                           of tools that it is meant for? */
			while (start > parser->linebuf && *start != ':')
				start--;
			
			if (*start == ':') {
				/* why yes, yes it does... */
				*start++ = '\0';
				parser->current->tools = parse_tool_list (parser->linebuf);
			}
			
			parser->current->type = vg_rule_type_from_name ((char *)start);
			g_assert (parser->current->type != VG_RULE_LAST);
			if (parser->current->type == VG_RULE_PARAM)
				parser->state = VG_RULE_PARSER_STATE_RULE_SYSCALL;
			else
				parser->state = VG_RULE_PARSER_STATE_RULE_CALLER;
			break;
		case VG_RULE_PARSER_STATE_RULE_SYSCALL:
			parser->current->syscall = g_strndup ((char *)start, (inptr - start));
			parser->state = VG_RULE_PARSER_STATE_RULE_CALLER;
			break;
		case VG_RULE_PARSER_STATE_RULE_CALLER:
			vg_parser_parse_caller (parser);
			break;
		}
		
		parser->lineleft += (parser->lineptr - parser->linebuf);
		parser->lineptr = parser->linebuf;
		
		inptr++;
	}
	
	priv->inptr = inptr;
	
	return 1;
}


void
vg_rule_parser_flush (VgRuleParser *parser)
{
	;
}


int
vg_suppressions_file_write_header (int fd, const char *summary)
{
	GString *string;
	
	string = g_string_new ("##----------------------------------------------------------------------##\n\n");
	g_string_append (string, "# ");
	g_string_append (string, summary);
	g_string_append (string, "\n\n");
	
	/* format specification header */
	g_string_append (string, "# Format of this file is:\n");
	g_string_append (string, "# {\n");
	g_string_append (string, "#     name_of_suppression\n");
	g_string_append (string, "#     tool_name:supp_kind\n");
	g_string_append (string, "#     (optional extra info for some suppression types)\n");
	g_string_append (string, "#     caller0 name, or /name/of/so/file.so\n");
	g_string_append (string, "#     caller1 name, or ditto\n");
	g_string_append (string, "#     (optionally: caller2 name)\n");
	g_string_append (string, "#     (optionally: caller3 name)\n");
	g_string_append (string, "# }\n");
	g_string_append (string, "#\n");
	g_string_append (string, "# For Memcheck, the supp_kinds are:\n");
	g_string_append (string, "#\n");
	g_string_append (string, "#     Param Value1 Value2 Value4 Value8\n");
	g_string_append (string, "#     Free Addr1 Addr2 Addr4 Addr8 Leak\n");
	g_string_append (string, "#     Cond (previously known as Value0)\n");
	g_string_append (string, "#\n");
	g_string_append (string, "# and the optional extra info is:\n");
	g_string_append (string, "#     if Param: name of system call param\n");
	g_string_append (string, "#     if Free: name of free-ing fn)\n\n");
	
	if (vg_write (fd, string->str, string->len) == -1) {
		g_string_free (string, TRUE);
		return -1;
	}
	
	g_string_free (string, TRUE);
	
	return 0;
}


int
vg_suppressions_file_append_rule (int fd, VgRule *rule)
{
	GString *string;
	VgCaller *caller;
	VgTool *tool;
	
	string = g_string_new ("{\n   ");
	g_string_append (string, rule->name);
	g_string_append (string, "\n   ");
	
	tool = rule->tools;
	if (tool != NULL) {
		while (tool != NULL) {
			g_string_append (string, tool->name);
			if (tool->next)
				g_string_append_c (string, ',');
			tool = tool->next;
		}
		
		g_string_append_c (string, ':');
	}
	
	g_string_append (string, vg_rule_types[rule->type]);
	
	if (rule->type == VG_RULE_PARAM) {
		g_string_append (string, "\n   ");
		g_string_append (string, rule->syscall);
	}
	
	caller = rule->callers;
	while (caller != NULL) {
		g_string_append_printf (string, "\n   %s:%s", vg_caller_types[caller->type], caller->name);
		caller = caller->next;
	}
	
	g_string_append (string, "\n}\n");
	
	if (vg_write (fd, string->str, string->len) == -1) {
		g_string_free (string, TRUE);
		return -1;
	}
	
	g_string_free (string, TRUE);
	
	return 0;
}


#if TEST
static void
my_rule_cb (VgRuleParser *parser, VgRule *rule, void *user_data)
{
	vg_suppressions_file_append_rule (1, rule);
}

int main (int argc, char **argv)
{
	VgRuleParser *parser;
	int fd;
	
	if ((fd = open (argv[1], O_RDONLY)) == -1)
		return 0;
	
	vg_suppressions_file_write_header (1, "Errors to suppress by default for glibc 2.1.3");
	
	parser = vg_rule_parser_new (fd, my_rule_cb, NULL);
	
	while (vg_rule_parser_step (parser) > 0)
		;
	
	vg_rule_parser_free (parser);
	
	close (fd);
	
	return 0;
}
#endif
