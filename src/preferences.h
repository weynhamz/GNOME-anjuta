/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preferences.h
 * Copyright (C) 2000 - 2003  Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include <libanjuta/anjuta-preferences.h>

#ifdef __cplusplus
extern "C"
{
#endif

void anjuta_preferences_initialize (AnjutaPreferences *pref);

/*
 * Preferences KEY definitions.
 *
 * Use the keys instead of using the strings directly.
 *
 * Call these as the second arg of the
 * functions preferences_get_*() or preferences_set_*() to
 * read or write a preference value the preferences variables.
*/

#define PROJECTS_DIRECTORY         "projects.directory"
#define TARBALLS_DIRECTORY         "tarballs.directory"
#define RPMS_DIRECTORY             "rpms.directory"
#define SRPMS_DIRECTORY            "srpms.directory"
#define MAXIMUM_RECENT_PROJECTS    "maximum.recent.projects"
#define MAXIMUM_RECENT_FILES       "maximum.recent.files"
#define MAXIMUM_COMBO_HISTORY      "maximum.combo.history"
#define DIALOG_ON_BUILD_COMPLETE   "dialog.on.build.complete"
#define BEEP_ON_BUILD_COMPLETE     "beep.on.build.complete"
#define RELOAD_LAST_PROJECT        "reload.last.project"

#define BUILD_OPTION_KEEP_GOING    "build.option.keep.going"
#define BUILD_OPTION_DEBUG         "build.option.debug"
#define BUILD_OPTION_SILENT        "build.option.silent"
#define BUILD_OPTION_WARN_UNDEF    "build.option.warn.undef"
#define BUILD_OPTION_JOBS          "build.option.jobs"
#define BUILD_OPTION_AUTOSAVE      "build.option.autosave"

#define TRUNCAT_MESSAGES           "truncat.messages"
#define TRUNCAT_MESG_FIRST         "truncat.mesg.first"
#define TRUNCAT_MESG_LAST          "truncat.mesg.last"
#define MESSAGES_TAG_POS           "messages.tag.position"

#define MESSAGES_COLOR_ERROR       "messages.color.error"
#define MESSAGES_COLOR_WARNING     "messages.color.warning"
#define MESSAGES_COLOR_MESSAGES1   "messages.color.messages1"
#define MESSAGES_COLOR_MESSAGES2   "messages.color.messages2"
#define MESSAGES_INDICATORS_AUTOMATIC "indicators.automatic"

#define AUTOMATIC_TAGS_UPDATE      "automatic.tags.update"
#define BUILD_SYMBOL_BROWSER       "build.symbol.browser"
#define BUILD_FILE_BROWSER         "build.file.browser"
#define SHOW_TOOLTIPS              "show.tooltips"

#define USE_COMPONENTS             "components.enable"

#define IDENT_NAME                 "ident.name"
#define IDENT_EMAIL                "ident.email"

#ifdef __cplusplus
};
#endif

#endif
