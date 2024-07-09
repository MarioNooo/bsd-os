/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995,1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI aout_nlist_scan.c,v 2.4 2001/10/03 17:29:52 polk Exp
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)nlist.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <errno.h>
#include <a.out.h>
#include <stddef.h>

#ifdef OMAGIC

#include "nlist_var.h"

int
_aout_nlist_scan(nlist_callback_fn callback, void *arg, const void *v,
    size_t size)
{
	const struct nlist *symtab, *symtop, *s;
	const struct exec *ah = v;
	const char *base = v;
	const char *strtab;
	const char *name;
	int r;

	if (ah->a_syms == 0)
		return (0);

	/*
	 * Check for truncated files while setting up pointers.
	 * XXX We don't really do enough sanity checking to prevent core dumps.
	 */
	if (N_SYMOFF(*ah) > size)
		return (EFTYPE);

	symtab = (const struct nlist *)(base + N_SYMOFF(*ah));
	symtop = &symtab[ah->a_syms / sizeof (symtab[0])];

	if (N_STROFF(*ah) + sizeof (u_long) > size)
		return (EFTYPE);

	strtab = base + N_STROFF(*ah);

	if (N_STROFF(*ah) + *(u_long *)strtab > size)
		return (EFTYPE);

	for (s = symtab; s < symtop; ++s) {
		if (s->n_un.n_strx)
			name = &strtab[s->n_un.n_strx];
		else
			name = NULL;
		if ((r = (*callback)(s, name, arg)) != 0)
			return (r < 0 ? errno : 0);
	}

	return (0);
}

#endif
