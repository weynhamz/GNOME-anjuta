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

#ifndef _ANJUTA_PKG_SCANNER_H_
#define _ANJUTA_PKG_SCANNER_H_

#include <libanjuta/anjuta-async-command.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_PKG_SCANNER             (anjuta_pkg_scanner_get_type ())
#define ANJUTA_PKG_SCANNER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PKG_SCANNER, AnjutaPkgScanner))
#define ANJUTA_PKG_SCANNER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_PKG_SCANNER, AnjutaPkgScannerClass))
#define ANJUTA_IS_PKG_SCANNER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_PKG_SCANNER))
#define ANJUTA_IS_PKG_SCANNER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_PKG_SCANNER))
#define ANJUTA_PKG_SCANNER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_PKG_SCANNER, AnjutaPkgScannerClass))

typedef struct _AnjutaPkgScannerClass AnjutaPkgScannerClass;
typedef struct _AnjutaPkgScanner AnjutaPkgScanner;
typedef struct _AnjutaPkgScannerPrivate AnjutaPkgScannerPrivate;

struct _AnjutaPkgScannerClass
{
	AnjutaAsyncCommandClass parent_class;

};

struct _AnjutaPkgScanner
{
	AnjutaAsyncCommand parent_instance;

	AnjutaPkgScannerPrivate* priv;
};

GType anjuta_pkg_scanner_get_type (void) G_GNUC_CONST;
AnjutaCommand* anjuta_pkg_scanner_new (const gchar* package, const gchar* version);
const gchar* anjuta_pkg_scanner_get_package (AnjutaPkgScanner* scanner);
const gchar* anjuta_pkg_scanner_get_version (AnjutaPkgScanner* scanner);
GList* anjuta_pkg_scanner_get_files (AnjutaPkgScanner* scanner);


G_END_DECLS

#endif /* _ANJUTA_PKG_SCANNER_H_ */
