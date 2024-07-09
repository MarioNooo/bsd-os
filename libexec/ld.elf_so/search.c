/*	$NetBSD: search.c,v 1.14 2002/10/01 14:16:53 junyoung Exp $	 */

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>

#include "debug.h"
#include "rtld.h"

/*
 * Data declarations.
 */
Search_Path    *_rtld_invalid_paths;

static Obj_Entry *_rtld_search_library_path __P((const char *, size_t,
    const char *, size_t, int));

static Obj_Entry *
_rtld_search_library_path(name, namelen, dir, dirlen, mode)
	const char *name;
	size_t namelen;
	const char *dir;
	size_t dirlen;
	int mode;
{
	char *pathname;
	size_t pathnamelen;
	Obj_Entry *obj = NULL;
	Search_Path *sp;
	char *n, *oldname, *newname = NULL;

	pathnamelen = dirlen + 1 + namelen;

	for (sp = _rtld_invalid_paths; sp != NULL; sp = sp->sp_next) {
		if (sp->sp_pathlen == pathnamelen &&
		    !strncmp(name, sp->sp_path + dirlen + 1, namelen) &&
		    !strncmp(dir, sp->sp_path, dirlen)) {
			return NULL;
		}
	}

	pathname = xmalloc(pathnamelen + 1);
	(void)strncpy(pathname, dir, dirlen);
	pathname[dirlen] = '/';
	/*
	 * If the library name ends in ".so" it doesn't have a major number.
	 * Fake a major of ".0" for it.  ("Ends in" means "the last three
	 * characters are," thus the strstr().
	 */
	n = (char *)name + (namelen - 3);
	if (strstr(n, ".so") != NULL) {
		newname = xmalloc(namelen + 3);
		strcpy(newname, name);
		strcat(newname, ".0");
		oldname = name;
		name = newname;
		dbg(("  No major, forcing \"%s\"", name));
	}
	while (obj == NULL) {
		strcpy(pathname + dirlen + 1, name);

		dbg(("  Trying \"%s\"", pathname));
		obj = _rtld_load_object(pathname, mode);
		if (obj == NULL) {
			Search_Path *path;

			/*
			 * We tried to add ".0" but didn't find it.  Revert
			 * to the straight name.
			 */
			if (newname != NULL) {
				dbg(("   No \"%s,\" reverting to \"%s\"", pathname, oldname));
				free(newname);
				newname = NULL;
				name = oldname;
			}
			else {
				dbg(("   No \"%s\"", pathname));
				path = NEW(Search_Path);
				path->sp_pathlen = pathnamelen;
				path->sp_path = pathname;
				path->sp_next = _rtld_invalid_paths;
				_rtld_invalid_paths = path;
				break;
			}
		}
	}
	return obj;
}

/*
 * Find the library with the given name, and return its full pathname.
 * The returned string is dynamically allocated.  Generates an error
 * message and returns NULL if the library cannot be found.
 *
 * If the second argument is non-NULL, then it refers to an already-
 * loaded shared object, whose library search path will be searched.
 */
Obj_Entry *
_rtld_load_library(name, refobj, mode)
	const char *name;
	const Obj_Entry *refobj;
	int mode;
{
	Search_Path *sp;
	char *pathname;
	int namelen;
	Obj_Entry *obj;

	if (strchr(name, '/') != NULL) {	/* Hard coded pathname */
		if (name[0] != '/' && !_rtld_trust) {
			_rtld_error(
			"absolute pathname required for shared object \"%s\"",
			    name);
			return NULL;
		}
		pathname = xstrdup(name);
		goto found;
	}
	dbg((" Searching for \"%s\" (%p)", name, refobj));

	namelen = strlen(name);

	for (sp = _rtld_paths; sp != NULL; sp = sp->sp_next)
		if ((obj = _rtld_search_library_path(name, namelen,
		    sp->sp_path, sp->sp_pathlen, mode)) != NULL)
			return obj;

	if (refobj != NULL)
		for (sp = refobj->rpaths; sp != NULL; sp = sp->sp_next)
			if ((obj = _rtld_search_library_path(name,
			    namelen, sp->sp_path, sp->sp_pathlen, mode)) != NULL)
				return obj;

	for (sp = _rtld_default_paths; sp != NULL; sp = sp->sp_next)
		if ((obj = _rtld_search_library_path(name, namelen,
		    sp->sp_path, sp->sp_pathlen, mode)) != NULL)
			return obj;

	_rtld_error("Shared object \"%s\" not found", name);
	return NULL;

found:
	obj = _rtld_load_object(pathname, mode);
	if (obj == NULL)
		free(pathname);
	return obj;
}