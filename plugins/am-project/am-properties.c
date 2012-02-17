/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-properties.c
 *
 * Copyright (C) 2010  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "am-properties.h"

#include "am-project-private.h"

#include "ac-scanner.h"
#include "am-scanner.h"
#include "ac-writer.h"

#include <glib/gi18n.h>

#include <ctype.h>

/* Types
  *---------------------------------------------------------------------------*/

/* Constants
  *---------------------------------------------------------------------------*/

static AmpPropertyInfo AmpProjectProperties[] =
{
	{
		{"NAME",
		N_("Name:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
		N_("Project name, it can contain spaces by example 'GNU Autoconf'")},
		AC_TOKEN_AC_INIT, 0, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"VERSION",
		N_("Version:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
		N_("Project version, typically a few numbers separated by dot by example '1.0.0'")},
		AC_TOKEN_AC_INIT, 1, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"BUGREPORT",
		N_("Bug report URL:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
		N_("An email address or a link to a web page where the user can report bug. It is optional.")},
		AC_TOKEN_AC_INIT, 2, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"PACKAGE",
		N_("Package name:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
		N_("Package name, it can contains only alphanumerics and underscore characters."
		   "It is generated from the project name if not provided.")},
		AC_TOKEN_AC_INIT, 3, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"URL",
		N_("URL:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
		N_("An link to the project web page if provided.")},
		AC_TOKEN_AC_INIT, 4, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"LT_INIT",
		N_("Libtool support:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC | ANJUTA_PROJECT_PROPERTY_HIDDEN,
			N_("Add support to compile shared and static libraries with libtool.")},
		AC_TOKEN_LT_INIT, -1, "LT_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional linker flags for all targets in this group.")},
		AM_TOKEN__LDFLAGS,	0, "AM_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional C preprocessor flags for all targets in this group.")},
		AM_TOKEN__CPPFLAGS, 0, "AM_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional C compiler flags for all targets in this group.")},
		AM_TOKEN__CFLAGS, 0, "AM_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional C++ compiler flags for all targets in this group.")},
		AM_TOKEN__CXXFLAGS, 0, "AM_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Java compiler flags for all targets in this group.")},
		AM_TOKEN__JAVACFLAGS, 0, "AM_JAVAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Vala compiler flags for all targets in this group.")},
		AM_TOKEN__VALAFLAGS, 0, "AM_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Fortran compiler flags for all targets in this group.")},
		AM_TOKEN__FCFLAGS, 0, "AM_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Objective C compiler flags for all targets in this group.")},
		AM_TOKEN__OBJCFLAGS,	0, "AM_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Lex or Flex lexical analyser generator flags for all targets in this group.")},
		AM_TOKEN__LFLAGS, 0, "AM_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Yacc or Bison parser generator flags for all targets in this group.")},
		AM_TOKEN__YFLAGS, 0, "AM_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"INSTALLDIRS",
		N_("Installation directories:"),
			ANJUTA_PROJECT_PROPERTY_MAP,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("List of custom installation directories used by targets in this group.")},
		AM_TOKEN_DIR, 0, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpProjectPropertyList = NULL;


static AmpPropertyInfo AmpGroupNodeProperties[] =
{
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional linker flags for all targets in this group.")},
		AM_TOKEN__LDFLAGS,	0, "AM_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional C preprocessor flags for all targets in this group.")},
		AM_TOKEN__CPPFLAGS, 0, "AM_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional C compiler flags for all targets in this group.")},
		AM_TOKEN__CFLAGS, 0, "AM_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional C++ compiler flags for all targets in this group.")},
		AM_TOKEN__CXXFLAGS, 0, "AM_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Java compiler flags for all targets in this group.")},
		AM_TOKEN__JAVACFLAGS, 0, "AM_JAVAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Vala compiler flags for all targets in this group.")},
		AM_TOKEN__VALAFLAGS, 0, "AM_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Fortran compiler flags for all targets in this group.")},
		AM_TOKEN__FCFLAGS, 0, "AM_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Objective C compiler flags for all targets in this group.")},
		AM_TOKEN__OBJCFLAGS,	0, "AM_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Lex or Flex lexical analyser generator flags for all targets in this group.")},
		AM_TOKEN__LFLAGS, 0, "AM_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Common additional Yacc or Bison parser generator flags for all targets in this group.")},
		AM_TOKEN__YFLAGS, 0, "AM_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"INSTALLDIRS",
		N_("Installation directories:"),
			ANJUTA_PROJECT_PROPERTY_MAP,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("List of custom installation directories used by targets in this group.")},
		AM_TOKEN_DIR, 0, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpGroupNodePropertyList = NULL;


