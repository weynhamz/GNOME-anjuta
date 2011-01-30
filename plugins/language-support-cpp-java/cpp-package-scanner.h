/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2011 <jhs@Obelix>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CPP_PACKAGE_SCANNER_H_
#define _CPP_PACKAGE_SCANNER_H_

#include <libanjuta/anjuta-async-command.h>

G_BEGIN_DECLS

#define CPP_TYPE_PACKAGE_SCANNER             (cpp_package_scanner_get_type ())
#define CPP_PACKAGE_SCANNER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPP_TYPE_PACKAGE_SCANNER, CppPackageScanner))
#define CPP_PACKAGE_SCANNER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CPP_TYPE_PACKAGE_SCANNER, CppPackageScannerClass))
#define CPP_IS_PACKAGE_SCANNER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPP_TYPE_PACKAGE_SCANNER))
#define CPP_IS_PACKAGE_SCANNER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CPP_TYPE_PACKAGE_SCANNER))
#define CPP_PACKAGE_SCANNER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CPP_TYPE_PACKAGE_SCANNER, CppPackageScannerClass))

typedef struct _CppPackageScannerClass CppPackageScannerClass;
typedef struct _CppPackageScanner CppPackageScanner;

struct _CppPackageScannerClass
{
	AnjutaAsyncCommandClass parent_class;

};

struct _CppPackageScanner
{
	AnjutaAsyncCommand parent_instance;

	gchar* package;
	gchar* version;
	GList* files;

};

GType cpp_package_scanner_get_type (void) G_GNUC_CONST;
AnjutaCommand* cpp_package_scanner_new (const gchar* package, const gchar* version);
const gchar* cpp_package_scanner_get_package (CppPackageScanner* scanner);
const gchar* cpp_package_scanner_get_version (CppPackageScanner* scanner);
GList* cpp_package_scanner_get_files (CppPackageScanner* scanner);


G_END_DECLS

#endif /* _CPP_PACKAGE_SCANNER_H_ */
