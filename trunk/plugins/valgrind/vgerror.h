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


#ifndef __VG_ERROR_H__
#define __VG_ERROR_H__

#include <glib.h>

#include <time.h>

#include "list.h"
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

typedef enum {
	VG_WHERE_AT,
	VG_WHERE_BY,
} vgwhere_t;

typedef enum {
	VG_STACK_SOURCE,
	VG_STACK_OBJECT,
} vgstack_t;

#define VG_STACK_ADDR_UNKNOWN ((unsigned int) -1)

typedef struct _VgError VgError;
typedef struct _VgErrorStack VgErrorStack;
typedef struct _VgErrorSummary VgErrorSummary;
typedef struct _VgErrorParser VgErrorParser;

struct _VgErrorStack {
	struct _VgErrorStack *next;       /* next stack frame */
	struct _VgErrorSummary *summary;  /* parent summary */
	vgwhere_t where;                  /* "at", "by" */
	unsigned int addr;                /* symbol address */
	vgstack_t type;                   /* func/obj */
	char *symbol;                     /* symbol name */
	union {
		struct {
			char *filename;
			size_t lineno;
		} src;
		char *object;
	} info;
};

struct _VgErrorSummary {
	struct _VgErrorSummary *next;
	struct _VgErrorStack *frames;
	struct _VgError *parent;
	char *report;
};

typedef unsigned long vgthread_t;

typedef struct _time_stamp {
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int min;
	unsigned int sec;
	unsigned int msec;
} time_stamp_t;

struct _VgError {
	VgErrorSummary *summary;          /* first summary is the error, additional summary nodes are just more specifics */
	time_stamp_t stamp;
	vgthread_t thread;
	pid_t pid;
};

typedef void (*VgErrorCallback) (VgErrorParser *parser, VgError *err, void *user_data);

struct _VgErrorParser {
	Parser parser;
	
	GHashTable *pid_hash;
	List errlist;
	
	VgErrorCallback error_cb;
	void *user_data;
};

VgErrorParser *vg_error_parser_new (int fd, VgErrorCallback error_cb, void *user_data);
void vg_error_parser_free (VgErrorParser *parser);

int vg_error_parser_step (VgErrorParser *parser);
void vg_error_parser_flush (VgErrorParser *parser);

void vg_error_free (VgError *err);

void vg_error_to_string (VgError *err, GString *str);

#ifdef _cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_ERROR_H__ */
