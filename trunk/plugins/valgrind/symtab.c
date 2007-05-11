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

// fixme
#define LDD_PATH "/usr/bin/ldd"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "symtab.h"
#include "process.h"
#include "ldd.h"

#define d(x)
#define w(x) x

#define POINTER_ARITHMETIC(POINTER, OFFSET) \
    (void *)((char *)(POINTER) + (OFFSET))

static asymbol **
slurp_symtab (bfd *abfd, long *symcount)
{
	asymbol **syms = (asymbol **) NULL;
	long storage;
	
	if (!(bfd_get_file_flags (abfd) & HAS_SYMS)) {
		w(fprintf (stderr, "No symbols in \"%s\".\n", bfd_get_filename (abfd)));
		*symcount = 0;
		return NULL;
	}
	
	storage = bfd_get_symtab_upper_bound (abfd);
	if (storage < 0) {
		w(fprintf (stderr, "%s: Invalid upper-bound\n", bfd_get_filename (abfd)));
		*symcount = 0;
		return NULL;
	} else if (storage == 0) {
		*symcount = 0;
		return NULL;
	}
	
	syms = g_malloc (storage);
	
	*symcount = bfd_canonicalize_symtab (abfd, syms);
	
	if (*symcount < 0) {
		w(fprintf (stderr, "%s: Invalid symbol count\n", bfd_get_filename (abfd)));
		g_free (syms);
		return NULL;
	}
	
	if (*symcount == 0) {
		w(fprintf (stderr, "%s: No symbols\n", bfd_get_filename (abfd)));
	}
	
	return syms;
}

static SymTabMap *
symtab_map_new (const char *filename, const char *libname, void *load_addr)
{
	SymTabMap *map;
	asection *section;
	
	map = g_new (SymTabMap, 1);
	map->next = NULL;
	
	map->abfd = bfd_openr (filename, NULL);
	if (map->abfd == NULL) {
		g_free (map);
		return NULL;
	}
	
	if (!bfd_check_format (map->abfd, bfd_object)) {
		bfd_close (map->abfd);
		g_free (map);
		return NULL;
	}
	
	map->syms = slurp_symtab (map->abfd, &map->symcount);
	if (!map->syms) {
		bfd_close (map->abfd);
		g_free (map);
		return NULL;
	}
	
	section = bfd_get_section_by_name (map->abfd, ".text");
	if (!section) {
		g_free (map->syms);
		bfd_close (map->abfd);
		g_free (map);
		return NULL;
	}

	map->text_section = section;
	map->text_start = POINTER_ARITHMETIC(load_addr, bfd_section_vma (map->abfd, section));
#ifdef HAVE_BFD_GET_SECTION_SIZE_BEFORE_RELOC
	map->text_end = POINTER_ARITHMETIC(map->text_start, bfd_get_section_size_before_reloc (section));
#else
	map->text_end = POINTER_ARITHMETIC(map->text_start, bfd_get_section_size (section));
#endif
	
	map->filename = g_strdup (filename);
	map->libname = g_strdup (libname);
	map->load_addr = load_addr;
	
	return map;
}

static void
load_shared_lib (LddParser *ldd, LddSharedLib *shlib, void *user_data)
{
	SymTab *symtab = user_data;
	SymTabMap *lib;
	
	if (!(lib = symtab_map_new ((char *)shlib->path, (char *)shlib->libname, (void *) shlib->addr))) {
		ldd_shared_lib_free (shlib);
		return;
	}
	
	symtab->tail->next = lib;
	symtab->tail = lib;
	
	ldd_shared_lib_free (shlib);
}


