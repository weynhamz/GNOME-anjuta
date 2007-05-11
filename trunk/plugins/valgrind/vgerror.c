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
#include <errno.h>

#include "vgerror.h"
#include "vgstrpool.h"


#define d(x)

#define TIME_STAMP_FORMAT "%u-%.2u-%.2u %.2u:%.2u:%.2u.%.3u"

enum {
	VG_ERROR_PARSER_STATE_INIT,
	VG_ERROR_PARSER_STATE_NEW_ERROR,
	VG_ERROR_PARSER_STATE_PARTIAL_ERROR,
	VG_ERROR_PARSER_STATE_WARNING = (1 << 8)
};

typedef struct _VgErrorListNode {
	struct _VgErrorListNode *next;
	struct _VgErrorListNode *prev;
	
	int state;
	
	pid_t pid;
	
	VgError *err_cur;
	
	VgErrorSummary *summ_cur;
	VgErrorSummary *summ_tail;
	
	VgErrorStack *stack_tail;
} VgErrorListNode;


VgErrorParser *
vg_error_parser_new (int fd, VgErrorCallback error_cb, void *user_data)
{
	VgErrorParser *parser;
	
	parser = g_new (VgErrorParser, 1);
	parser_init ((Parser *) parser, fd);
	
	parser->pid_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	list_init (&parser->errlist);
	
	parser->error_cb = error_cb;
	parser->user_data = user_data;
	
	return parser;
}


void
vg_error_parser_free (VgErrorParser *parser)
{
	VgErrorListNode *n;
	
	if (parser == NULL)
		return;
	
	g_hash_table_destroy (parser->pid_hash);
	
	while (!list_is_empty (&parser->errlist)) {
		n = (VgErrorListNode *) list_unlink_head (&parser->errlist);
		
		if (n->err_cur)
			vg_error_free (n->err_cur);
		
		g_free (n);
	}
	
	g_free (parser);
}


static int
vg_error_get_state (VgErrorParser *parser, pid_t pid)
{
	VgErrorListNode *n;
	
	if (!(n = g_hash_table_lookup (parser->pid_hash, GINT_TO_POINTER (pid))))
		return VG_ERROR_PARSER_STATE_INIT;
	
	return n->state;
}

static void
vg_error_set_state (VgErrorParser *parser, pid_t pid, int state)
{
	VgErrorListNode *n;
	
	if (!(n = g_hash_table_lookup (parser->pid_hash, GINT_TO_POINTER (pid)))) {
		d(fprintf (stderr, "VgErrorParser setting state on unknown pid??\n"));
		return;
	}
	
	n->state = state;
}

static VgError *
vg_error_new (VgErrorParser *parser, pid_t pid, time_stamp_t *stamp)
{
	VgErrorListNode *n;
	VgError *err;
	
	if (!(n = g_hash_table_lookup (parser->pid_hash, GINT_TO_POINTER (pid)))) {
		n = g_new (VgErrorListNode, 1);
		n->pid = pid;
		
		list_append_node (&parser->errlist, (ListNode *) n);
		g_hash_table_insert (parser->pid_hash, GINT_TO_POINTER (pid), n);
	} else if (n->state == VG_ERROR_PARSER_STATE_NEW_ERROR) {
		return n->err_cur;
	}
	
	err = g_new (VgError, 1);
	memcpy (&err->stamp, stamp, sizeof (err->stamp));
	err->pid = pid;
	err->thread = (vgthread_t) -1;
	err->summary = NULL;
	
	n->err_cur = err;
	n->summ_cur = NULL;
	n->summ_tail = (VgErrorSummary *) &err->summary;
	n->stack_tail = NULL;
	n->state = VG_ERROR_PARSER_STATE_NEW_ERROR;
	
	return err;
}

static void
vg_error_pop (VgErrorParser *parser, pid_t pid)
{
	VgErrorListNode *n;
	
	if (!(n = g_hash_table_lookup (parser->pid_hash, GINT_TO_POINTER (pid))))
		return;
	
	if (n->err_cur) {
		if (n->err_cur->summary) {
			parser->error_cb (parser, n->err_cur, parser->user_data);
		} else {
			d(fprintf (stderr, "Incomplete valgrind error being popped??\n"));
			g_free (n->err_cur);
		}
	} else {
		d(fprintf (stderr, "VgErrorParser stack underflow??\n"));
	}
	
	n->state = VG_ERROR_PARSER_STATE_INIT;
	
	n->err_cur = NULL;
	n->summ_cur = NULL;
	n->summ_tail = NULL;
	n->stack_tail = NULL;
}