static AmpPropertyInfo AmpTargetNodeProperties[] = {
	{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build but do not install the target.")},
		AM_TOKEN__PROGRAMS,	 3, NULL,
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING,
		"0"
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "bin",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional linker flags for this target.")},
		AM_TOKEN_TARGET_LDFLAGS, 0, "_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LIBADD",
		N_("Additional libraries:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional libraries for this target.")},
		AM_TOKEN_TARGET_LIBADD, 0, "_LIBADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LDADD",
		N_("Additional objects:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional object files for this target.")},
		AM_TOKEN_TARGET_LDADD,	0, "_LDADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C preprocessor flags for this target.")},
		AM_TOKEN_TARGET_CPPFLAGS,	0, "_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C compiler flags for this target.")},
		AM_TOKEN_TARGET_CFLAGS, 0, 	"_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C++ compiler flags for this target.")},
		AM_TOKEN_TARGET_CXXFLAGS,	0, "_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Java compiler flags for this target.")},
		AM_TOKEN_TARGET_JAVACFLAGS, 0, "_JAVACFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Vala compiler flags for this target.")},
		AM_TOKEN_TARGET_VALAFLAGS,0, "_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Fortran compiler flags for this target.")},
		AM_TOKEN_TARGET_FCFLAGS, 0, "_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Objective C compiler flags for this target.")},
		AM_TOKEN_TARGET_OBJCFLAGS, 0, "_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Lex or Flex lexical analyser generator flags for this target.")},
		AM_TOKEN_TARGET_LFLAGS, 0, "_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Yacc or Bison parser generator flags for this target.")},
		AM_TOKEN_TARGET_YFLAGS,	0, 	"_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Include this target in the distributed package.")},
		AM_TOKEN__PROGRAMS, 	2, NULL,
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"CHECKONLY",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build this target only when running automatic tests.")},
		AM_TOKEN__PROGRAMS, 	4, 	NULL,
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. ")},
		AM_TOKEN__PROGRAMS, 	1, NULL,
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app.")},
		AM_TOKEN__PROGRAMS, 	0, NULL,
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{}
};

static GList* AmpTargetNodePropertyList = NULL;


static AmpPropertyInfo AmpProgramTargetProperties[] = {
	{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build but do not install the target.")},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING,
		"0"
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "bin",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional linker flags for this target.")},
		AM_TOKEN_TARGET_LDFLAGS, 0, "_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LDADD",
		N_("Libraries:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional libraries for this target.")},
		AM_TOKEN_TARGET_LDADD,	0, "_LDADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C preprocessor flags for this target.")},
		AM_TOKEN_TARGET_CPPFLAGS,	0, "_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C compiler flags for this target.")},
		AM_TOKEN_TARGET_CFLAGS, 0, 	"_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C++ compiler flags for this target.")},
		AM_TOKEN_TARGET_CXXFLAGS,	0, "_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Java compiler flags for this target.")},
		AM_TOKEN_TARGET_JAVACFLAGS, 0, "_JAVACFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Vala compiler flags for this target.")},
		AM_TOKEN_TARGET_VALAFLAGS,0, "_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Fortran compiler flags for this target.")},
		AM_TOKEN_TARGET_FCFLAGS, 0, "_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Objective C compiler flags for this target.")},
		AM_TOKEN_TARGET_OBJCFLAGS, 0, "_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Lex or Flex lexical analyser generator flags for this target.")},
		AM_TOKEN_TARGET_LFLAGS, 0, "_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Yacc or Bison parser generator flags for this target.")},
		AM_TOKEN_TARGET_YFLAGS,	0, 	"_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Include this target in the distributed package.")},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE,
		"1"
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build this target only when running automatic tests.")},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. ")},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app.")},
		AM_TOKEN__PROGRAMS, 	0, "nobase_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{}
};

static GList* AmpProgramTargetPropertyList = NULL;


