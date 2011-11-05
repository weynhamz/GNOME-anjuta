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

static AmpProperty AmpProjectProperties[] =
{
	{
		{"NAME",
		N_("Name:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
		N_("Project name, it can contain spaces by example 'GNU Autoconf'")},
		AC_TOKEN_AC_INIT, 0, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"VERSION",
		N_("Version:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
		N_("Project version, typically a few numbers separated by dot by example '1.0.0'")},
		AC_TOKEN_AC_INIT, 1, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"BUGREPORT",
		N_("Bug report URL:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
		N_("An email address or a link to a web page where the user can report bug. It is optional.")},
		AC_TOKEN_AC_INIT, 2, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"PACKAGE",
		N_("Package name:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
		N_("Package name, it can contains only alphanumerics and underscore characters."
		   "It is generated from the project name if not provided.")},
		AC_TOKEN_AC_INIT, 3, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"URL",
		N_("URL:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
		N_("An link to the project web page if provided.")},
		AC_TOKEN_AC_INIT, 4, "AC_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"LT_INIT",
		N_("Libtool support:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE | ANJUTA_PROJECT_PROPERTY_HIDDEN,
			N_("Add support to compile shared and static libraries with libtool.")},
		AC_TOKEN_LT_INIT, -1, "LT_INIT(",
		AM_PROPERTY_IN_CONFIGURE
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional linker flags for all targets in this group.")},
		AM_TOKEN__LDFLAGS,	0, "AM_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional C preprocessor flags for all targets in this group.")},
		AM_TOKEN__CPPFLAGS, 0, "AM_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional C compiler flags for all targets in this group.")},
		AM_TOKEN__CFLAGS, 0, "AM_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional C++ compiler flags for all targets in this group.")},
		AM_TOKEN__CXXFLAGS, 0, "AM_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Java compiler flags for all targets in this group.")},
		AM_TOKEN__JAVACFLAGS, 0, "AM_JAVAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Vala compiler flags for all targets in this group.")},
		AM_TOKEN__VALAFLAGS, 0, "AM_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Fortran compiler flags for all targets in this group.")},
		AM_TOKEN__FCFLAGS, 0, "AM_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Objective C compiler flags for all targets in this group.")},
		AM_TOKEN__OBJCFLAGS,	0, "AM_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Lex or Flex lexical analyser generator flags for all targets in this group.")},
		AM_TOKEN__LFLAGS, 0, "AM_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Yacc or Bison parser generator flags for all targets in this group.")},
		AM_TOKEN__YFLAGS, 0, "AM_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"INSTALLDIRS",
		N_("Installation directories:"),
			ANJUTA_PROJECT_PROPERTY_MAP,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("List of custom installation directories used by targets in this group.")},
		AM_TOKEN_DIR, 0, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpProjectPropertyList = NULL;


static AmpProperty AmpGroupNodeProperties[] =
{
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional linker flags for all targets in this group.")},
		AM_TOKEN__LDFLAGS,	0, "AM_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional C preprocessor flags for all targets in this group.")},
		AM_TOKEN__CPPFLAGS, 0, "AM_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional C compiler flags for all targets in this group.")},
		AM_TOKEN__CFLAGS, 0, "AM_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional C++ compiler flags for all targets in this group.")},
		AM_TOKEN__CXXFLAGS, 0, "AM_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Java compiler flags for all targets in this group.")},
		AM_TOKEN__JAVACFLAGS, 0, "AM_JAVAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Vala compiler flags for all targets in this group.")},
		AM_TOKEN__VALAFLAGS, 0, "AM_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Fortran compiler flags for all targets in this group.")},
		AM_TOKEN__FCFLAGS, 0, "AM_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Objective C compiler flags for all targets in this group.")},
		AM_TOKEN__OBJCFLAGS,	0, "AM_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Lex or Flex lexical analyser generator flags for all targets in this group.")},
		AM_TOKEN__LFLAGS, 0, "AM_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Common additional Yacc or Bison parser generator flags for all targets in this group.")},
		AM_TOKEN__YFLAGS, 0, "AM_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"INSTALLDIRS",
		N_("Installation directories:"),
			ANJUTA_PROJECT_PROPERTY_MAP,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("List of custom installation directories used by targets in this group.")},
		AM_TOKEN_DIR, 0, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpGroupNodePropertyList = NULL;