static void
vg_error_pop_all (VgErrorParser *parser)
{
	VgErrorListNode *n;
	
	n = (VgErrorListNode *) parser->errlist.head;
	while (n->next != NULL) {
		vg_error_pop (parser, n->pid);
		n = n->next;
	}
}

static void
vg_error_summary_append (VgErrorParser *parser, pid_t pid, const char *report, int len)
{
	VgErrorSummary *summary;
	VgErrorListNode *n;
	
	if (!(n = g_hash_table_lookup (parser->pid_hash, GINT_TO_POINTER (pid)))) {
		d(fprintf (stderr, "VgErrorParser appending summary to non-existant error report??\n"));
		return;
	}
	
	summary = g_new (VgErrorSummary, 1);
	summary->next = NULL;
	summary->parent = n->err_cur;
	summary->frames = NULL;
	summary->report = vg_strndup (report, len);
	
	n->summ_cur = summary;
	n->summ_tail->next = summary;
	n->summ_tail = summary;
	
	n->stack_tail = (VgErrorStack *) &summary->frames;
}

static VgErrorStack *
vg_error_stack_new (VgErrorParser *parser, pid_t pid)
{
	VgErrorStack *stack;
	VgErrorListNode *n;
	
	if (!(n = g_hash_table_lookup (parser->pid_hash, GINT_TO_POINTER (pid)))) {
		d(fprintf (stderr, "VgErrorParser appending stack frame to non-existant error report??\n"));
		return NULL;
	}
	
	stack = g_new (VgErrorStack, 1);
	stack->next = NULL;
	stack->summary = n->summ_cur;
	stack->where = VG_WHERE_AT;
	stack->addr = VG_STACK_ADDR_UNKNOWN;
	stack->type = VG_STACK_SOURCE;
	stack->symbol = NULL;
	stack->info.src.filename = NULL;
	stack->info.src.lineno = 0;
	stack->info.object = NULL;
	
	n->stack_tail->next = stack;
	n->stack_tail = stack;
	
	return stack;
}