static AmpPropertyInfo AmpLibraryTargetProperties[] = {
		{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build but do not install the target.")},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING,
		"0"
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "lib",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional linker flags for this target.")},
		AM_TOKEN_TARGET_LDFLAGS, 0, "_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LIBADD",
		N_("Libraries:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional libraries for this target.")},
		AM_TOKEN_TARGET_LIBADD,	0, "_LIBADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C preprocessor flags for this target.")},
		AM_TOKEN_TARGET_CPPFLAGS,	0, "_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C compiler flags for this target.")},
		AM_TOKEN_TARGET_CFLAGS, 0, 	"_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C++ compiler flags for this target.")},
		AM_TOKEN_TARGET_CXXFLAGS,	0, "_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Java compiler flags for this target.")},
		AM_TOKEN_TARGET_JAVACFLAGS, 0, "_JAVACFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Vala compiler flags for this target.")},
		AM_TOKEN_TARGET_VALAFLAGS,0, "_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Fortran compiler flags for this target.")},
		AM_TOKEN_TARGET_FCFLAGS, 0, "_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Objective C compiler flags for this target.")},
		AM_TOKEN_TARGET_OBJCFLAGS, 0, "_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Lex or Flex lexical analyser generator flags for this target.")},
		AM_TOKEN_TARGET_LFLAGS, 0, "_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Yacc or Bison parser generator flags for this target.")},
		AM_TOKEN_TARGET_YFLAGS,	0, 	"_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Include this target in the distributed package.")},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE,
		"1"
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build this target only when running automatic tests.")},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. ")},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app.")},
		AM_TOKEN__PROGRAMS, 	0, "nobase_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{}
};


static GList* AmpModuleTargetPropertyList = NULL;

static AmpPropertyInfo AmpModuleTargetProperties[] = {
		{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build but do not install the target.")},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING,
		"0"
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "lib",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional linker flags for this target.")},
		AM_TOKEN_TARGET_LDFLAGS, 0, "_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_MANDATORY_VALUE,
		"-module -avoid-version"
	},
	{
		{"LIBADD",
		N_("Libraries:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional libraries for this target.")},
		AM_TOKEN_TARGET_LIBADD,	0, "_LIBADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C preprocessor flags for this target.")},
		AM_TOKEN_TARGET_CPPFLAGS,	0, "_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C compiler flags for this target.")},
		AM_TOKEN_TARGET_CFLAGS, 0, 	"_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional C++ compiler flags for this target.")},
		AM_TOKEN_TARGET_CXXFLAGS,	0, "_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Java compiler flags for this target.")},
		AM_TOKEN_TARGET_JAVACFLAGS, 0, "_JAVACFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Vala compiler flags for this target.")},
		AM_TOKEN_TARGET_VALAFLAGS,0, "_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Fortran compiler flags for this target.")},
		AM_TOKEN_TARGET_FCFLAGS, 0, "_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Objective C compiler flags for this target.")},
		AM_TOKEN_TARGET_OBJCFLAGS, 0, "_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Lex or Flex lexical analyser generator flags for this target.")},
		AM_TOKEN_TARGET_LFLAGS, 0, "_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional Yacc or Bison parser generator flags for this target.")},
		AM_TOKEN_TARGET_YFLAGS,	0, 	"_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Include this target in the distributed package.")},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE,
		"1"
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build this target only when running automatic tests.")},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. ")},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app.")},
		AM_TOKEN__PROGRAMS, 	0, "nobase_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{}
};

static GList* AmpLibraryTargetPropertyList = NULL;


static AmpPropertyInfo AmpManTargetProperties[] = {
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional dependencies for this target.")},
		0, 0, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. ")},
		AM_TOKEN__PROGRAMS,	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"MAN",
		N_("Manual section:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Section where are installed the man pages. Valid section names are the digits ‘0’ through ‘9’, and the letters ‘l’ and ‘n’. ")},
		AM_TOKEN__PROGRAMS,	 5, "man_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpManTargetPropertyList = NULL;


static AmpPropertyInfo AmpDataTargetProperties[] = {
	{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build but do not install the target.")},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING,
		"0"
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "data",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Include this target in the distributed package.")},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE,
		"1"
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build this target only when running automatic tests.")},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. ")},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app.")},
		AM_TOKEN__PROGRAMS, 	0, "nobase_",
		AM_PROPERTY_IN_MAKEFILE,
		"0"
	},
	{}
};

static GList* AmpDataTargetPropertyList = NULL;


static AmpPropertyInfo AmpScriptTargetProperties[] = {
	{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build but do not install the target.")},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "bin",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Include this target in the distributed package.")},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Build this target only when running automatic tests.")},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. ")},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_STATIC,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app.")},
		AM_TOKEN__PROGRAMS, 	0, "nobase_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpScriptTargetPropertyList = NULL;


/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static GList *
amp_create_property_list (GList **list, AmpPropertyInfo *properties)
{
	if (*list == NULL)
	{
		AmpPropertyInfo *info;
		AnjutaProjectPropertyInfo *link = NULL;

		for (info = properties; info->base.name != NULL; info++)
		{
			info->link = link;
			*list = g_list_prepend (*list, info);
			link = info->flags & AM_PROPERTY_DISABLE_FOLLOWING ? (AnjutaProjectPropertyInfo *)info : NULL;
			info->base.default_value = amp_property_new (NULL, 0, 0, info->value, NULL);
			info->base.default_value->info = (AnjutaProjectPropertyInfo *)info;
		}
		*list = g_list_reverse (*list);
	}

	return *list;
}

/* Public functions
 *---------------------------------------------------------------------------*/

/* Properties info objects
 *---------------------------------------------------------------------------*/

AnjutaProjectPropertyInfo *
amp_property_info_new (AnjutaTokenType type, gint position)
{
	AmpPropertyInfo* info;

	info = g_slice_new0(AmpPropertyInfo);
	info->token_type = type;
	info->position = position;

	return (AnjutaProjectPropertyInfo *)info;
}

void
amp_property_info_free (AnjutaProjectPropertyInfo *info)
{
	if (!(info->flags & ANJUTA_PROJECT_PROPERTY_STATIC))
	{
		g_slice_free (AmpPropertyInfo, (AmpPropertyInfo *)info);
	}
}


/* Properties objects
 *---------------------------------------------------------------------------*/

AnjutaProjectProperty *
amp_property_new (const gchar *name, AnjutaTokenType type, gint position, const gchar *value, AnjutaToken *token)
{
	AmpProperty*prop;

	prop = g_slice_new0(AmpProperty);
	prop->base.name = g_strdup (name);
	prop->base.value = g_strdup (value);
	prop->token = token;
	if ((type != 0) || (position != 0))
	{
		/* Add a dummy properties info to keep track of type and position */
		prop->base.info = amp_property_info_new (type, position);
	}

	return (AnjutaProjectProperty *)prop;
}

void
amp_property_free (AnjutaProjectProperty *prop)
{
	amp_property_info_free (prop->info);
	g_free (prop->name);
	g_free (prop->value);
	g_slice_free (AmpProperty, (AmpProperty *)prop);
}

/* Set property list
 *---------------------------------------------------------------------------*/

gboolean
amp_node_property_load (AnjutaProjectNode *node, gint token_type, gint position, const gchar *value, AnjutaToken *token)
{
	GList *item;
	gboolean set = FALSE;

	for (item = anjuta_project_node_get_properties_info (node); item != NULL; item = g_list_next (item))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)item->data;

		if ((info->token_type == token_type) && (info->position == position))
		{
			AnjutaProjectProperty *new_prop;

			new_prop = anjuta_project_node_get_property (node, info->base.id);
			if ((new_prop == NULL) || (new_prop == new_prop->info->default_value))
			{
				new_prop = anjuta_project_node_insert_property (node, (AnjutaProjectPropertyInfo *)info, amp_property_new (NULL, 0, 0, NULL, token));
			}

			g_free (new_prop->value);
			new_prop->value = g_strdup (value);
			set = TRUE;
		}
	}

	return set;
}

gboolean
amp_node_property_add (AnjutaProjectNode *node, AnjutaProjectProperty *new_prop)
{
	GList *item;
	gboolean set = FALSE;

	for (item = anjuta_project_node_get_properties_info (node); item != NULL; item = g_list_next (item))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)item->data;
		AnjutaToken *arg;
		GString *list;

		if ((info->token_type == ((AmpPropertyInfo *)new_prop->info)->token_type) && (info->position == ((AmpPropertyInfo *)new_prop->info)->position))
		{
			if (info->base.type != ANJUTA_PROJECT_PROPERTY_MAP)
			{
				/* Replace property */
				AnjutaProjectProperty *old_prop;

				old_prop = anjuta_project_node_get_map_property (node, info->base.id, new_prop->name);
				if ((old_prop != NULL) && (old_prop->info->default_value != old_prop))
				{
					anjuta_project_node_remove_property (node, old_prop);
					amp_property_free (old_prop);
				}
			}
			switch (info->base.type)
			{
			case  ANJUTA_PROJECT_PROPERTY_LIST:
				/* Re-evaluate token to remove useless space between item */

				list = g_string_new (new_prop->value);
				g_string_assign (list, "");
				for (arg = anjuta_token_first_word (((AmpProperty *)new_prop)->token); arg != NULL; arg = anjuta_token_next_word (arg))
				{
					gchar *value = anjuta_token_evaluate (arg);

					if (value != NULL)
					{
						if (list->len != 0) g_string_append_c (list, ' ');
						g_string_append (list, value);
					}
				}
				g_free (new_prop->value);
				new_prop->value = g_string_free (list, FALSE);
				break;
			case ANJUTA_PROJECT_PROPERTY_MAP:
			case ANJUTA_PROJECT_PROPERTY_STRING:
				/* Strip leading and trailing space */
				if (new_prop->value != NULL) new_prop->value = g_strstrip (new_prop->value);
				break;
			default:
				break;
			}

			if (g_strcmp0 (new_prop->value, info->base.default_value->value) != 0)
			{
				amp_property_info_free (new_prop->info);
				anjuta_project_node_insert_property (node, (AnjutaProjectPropertyInfo *)info, new_prop);
				set = TRUE;
			}
			break;
		}
	}

	if (!set) amp_property_free (new_prop);

	return set;
}