SymTab *
symtab_new (const char *filename)
{
	const char *basename;
	SymTab *symtab;
	LddParser *ldd;
	char *argv[3];
	pid_t pid;
	int fd;
	
	symtab = g_new (SymTab, 1);
	symtab->libs = NULL;
	symtab->tail = (SymTabMap *) &symtab->libs;
	
	if (!(basename = strrchr (filename, '/')))
		basename = filename;
	else
		basename++;
	
	if (!(symtab->prog = symtab_map_new (filename, basename, NULL))) {
		g_free (symtab);
		return NULL;
	}
	
	argv[0] = LDD_PATH;
	argv[1] = (char *) filename;
	argv[2] = NULL;
	
	if ((pid = process_fork (LDD_PATH, argv, FALSE, -1, NULL, &fd, NULL, NULL)) == -1)
		return symtab;
	
	ldd = ldd_parser_new (fd, load_shared_lib, symtab);
	while (ldd_parser_step (ldd) > 0)
		;
	
	ldd_parser_flush (ldd);
	ldd_parser_free (ldd);
	close (fd);
	
	process_wait (pid);
	
	symtab->prog->next = symtab->libs;
	
	return symtab;
}

static void
symtab_map_free (SymTabMap *map)
{
	g_free (map->filename);
	g_free (map->libname);
	bfd_close (map->abfd);
	g_free (map->syms);
	g_free (map);
}

void
symtab_free (SymTab *symtab)
{
	SymTabMap *n, *nn;
	
	if (symtab == NULL)
		return;
	
	symtab_map_free (symtab->prog);
	
	n = symtab->libs;
	while (n != NULL) {
		nn = n->next;
		symtab_map_free (n);
		n = nn;
	}
	
	g_free (symtab);
}


#define DMGL_PARAMS     (1 << 0)        /* Include function args */
#define DMGL_ANSI       (1 << 1)        /* Include const, volatile, etc */

extern char *cplus_demangle (const char *mangled, int options);

static char *
demangle (bfd *abfd, const char *name, int demangle_cpp)
{
	char *demangled = NULL;
	
	if (bfd_get_symbol_leading_char (abfd) == *name)
		name++;
	
	if (demangle_cpp)
		demangled = cplus_demangle (name, DMGL_PARAMS | DMGL_ANSI);
	
	return g_strdup (name);
}

static SymTabMap *
symtab_find_lib (SymTab *symtab, void *addr)
{
	SymTabMap *map, *prev;
	
	d(fprintf (stderr, "looking for library with symbols for %p\n", addr));
	
	prev = map = symtab->prog;
	while (map) {
		d(fprintf (stderr, "%s: load_addr=%p; text_start=%p; text_end=%p\n",
			   map->libname, map->load_addr, map->text_start, map->text_end));
		
		if (addr > map->text_start && addr < map->text_end)
			return prev;
		
		map = map->next;
	}
	
	return NULL;
}

SymTabSymbol *
symtab_resolve_addr (SymTab *symtab, void *addr, int demangle_cpp)
{
	SymTabSymbol *sym;
	const char *name;
	SymTabMap *lib;
    bfd_vma offset;
	
	if (!(lib = symtab_find_lib (symtab, addr))) {
		d(fprintf (stderr, "can't figure out which lib %p is in\n", addr));
		return NULL;
	}
	
	if (lib->abfd->iostream == NULL) {
		lib->abfd->iostream = (void *) fopen (lib->filename, "r+");
		if (lib->abfd->iostream == NULL)
			return NULL;
	}
	
	sym = g_new (SymTabSymbol, 1);
	
    offset = (bfd_vma)((char *)addr - (char *)lib->load_addr);
	
	if (bfd_find_nearest_line (lib->abfd, lib->text_section, lib->syms,
				   offset - lib->text_section->vma,
				   &sym->filename, &name, &sym->lineno)) {
		if (name)
			sym->function = demangle (lib->abfd, name, demangle_cpp);
		else
			sym->function = NULL;
	} else {
		d(fprintf (stderr, "bfd failed to find symbols for %p\n", addr));
		g_free (sym);
		sym = NULL;
	}
	
	return sym;
}


void
symtab_symbol_free (SymTabSymbol *sym)
{
	if (sym == NULL)
		return;
	
	g_free (sym->function);
	g_free (sym);
}