int
vg_error_parser_step (VgErrorParser *parser)
{
	register char *inptr;
	char *start, *end;
	time_stamp_t stamp;
	vgthread_t thread;
	unsigned int num;
	int state, ret;
	VgError *err;
	Parser *priv;
	pid_t pid;
	
	priv = (Parser *) parser;
	
	if ((ret = parser_fill (priv)) == 0) {
		vg_error_pop_all (parser);
		return 0;
	} else if (ret == -1) {
		return -1;
	}
	
	start = inptr = (char *)priv->inptr;
	
	while (inptr < (char *)priv->inend) {
		*priv->inend = '\n';
		while (*inptr != '\n')
			inptr++;
		
		d(fprintf (stderr, "parser_step: '%.*s'\n", inptr - start, start));
		
		if (inptr == (char *)priv->inend)
			break;
		
		if (start[0] != '=' || start[1] != '=') {
			d(fprintf (stderr, "Unexpected data received from valgrind: '%.*s'\n", inptr - start, start));
			inptr++;
			start = inptr;
			continue;
		}
		
		stamp.year = 0;
		
		start += 2;
		if ((num = strtoul (start, &end, 10)) == 0 || end == start || *end != '=') {
			/* possible time stamp */
			if (*end != '-') {
				d(fprintf (stderr, "Invalid pid or time stamp received from valgrind: '%.*s'\n", end - start, start));
				inptr++;
				start = inptr;
				continue;
			}
			
			stamp.year = num;
			start = end + 1;
			
			if ((num = strtoul (start, &end, 10)) == 0 || num > 12 || end == start || *end != '-') {
				d(fprintf (stderr, "Invalid pid or time stamp received from valgrind: '%.*s'\n", end - start, start));
				inptr++;
				start = inptr;
				continue;
			}
			
			stamp.month = num;
			start = end + 1;
			
			if ((num = strtoul (start, &end, 10)) == 0 || num > 31 || end == start || *end != ' ') {
				d(fprintf (stderr, "Invalid pid or time stamp received from valgrind: '%.*s'\n", end - start, start));
				inptr++;
				start = inptr;
				continue;
			}
			
			stamp.day = num;
			start = end + 1;
			
			if ((num = strtoul (start, &end, 10)) > 23 || end == start || *end != ':') {
				d(fprintf (stderr, "Invalid pid or time stamp received from valgrind: '%.*s'\n", end - start, start));
				inptr++;
				start = inptr;
				continue;
			}
			
			stamp.hour = num;
			start = end + 1;
			
			if ((num = strtoul (start, &end, 10)) > 59 || end == start || *end != ':') {
				d(fprintf (stderr, "Invalid pid or time stamp received from valgrind: '%.*s'\n", end - start, start));
				inptr++;
				start = inptr;
				continue;
			}
			
			stamp.min = num;
			start = end + 1;
			
			if ((num = strtoul (start, &end, 10)) > 59 || end == start || *end != '.') {
				d(fprintf (stderr, "Invalid pid or time stamp received from valgrind: '%.*s'\n", end - start, start));
				inptr++;
				start = inptr;
				continue;
			}
			
			stamp.sec = num;
			start = end + 1;
			
			if ((num = strtoul (start, &end, 10)) > 1000 || end == start || *end != ' ') {
				d(fprintf (stderr, "Invalid pid or time stamp received from valgrind: '%.*s'\n", end - start, start));
				inptr++;
				start = inptr;
				continue;
			}
			
			stamp.msec = num;
			start = end + 1;
			
			if ((pid = strtoul (start, &end, 10)) == 0 || end == start || *end != '=') {
				d(fprintf (stderr, "Invalid pid or time stamp received from valgrind: '%.*s'\n", end - start, start));
				inptr++;
				start = inptr;
				continue;
			}
		} else {
			pid = num;
		}
		
		if (end[0] != '=' || end[1] != '=') {
			d(fprintf (stderr, "Unexpected data received from valgrind: '%.*s'\n", (inptr - start) + 2, start - 2));
			inptr++;
			start = inptr;
			continue;
		}
		
		start = end + 3;
		
		*inptr = '\0';
		if ((state = vg_error_get_state (parser, pid)) & VG_ERROR_PARSER_STATE_WARNING) {
			while (*start == ' ')
				start++;
			
			if (strcmp (start, "your program may misbehave as a result") == 0) {
				/* this marks the end of the Valgrind warning spewage */
				vg_error_set_state (parser, pid, state & ~VG_ERROR_PARSER_STATE_WARNING);
			}
			
			inptr++;
			start = inptr;
			continue;
		}
		
		if (state != VG_ERROR_PARSER_STATE_PARTIAL_ERROR) {
			/* brand new error - first line is the general report or thread-id (1.9.6 and later) */
			err = vg_error_new (parser, pid, &stamp);
			
			if (start[0] == ' ') {
				d(fprintf (stderr, "Unexpected SPACE received from valgrind: '%.*s'\n", inptr - start, start));
				
				while (*start == ' ')
					start++;
			}
			
			if (start < inptr) {
				if (strncmp (start, "discard syms in ", 16) == 0) {
					/* "discard syms in /path/to/lib/libfoo.so" */
					d(fprintf (stderr, "dropping 'discard syms in' spewage\n"));
				} else if (strstr (start, "IGNORED call to:") != NULL) {
					d(fprintf (stderr, "dropping ignored call notification\n"));
				} else if (strstr (start, "KLUDGED call to:") != NULL) {
					/* "valgrind's libpthread.so: KLUDGED call to: pthread_getschedparam" */
					d(fprintf (stderr, "dropping kludged call notification\n"));
				} else if (strncmp (start, "warning: ", 9) == 0) {
					/* "warning: Valgrind's pthread_getschedparam is incomplete" */
					d(fprintf (stderr, "dropping warning message\n"));
					vg_error_set_state (parser, pid, state | VG_ERROR_PARSER_STATE_WARNING);
				} else if (strncmp (start, "Thread ", 7) == 0) {
					start += 7;
					thread = strtoul (start, &end, 10);
					if (*end != ':') {
						start -= 7;
						vg_error_summary_append (parser, pid, start, inptr - start);
						vg_error_set_state (parser, pid, VG_ERROR_PARSER_STATE_PARTIAL_ERROR);
					} else {
						err->thread = thread;
					}
				} else {
					vg_error_summary_append (parser, pid, start, inptr - start);
					vg_error_set_state (parser, pid, VG_ERROR_PARSER_STATE_PARTIAL_ERROR);
				}
			}
		} else {
			/* another summary, a new stack frame, or end-of-info (ie. a blank line) */
			while (*start == ' ')
				start++;
			
			if (start < inptr) {
				if (!strncmp (start, "discard syms in ", 16)) {
					/* "discard syms in /path/to/lib/libfoo.so" */
					d(fprintf (stderr, "dropping 'discard syms in' spew received from Valgrind\n"));
				} else if (strstr (start, "IGNORED call to:") != NULL) {
					d(fprintf (stderr, "dropping ignored call notification\n"));
				} else if (strstr (start, "KLUDGED call to:") != NULL) {
					/* "valgrind's libpthread.so: KLUDGED call to: pthread_getschedparam" */
					d(fprintf (stderr, "dropping kludged call notification\n"));
				} else if (strncmp (start, "warning: ", 9) == 0) {
					/* "warning: Valgrind's pthread_getschedparam is incomplete" */
					d(fprintf (stderr, "dropping warning message\n"));
					vg_error_set_state (parser, pid, state | VG_ERROR_PARSER_STATE_WARNING);
				} else if (strncmp (start, "at ", 3) != 0 && strncmp (start, "by ", 3) != 0) {
					/* another summary report */
					vg_error_summary_append (parser, pid, start, inptr - start);
				} else {
					VgErrorStack *stack;
					
					stack = vg_error_stack_new (parser, pid);
					stack->where = *start == 'a' ? VG_WHERE_AT : VG_WHERE_BY;
					start += 3;
					
					if (*start == '<') {
						/* unknown address */
						stack->addr = VG_STACK_ADDR_UNKNOWN;
						
						while (start < inptr && *start != '>')
							start++;
						
						if (*start == '>')
							start++;
					} else {
						/* symbol address in hex */
						stack->addr = strtoul (start, (char **) &end, 16);
						start = end;
						
						if (*start == ':')
							start++;
					}
					
					if (*start == ' ')
						start++;
					
					if (strncmp (start, "??? ", 4) == 0) {
						stack->symbol = NULL;
						start += 3;
					} else if (*start == '(') {
						/* not a c/c++ program, hence no symbol */
						stack->symbol = NULL;
						start--;
					} else {
						/* symbol name */
						end = start;
						while (end < inptr && *end != ' ' && *end != '(')
							end++;
						
						if (*end == '(') {
							/* symbol name has a param list - probably a c++ symbol */
							while (end < inptr && *end != ')')
								end++;
							
							if (*end == ')')
								end++;
						}
						
						stack->symbol = vg_strndup (start, end - start);
						start = end;
					}
					
					if (*start == ' ')
						start++;
					
					if (*start == '(') {
						start++;
						
						/* if we have "([with]in foo)" then foo is an object... */
						if (strncmp (start, "within ", 7) == 0) {
							/* (within /usr/bin/emacs) */
							stack->type = VG_STACK_OBJECT;
							start += 7;
						} else if (strncmp (start, "in ", 3) == 0) {
							/* (in /lib/foo.so) */
							stack->type = VG_STACK_OBJECT;
							start += 3;
						} else {
							stack->type = VG_STACK_SOURCE;
						}
						
						end = start;
						while (end < inptr && *end != ':' && *end != ')')
							end++;
						
						/* src filename or shared object */
						if (stack->type == VG_STACK_SOURCE) {
							stack->info.src.filename = vg_strndup (start, end - start);
							
							start = end;
							if (*start++ == ':')
								stack->info.src.lineno = strtoul (start, (char **) &end, 10);
							else
								stack->info.src.lineno = 0;
						} else {
							stack->info.object = vg_strndup (start, end - start);
						}
						
						start = end;
					}
				}
			} else {
				/* end-of-info (ie. a blank line) */
				vg_error_pop (parser, pid);
			}
		}
		
		inptr++;
		start = inptr;
	}
	
	priv->inptr = (unsigned char *)start;
	
	return 1;
}