static AmpProperty AmpTargetNodeProperties[] = {
	{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY,
			N_("Build but do not install the target."),
			"0"},
		AM_TOKEN__PROGRAMS,	 3, NULL,
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "bin",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional linker flags for this target.")},
		AM_TOKEN_TARGET_LDFLAGS, 0, "_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LIBADD",
		N_("Additional libraries:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional libraries for this target.")},
		AM_TOKEN_TARGET_LIBADD, 0, "_LIBADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LDADD",
		N_("Additional objects:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional object files for this target.")},
		AM_TOKEN_TARGET_LDADD,	0, "_LDADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C preprocessor flags for this target.")},
		AM_TOKEN_TARGET_CPPFLAGS,	0, "_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C compiler flags for this target.")},
		AM_TOKEN_TARGET_CFLAGS, 0, 	"_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C++ compiler flags for this target.")},
		AM_TOKEN_TARGET_CXXFLAGS,	0, "_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Java compiler flags for this target.")},
		AM_TOKEN_TARGET_JAVACFLAGS, 0, "_JAVACFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Vala compiler flags for this target.")},
		AM_TOKEN_TARGET_VALAFLAGS,0, "_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Fortran compiler flags for this target.")},
		AM_TOKEN_TARGET_FCFLAGS, 0, "_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Objective C compiler flags for this target.")},
		AM_TOKEN_TARGET_OBJCFLAGS, 0, "_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Lex or Flex lexical analyser generator flags for this target.")},
		AM_TOKEN_TARGET_LFLAGS, 0, "_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Yacc or Bison parser generator flags for this target.")},
		AM_TOKEN_TARGET_YFLAGS,	0, 	"_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY,
			N_("Include this target in the distributed package."),
			"0"},
		AM_TOKEN__PROGRAMS, 	2, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CHECKONLY",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY,
			N_("Build this target only when running automatic tests."),
			"0"},
		AM_TOKEN__PROGRAMS, 	4, 	NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. "),
			"0"},
		AM_TOKEN__PROGRAMS, 	1, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app."),
			"0"},
		AM_TOKEN__PROGRAMS, 	0, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpTargetNodePropertyList = NULL;


static AmpProperty AmpProgramTargetProperties[] = {
	{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Build but do not install the target."),
			"0"},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "bin",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional linker flags for this target.")},
		AM_TOKEN_TARGET_LDFLAGS, 0, "_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LDADD",
		N_("Libraries:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional libraries for this target.")},
		AM_TOKEN_TARGET_LDADD,	0, "_LDADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C preprocessor flags for this target.")},
		AM_TOKEN_TARGET_CPPFLAGS,	0, "_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C compiler flags for this target.")},
		AM_TOKEN_TARGET_CFLAGS, 0, 	"_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C++ compiler flags for this target.")},
		AM_TOKEN_TARGET_CXXFLAGS,	0, "_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Java compiler flags for this target.")},
		AM_TOKEN_TARGET_JAVACFLAGS, 0, "_JAVACFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Vala compiler flags for this target.")},
		AM_TOKEN_TARGET_VALAFLAGS,0, "_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Fortran compiler flags for this target.")},
		AM_TOKEN_TARGET_FCFLAGS, 0, "_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Objective C compiler flags for this target.")},
		AM_TOKEN_TARGET_OBJCFLAGS, 0, "_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Lex or Flex lexical analyser generator flags for this target.")},
		AM_TOKEN_TARGET_LFLAGS, 0, "_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Yacc or Bison parser generator flags for this target.")},
		AM_TOKEN_TARGET_YFLAGS,	0, 	"_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Include this target in the distributed package."),
			"1"},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Build this target only when running automatic tests."),
			"0"},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. "),
			"0"},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app."),
			"0"},
		AM_TOKEN__PROGRAMS, 	0, "nobase_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpProgramTargetPropertyList = NULL;


static AmpProperty AmpLibraryTargetProperties[] = {
		{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Build but do not install the target."),
			"0"},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "lib",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"LDFLAGS",
		N_("Linker flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional linker flags for this target.")},
		AM_TOKEN_TARGET_LDFLAGS, 0, "_LDFLAGS",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"LIBADD",
		N_("Libraries:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional libraries for this target.")},
		AM_TOKEN_TARGET_LIBADD,	0, "_LIBADD",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CPPFLAGS",
		N_("C preprocessor flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C preprocessor flags for this target.")},
		AM_TOKEN_TARGET_CPPFLAGS,	0, "_CPPFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CFLAGS",
		N_("C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C compiler flags for this target.")},
		AM_TOKEN_TARGET_CFLAGS, 0, 	"_CFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"CXXFLAGS",
		N_("C++ compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional C++ compiler flags for this target.")},
		AM_TOKEN_TARGET_CXXFLAGS,	0, "_CXXFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"JAVAFLAGS",
		N_("Java compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Java compiler flags for this target.")},
		AM_TOKEN_TARGET_JAVACFLAGS, 0, "_JAVACFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"VALAFLAGS",
		N_("Vala compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Vala compiler flags for this target.")},
		AM_TOKEN_TARGET_VALAFLAGS,0, "_VALAFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"FCFLAGS",
		N_("Fortran compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Fortran compiler flags for this target.")},
		AM_TOKEN_TARGET_FCFLAGS, 0, "_FCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"OBJCFLAGS",
		N_("Objective C compiler flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Objective C compiler flags for this target.")},
		AM_TOKEN_TARGET_OBJCFLAGS, 0, "_OBJCFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"LFLAGS",
		N_("Lex/Flex flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Lex or Flex lexical analyser generator flags for this target.")},
		AM_TOKEN_TARGET_LFLAGS, 0, "_LFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"YFLAGS",
		N_("Yacc/Bison flags:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional Yacc or Bison parser generator flags for this target.")},
		AM_TOKEN_TARGET_YFLAGS,	0, 	"_YFLAGS",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_COMPILATION_FLAG
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Include this target in the distributed package."),
			"1"},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Build this target only when running automatic tests."),
			"0"},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. "),
			"0"},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app."),
			"0"},
		AM_TOKEN__PROGRAMS, 	0, "nobase_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpLibraryTargetPropertyList = NULL;


static AmpProperty AmpManTargetProperties[] = {
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY,
			N_("Additional dependencies for this target.")},
		0, 0, NULL,
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. "),
			"0"},
		AM_TOKEN__PROGRAMS,	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"MAN",
		N_("Manual section:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Section where are installed the man pages. Valid section names are the digits ‘0’ through ‘9’, and the letters ‘l’ and ‘n’. ")},
		AM_TOKEN__PROGRAMS,	 5, "man_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpManTargetPropertyList = NULL;


static AmpProperty AmpDataTargetProperties[] = {
	{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Build but do not install the target."),
			"0"},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "data",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DIRECTORY
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Include this target in the distributed package."),
			"1"},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Build this target only when running automatic tests."),
			"0"},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. "),
			"0"},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Keep relative target path for installing it. "
			   "By example if you have a program subdir/app installed in bin directory it will be installed in bin/subdir/app not in bin/app."),
			"0"},
		AM_TOKEN__PROGRAMS, 	0, "nobase_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{}
};

static GList* AmpDataTargetPropertyList = NULL;


