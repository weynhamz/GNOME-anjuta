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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include "vgrulepattern.h"

struct _VgRulePattern {
	GPtrArray *patterns;
	vgrule_t type;
	char *syscall;
};


VgRulePattern *
vg_rule_pattern_new (VgRule *rule)
{
	VgRulePattern *pat;
	VgCaller *c;
	
	pat = g_new (VgRulePattern, 1);
	pat->patterns = g_ptr_array_new ();
	pat->type = rule->type;
	pat->syscall = g_strdup (rule->syscall);
	
	c = rule->callers;
	while (c != NULL) {
		regex_t *regex;
		
		regex = g_new (regex_t, 1);
		
		if (regcomp (regex, c->name, REG_EXTENDED | REG_NOSUB) != 0) {
			g_free (regex);
			break;
		}
		
		g_ptr_array_add (pat->patterns, regex);
		
		c = c->next;
	}
	
	return pat;
}


void
vg_rule_pattern_free (VgRulePattern *pat)
{
	int i;
	
	if (pat == NULL)
		return;
	
	for (i = 0; i < pat->patterns->len; i++) {
		regex_t *regex = pat->patterns->pdata[i];
		
		regfree (regex);
		g_free (regex);
	}
	
	g_ptr_array_free (pat->patterns, TRUE);
	g_free (pat->syscall);
	g_free (pat);
}


gboolean
vg_rule_pattern_matches (VgRulePattern *pat, VgError *err)
{
	VgErrorStack *s = err->summary->frames;
	vgrule_t type;
	int i;
	
	if (s == NULL)
		return FALSE;
	
	if (!vg_rule_type_from_report (err->summary->report, &type, NULL) || type != pat->type)
		return FALSE;
	
	if (pat->type == VG_RULE_PARAM) {
		const char *syscall;
		int n;
		
		syscall = err->summary->report + 14;
		n = strcspn (syscall, " ");
		
		if (n != strlen (pat->syscall) || strncmp (pat->syscall, syscall, n) != 0)
			return FALSE;
	}
	
	for (i = 0; s != NULL && i < pat->patterns->len; i++) {
		regex_t *regex = pat->patterns->pdata[i];
		const char *str;
		
		if (s->symbol) {
			str = s->symbol;
		} else if (s->type == VG_STACK_OBJECT) {
			str = s->info.object;
		} else {
			return FALSE;
		}
		
		if (regexec (regex, str, 0, NULL, 0) != 0)
			return FALSE;
		
		s = s->next;
	}
	
	if (i == pat->patterns->len)
		return TRUE;
	
	return FALSE;
}
