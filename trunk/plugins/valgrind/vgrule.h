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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef __VG_RULE_H__
#define __VG_RULE_H__

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


typedef enum {
	VG_CALLER_FUNCTION,
	VG_CALLER_OBJECT,
	VG_CALLER_LAST
} vgcaller_t;

typedef struct _VgCaller {
	struct _VgCaller *next;
	vgcaller_t type;
	char *name;
} VgCaller;

typedef struct _VgTool {
	struct _VgTool *next;
	char *name;
} VgTool;

typedef enum {
	VG_RULE_ADDR1,
	VG_RULE_ADDR2,
	VG_RULE_ADDR4,
	VG_RULE_ADDR8,
	VG_RULE_COND,
	VG_RULE_FREE,
	VG_RULE_LEAK,
	VG_RULE_PARAM,
	VG_RULE_PTHREAD,
	VG_RULE_VALUE1,
	VG_RULE_VALUE2,
	VG_RULE_VALUE4,
	VG_RULE_VALUE8,
	VG_RULE_LAST
} vgrule_t;

typedef struct _VgRule {
	char *name;
	VgTool *tools;
	vgrule_t type;
	char *syscall;
	VgCaller *callers;
} VgRule;

const char *vg_caller_type_to_name (vgcaller_t type);
vgcaller_t vg_caller_type_from_name (const char *name);

VgCaller *vg_caller_new (vgcaller_t type, const char *name);
void vg_caller_free (VgCaller *caller);

const char *vg_rule_type_to_name (vgrule_t type);
vgrule_t vg_rule_type_from_name (const char *name);

int vg_rule_type_from_report (const char *report, vgrule_t *type, char **syscall);

VgRule *vg_rule_new (vgrule_t type, const char *name);
void vg_rule_add_tool (VgRule *rule, const char *tool);
void vg_rule_free (VgRule *rule);


typedef struct _VgRuleParser VgRuleParser;

typedef void (* VgRuleParserRuleCallback) (VgRuleParser *parser, VgRule *rule, void *user_data);

struct _VgRuleParser {
	Parser parser;
	
	unsigned char *linebuf;
	unsigned char *lineptr;
	unsigned int lineleft;
	
	VgRule *current;
	VgCaller *tail;
	
	VgRuleParserRuleCallback rule_cb;
	void *user_data;
	
	int state;
};


VgRuleParser *vg_rule_parser_new (int fd, VgRuleParserRuleCallback rule_cb, void *user_data);
void vg_rule_parser_free (VgRuleParser *parser);

int vg_rule_parser_step (VgRuleParser *parser);
void vg_rule_parser_flush (VgRuleParser *parser);

int vg_suppressions_file_write_header (int fd, const char *summary);
int vg_suppressions_file_append_rule (int fd, VgRule *rule);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_RULE_H__ */