static AmpProperty AmpScriptTargetProperties[] = {
	{
		{"NOINST",
		N_("Do not install:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_ONLY,
			N_("Build but do not install the target.")},
		AM_TOKEN__PROGRAMS,	 3, "noinst_",
		AM_PROPERTY_IN_MAKEFILE | AM_PROPERTY_DISABLE_FOLLOWING
	},
	{
		{"INSTALLDIR",
		N_("Installation directory:"),
			ANJUTA_PROJECT_PROPERTY_STRING,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("It has to be a standard directory or a custom one defined in group properties.")},
		AM_TOKEN__PROGRAMS, 	6, "bin",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"EXTRA_DIST",
		N_("Additional dependencies:"),
			ANJUTA_PROJECT_PROPERTY_LIST,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Additional dependencies for this target.")},
		AM_TOKEN_TARGET_DEPENDENCIES, 0, "EXTRA_DIST",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"DIST",
		N_("Include in distribution:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Include this target in the distributed package.")},
		AM_TOKEN__PROGRAMS, 	2, "nodist_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"CHECK",
		N_("Build for check only:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Build this target only when running automatic tests.")},
		AM_TOKEN__PROGRAMS, 	4, 	"check_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOTRANS",
		N_("Do not use prefix:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
			N_("Do not rename the target with an optional prefix, used to avoid overwritting system program. ")},
		AM_TOKEN__PROGRAMS, 	1, "notrans_",
		AM_PROPERTY_IN_MAKEFILE
	},
	{
		{"NOBASE",
		N_("Keep target path:"),
			ANJUTA_PROJECT_PROPERTY_BOOLEAN,
			ANJUTA_PROJECT_PROPERTY_READ_WRITE,
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
amp_create_property_list (GList **list, AmpProperty *properties)
{
	if (*list == NULL)
	{
		AmpProperty *prop;
		AmpProperty *link = NULL;

		for (prop = properties; prop->base.name != NULL; prop++)
		{
			prop->link = link;
			*list = g_list_prepend (*list, prop);
			link = prop->flags & AM_PROPERTY_DISABLE_FOLLOWING ? prop : NULL;
		}
		*list = g_list_reverse (*list);
	}

	return *list;
}

/* Public functions
 *---------------------------------------------------------------------------*/


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
	prop->token_type = type;
	prop->position = position;

	return (AnjutaProjectProperty *)prop;
}

void
amp_property_free (AnjutaProjectProperty *prop)
{
	if (prop->native == NULL) return;

	if ((prop->name != NULL) && (prop->name != prop->native->name))
	{
		g_free (prop->name);
	}
	if ((prop->value != NULL) && (prop->value != prop->native->value))
	{
		g_free (prop->value);
	}
	g_slice_free (AmpProperty, (AmpProperty *)prop);
}

/* Set property list
 *---------------------------------------------------------------------------*/

gboolean
amp_node_property_load (AnjutaProjectNode *node, gint token_type, gint position, const gchar *value, AnjutaToken *token)
{
	GList *item;
	gboolean set = FALSE;

	for (item = anjuta_project_node_get_native_properties (node); item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;

		if ((prop->token_type == token_type) && (prop->position == position))
		{
			AnjutaProjectProperty *new_prop;

			new_prop = anjuta_project_node_get_property (node, (AnjutaProjectProperty *)prop);
			if (new_prop->native == NULL)
			{
				new_prop = anjuta_project_node_insert_property (node, new_prop, amp_property_new (NULL, token_type, position, NULL, token));
			}

			if ((new_prop->value != NULL) && (new_prop->value != prop->base.value)) g_free (new_prop->value);
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

	for (item = anjuta_project_node_get_native_properties (node); item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;
		AnjutaToken *arg;
		GString *list;

		if ((prop->token_type == ((AmpProperty *)new_prop)->token_type) && (prop->position == ((AmpProperty *)new_prop)->position))
		{
			if (prop->base.type != ANJUTA_PROJECT_PROPERTY_MAP)
			{
				/* Replace property */
				AnjutaProjectProperty *old_prop;

				old_prop = anjuta_project_node_remove_property (node, (AnjutaProjectProperty *)prop);
				if (old_prop != NULL)
				{
					amp_property_free (old_prop);
				}
			}
			anjuta_project_node_insert_property (node, (AnjutaProjectProperty *)prop, new_prop);

			switch (prop->base.type)
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
				if (new_prop->value != prop->base.value) g_free (new_prop->value);
				new_prop->value = g_string_free (list, FALSE);
				break;
			case ANJUTA_PROJECT_PROPERTY_MAP:
			case ANJUTA_PROJECT_PROPERTY_STRING:
				/* Strip leading and trailing space */
				if (new_prop->value != prop->base.value)
				{
					new_prop->value = g_strstrip (new_prop->value);
				}
				break;
			default:
				break;
			}

			set = TRUE;
			break;
		}
	}

	if (!set) amp_property_free (new_prop);

	return set;
}

AnjutaProjectProperty *
amp_node_property_set (AnjutaProjectNode *node, AnjutaProjectProperty *prop, const gchar* value)
{
	AnjutaProjectProperty *new_prop;
	gchar *name;


	if ((value != NULL) && (prop->type == ANJUTA_PROJECT_PROPERTY_MAP))
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

	new_prop = anjuta_project_node_get_map_property (node, prop, name);
	if (new_prop != NULL)
	{
		if (new_prop->native != NULL)
		{
			/* Custom property already exist, replace value */
			if ((new_prop->value != NULL) && (new_prop->value != new_prop->native->value)) g_free (new_prop->value);
			new_prop->value = g_strdup (value);
		}
		else
		{
			/* Add new custom property */
			new_prop = amp_property_new (name, ((AmpProperty *)new_prop)->token_type, ((AmpProperty *)prop)->position, value, NULL);
			anjuta_project_node_insert_property (node, prop, new_prop);
		}
	}
	g_free (name);

	return new_prop;
}

AnjutaProjectProperty *
amp_node_get_property_from_token (AnjutaProjectNode *node, gint token, gint pos)
{
	GList *item;

	for (item = anjuta_project_node_get_native_properties (node); item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;

		if ((prop->token_type == token) && (prop->position == pos))
		{
			return anjuta_project_node_get_property (node, (AnjutaProjectProperty *)prop);
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
amp_node_property_has_flags (AnjutaProjectNode *node, AnjutaProjectProperty *prop, const gchar *value)
{
	return am_node_property_find_flags (prop, value, strlen (value)) != NULL ? TRUE : FALSE;
}

AnjutaProjectProperty *
amp_node_property_remove_flags (AnjutaProjectNode *node, AnjutaProjectProperty *prop, const gchar *value)
{
	AnjutaProjectProperty *new_prop = NULL;
	const gchar *found;
	gsize len = strlen (value);

	found = am_node_property_find_flags (prop, value, len);

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
			new_prop = amp_node_property_set (node, prop, NULL);
		}
		else
		{
			gchar *new_value;

			new_value = g_new(gchar, new_len + 1);

			if (found != prop->value) memcpy (new_value, prop->value, found - prop->value);
			memcpy (new_value + (found - prop->value), found + len, new_len + 1 - (found - prop->value));
			new_prop = amp_node_property_set (node, prop, new_value);
			g_free (new_value);
		}
	}

	return new_prop;
}

AnjutaProjectProperty *
amp_node_property_add_flags (AnjutaProjectNode *node, AnjutaProjectProperty *prop, const gchar *value)
{
	AnjutaProjectProperty *new_prop;

	if (prop->value == NULL)
	{
		new_prop = amp_node_property_set (node, prop, value);
	}
	else
	{
		gchar *new_value;

		new_value = g_strconcat (prop->value, " ", value, NULL);
		new_prop = amp_node_property_set (node, prop, new_value);
		g_free (new_value);
	}

	return new_prop;
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