AnjutaProjectProperty *
amp_node_property_set (AnjutaProjectNode *node, const gchar *id, const gchar* value)
{
	AnjutaProjectPropertyInfo *info;
	gchar *name;
	AnjutaProjectProperty *prop;

	info = anjuta_project_node_get_property_info (node, id);
	if ((value != NULL) && (info->type == ANJUTA_PROJECT_PROPERTY_MAP))
	{
		name = strchr (value, '=');
		if (name != NULL)
		{
			gsize len = name - value;

			name = g_strndup (value, len);
			value += len + 1;
		}
	}
	else
	{
		name = NULL;
	}

	prop = amp_node_map_property_set (node, id, name, value);
	g_free (name);

	return prop;
}

AnjutaProjectProperty *
amp_node_map_property_set (AnjutaProjectNode *node, const gchar *id, const gchar *name, const gchar* value)
{
	AnjutaProjectProperty *new_prop;

	new_prop = anjuta_project_node_get_map_property (node, id, name);
	if ((new_prop != NULL) && (new_prop->info->default_value != new_prop))
	{
		/* Property already exist, replace value */
		g_free (new_prop->value);
		new_prop->value = g_strdup (value);
	}
	else
	{
		/* Add new property */
		AnjutaProjectPropertyInfo *info;

		info = anjuta_project_node_get_property_info (node, id);
		new_prop = amp_property_new (name, 0, 0, value, NULL);
		anjuta_project_node_insert_property (node, info, new_prop);
	}

	return new_prop;
}


AnjutaProjectPropertyInfo *
amp_node_get_property_info_from_token (AnjutaProjectNode *node, gint token, gint pos)
{
	GList *item;

	for (item = anjuta_project_node_get_properties_info (node); item != NULL; item = g_list_next (item))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)item->data;

		if ((info->token_type == token) && (info->position == pos))
		{
			return (AnjutaProjectPropertyInfo *)info;
		}
	}

	return NULL;
}

AnjutaProjectProperty *
amp_node_get_property_from_token (AnjutaProjectNode *node, gint token, gint pos)
{
	GList *item;

	for (item = anjuta_project_node_get_properties_info (node); item != NULL; item = g_list_next (item))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)item->data;

		if ((info->token_type == token) && (info->position == pos))
		{
			return anjuta_project_node_get_property (node, info->base.id);
		}
	}

	return NULL;
}



/* Modify flags value
 *---------------------------------------------------------------------------*/

static const gchar *
am_node_property_find_flags (AnjutaProjectProperty *prop, const gchar *value, gsize len)
{
	const gchar *found;

	g_return_val_if_fail (prop != NULL, NULL);

	for (found = prop->value; found != NULL;)
	{
		found = strstr (found, value);
		if (found != NULL)
		{
			/* Check that flags is alone */
			if (((found == prop->value) || isspace (*(found - 1))) &&
			     ((*(found + len) == '\0') || isspace (*(found + len))))
			{
				return found;
			}
			found += len;
		}
	}

	return NULL;
}

gboolean
amp_node_property_has_flags (AnjutaProjectNode *node, const gchar *id, const gchar *value)
{
	AnjutaProjectProperty *prop;

	prop = anjuta_project_node_get_property (node, id);
	if (prop != NULL)
	{
		return am_node_property_find_flags (prop, value, strlen (value)) != NULL ? TRUE : FALSE;
	}
	else
	{
		return FALSE;
	}
}

