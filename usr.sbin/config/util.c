/*	BSDI util.c,v 2.9 2003/04/01 16:05:51 prb Exp	*/

/* 
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)util.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <string.h>
#include <unistd.h>
#include "config.h"

static void nomem __P((void));
static void vxerror __P((const char *, int, const char *, va_list));

const char *srcfile;

/* 
 * Malloc, with abort on error.
 */
void *
emalloc(size)
	size_t size;
{
	void *p;

	if ((p = malloc(size)) == NULL)
		nomem();
	return (p);
}

/* 
 * Realloc, with abort on error.
 */
void *
erealloc(p, size)
	void *p;
	size_t size;
{

	if ((p = realloc(p, size)) == NULL)
		nomem();
	return (p);
}

static void
nomem()
{

	(void)fprintf(stderr, "config: out of memory\n");
	exit(1);
}

/*
 * Prepend the compilation directory to a file name.
 */
char *
path(file)
	const char *file;
{
	char *cp;
	const char *directory;	/* Where our compile directory will be */
	size_t dirsize;		/* length of directory, including null */
	const char *cdb = confdirbase;

	if ((directory = check("COMPILE")) != NULL) {
		cdb = "";
		if (check("CONFDIR") == NULL) {
			cp = getcwd(NULL, 0);
			if (cp == NULL)
				err(1, "could not determine current working directory");
			assign("CONFDIR", cp, 0);
		}
	} else
		directory = oldformat ? "../../compile/" : "compile/";
	dirsize = strlen(directory) + 1;

	if (file == NULL) {
		cp = emalloc(dirsize + strlen(cdb));
		(void)sprintf(cp, "%s%s", directory, cdb);
	} else {
		cp = emalloc(dirsize + strlen(cdb) + 1 + strlen(file));
		(void)sprintf(cp, "%s%s/%s", directory, cdb, file);
	}
	return (cp);
}

char *
mkupper(low, up)
	const char *low;
	char *up;
{
	char *r = up;

	while ((*up = *low) != 0) {
		if (islower(*low))
			*up = toupper(*low);
		++up;
		++low;
	}
	return (r);
}

#ifdef __CYGWIN__
/* 
 * Convert a possible win32 path to a posix path. The size of <posixname>
 * must be big enough to hold the resulting posix pathname.
 */

const char *
convertposix(name)
	const char *name;
{
	char buf[MAXPATHLEN * 2];	/* should be enough */

	if (name == NULL)
		return NULL;
	
	cygwin_conv_to_posix_path(name, buf);
	
	return intern(buf);
}
#endif

static struct nvlist *nvhead;

/*
 * Allocate and return a new <name, values, next> node.  The `next'
 * field is usually NULL.  Only one of <str/ptr> can be non-NULL.
 */
struct nvlist *
newnvx(name, str, ptr, i, next)
	const char *name, *str;
	void *ptr;
	int i;
	struct nvlist *next;
{
	register struct nvlist *nv;

	if ((nv = nvhead) == NULL)
		nv = emalloc(sizeof(*nv));
	else
		nvhead = nv->nv_next;
	nv->nv_next = next;
	nv->nv_name = name;
	if (ptr == NULL)
		nv->nv_str = str;
	else {
		if (str != NULL)
			panic("newnvx");
		nv->nv_ptr = ptr;
	}
	nv->nv_int = i;
	return (nv);
}

/*
 * Free an nvlist structure (just one).
 */
void
nvfree(nv)
	register struct nvlist *nv;
{

	nv->nv_next = nvhead;
	nvhead = nv;
}

/*
 * Free an nvlist (the whole list).
 */
void
nvfreel(nv)
	register struct nvlist *nv;
{
	register struct nvlist *next;

	for (; nv != NULL; nv = next) {
		next = nv->nv_next;
		nv->nv_next = nvhead;
		nvhead = nv;
	}
}

/*
 * Free an nvlist item if not already free.  Used in cleaning up after
 * syntax errors, which can occur after some elements are cleaned up
 * and others are still `live'.
 */
void
nvmaybefree(nv)
	register struct nvlist *nv;
{
	register struct nvlist *p;

	for (p = nvhead; p != NULL; p = p->nv_next)
		if (p == nv)
			return;
	nv->nv_next = nvhead;
	nvhead = nv;
}

/*
 * External (config file) error.  Complain, using current file
 * and line number.
 */
void
#if __STDC__
error(const char *fmt, ...)
#else
error(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	extern const char *srcfile;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
/* XXX - THIS IS WRONG !  perhaps it should mm_line somtimes */
	vxerror(srcfile, currentline(), fmt, ap);
	va_end(ap);
}

/*
 * Delayed config file error (i.e., something was wrong but we could not
 * find out about it until later).
 */
void
#if __STDC__
xerror(const char *file, int line, const char *fmt, ...)
#else
xerror(file, line, fmt, va_alist)
	const char *file;
	int line;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vxerror(file, line, fmt, ap);
	va_end(ap);
}

/*
 * Internal form of error() and xerror().
 */
static void
vxerror(file, line, fmt, ap)
	const char *file;
	int line;
	const char *fmt;
	va_list ap;
{

	if (file != NULL)
		(void)fprintf(stderr, "%s:%d: ", file, line);
	(void)vfprintf(stderr, fmt, ap);
	(void)putc('\n', stderr);
	errors++;
}

/*
 * Internal error, abort.
 */
__dead void
#if __STDC__
panic(const char *fmt, ...)
#else
panic(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "config: panic: ");
	(void)vfprintf(stderr, fmt, ap);
	(void)putc('\n', stderr);
	va_end(ap);
	exit(2);
}
