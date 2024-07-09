/*-
 * Copyright (c) 1995,1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI fdnlist.c,v 2.2 1998/09/08 03:54:43 torek Exp
 */

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <a.out.h>
#include <errno.h>
#include <string.h>

#include "nlist_var.h"

#define	ISLAST(p) \
	((p)->n_un.n_name == NULL || (p)->n_un.n_name[0] == '\0')

static int callback(const struct nlist *, const char *, void *);

/*
 * Scan the symbol table of the given opened executable file
 * for matches in the given list of symbols.  We just map it in
 * and have _nlist_mem do the job.
 */
int
__fdnlist(int f, struct nlist *list)
{
	int ret, saverr;
	void *v;
	size_t size;
	struct stat st;

	if (fstat(f, &st) < 0)
		return (-1);
	if (st.st_size > SIZE_T_MAX) {
		errno = EFBIG;
		return (-1);
	}
	size = st.st_size;
	if ((v = mmap(NULL, size, PROT_READ, MAP_SHARED, f, 0)) == MAP_FAILED)
		return (-1);
	ret = _nlist_mem(list, v, size);
	saverr = errno;
	(void)munmap(v, size);
	errno = saverr;
	return (ret);
}

/*
 * Scan the symbol table of the given memory (representing an object file)
 * for matches in the given list of symbols.
 */
int
_nlist_mem(struct nlist *list, const void *base, size_t size)
{
	struct nlist *p;
	int nent;

	/*
	 * Clean out any left-over information for all valid entries.
	 * Type and value are defined to be 0 if not found.
	 */
	for (nent = 0, p = list; !ISLAST(p); ++p) {
		p->n_type = 0;
		p->n_value = 0;
		++nent;
	}
	if (nent == 0)
		return (0);

	/* Scan the symbol table using our callback function.  */
	if (_nlist_apply(callback, list, base, size) == -1)
		return (-1);

	/* Return the number of invalid entries.  */
	for (nent = 0, p = list; !ISLAST(p); ++p)
		if (p->n_type == 0)
			++nent;
	return (nent);
}

/*
 * Check each symbol in the binary file for a match in our list.
 * If there are multiple matches in the symbol table, we pick the first.
 * If we match every item in our list, we stop scanning.
 */
static int
callback(const struct nlist *s, const char *name, void *arg)
{
	struct nlist *list = arg;
	struct nlist *p;
	int finished;

	/*
	 * We reject nameless symbols, debugging symbols, undefined symbols
	 * and local symbols.
	 */
	if (name == NULL || name[0] == '\0' || (s->n_type & N_STAB) != 0 ||
	    ((s->n_type & N_TYPE) == N_UNDF && s->n_value == 0) ||
	    (s->n_type & N_EXT) != N_EXT)
		return (0);

	finished = 1;			/* assume we got them all */
	for (p = list; !ISLAST(p); ++p) {
		if (p->n_type != 0)	/* already got this one */
			continue;
		/* See if we match with or without an initial underscore.  */
		if (strcmp(name, p->n_un.n_name) != 0 &&
		    (p->n_un.n_name[0] != '_' ||
		    strcmp(name, p->n_un.n_name + 1) != 0)) {
			finished = 0;	/* still need at least one */
			continue;
		}
		p->n_value = s->n_value;
		p->n_type = s->n_type;
		p->n_desc = s->n_desc;		/* XXX not required */
		p->n_other = s->n_other;	/* XXX not required */
	}

	return (finished);
}