void
vg_error_parser_flush (VgErrorParser *parser)
{
	VgErrorListNode *n;
	
	n = (VgErrorListNode *) parser->errlist.head;
	while (n->next != NULL) {
		if (n->err_cur) {
			if (n->state == VG_ERROR_PARSER_STATE_PARTIAL_ERROR) {
				vg_error_pop (parser, n->pid);
			} else {
				g_free (n->err_cur);
				n->err_cur = NULL;
			}
		}
		
		n = n->next;
	}
}


static void
vg_error_stack_free (VgErrorStack *stack)
{
	vg_strfree (stack->symbol);
	if (stack->type == VG_STACK_SOURCE)
		vg_strfree (stack->info.src.filename);
	else
		vg_strfree (stack->info.object);
	
	g_free (stack);
}

static void
vg_error_summary_free (VgErrorSummary *summary)
{
	VgErrorStack *frame, *next;
	
	vg_strfree (summary->report);
	
	frame = summary->frames;
	while (frame != NULL) {
		next = frame->next;
		vg_error_stack_free (frame);
		frame = next;
	}
	
	g_free (summary);
}


void
vg_error_free (VgError *err)
{
	VgErrorSummary *summary, *next;
	
	if (err == NULL)
		return;
	
	summary = err->summary;
	while (summary != NULL) {
		next = summary->next;
		vg_error_summary_free (summary);
		summary = next;
	}
	
	g_free (err);
}