AnjutaProjectProperty *
amp_node_property_remove_flags (AnjutaProjectNode *node, const gchar *id, const gchar *value)
{
	AnjutaProjectProperty *prop;
	const gchar *found = NULL;
	gsize len = strlen (value);

	prop = anjuta_project_node_get_property (node, id);
	if (prop != NULL)
	{
		found = am_node_property_find_flags (prop, value, len);
	}

	if (found != NULL)
	{
		gsize new_len;

		if (found == prop->value)
		{
			while (isspace (*(found + len))) len++;
		}
		else if (*(found + len) == '\0')
		{
			while ((found != prop->value) && isspace(*(found - 1)))
			{
				found--;
				len++;
			}
		}
		else
		{
			while (isspace (*(found + len))) len++;
		}

		new_len = strlen (prop->value) - len;

		if (new_len == 0)
		{
			prop = amp_node_property_set (node, id, NULL);
		}
		else
		{
			gchar *new_value;

			new_value = g_new(gchar, new_len + 1);

			if (found != prop->value) memcpy (new_value, prop->value, found - prop->value);
			memcpy (new_value + (found - prop->value), found + len, new_len + 1 - (found - prop->value));
			prop = amp_node_property_set (node, id, new_value);
			g_free (new_value);
		}
	}

	return prop;
}

AnjutaProjectProperty *
amp_node_property_add_flags (AnjutaProjectNode *node, const gchar *id, const gchar *value)
{
	AnjutaProjectProperty *prop;

	prop = anjuta_project_node_get_property (node, id);
	if (prop == NULL)
	{
		prop = amp_node_property_set (node, id, value);
	}
	else
	{
		gchar *new_value;

		new_value = prop->value == NULL ? g_strdup (value) : g_strconcat (prop->value, " ", value, NULL);
		prop = amp_node_property_set (node, id, new_value);
		g_free (new_value);
	}

	return prop;
}

/* Add mandatory properties to a new node */
gboolean
amp_node_property_add_mandatory (AnjutaProjectNode *node)
{
	GList *item;
	gboolean added = FALSE;

	for (item = anjuta_project_node_get_properties_info (node); item != NULL; item = g_list_next (item))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)item->data;

		/* FIXME: Does not support map property */
		if ((info->flags & AM_PROPERTY_MANDATORY_VALUE) && (info->value != NULL) && (info->base.type != ANJUTA_PROJECT_PROPERTY_MAP))
		{
			AnjutaProjectProperty *new_prop;

			new_prop = amp_property_new (NULL, 0, 0, info->value, NULL);
			anjuta_project_node_insert_property (node, (AnjutaProjectPropertyInfo *)info, new_prop);
			added = TRUE;
		}
	}

	return added;
}


/* Get property list
 *---------------------------------------------------------------------------*/

GList*
amp_get_project_property_list (void)
{
	return amp_create_property_list (&AmpProjectPropertyList, AmpProjectProperties);
}

GList*
amp_get_group_property_list (void)
{
	return amp_create_property_list (&AmpGroupNodePropertyList, AmpGroupNodeProperties);
}

GList*
amp_get_target_property_list (AnjutaProjectNodeType type)
{
	switch (type & ANJUTA_PROJECT_ID_MASK)
	{
	case ANJUTA_PROJECT_PROGRAM:
		return amp_create_property_list (&AmpProgramTargetPropertyList, AmpProgramTargetProperties);
	case ANJUTA_PROJECT_SHAREDLIB:
	case ANJUTA_PROJECT_STATICLIB:
		return amp_create_property_list (&AmpLibraryTargetPropertyList, AmpLibraryTargetProperties);
	case ANJUTA_PROJECT_LT_MODULE:
		return amp_create_property_list (&AmpModuleTargetPropertyList, AmpModuleTargetProperties);
	case ANJUTA_PROJECT_MAN:
		return amp_create_property_list (&AmpManTargetPropertyList, AmpManTargetProperties);
	case ANJUTA_PROJECT_DATA:
		return amp_create_property_list (&AmpDataTargetPropertyList, AmpDataTargetProperties);
	case ANJUTA_PROJECT_SCRIPT:
		return amp_create_property_list (&AmpScriptTargetPropertyList, AmpScriptTargetProperties);
	default:
		return amp_create_property_list (&AmpTargetNodePropertyList, AmpTargetNodeProperties);
	}
}

GList*
amp_get_source_property_list (void)
{
	return NULL;
}

GList*
amp_get_module_property_list (void)
{
	return NULL;
}

GList*
amp_get_package_property_list (void)
{
	return NULL;
}
