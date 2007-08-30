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


#ifndef __VG_RULE_PATTERN_H__
#define __VG_RULE_PATTERN_H__

#include "vgrule.h"
#include "vgerror.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

typedef struct _VgRulePattern VgRulePattern;

VgRulePattern *vg_rule_pattern_new (VgRule *rule);
void vg_rule_pattern_free (VgRulePattern *pat);

gboolean vg_rule_pattern_matches (VgRulePattern *pat, VgError *err);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_RULE_PATTERN_H__ */