static void
vg_error_stack_to_string (VgErrorStack *stack, GString *str)
{
	time_stamp_t *stamp = &stack->summary->parent->stamp;
	pid_t pid = stack->summary->parent->pid;
	
	g_string_append (str, "==");
	
	if (stamp->year != 0) {
		g_string_append_printf (str, TIME_STAMP_FORMAT " ", stamp->year,
					stamp->month, stamp->day, stamp->hour,
					stamp->min, stamp->sec, stamp->msec);
	}
	
	g_string_append_printf (str, "%u==    %s ", pid, stack->where == VG_WHERE_AT ? "at" : "by");
	
	if (stack->addr != VG_STACK_ADDR_UNKNOWN)
		g_string_append_printf (str, "0x%.8x: ", stack->addr);
	else
		g_string_append (str, "<unknown address> ");
	
	g_string_append (str, stack->symbol ? stack->symbol : "???");
	
	if (stack->type == VG_STACK_SOURCE) {
		g_string_append_printf (str, " (%s:%u)\n", stack->info.src.filename, stack->info.src.lineno);
	} else {
		int in;
		
		in = !strcmp (stack->info.object + strlen (stack->info.object) - 3, ".so");
		in = in || strstr (stack->info.object, ".so.") != NULL;
		g_string_append_printf (str, " (%s %s)\n", in ? "in" : "within", stack->info.object);
	}
}

static void
vg_error_summary_to_string (VgErrorSummary *summary, int indent, GString *str)
{
	time_stamp_t *stamp = &summary->parent->stamp;
	VgErrorStack *s;
	
	g_string_append (str, "==");
	
	if (stamp->year != 0) {
		g_string_append_printf (str, TIME_STAMP_FORMAT " ", stamp->year,
					stamp->month, stamp->day, stamp->hour,
					stamp->min, stamp->sec, stamp->msec);
	}
	
	g_string_append_printf (str, "%u== %s", summary->parent->pid, indent ? "   " : "");
	g_string_append (str, summary->report);
	g_string_append_c (str, '\n');
	
	s = summary->frames;
	while (s != NULL) {
		vg_error_stack_to_string (s, str);
		s = s->next;
	}
}


void
vg_error_to_string (VgError *err, GString *str)
{
	VgErrorSummary *s;
	int indent = 0;
	
	if (err->thread != (vgthread_t) -1) {
		g_string_append (str, "==");
		if (err->stamp.year != 0) {
			g_string_append_printf (str, TIME_STAMP_FORMAT " ", err->stamp.year,
						err->stamp.month, err->stamp.day, err->stamp.hour,
						err->stamp.min, err->stamp.sec, err->stamp.msec);
		}
		g_string_append_printf (str, "%u== Thread %ld:\n", err->pid, err->thread);
	}
	
	s = err->summary;
	while (s != NULL) {
		vg_error_summary_to_string (s, indent, str);
		indent = indent || s->frames;
		s = s->next;
	}
	
	g_string_append (str, "==");
	if (err->stamp.year != 0) {
		g_string_append_printf (str, TIME_STAMP_FORMAT " ", err->stamp.year,
					err->stamp.month, err->stamp.day, err->stamp.hour,
					err->stamp.min, err->stamp.sec, err->stamp.msec);
	}
	g_string_append_printf (str, "%u==\n", err->pid);
}
